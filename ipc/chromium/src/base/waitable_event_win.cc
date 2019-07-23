



#include "base/waitable_event.h"

#include <math.h>
#include <windows.h>

#include "base/logging.h"
#include "base/time.h"

namespace base {

WaitableEvent::WaitableEvent(bool manual_reset, bool signaled)
    : handle_(CreateEvent(NULL, manual_reset, signaled, NULL)) {
  
  
  CHECK(handle_);
}

WaitableEvent::WaitableEvent(HANDLE handle)
    : handle_(handle) {
  CHECK(handle) << "Tried to create WaitableEvent from NULL handle";
}

WaitableEvent::~WaitableEvent() {
  CloseHandle(handle_);
}

HANDLE WaitableEvent::Release() {
  HANDLE rv = handle_;
  handle_ = INVALID_HANDLE_VALUE;
  return rv;
}

void WaitableEvent::Reset() {
  ResetEvent(handle_);
}

void WaitableEvent::Signal() {
  SetEvent(handle_);
}

bool WaitableEvent::IsSignaled() {
  return TimedWait(TimeDelta::FromMilliseconds(0));
}

bool WaitableEvent::Wait() {
  DWORD result = WaitForSingleObject(handle_, INFINITE);
  
  
  DCHECK(result == WAIT_OBJECT_0) << "WaitForSingleObject failed";
  return result == WAIT_OBJECT_0;
}

bool WaitableEvent::TimedWait(const TimeDelta& max_time) {
  DCHECK(max_time >= TimeDelta::FromMicroseconds(0));
  
  
  
  double timeout = ceil(max_time.InMillisecondsF());
  DWORD result = WaitForSingleObject(handle_, static_cast<DWORD>(timeout));
  switch (result) {
    case WAIT_OBJECT_0:
      return true;
    case WAIT_TIMEOUT:
      return false;
  }
  
  
  NOTREACHED() << "WaitForSingleObject failed";
  return false;
}


size_t WaitableEvent::WaitMany(WaitableEvent** events, size_t count) {
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  CHECK(count <= MAXIMUM_WAIT_OBJECTS)
      << "Can only wait on " << MAXIMUM_WAIT_OBJECTS << " with WaitMany";

  for (size_t i = 0; i < count; ++i)
    handles[i] = events[i]->handle();

  DWORD result =
      WaitForMultipleObjects(count, handles,
                             FALSE,      
                             INFINITE);  
  if (result < WAIT_OBJECT_0 || result >= WAIT_OBJECT_0 + count) {
    NOTREACHED() << "WaitForMultipleObjects failed: " << GetLastError();
    return 0;
  }

  return result - WAIT_OBJECT_0;
}

}  
