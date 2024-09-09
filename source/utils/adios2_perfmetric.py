#!/usr/bin/env python3

"""
Give a performance metric from profiling.json in the .bp/ dataset
"""

import json
import sys
import argparse
import os.path


def SetupArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--summary",
        "-s",
        action="store_true",
        default=False,
        help="Only print the metric, not a short report",
    )
    parser.add_argument("FILE", help="Name of the profiling JSON file or BP directory")
    args = parser.parse_args()

    # print(args)
    return args


def CheckFileName(path: str) -> str:
    if not os.path.exists(path):
        print("ERROR: File " + path + " does not exist", flush=True)
        exit(1)
    if os.path.isdir(path):
        return CheckFileName(os.path.join(path, "profiling.json"))
    return path


def ReadJSON(filename: str):
    with open(filename) as jsonfile:
        parsed = json.load(jsonfile)
    return parsed


def GetTimeValue(d: dict, key: str) -> int:
    if key in d:
        return d[key]["mus"]
    else:
        return 0


def ComputeMetric(p: list):
    ESmin = sys.maxsize
    ESmax = 0
    AWDmin = sys.maxsize
    AWDmax = 0
    n_data_bytes = 0
    n_metadata_bytes = 0
    io_time_data = 0
    io_time_metadata = 0
    for rank in p:
        ES = rank["ES"]["mus"]
        ES_AWD = rank["ES_AWD"]["mus"]

        if "transport_0" in rank:
            tr = rank["transport_0"]
            n_data_bytes += tr["wbytes"]
            io_time_data += (
                GetTimeValue(tr, "open") + GetTimeValue(tr, "write") + GetTimeValue(tr, "close")
            )

        if "transport_1" in rank:
            tr = rank["transport_1"]
            n_metadata_bytes += tr["wbytes"]
            io_time_metadata += (
                GetTimeValue(tr, "open") + GetTimeValue(tr, "write") + GetTimeValue(tr, "close")
            )

        ESmin = min(ESmin, ES)
        ESmax = max(ESmax, ES)
        AWDmin = min(AWDmin, ES_AWD)
        AWDmax = max(AWDmax, ES_AWD)

    m = {}
    m["n_writers"] = len(p)
    m["ESmin"] = ESmin / 1000000.0
    m["ESmax"] = ESmax / 1000000.0
    m["AWDmin"] = AWDmin / 1000000.0
    m["AWDmax"] = AWDmax / 1000000.0
    m["io_time_data"] = io_time_data / 1000000.0
    m["io_time_metadata"] = io_time_metadata / 1000000.0
    m["n_data_bytes"] = n_data_bytes
    m["n_metadata_bytes"] = n_metadata_bytes
    m["io_perf_megabyte_per_second"] = (n_data_bytes + n_metadata_bytes) / m["ESmax"] / 1024 / 1024
    return m


def PrintMetrics(m: dict, summary: bool):
    if summary:
        print(f"{m['io_perf_megabyte_per_second']:.0f}")
    else:
        print(f"Number of writers            = {m['n_writers']}")
        print(f"Data in bytes                = {m['n_data_bytes']:,}")
        print(f"Metadata in bytes            = {m['n_metadata_bytes']:,}")
        print(f"ADIOS EndStep time (s)       = {m['ESmin']} .. {m['ESmax']}")
        print(f"ADIOS data write time (s)    = {m['AWDmin']} .. {m['AWDmax']}")
        print(f"ADIOS cumulative raw data write time (s)      = {m['io_time_data']}")
        print(f"ADIOS cumulative raw metadata write time (s)  = {m['io_time_metadata']}")
        print(f"Computed (MB/s)              = {m['io_perf_megabyte_per_second']:.0f}")


if __name__ == "__main__":
    args = SetupArgs()
    fname = CheckFileName(args.FILE)
    parsed = ReadJSON(fname)
    metrics = ComputeMetric(
        parsed,
    )
    PrintMetrics(metrics, args.summary)
