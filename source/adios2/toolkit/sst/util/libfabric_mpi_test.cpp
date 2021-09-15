/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test if the libfabric library is compatible with MPI
 *
 *  Created on: Sept 7, 2021
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#include <iostream>
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <vector>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>

#ifdef SST_HAVE_FI_GNI
#include <rdma/fi_ext_gni.h>
#ifdef SST_HAVE_CRAY_DRC
#include <rdmacred.h>

#define DP_DRC_MAX_TRY 60
#define DP_DRC_WAIT_USEC 1000000

#endif /* SST_HAVE_CRAY_DRC */
#endif /* SST_HAVE_FI_GNI */

#define DP_AV_DEF_SIZE 512

MPI_Comm comm;    // Communicator of producers OR consumers
int mpi_rank;     // rank of process among producers OR consumers
int mpi_size;     // number of processes of producers OR consumers
int wrank, wsize; // rank and size in world comm
int nProducers;
int nConsumers;
bool amProducer;

std::vector<int> allranks; // array for MPI_Gather()

std::vector<int> test_level_1_fi_info();
std::vector<int> test_level_2_fi_enable(std::vector<int> &fabric_candidates);
std::vector<int> test_level_3_rdma(std::vector<int> &fabric_candidates);

void PrintUsage() noexcept
{
    std::cout << "Usage: libfabric_mpi_test [producerRanks] " << std::endl;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    if (argc > 1)
    {
        amProducer = false;
        nProducers = 0;
        int j = 1;
        char *end;
        while (argc > j)
        {
            errno = 0;
            unsigned long v = std::strtoul(argv[j], &end, 10);
            if ((errno || (end != 0 && *end != '\0')) && !wrank)
            {
                std::string errmsg(
                    "ERROR: Invalid integer number in argument " +
                    std::to_string(j) + ": '" + std::string(argv[j]) + "'\n");
                PrintUsage();
                throw std::invalid_argument(errmsg);
            }
            if (v >= (unsigned long)wsize && !wrank)
            {
                std::string errmsg(
                    "ERROR: Argument " + std::to_string(j) + ": '" +
                    std::string(argv[j]) +
                    "' is larger than the total number of processes\n");
                PrintUsage();
                throw std::invalid_argument(errmsg);
            }
            if (v == (unsigned long)wrank)
            {
                amProducer = true;
                ++nProducers;
            }
            ++j;
        }
    }
    else
    {
        amProducer = (wrank < wsize / 2);
        nProducers = wsize / 2;
    }
    nConsumers = wsize - nProducers;
    std::cout << "Rank " << wrank << " is a "
              << (amProducer ? "Producer" : "Consumer") << std::endl;

    MPI_Comm_split(MPI_COMM_WORLD, (int)amProducer, 0, &comm);
    MPI_Comm_rank(comm, &mpi_rank);
    MPI_Comm_size(comm, &mpi_size);
    MPI_Barrier(comm);

    if (!wrank)
    {
        allranks.resize(wsize);
    }

    std::vector<int> fabrics_candidates;
    if (!wrank)
    {
        std::vector<int> fabrics_candidates1 = test_level_1_fi_info();
        if (fabrics_candidates1.size() > 0)
        {
            fabrics_candidates = test_level_2_fi_enable(fabrics_candidates1);
        }

        int n = static_cast<int>(fabrics_candidates.size());
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (n > 0)
        {
            MPI_Bcast(fabrics_candidates.data(), n, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }
    else
    {
        int n;
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (n > 0)
        {
            fabrics_candidates.resize(n);
            MPI_Bcast(fabrics_candidates.data(), n, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }

    if (!fabrics_candidates.empty())
    {
        std::vector<int> working_fabrics;
        working_fabrics = test_level_3_rdma(fabrics_candidates);

        if (!wrank)
        {
            std::cout
                << "Summary: libfabric interfaces that work for ADIOS RDMA "
                   "needs are: ";
            for (auto i : working_fabrics)
            {
                std::cout << i << " ";
            }
            std::cout << std::endl;
        }
    }
    MPI_Finalize();
    return 0;
}

struct fabric_state
{
    struct fi_context *ctx;
    struct fi_info *info;
    int local_mr_req;
    int rx_cq_data;
    size_t addr_len;
    size_t msg_prefix_size;
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_ep *signal;
    struct fid_cq *cq_signal;
    struct fid_av *av;
    pthread_t listener;
#ifdef SST_HAVE_CRAY_DRC
    drc_info_handle_t drc_info;
    uint32_t credential;
    struct fi_gni_auth_key *auth_key;
#endif /* SST_HAVE_CRAY_DRC */
    struct fabric_state *next;
};

struct fi_info *adios_fabric_hints()
{
    struct fi_info *hints;
    hints = fi_allocinfo();
    /*hints->caps = FI_MSG | FI_SEND | FI_RECV | FI_REMOTE_READ |
                  FI_REMOTE_WRITE | FI_RMA | FI_READ | FI_WRITE;
    hints->mode = FI_CONTEXT | FI_LOCAL_MR | FI_CONTEXT2 | FI_MSG_PREFIX |
                  FI_ASYNC_IOV | FI_RX_CQ_DATA;
    hints->domain_attr->mr_mode = FI_MR_BASIC;
    hints->domain_attr->control_progress = FI_PROGRESS_AUTO;
    hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
    hints->ep_attr->type = FI_EP_RDM;*/

    // hints->caps = FI_MSG | FI_RMA;
    // hints->ep_attr->type = FI_EP_RDM;
    return hints;
}

bool is_eligible_fabric(struct fi_info *info)
{
    if (!(info->ep_attr->type & FI_EP_RDM))
    {
        std::cout << "     X is not of FI_EP_RDM type"
                  << "\n    ep_type: "
                  << fi_tostr(&info->ep_attr->type, FI_TYPE_EP_TYPE)
                  << std::endl;
        return false;
    }

    if (!(info->caps & FI_RMA))
    {
        std::cout << "     X does not support FI_RMA capability"
                  << "\n    capabilities: "
                  << fi_tostr(&info->caps, FI_TYPE_CAPS) << std::endl;
        return false;
    }
    else if (!(info->caps & FI_MSG))
    {
        std::cout << "     X does not support FI_MSG capability"
                  << "\n    capabilities: "
                  << fi_tostr(&info->caps, FI_TYPE_CAPS) << std::endl;
        return false;
    }

    int flag = (FI_MR_LOCAL | FI_MR_VIRT_ADDR | FI_MR_ALLOCATED);
    if ((info->domain_attr->mr_mode & flag) != flag &&
        !(info->domain_attr->mr_mode & FI_MR_BASIC))
    {
        std::cout << "     X does not support (FI_MR_LOCAL & FI_MR_VIRT_ADDR & "
                     "FI_MR_ALLOCATED) nor FI_MR_BASIC modes"
                  << "\n    mr_modes: "
                  << fi_tostr(&info->domain_attr->mr_mode, FI_TYPE_MR_MODE)
                  << std::endl;
        return false;
    }

    if (!(info->domain_attr->control_progress & FI_PROGRESS_AUTO))
    {
        std::cout << "     X does not support FI_PROGRESS_AUTO control progress"
                  << "\n    control progress: "
                  << fi_tostr(&info->domain_attr->control_progress,
                              FI_TYPE_PROGRESS)
                  << std::endl;
        return false;
    }

    if (!(info->domain_attr->data_progress & FI_PROGRESS_AUTO))
    {
        std::cout << "     X does not support FI_PROGRESS_AUTO data progress"
                  << "\n    data progress: "
                  << fi_tostr(&info->domain_attr->data_progress,
                              FI_TYPE_PROGRESS)
                  << std::endl;
        return false;
    }

    std::cout << "     Passed eligibility checks" << std::endl;

    return true;
}

int score_fabric_features(struct fi_info *info)
{
    int score = 0;
    score += (int)(info->mode & FI_CONTEXT2);
    score += (int)(info->domain_attr->mr_mode & FI_CONTEXT2);

    return score;
}

std::vector<int> test_level_1_fi_info()
{
    std::vector<int> fabrics_candidates;
    std::cout << "Level 1: find candidate libfabric interfaces" << std::endl;

    // struct fi_info *hints = adios_fabric_hints();
    struct fi_info *info;
    fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, NULL /*hints*/, &info);
    // fi_freeinfo(hints);
    if (!info)
    {
        std::cout << "ERROR: no fabrics detected." << std::endl;
        return fabrics_candidates;
    }
    struct fi_info *originfo = info;

    int n = 0;
    while (info)
    {
        std::cout << "Provider: " << info->fabric_attr->prov_name
                  << "    domain: " << info->domain_attr->name << std::endl;
        if (is_eligible_fabric(info))
        {
            fabrics_candidates.push_back(n);
        }
        info = info->next;
        ++n;
    }

    std::cout << "Level 1 found " << fabrics_candidates.size()
              << " candidate libfabric interfaces" << std::endl;

    info = originfo;
    int candidatepos = 0;
    n = 0;
    while (info)
    {
        while (fabrics_candidates[candidatepos] < n)
        {
            ++candidatepos;
        }
        if (fabrics_candidates[candidatepos] == n)
        {
            std::cout << n << ": Provider: " << info->fabric_attr->prov_name
                      << "    domain: " << info->domain_attr->name << std::endl;
        }
        info = info->next;
        ++n;
    }

    fi_freeinfo(originfo);
    return fabrics_candidates;
}

int init_fabric(struct fi_info *info, struct fabric_state *fabric)
{
    fabric->info = fi_dupinfo(info);
    if (!fabric->info)
    {
        std::cout << "    ERROR: copying the fabric info failed.\n";
        return -1;
    }

    if (info->mode & FI_CONTEXT2)
    {
        fabric->ctx = (fi_context *)calloc(2, sizeof(*fabric->ctx));
    }
    else if (info->mode & FI_CONTEXT)
    {
        fabric->ctx = (fi_context *)calloc(1, sizeof(*fabric->ctx));
    }
    else
    {
        fabric->ctx = NULL;
    }

    if (info->mode & FI_LOCAL_MR)
    {
        fabric->local_mr_req = 1;
    }
    else
    {
        fabric->local_mr_req = 0;
    }

    if (info->mode & FI_MSG_PREFIX)
    {
        fabric->msg_prefix_size = info->ep_attr->msg_prefix_size;
    }
    else
    {
        fabric->msg_prefix_size = 0;
    }

    if (info->mode & FI_RX_CQ_DATA)
    {
        fabric->rx_cq_data = 1;
    }
    else
    {
        fabric->rx_cq_data = 0;
    }

    fabric->addr_len = info->src_addrlen;

    info->domain_attr->mr_mode = FI_MR_BASIC;
#ifdef SST_HAVE_CRAY_DRC
    if (strstr(info->fabric_attr->prov_name, "gni") && fabric->auth_key)
    {
        info->domain_attr->auth_key = (uint8_t *)fabric->auth_key;
        info->domain_attr->auth_key_size = sizeof(struct fi_gni_raw_auth_key);
    }
#endif /* SST_HAVE_CRAY_DRC */

    // std::cout
    //    << "    INFO: fabric parameters to use at fabric initialization: \n"
    //    << fi_tostr(fabric->info, FI_TYPE_INFO);

    std::cout << "    Testing: fi_fabric()...\n";
    int result = fi_fabric(info->fabric_attr, &fabric->fabric, fabric->ctx);
    if (result != FI_SUCCESS)
    {
        std::cout << "    ERROR: opening fabric access failed with " << result
                  << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }
    std::cout << "    Testing: fi_domain()...\n";
    result = fi_domain(fabric->fabric, info, &fabric->domain, fabric->ctx);
    if (result != FI_SUCCESS)
    {
        std::cout << "    ERROR: accessing domain failed with " << result
                  << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }
    std::cout << "    Testing: fi_endpoint() with FI_EP_RDM reliable datagram "
                 "messages ...\n";
    info->ep_attr->type = FI_EP_RDM;
    result = fi_endpoint(fabric->domain, info, &fabric->signal, fabric->ctx);
    if (result != FI_SUCCESS || !fabric->signal)
    {
        std::cout << "    ERROR: opening endpoint failed with " << result
                  << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    std::cout << "    Testing: fi_av_open() with FI_AV_MAP mode...\n";
    struct fi_av_attr av_attr;
    struct fi_cq_attr cq_attr;

    av_attr.type = FI_AV_MAP;
    av_attr.count = DP_AV_DEF_SIZE;
    av_attr.ep_per_node = 0;
    result = fi_av_open(fabric->domain, &av_attr, &fabric->av, fabric->ctx);
    if (result != FI_SUCCESS)
    {
        std::cout
            << "    ERROR: could not initialize address vector, failed with "
            << result << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }
    std::cout
        << "    Testing: fi_ep_bind() to bind endpoint to address vector...\n";
    result = fi_ep_bind(fabric->signal, &fabric->av->fid, 0);
    if (result != FI_SUCCESS)
    {
        std::cout << "    ERROR: could not bind endpoint to address vector, "
                     "failed with "
                  << result << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    std::cout << "    Testing: fi_cq_open()...\n";
    cq_attr.size = 0;
    cq_attr.format = FI_CQ_FORMAT_DATA;
    cq_attr.wait_obj = FI_WAIT_UNSPEC;
    cq_attr.wait_cond = FI_CQ_COND_NONE;
    result =
        fi_cq_open(fabric->domain, &cq_attr, &fabric->cq_signal, fabric->ctx);
    if (result != FI_SUCCESS)
    {
        std::cout << "    ERROR: opening completion queue failed with "
                  << result << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    std::cout << "    Testing: fi_ep_bind() to bind endpoint to completion "
                 "queue...\n";
    result = fi_ep_bind(fabric->signal, &fabric->cq_signal->fid,
                        FI_TRANSMIT | FI_RECV);
    if (result != FI_SUCCESS)
    {
        std::cout << "    ERROR: could not bind endpoint to completion queue, "
                     "failed with "
                  << result << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    std::cout << "    Testing: fi_enable()...\n";
    result = fi_enable(fabric->signal);
    if (result != FI_SUCCESS)
    {
        std::cout << "ERROR: enable endpoint, failed with " << result << " ("
                  << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    std::cout << "INFO: fabric successfully initialized.\n";

    return 0;
}

int fini_fabric(struct fabric_state *fabric)
{
    int result;
    do
    {
        result = fi_close((struct fid *)fabric->signal);
    } while (result == -FI_EBUSY);

    if (result != FI_SUCCESS)
    {
        std::cout << "ERROR: could not close ep, failed with " << result << " ("
                  << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    result = fi_close((struct fid *)fabric->cq_signal);
    if (result != FI_SUCCESS)
    {
        std::cout << "ERROR: could not close cq, failed with " << result << " ("
                  << fi_strerror(std::abs(result)) << ")\n";
    }

    result = fi_close((struct fid *)fabric->av);
    if (result != FI_SUCCESS)
    {
        std::cout << "ERROR: could not close av, failed with " << result << " ("
                  << fi_strerror(std::abs(result)) << ")\n";
    }
    result = fi_close((struct fid *)fabric->domain);
    if (result != FI_SUCCESS)
    {
        std::cout << "ERROR: could not close domain, failed with " << result
                  << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    result = fi_close((struct fid *)fabric->fabric);
    if (result != FI_SUCCESS)
    {
        std::cout << "ERROR: could not close fabric, failed with " << result
                  << " (" << fi_strerror(std::abs(result)) << ")\n";
        return -1;
    }

    fi_freeinfo(fabric->info);

    if (fabric->ctx)
    {
        free(fabric->ctx);
    }

#ifdef SST_HAVE_CRAY_DRC
    if (Fabric->auth_key)
    {
        free(Fabric->auth_key);
    }
#endif /* SST_HAVE_CRAY_DRC */

    std::cout << "finalized fabric.\n";

    return 0;
}

std::vector<int> test_level_2_fi_enable(std::vector<int> &fabric_candidates)
{
    std::vector<int> fabrics_enabled;
    std::cout
        << "Level 2: test candidate libfabric interfaces: create an endpoint "
        << std::endl;

    // struct fi_info *hints = adios_fabric_hints();
    struct fi_info *info;
    fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, NULL /*hints*/, &info);
    struct fi_info *originfo = info;

    int n = 0;
    int candidatepos = 0;
    struct fabric_state fabric;
    while (info)
    {
        while (fabric_candidates[candidatepos] < n)
        {
            ++candidatepos;
        }
        if (fabric_candidates[candidatepos] == n)
        {
            std::cout << "Provider: " << info->fabric_attr->prov_name
                      << "    domain: " << info->domain_attr->name << std::endl;
            if (init_fabric(info, &fabric) == 0)
            {
                fini_fabric(&fabric);
                fabrics_enabled.push_back(n);
            }
        }
        info = info->next;
        ++n;
    }

    fi_freeinfo(originfo);
    std::cout << "Level 2 found " << fabrics_enabled.size()
              << " libfabric interfaces that can create a local endpoint"
              << std::endl;

    return fabrics_enabled;
}

void do_listen();
void do_connect();
std::vector<int> test_level_3_rdma(std::vector<int> &fabric_candidates)
{
    std::vector<int> fabrics_working;
    if (amProducer)
    {
        do_listen();
    }
    else
    {
        do_connect();
    }
    return fabrics_working;
}

void do_connect()
{

    // MPI communication test (which also makes producers disconnect first,
    // consumer last)
    MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    std::cout << "Rank " << wrank << " Connection success, all is well!"
              << std::endl;
}

void do_listen()
{
    std::cout << "Rank " << wrank << " Connection success, all is well!"
              << std::endl;

    // MPI communication test
    MPI_Gather(&wrank, 1, MPI_INT, allranks.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);
}
