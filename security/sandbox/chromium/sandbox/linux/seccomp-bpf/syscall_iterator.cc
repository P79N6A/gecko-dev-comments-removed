



#include "sandbox/linux/seccomp-bpf/syscall_iterator.h"

#include "base/basictypes.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"

namespace sandbox {

uint32_t SyscallIterator::Next() {
  if (done_) {
    return num_;
  }

  uint32_t val;
  do {
    
    
    COMPILE_ASSERT(MIN_SYSCALL == 0u, min_syscall_should_always_be_zero);
    val = num_;

    
    
    if (num_ <= MAX_PUBLIC_SYSCALL) {
      if (invalid_only_ && num_ < MAX_PUBLIC_SYSCALL) {
        num_ = MAX_PUBLIC_SYSCALL;
      } else {
        ++num_;
      }
#if defined(__arm__)
      
      
      
    } else if (num_ < MIN_PRIVATE_SYSCALL - 1) {
      num_ = MIN_PRIVATE_SYSCALL - 1;
    } else if (num_ <= MAX_PRIVATE_SYSCALL) {
      if (invalid_only_ && num_ < MAX_PRIVATE_SYSCALL) {
        num_ = MAX_PRIVATE_SYSCALL;
      } else {
        ++num_;
      }
    } else if (num_ < MIN_GHOST_SYSCALL - 1) {
      num_ = MIN_GHOST_SYSCALL - 1;
    } else if (num_ <= MAX_SYSCALL) {
      if (invalid_only_ && num_ < MAX_SYSCALL) {
        num_ = MAX_SYSCALL;
      } else {
        ++num_;
      }
#endif






    } else if (num_ < 0x7FFFFFFFu) {
      num_ = 0x7FFFFFFFu;
    } else if (num_ < 0x80000000u) {
      num_ = 0x80000000u;
    } else if (num_ < 0xFFFFFFFFu) {
      num_ = 0xFFFFFFFFu;
    }
  } while (invalid_only_ && IsValid(val));

  done_ |= val == 0xFFFFFFFFu;
  return val;
}

bool SyscallIterator::IsValid(uint32_t num) {
  uint32_t min_syscall = MIN_SYSCALL;
  if (num >= min_syscall && num <= MAX_PUBLIC_SYSCALL) {
    return true;
  }
  if (IsArmPrivate(num)) {
    return true;
  }
  return false;
}

#if defined(__arm__) && (defined(__thumb__) || defined(__ARM_EABI__))
bool SyscallIterator::IsArmPrivate(uint32_t num) {
  return (num >= MIN_PRIVATE_SYSCALL && num <= MAX_PRIVATE_SYSCALL) ||
         (num >= MIN_GHOST_SYSCALL && num <= MAX_SYSCALL);
}
#else
bool SyscallIterator::IsArmPrivate(uint32_t) { return false; }
#endif

}  
