



#include "sandbox/linux/seccomp-bpf/syscall_iterator.h"

#include "base/logging.h"
#include "base/macros.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"

namespace sandbox {

namespace {

#if defined(__mips__) && (_MIPS_SIM == _MIPS_SIM_ABI32)

COMPILE_ASSERT(MIN_SYSCALL == __NR_Linux, min_syscall_should_be_4000);
#else

COMPILE_ASSERT(MIN_SYSCALL == 0u, min_syscall_should_always_be_zero);
#endif


struct SyscallRange {
  uint32_t first;
  uint32_t last;
};

const SyscallRange kValidSyscallRanges[] = {
    
    
    {MIN_SYSCALL, MAX_PUBLIC_SYSCALL},
#if defined(__arm__)
    
    
    
    {MIN_PRIVATE_SYSCALL, MAX_PRIVATE_SYSCALL},
    {MIN_GHOST_SYSCALL, MAX_SYSCALL},
#endif
};

}  

SyscallSet::Iterator SyscallSet::begin() const {
  return Iterator(set_, false);
}

SyscallSet::Iterator SyscallSet::end() const {
  return Iterator(set_, true);
}

bool SyscallSet::IsValid(uint32_t num) {
  for (const SyscallRange& range : kValidSyscallRanges) {
    if (num >= range.first && num <= range.last) {
      return true;
    }
  }
  return false;
}

bool operator==(const SyscallSet& lhs, const SyscallSet& rhs) {
  return (lhs.set_ == rhs.set_);
}

SyscallSet::Iterator::Iterator(Set set, bool done)
    : set_(set), done_(done), num_(0) {
  
  if (!done && set_ == (IsValid(num_) ? Set::INVALID_ONLY : Set::VALID_ONLY)) {
    ++*this;
  }
}

uint32_t SyscallSet::Iterator::operator*() const {
  DCHECK(!done_);
  return num_;
}

SyscallSet::Iterator& SyscallSet::Iterator::operator++() {
  DCHECK(!done_);

  num_ = NextSyscall();
  if (num_ == 0) {
    done_ = true;
  }

  return *this;
}



uint32_t SyscallSet::Iterator::NextSyscall() const {
  const bool want_valid = (set_ != Set::INVALID_ONLY);
  const bool want_invalid = (set_ != Set::VALID_ONLY);

  for (const SyscallRange& range : kValidSyscallRanges) {
    if (want_invalid && range.first > 0 && num_ < range.first - 1) {
      
      
      return range.first - 1;
    }
    if (want_valid && num_ < range.first) {
      return range.first;
    }
    if (want_valid && num_ < range.last) {
      return num_ + 1;
    }
    if (want_invalid && num_ <= range.last) {
      return range.last + 1;
    }
  }

  if (want_invalid) {
    
    
    
    
    
    
    
    if (num_ < 0x7FFFFFFFu)
      return 0x7FFFFFFFu;
    if (num_ < 0x80000000u)
      return 0x80000000u;

    if (num_ < 0xFFFFFFFFu)
      return 0xFFFFFFFFu;
  }

  return 0;
}

bool operator==(const SyscallSet::Iterator& lhs,
                const SyscallSet::Iterator& rhs) {
  DCHECK(lhs.set_ == rhs.set_);
  return (lhs.done_ == rhs.done_) && (lhs.num_ == rhs.num_);
}

bool operator!=(const SyscallSet::Iterator& lhs,
                const SyscallSet::Iterator& rhs) {
  return !(lhs == rhs);
}

}  
