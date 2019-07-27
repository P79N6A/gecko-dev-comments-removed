



#include "base/threading/thread_local.h"

#include <windows.h>

#include "base/logging.h"

namespace base {
namespace internal {


void ThreadLocalPlatform::AllocateSlot(SlotType* slot) {
  *slot = TlsAlloc();
  CHECK_NE(*slot, TLS_OUT_OF_INDEXES);
}


void ThreadLocalPlatform::FreeSlot(SlotType slot) {
  if (!TlsFree(slot)) {
    NOTREACHED() << "Failed to deallocate tls slot with TlsFree().";
  }
}


void* ThreadLocalPlatform::GetValueFromSlot(SlotType slot) {
  return TlsGetValue(slot);
}


void ThreadLocalPlatform::SetValueInSlot(SlotType slot, void* value) {
  if (!TlsSetValue(slot, value)) {
    LOG(FATAL) << "Failed to TlsSetValue().";
  }
}

}  
}  
