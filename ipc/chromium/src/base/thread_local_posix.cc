



#include "base/thread_local.h"

#include <pthread.h>

#include "base/logging.h"

namespace base {


void ThreadLocalPlatform::AllocateSlot(SlotType& slot) {
  int error = pthread_key_create(&slot, NULL);
  CHECK(error == 0);
}


void ThreadLocalPlatform::FreeSlot(SlotType& slot) {
  int error = pthread_key_delete(slot);
  DCHECK(error == 0);
}


void* ThreadLocalPlatform::GetValueFromSlot(SlotType& slot) {
  return pthread_getspecific(slot);
}


void ThreadLocalPlatform::SetValueInSlot(SlotType& slot, void* value) {
  int error = pthread_setspecific(slot, value);
  CHECK(error == 0);
}

}  
