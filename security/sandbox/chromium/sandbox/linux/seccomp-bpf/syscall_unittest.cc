



#include <asm/unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <vector>

#include "base/basictypes.h"
#include "base/posix/eintr_wrapper.h"
#include "sandbox/linux/seccomp-bpf/bpf_tests.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"
#include "sandbox/linux/seccomp-bpf/syscall.h"
#include "sandbox/linux/tests/unit_tests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {

namespace {



#ifdef __NR_mmap2
const int kMMapNr = __NR_mmap2;
#else
const int kMMapNr = __NR_mmap;
#endif

TEST(Syscall, WellKnownEntryPoint) {



#if !defined(__arm__)
  EXPECT_NE(SandboxSyscall(-1), syscall(-1));
#endif



#if defined(__i386__)
  EXPECT_EQ(0x80CDu, ((uint16_t*)SandboxSyscall(-1))[-1]);  
#elif defined(__x86_64__)
  EXPECT_EQ(0x050Fu, ((uint16_t*)SandboxSyscall(-1))[-1]);  
#elif defined(__arm__)
#if defined(__thumb__)
  EXPECT_EQ(0xDF00u, ((uint16_t*)SandboxSyscall(-1))[-1]);  
#else
  EXPECT_EQ(0xEF000000u, ((uint32_t*)SandboxSyscall(-1))[-1]);  
#endif
#else
#warning Incomplete test case; need port for target platform
#endif
}

TEST(Syscall, TrivialSyscallNoArgs) {
  
  EXPECT_EQ(SandboxSyscall(__NR_getpid), syscall(__NR_getpid));
}

TEST(Syscall, TrivialSyscallOneArg) {
  int new_fd;
  
  ASSERT_GE(new_fd = SandboxSyscall(__NR_dup, 2), 0);
  int close_return_value = IGNORE_EINTR(SandboxSyscall(__NR_close, new_fd));
  ASSERT_EQ(close_return_value, 0);
}


intptr_t CopySyscallArgsToAux(const struct arch_seccomp_data& args, void* aux) {
  
  std::vector<uint64_t>* const seen_syscall_args =
      static_cast<std::vector<uint64_t>*>(aux);
  BPF_ASSERT(arraysize(args.args) == 6);
  seen_syscall_args->assign(args.args, args.args + arraysize(args.args));
  return -ENOMEM;
}

ErrorCode CopyAllArgsOnUnamePolicy(SandboxBPF* sandbox, int sysno, void* aux) {
  if (!SandboxBPF::IsValidSyscallNumber(sysno)) {
    return ErrorCode(ENOSYS);
  }
  if (sysno == __NR_uname) {
    return sandbox->Trap(CopySyscallArgsToAux, aux);
  } else {
    return ErrorCode(ErrorCode::ERR_ALLOWED);
  }
}



BPF_TEST(Syscall,
         SyntheticSixArgs,
         CopyAllArgsOnUnamePolicy,
         std::vector<uint64_t> ) {
  const int kExpectedValue = 42;
  
  
  
  
  int syscall_args[6];
  for (size_t i = 0; i < arraysize(syscall_args); ++i) {
    syscall_args[i] = kExpectedValue + i;
  }

  
  
  BPF_ASSERT(SandboxSyscall(__NR_uname,
                            syscall_args[0],
                            syscall_args[1],
                            syscall_args[2],
                            syscall_args[3],
                            syscall_args[4],
                            syscall_args[5]) == -ENOMEM);

  
  BPF_ASSERT(BPF_AUX.size() == 6);

  
  
  
  BPF_ASSERT(BPF_AUX[0] == static_cast<uint64_t>(syscall_args[0]));
  BPF_ASSERT(BPF_AUX[1] == static_cast<uint64_t>(syscall_args[1]));
  BPF_ASSERT(BPF_AUX[2] == static_cast<uint64_t>(syscall_args[2]));
  BPF_ASSERT(BPF_AUX[3] == static_cast<uint64_t>(syscall_args[3]));
  BPF_ASSERT(BPF_AUX[4] == static_cast<uint64_t>(syscall_args[4]));
  BPF_ASSERT(BPF_AUX[5] == static_cast<uint64_t>(syscall_args[5]));
}

TEST(Syscall, ComplexSyscallSixArgs) {
  int fd;
  ASSERT_LE(0, fd = SandboxSyscall(__NR_open, "/dev/null", O_RDWR, 0L));

  
  char* addr0;
  ASSERT_NE((char*)NULL,
            addr0 = reinterpret_cast<char*>(
                SandboxSyscall(kMMapNr,
                               (void*)NULL,
                               4096,
                               PROT_READ,
                               MAP_PRIVATE | MAP_ANONYMOUS,
                               fd,
                               0L)));

  
  char* addr1;
  ASSERT_EQ(addr0,
            addr1 = reinterpret_cast<char*>(
                SandboxSyscall(kMMapNr,
                               addr0,
                               4096L,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                               fd,
                               0L)));
  ++*addr1;  

  
  EXPECT_EQ(0, SandboxSyscall(__NR_munmap, addr1, 4096L));
  EXPECT_EQ(0, IGNORE_EINTR(SandboxSyscall(__NR_close, fd)));

  
  
  ASSERT_GE(fd = SandboxSyscall(__NR_open, "/proc/self/exe", O_RDONLY, 0L), 0);
  char* addr2, *addr3;
  ASSERT_NE((char*)NULL,
            addr2 = reinterpret_cast<char*>(SandboxSyscall(
                kMMapNr, (void*)NULL, 8192L, PROT_READ, MAP_PRIVATE, fd, 0L)));
  ASSERT_NE((char*)NULL,
            addr3 = reinterpret_cast<char*>(SandboxSyscall(kMMapNr,
                                                           (void*)NULL,
                                                           4096L,
                                                           PROT_READ,
                                                           MAP_PRIVATE,
                                                           fd,
#if defined(__NR_mmap2)
                                                           1L
#else
                                                           4096L
#endif
                                                           )));
  EXPECT_EQ(0, memcmp(addr2 + 4096, addr3, 4096));

  
  
  char buf[8192];
  EXPECT_EQ(8192, SandboxSyscall(__NR_read, fd, buf, 8192L));
  EXPECT_EQ(0, memcmp(addr2, buf, 8192));

  
  EXPECT_EQ(0, SandboxSyscall(__NR_munmap, addr2, 8192L));
  EXPECT_EQ(0, SandboxSyscall(__NR_munmap, addr3, 4096L));
  EXPECT_EQ(0, IGNORE_EINTR(SandboxSyscall(__NR_close, fd)));
}

}  

}  
