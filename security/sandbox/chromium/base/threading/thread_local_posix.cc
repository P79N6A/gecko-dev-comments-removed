



#include "base/threading/thread_local.h"

#include <pthread.h>

#include "base/logging.h"

#if !defined(OS_ANDROID)

namespace base {
namespace internal {


void ThreadLocalPlatform::AllocateSlot(SlotType* slot) {
  int error = pthread_key_create(slot, NULL);
  CHECK_EQ(error, 0);
}


void ThreadLocalPlatform::FreeSlot(SlotType slot) {
  int error = pthread_key_delete(slot);
  DCHECK_EQ(0, error);
}


void* ThreadLocalPlatform::GetValueFromSlot(SlotType slot) {
  return pthread_getspecific(slot);
}


void ThreadLocalPlatform::SetValueInSlot(SlotType slot, void* value) {
  int error = pthread_setspecific(slot, value);
  DCHECK_EQ(error, 0);
}

}  
}  

#endif  
