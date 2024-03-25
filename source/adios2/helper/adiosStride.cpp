#include "adios2/helper/adiosStride.h"
#include "adios2/helper/adiosMath.h"
#include "adios2/helper/adiosString.h"

#include <iostream>

namespace adios2
{
namespace helper
{

template <class T>
static void print1D(const std::string name, const T *in, const CoreDims &count)
{
    std::cout << name << " = {\n  "; // << std::setprecision(2);
    for (size_t i = 0; i < count[0]; ++i)
    {
        std::cout << in[i] << " ";
    }
    std::cout << "}" << std::endl;
}

template <class T>
static void print2D(const std::string name, const T *in, const CoreDims &count)
{
    std::cout << name << " = {\n  "; // << std::setprecision(2);
    size_t pos = 0;
    for (size_t i = 0; i < count[0]; ++i)
    {
        for (size_t j = 0; j < count[1]; ++j)
        {
            std::cout << in[pos] << " ";
            ++pos;
        }
        std::cout << "\n  ";
    }
    std::cout << "}" << std::endl;
}

template <class T>
static void print3D(const std::string name, const T *in, const CoreDims &count)
{
    std::cout << name << " = \n{"; // << std::setprecision(2);
    size_t pos = 0;
    for (size_t i = 0; i < count[0]; ++i)
    {
        std::cout << "\n    {";
        for (size_t j = 0; j < count[1]; ++j)
        {
            std::cout << "\n        ";
            for (size_t k = 0; k < count[2]; ++k)
            {
                std::cout << in[pos] << " ";
                ++pos;
            }
        }
        std::cout << "\n    }";
    }
    std::cout << "\n}" << std::endl;
}

template <class T>
void StrideCopy1D(const T *in, const CoreDims &inStart, const CoreDims &inCount,
                  const bool inIsLittleEndian, T *out, const CoreDims &outStart,
                  const CoreDims &outCount, const bool outIsLittleEndian,
                  const CoreDims &strideStart, const CoreDims &strideCount,
                  const DoubleMatrix &stencil, MemorySpace MemSpace = MemorySpace::Host)
{
    /*std::cout << "StrideCopy1D: inStart = " << DimsToString(inStart)
              << " inCount = " << DimsToString(inCount) << " outStart = " << DimsToString(outStart)
              << " outCount = " << DimsToString(outCount)
              << " strideStart = " << DimsToString(strideStart)
              << " srideCount = " << DimsToString(strideCount) << std::endl;*/
    // print1D("Incoming block", in, inCount);

    size_t inPos = strideStart[0];
    for (size_t z = 0; z < outCount[0]; ++z)
    {
        out[z] = in[inPos];
        inPos += strideCount[0];
    }

    // print1D("Outgoing block", out, outCount);
}

template <class T>
void StrideCopy2D(const T *in, const CoreDims &inStart, const CoreDims &inCount,
                  const bool inIsLittleEndian, T *out, const CoreDims &outStart,
                  const CoreDims &outCount, const bool outIsLittleEndian,
                  const CoreDims &strideStart, const CoreDims &strideCount,
                  const DoubleMatrix &stencil, MemorySpace MemSpace = MemorySpace::Host)
{
    std::cout << "StrideCopy2D: inStart = " << DimsToString(inStart)
              << " inCount = " << DimsToString(inCount) << " outStart = " << DimsToString(outStart)
              << " outCount = " << DimsToString(outCount)
              << " strideStart = " << DimsToString(strideStart)
              << " srideCount = " << DimsToString(strideCount) << std::endl;
    print2D("Incoming block", in, inCount);
#if 0
    size_t outPos = 0;
    for (size_t i = 0; i < outCount[0]; ++i)
    {
        size_t rowIn = strideStart[0] + i * strideCount[0];
        size_t inPos = rowIn * inCount[1] + strideStart[1];
        for (size_t j = 0; j < outCount[1]; ++j)
        {
            out[outPos] = in[inPos];
            ++outPos;
            inPos += strideCount[1];
        }
    }
#else
    // print2D("Incoming stencil", stencil.data.data(), CoreDims(stencil.shape));
    //  window of values to calculate with the stencil
    std::vector<T> window(stencil.shape[0] * stencil.shape[1]);
    {
        // initialize window: center value is in[strideStart[]],
        // the left and top off center is initialized with the same value
        // to the right/below, resp. if they fall outside of the boundary
        // (top-left corner) of the input buffer (i.e. no data available)

        // start with last position in data to start filling the stencil backwards
        const size_t nX = strideStart[0] + (stencil.shape[0] / 2);
        const size_t nY = strideStart[1] + (stencil.shape[1] / 2);
        size_t winPos = window.size() - 1;

        size_t idx0 = 0;
        for (; idx0 <= nX && idx0 < stencil.shape[0]; ++idx0)
        {
            // rows within the input array
            size_t inPos = (nX - idx0) * inCount[1] + nY;
            size_t idx1 = 0;
            for (; idx1 <= nY && idx1 < stencil.shape[1]; ++idx1)
            {
                // columns within the input array
                window[winPos] = in[inPos];
                --winPos;
                --inPos;
            }
            for (; idx1 < stencil.shape[1]; ++idx1)
            {
                // window point left to edge of input, copy value on the right.
                window[winPos] = window[winPos + 1];
                --winPos;
            }
        }
        for (; idx0 < stencil.shape[0]; ++idx0)
        {
            // rows outside the input array: copy last row of window to this
            // one
            memcpy(window.data() + (stencil.shape[0] - idx0 - 1) * stencil.shape[1],
                   window.data() + (stencil.shape[0] - idx0) * stencil.shape[1],
                   stencil.shape[1] * sizeof(T));
        }
    }
    // print2D("Window initialized", window.data(), CoreDims(stencil.shape));

    size_t outPos = 0;
    for (size_t y = 0; y < outCount[0]; ++y)
    {
        size_t rowIn = y * strideCount[0] + strideStart[0];
        size_t inPos = rowIn * inCount[1] + strideStart[1];
        for (size_t z = 0; z < outCount[1]; ++z)
        {
            out[outPos] = in[inPos];
            ++outPos;
            inPos += strideCount[1];
        }
    }
#endif
    print2D("Outgoing block", out, outCount);
}

template <class T>
void StrideCopy3D(const T *in, const CoreDims &inStart, const CoreDims &inCount,
                  const bool inIsLittleEndian, T *out, const CoreDims &outStart,
                  const CoreDims &outCount, const bool outIsLittleEndian,
                  const CoreDims &strideStart, const CoreDims &strideCount,
                  const DoubleMatrix &stencil, MemorySpace MemSpace = MemorySpace::Host)
{
    std::cout << "StrideCopy3D: inStart = " << DimsToString(inStart)
              << " inCount = " << DimsToString(inCount) << " outStart = " << DimsToString(outStart)
              << " outCount = " << DimsToString(outCount)
              << " strideStart = " << DimsToString(strideStart)
              << " srideCount = " << DimsToString(strideCount) << std::endl;
    print3D("Incoming block", in, inCount);

    size_t outPos = 0;
    for (size_t x = 0; x < outCount[0]; ++x)
    {
        size_t inStartX = x * strideCount[0] + strideStart[0];
        size_t inPosX = inStartX * inCount[1] * inCount[2];
        std::cout << " Slice " << x << " inStartX = " << inStartX << " inPosX = " << inPosX
                  << std::endl;
        for (size_t y = 0; y < outCount[1]; ++y)
        {
            size_t rowIn = y * strideCount[1] + strideStart[1];
            size_t inPos = inPosX + rowIn * inCount[2] + strideStart[2];
            for (size_t z = 0; z < outCount[2]; ++z)
            {
                out[outPos] = in[inPos];
                ++outPos;
                inPos += strideCount[2];
            }
        }
    }

    print3D("Outgoing block", out, outCount);
}

template <class T>
void StrideCopyND(const T *in, const CoreDims &inStart, const CoreDims &inCount,
                  const bool inIsLittleEndian, T *out, const CoreDims &outStart,
                  const CoreDims &outCount, const bool outIsLittleEndian,
                  const CoreDims &strideStart, const CoreDims &strideCount,
                  MemorySpace MemSpace = MemorySpace::Host)
{
    auto lf_incrementCounter = [](std::vector<size_t> &counter, const CoreDims &outCount,
                                  const size_t ndim) {
        for (int d = (int)ndim - 1; d >= 0; --d)
        {
            if (counter[d] < outCount[d] - 1)
            {
                ++counter[d];
                break;
            }
            else
            {
                counter[d] = 0;
            }
        }
    };

    std::cout << "StrideCopyND: inStart = " << DimsToString(inStart)
              << " inCount = " << DimsToString(inCount) << " outStart = " << DimsToString(outStart)
              << " outCount = " << DimsToString(outCount)
              << " strideStart = " << DimsToString(strideStart)
              << " srideCount = " << DimsToString(strideCount) << std::endl;
    // print3D("Incoming block", in, inCount);

    size_t outPos = 0;
    size_t ndim = inCount.size();
    size_t nTotal = std::accumulate(outCount.begin(), outCount.end(), 1, std::multiplies<size_t>());

    std::vector<size_t> inProdSizes(ndim, 1); // total # of elems in lower dims
    for (size_t d = ndim - 1; d > 0; --d)
    {
        inProdSizes[d - 1] = inCount[d] * inProdSizes[d];
    }

    std::vector<size_t> outPosND(ndim, 0); // m-dim position in 'out' array
    std::vector<size_t> inPosND(ndim, 0);  // m-dim position in 'in' array
    while (outPos < nTotal)
    {
        size_t inPos1D = 0;
        for (size_t d = 0; d < ndim; ++d)
        {
            inPosND[d] = outPosND[d] * strideCount[d] + strideStart[d];
            inPos1D += inPosND[d] * inProdSizes[d];
        }
        // go through linear dimension in a tighter loop
        for (size_t z = 0; z < outCount[ndim - 1]; ++z)
        {
            out[outPos] = in[inPos1D];
            ++outPos;
            inPos1D += strideCount[ndim - 1];
        }
        // increment by one in first N-1 dimensions (leave alone the last dim)
        lf_incrementCounter(outPosND, outCount, ndim - 1);
    }

    // print3D("Outgoing block", out, outCount);
}

template <class T>
void StrideCopyNDStencil(const T *in, const CoreDims &inStart, const CoreDims &inCount,
                         const bool inIsLittleEndian, T *out, const CoreDims &outStart,
                         const CoreDims &outCount, const bool outIsLittleEndian,
                         const CoreDims &strideStart, const CoreDims &strideCount,
                         const DoubleMatrix &stencil, MemorySpace MemSpace = MemorySpace::Host)
{
    auto lf_incrementCounter = [](std::vector<size_t> &counter, const CoreDims &outCount,
                                  const size_t ndim) {
        for (int d = (int)ndim - 1; d >= 0; --d)
        {
            if (counter[d] < outCount[d] - 1)
            {
                ++counter[d];
                break;
            }
            else
            {
                counter[d] = 0;
            }
        }
    };

    auto lf_fillWindow = [](std::vector<T> &windowData, const std::vector<int64_t> &windowPos,
                            const size_t ndim, const std::vector<size_t> &inProdSizes,
                            const size_t stencilNumElems, const std::vector<size_t> &stencilShape,
                            const T *in, const T value) {
        std::vector<int64_t> pos(windowPos);
        for (size_t n = 0; n < stencilNumElems; ++n)
        {
            ;
        }
    };

    std::cout << "StrideCopyNDStencil: inStart = " << DimsToString(inStart)
              << " inCount = " << DimsToString(inCount) << " outStart = " << DimsToString(outStart)
              << " outCount = " << DimsToString(outCount)
              << " strideStart = " << DimsToString(strideStart)
              << " srideCount = " << DimsToString(strideCount) << std::endl;
    // print3D("Incoming block", in, inCount);

    size_t outPos = 0;
    size_t ndim = inCount.size();
    size_t outNumElems =
        std::accumulate(outCount.begin(), outCount.end(), 1, std::multiplies<size_t>());

    std::vector<size_t> inProdSizes(ndim, 1); // total # of elems in lower dims
    for (size_t d = ndim - 1; d > 0; --d)
    {
        inProdSizes[d - 1] = inCount[d] * inProdSizes[d];
    }

    size_t stencilNumElems = helper::GetTotalSize(stencil.shape);
    // stencil window on 'in', n-dim position, each pos[d] can be < 0 or > inCount[d]
    std::vector<int64_t> windowPosND(ndim);
    std::vector<T> windowData(stencilNumElems);
    std::vector<size_t> stencilBackOffset(ndim);
    for (size_t d = 0; d < ndim; ++d)
    {
        stencilBackOffset[d] = stencil.shape[d] / 2;
    }

    std::vector<size_t> outPosND(ndim, 0); // m-dim position in 'out' array
    std::vector<size_t> inPosND(ndim, 0);  // m-dim position in 'in' array
    while (outPos < outNumElems)
    {
        size_t inPos1D = 0;
        for (size_t d = 0; d < ndim; ++d)
        {
            inPosND[d] = outPosND[d] * strideCount[d] + strideStart[d];
            inPos1D += inPosND[d] * inProdSizes[d];
            windowPosND[d] = inPosND[d] - stencilBackOffset[d];
        }
        // go through linear dimension in a tighter loop
        for (size_t z = 0; z < outCount[ndim - 1]; ++z)
        {
            lf_fillWindow(windowData, windowPosND, ndim, inProdSizes, stencilNumElems,
                          stencil.shape, in, in[inPos1D]);
            out[outPos] = in[inPos1D];
            ++outPos;
            inPos1D += strideCount[ndim - 1];
        }
        // increment by one in first N-1 dimensions (leave alone the last dim)
        lf_incrementCounter(outPosND, outCount, ndim - 1);
    }

    // print3D("Outgoing block", out, outCount);
}

template <class T>
void StrideCopyT(const T *in, const CoreDims &inStart, const CoreDims &inCount,
                 const bool inIsLittleEndian, T *out, const CoreDims &outStart,
                 const CoreDims &outCount, const bool outIsLittleEndian,
                 const CoreDims &strideStart, const CoreDims &strideCount,
                 const DoubleMatrix &stencil, MemorySpace MemSpace = MemorySpace::Host)
{
    if (stencil.data.size() == 1)
    {
        return StrideCopyND(in, inStart, inCount, inIsLittleEndian, out, outStart, outCount,
                            outIsLittleEndian, strideStart, strideCount, MemSpace);
    }
    else
    {
        return StrideCopyNDStencil(in, inStart, inCount, inIsLittleEndian, out, outStart, outCount,
                                   outIsLittleEndian, strideStart, strideCount, stencil, MemSpace);
    }
    /*switch (inCount.size())
    {
    case 1:
        return StrideCopy1D(in, inStart, inCount, inIsLittleEndian, out, outStart, outCount,
                            outIsLittleEndian, strideStart, strideCount, stencil, MemSpace);
        break;
    case 2:
        return StrideCopy2D(in, inStart, inCount, inIsLittleEndian, out, outStart, outCount,
                            outIsLittleEndian, strideStart, strideCount, stencil, MemSpace);
        break;
    case 3:
        return StrideCopy3D(in, inStart, inCount, inIsLittleEndian, out, outStart, outCount,
                            outIsLittleEndian, strideStart, strideCount, stencil, MemSpace);
        break;
    default:
        helper::Throw<std::invalid_argument>(
            "Toolkit", "format::bp::BP5Deserializer", "StrideCopyT",
            "Dimension " + std::to_string(inCount.size()) + "not supported");
        break;
    }*/
}

void StrideCopy(const DataType dtype, const void *in, const CoreDims &inStart,
                const CoreDims &inCount, const bool inIsLittleEndian, void *out,
                const CoreDims &outStart, const CoreDims &outCount, const bool outIsLittleEndian,
                const CoreDims &strideStart, const CoreDims &strideCount,
                const DoubleMatrix &stencil, MemorySpace MemSpace)
{
    switch (dtype)
    {
    case DataType::None:
        break;
    case DataType::Char:
    case DataType::Int8:
        StrideCopyT<int8_t>((int8_t *)in, inStart, inCount, inIsLittleEndian, (int8_t *)out,
                            outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                            stencil, MemSpace);
        break;
    case DataType::Int16:
        StrideCopyT<int16_t>((int16_t *)in, inStart, inCount, inIsLittleEndian, (int16_t *)out,
                             outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                             stencil, MemSpace);
        break;
    case DataType::Int32:
        StrideCopyT<int32_t>((int32_t *)in, inStart, inCount, inIsLittleEndian, (int32_t *)out,
                             outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                             stencil, MemSpace);
        break;
    case DataType::Int64:
        StrideCopyT<int64_t>((int64_t *)in, inStart, inCount, inIsLittleEndian, (int64_t *)out,
                             outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                             stencil, MemSpace);
        break;
    case DataType::UInt8:
        StrideCopyT<uint8_t>((uint8_t *)in, inStart, inCount, inIsLittleEndian, (uint8_t *)out,
                             outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                             stencil, MemSpace);
        break;
    case DataType::UInt16:
        StrideCopyT<uint16_t>((uint16_t *)in, inStart, inCount, inIsLittleEndian, (uint16_t *)out,
                              outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                              stencil, MemSpace);
        break;
    case DataType::UInt32:
        StrideCopyT<uint32_t>((uint32_t *)in, inStart, inCount, inIsLittleEndian, (uint32_t *)out,
                              outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                              stencil, MemSpace);
        break;
    case DataType::UInt64:
        StrideCopyT<uint64_t>((uint64_t *)in, inStart, inCount, inIsLittleEndian, (uint64_t *)out,
                              outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                              stencil, MemSpace);
        break;
    case DataType::Float:
        StrideCopyT<float>((float *)in, inStart, inCount, inIsLittleEndian, (float *)out, outStart,
                           outCount, outIsLittleEndian, strideStart, strideCount, stencil,
                           MemSpace);
        break;
    case DataType::Double:
        StrideCopyT<double>((double *)in, inStart, inCount, inIsLittleEndian, (double *)out,
                            outStart, outCount, outIsLittleEndian, strideStart, strideCount,
                            stencil, MemSpace);
        break;
    case DataType::LongDouble:
        StrideCopyT<long double>((long double *)in, inStart, inCount, inIsLittleEndian,
                                 (long double *)out, outStart, outCount, outIsLittleEndian,
                                 strideStart, strideCount, stencil, MemSpace);
        break;
    case DataType::FloatComplex:
        StrideCopyT<std::complex<float>>((std::complex<float> *)in, inStart, inCount,
                                         inIsLittleEndian, (std::complex<float> *)out, outStart,
                                         outCount, outIsLittleEndian, strideStart, strideCount,
                                         stencil, MemSpace);
        break;
    case DataType::DoubleComplex:
        StrideCopyT<std::complex<double>>((std::complex<double> *)in, inStart, inCount,
                                          inIsLittleEndian, (std::complex<double> *)out, outStart,
                                          outCount, outIsLittleEndian, strideStart, strideCount,
                                          stencil, MemSpace);
        break;
    default:
        break;
    }
}

} // end namespace helper
} // end namespace adios2
