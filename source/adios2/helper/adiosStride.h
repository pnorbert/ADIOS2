#ifndef ADIOS2_HELPER_ADIOSSTRIDE_H_
#define ADIOS2_HELPER_ADIOSSTRIDE_H_

#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace helper
{

void StrideCopy(const DataType dtype, const void *in, const CoreDims &inStart,
                const CoreDims &inCount, const bool inIsLittleEndian, void *out,
                const CoreDims &outStart, const CoreDims &outCount, const bool outIsLittleEndian,
                const CoreDims &strideStart, const CoreDims &strideCount,
                const DoubleMatrix &stencil, MemorySpace MemSpace = MemorySpace::Host);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSSTRIDE_H_ */
