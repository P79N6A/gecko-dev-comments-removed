



#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"
#include "sandbox/linux/seccomp-bpf/syscall_iterator.h"
#include "sandbox/linux/tests/unit_tests.h"

namespace sandbox {

namespace {

SANDBOX_TEST(SyscallIterator, Monotonous) {
  for (int i = 0; i < 2; ++i) {
    bool invalid_only = !i;  
    SyscallIterator iter(invalid_only);
    uint32_t next = iter.Next();

    if (!invalid_only) {
      
      SANDBOX_ASSERT(next == 0);
    }
    for (uint32_t last = next; !iter.Done(); last = next) {
      next = iter.Next();
      SANDBOX_ASSERT(last < next);
    }
    
    SANDBOX_ASSERT(next == 0xFFFFFFFFu);
  }
}

SANDBOX_TEST(SyscallIterator, PublicSyscallRange) {
  SyscallIterator iter(false);
  uint32_t next = iter.Next();

  
  
  
  
  SANDBOX_ASSERT(MIN_SYSCALL == 0);
  SANDBOX_ASSERT(next == MIN_SYSCALL);
  for (uint32_t last = next; next < MAX_PUBLIC_SYSCALL + 1; last = next) {
    SANDBOX_ASSERT((next = iter.Next()) == last + 1);
  }
  SANDBOX_ASSERT(next == MAX_PUBLIC_SYSCALL + 1);
}

#if defined(__arm__)
SANDBOX_TEST(SyscallIterator, ARMPrivateSyscallRange) {
  SyscallIterator iter(false);
  uint32_t next = iter.Next();
  while (next < MIN_PRIVATE_SYSCALL - 1) {
    next = iter.Next();
  }
  
  
  SANDBOX_ASSERT(next == MIN_PRIVATE_SYSCALL - 1);
  for (uint32_t last = next; next < MAX_PRIVATE_SYSCALL + 1; last = next) {
    SANDBOX_ASSERT((next = iter.Next()) == last + 1);
  }
  SANDBOX_ASSERT(next == MAX_PRIVATE_SYSCALL + 1);
}

SANDBOX_TEST(SyscallIterator, ARMHiddenSyscallRange) {
  SyscallIterator iter(false);
  uint32_t next = iter.Next();
  while (next < MIN_GHOST_SYSCALL - 1) {
    next = iter.Next();
  }
  
  
  SANDBOX_ASSERT(next == MIN_GHOST_SYSCALL - 1);
  for (uint32_t last = next; next < MAX_SYSCALL + 1; last = next) {
    SANDBOX_ASSERT((next = iter.Next()) == last + 1);
  }
  SANDBOX_ASSERT(next == MAX_SYSCALL + 1);
}
#endif

SANDBOX_TEST(SyscallIterator, Invalid) {
  for (int i = 0; i < 2; ++i) {
    bool invalid_only = !i;  
    SyscallIterator iter(invalid_only);
    uint32_t next = iter.Next();

    while (next < MAX_SYSCALL + 1) {
      next = iter.Next();
    }

    SANDBOX_ASSERT(next == MAX_SYSCALL + 1);
    while (next < 0x7FFFFFFFu) {
      next = iter.Next();
    }

    
    SANDBOX_ASSERT(next == 0x7FFFFFFFu);
    next = iter.Next();
    SANDBOX_ASSERT(next == 0x80000000u);
    SANDBOX_ASSERT(!iter.Done());
    next = iter.Next();
    SANDBOX_ASSERT(iter.Done());
    SANDBOX_ASSERT(next == 0xFFFFFFFFu);
  }
}

SANDBOX_TEST(SyscallIterator, InvalidOnly) {
  bool invalid_only = true;
  SyscallIterator iter(invalid_only);
  uint32_t next = iter.Next();
  
  
  
  SANDBOX_ASSERT(MIN_SYSCALL == 0);
  SANDBOX_ASSERT(next == MAX_PUBLIC_SYSCALL + 1);

#if defined(__arm__)
  next = iter.Next();
  
  SANDBOX_ASSERT(next == MIN_PRIVATE_SYSCALL - 1);
  while (next <= MAX_PRIVATE_SYSCALL) {
    next = iter.Next();
  }

  next = iter.Next();
  
  SANDBOX_ASSERT(next == MIN_GHOST_SYSCALL - 1);
  while (next <= MAX_SYSCALL) {
    next = iter.Next();
  }
  SANDBOX_ASSERT(next == MAX_SYSCALL + 1);
#endif
}

}  

}  
