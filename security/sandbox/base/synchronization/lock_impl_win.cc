



#include "base/synchronization/lock_impl.h"

namespace base {
namespace internal {

LockImpl::LockImpl() {
  
  
  ::InitializeCriticalSectionAndSpinCount(&native_handle_, 2000);
}

LockImpl::~LockImpl() {
  ::DeleteCriticalSection(&native_handle_);
}

bool LockImpl::Try() {
  if (::TryEnterCriticalSection(&native_handle_) != FALSE) {
    return true;
  }
  return false;
}

void LockImpl::Lock() {
  ::EnterCriticalSection(&native_handle_);
}

void LockImpl::Unlock() {
  ::LeaveCriticalSection(&native_handle_);
}

}  
}  
