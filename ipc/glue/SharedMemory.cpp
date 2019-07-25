







































#include <math.h>

#include "mozilla/ipc/SharedMemory.h"

namespace mozilla {
namespace ipc {

 size_t
SharedMemory::PageAlignedSize(size_t aSize)
{
  size_t pageSize = SystemPageSize();
  size_t nPagesNeeded = size_t(ceil(double(aSize) / double(pageSize)));
  return pageSize * nPagesNeeded;
}

} 
} 
