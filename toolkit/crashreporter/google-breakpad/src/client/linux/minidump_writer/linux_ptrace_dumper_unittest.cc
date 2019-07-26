


































#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/linux/minidump_writer/linux_ptrace_dumper.h"
#include "client/linux/minidump_writer/minidump_writer_unittest_utils.h"
#include "common/linux/eintr_wrapper.h"
#include "common/linux/file_id.h"
#include "common/linux/ignore_ret.h"
#include "common/linux/safe_readlink.h"
#include "common/memory.h"
#include "common/using_std_string.h"

using namespace google_breakpad;

namespace {

typedef testing::Test LinuxPtraceDumperTest;

}  

TEST(LinuxPtraceDumperTest, Setup) {
  LinuxPtraceDumper dumper(getpid());
}

TEST(LinuxPtraceDumperTest, FindMappings) {
  LinuxPtraceDumper dumper(getpid());
  ASSERT_TRUE(dumper.Init());

  ASSERT_TRUE(dumper.FindMapping(reinterpret_cast<void*>(getpid)));
  ASSERT_TRUE(dumper.FindMapping(reinterpret_cast<void*>(printf)));
  ASSERT_FALSE(dumper.FindMapping(NULL));
}

TEST(LinuxPtraceDumperTest, ThreadList) {
  LinuxPtraceDumper dumper(getpid());
  ASSERT_TRUE(dumper.Init());

  ASSERT_GE(dumper.threads().size(), (size_t)1);
  bool found = false;
  for (size_t i = 0; i < dumper.threads().size(); ++i) {
    if (dumper.threads()[i] == getpid()) {
      found = true;
      break;
    }
  }
}



class StackHelper {
 public:
  StackHelper(int fd, char* mapping, size_t size)
    : fd_(fd), mapping_(mapping), size_(size) {}
  ~StackHelper() {
    munmap(mapping_, size_);
    close(fd_);
  }

 private:
  int fd_;
  char* mapping_;
  size_t size_;
};

TEST(LinuxPtraceDumperTest, MergedMappings) {
  string helper_path(GetHelperBinary());
  if (helper_path.empty()) {
    FAIL() << "Couldn't find helper binary";
    exit(1);
  }

  
  
  const size_t kPageSize = sysconf(_SC_PAGESIZE);
  const size_t kMappingSize = 3 * kPageSize;
  int fd = open(helper_path.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd) << "Failed to open file: " << helper_path
                    << ", Error: " << strerror(errno);
  char* mapping =
    reinterpret_cast<char*>(mmap(NULL,
                                 kMappingSize,
                                 PROT_READ,
                                 MAP_SHARED,
                                 fd,
                                 0));
  ASSERT_TRUE(mapping);

  const uintptr_t kMappingAddress = reinterpret_cast<uintptr_t>(mapping);

  
  StackHelper helper(fd, mapping, kMappingSize);

  
  char* inside_mapping =  reinterpret_cast<char*>(
      mmap(mapping + 2 *kPageSize,
           kPageSize,
           PROT_NONE,
           MAP_SHARED | MAP_FIXED,
           fd,
           
           
           kPageSize));
  ASSERT_TRUE(inside_mapping);

  
  LinuxPtraceDumper dumper(getpid());
  ASSERT_TRUE(dumper.Init());
  int mapping_count = 0;
  for (unsigned i = 0; i < dumper.mappings().size(); ++i) {
    const MappingInfo& mapping = *dumper.mappings()[i];
    if (strcmp(mapping.name, helper_path.c_str()) == 0) {
      
      
      EXPECT_EQ(kMappingAddress, mapping.start_addr);
      EXPECT_EQ(kMappingSize, mapping.size);
      EXPECT_EQ(0U, mapping.offset);
      mapping_count++;
    }
  }
  EXPECT_EQ(1, mapping_count);
}

TEST(LinuxPtraceDumperTest, VerifyStackReadWithMultipleThreads) {
  static const int kNumberOfThreadsInHelperProgram = 5;
  char kNumberOfThreadsArgument[2];
  sprintf(kNumberOfThreadsArgument, "%d", kNumberOfThreadsInHelperProgram);

  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  pid_t child_pid = fork();
  if (child_pid == 0) {
    
    close(fds[0]);

    string helper_path(GetHelperBinary());
    if (helper_path.empty()) {
      FAIL() << "Couldn't find helper binary";
      exit(1);
    }

    
    char pipe_fd_string[8];
    sprintf(pipe_fd_string, "%d", fds[1]);
    execl(helper_path.c_str(),
          "linux_dumper_unittest_helper",
          pipe_fd_string,
          kNumberOfThreadsArgument,
          NULL);
    
    printf("Errno from exec: %d", errno);
    FAIL() << "Exec of " << helper_path << " failed: " << strerror(errno);
    exit(0);
  }
  close(fds[1]);

  
  for (int threads = 0; threads < kNumberOfThreadsInHelperProgram; threads++) {
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fds[0];
    pfd.events = POLLIN | POLLERR;

    const int r = HANDLE_EINTR(poll(&pfd, 1, 1000));
    ASSERT_EQ(1, r);
    ASSERT_TRUE(pfd.revents & POLLIN);
    uint8_t junk;
    ASSERT_EQ(read(fds[0], &junk, sizeof(junk)), 
              static_cast<ssize_t>(sizeof(junk)));
  }
  close(fds[0]);

  
  
  
  usleep(100000);

  
  LinuxPtraceDumper dumper(child_pid);
  ASSERT_TRUE(dumper.Init());
  EXPECT_EQ((size_t)kNumberOfThreadsInHelperProgram, dumper.threads().size());
  EXPECT_TRUE(dumper.ThreadsSuspend());

  ThreadInfo one_thread;
  for (size_t i = 0; i < dumper.threads().size(); ++i) {
    EXPECT_TRUE(dumper.GetThreadInfoByIndex(i, &one_thread));
    const void* stack;
    size_t stack_len;
    EXPECT_TRUE(dumper.GetStackInfo(&stack, &stack_len,
        one_thread.stack_pointer));
    
    
#if defined(__ARM_EABI__)
    pid_t *process_tid_location = (pid_t *)(one_thread.regs.uregs[3]);
#elif defined(__i386)
    pid_t *process_tid_location = (pid_t *)(one_thread.regs.ecx);
#elif defined(__x86_64)
    pid_t *process_tid_location = (pid_t *)(one_thread.regs.rcx);
#else
#error This test has not been ported to this platform.
#endif
    pid_t one_thread_id;
    dumper.CopyFromProcess(&one_thread_id,
                           dumper.threads()[i],
                           process_tid_location,
                           4);
    EXPECT_EQ(dumper.threads()[i], one_thread_id);
  }
  EXPECT_TRUE(dumper.ThreadsResume());
  kill(child_pid, SIGKILL);

  
  int status;
  ASSERT_NE(-1, HANDLE_EINTR(waitpid(child_pid, &status, 0)));
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(SIGKILL, WTERMSIG(status));
}

TEST(LinuxPtraceDumperTest, BuildProcPath) {
  const pid_t pid = getpid();
  LinuxPtraceDumper dumper(pid);

  char maps_path[NAME_MAX] = "";
  char maps_path_expected[NAME_MAX];
  snprintf(maps_path_expected, sizeof(maps_path_expected),
           "/proc/%d/maps", pid);
  EXPECT_TRUE(dumper.BuildProcPath(maps_path, pid, "maps"));
  EXPECT_STREQ(maps_path_expected, maps_path);

  EXPECT_FALSE(dumper.BuildProcPath(NULL, pid, "maps"));
  EXPECT_FALSE(dumper.BuildProcPath(maps_path, 0, "maps"));
  EXPECT_FALSE(dumper.BuildProcPath(maps_path, pid, ""));
  EXPECT_FALSE(dumper.BuildProcPath(maps_path, pid, NULL));

  char long_node[NAME_MAX];
  size_t long_node_len = NAME_MAX - strlen("/proc/123") - 1;
  memset(long_node, 'a', long_node_len);
  long_node[long_node_len] = '\0';
  EXPECT_FALSE(dumper.BuildProcPath(maps_path, 123, long_node));
}

#if !defined(__ARM_EABI__)

TEST(LinuxPtraceDumperTest, MappingsIncludeLinuxGate) {
  LinuxPtraceDumper dumper(getpid());
  ASSERT_TRUE(dumper.Init());

  void* linux_gate_loc =
    reinterpret_cast<void *>(dumper.auxv()[AT_SYSINFO_EHDR]);
  ASSERT_TRUE(linux_gate_loc);
  bool found_linux_gate = false;

  const wasteful_vector<MappingInfo*> mappings = dumper.mappings();
  const MappingInfo* mapping;
  for (unsigned i = 0; i < mappings.size(); ++i) {
    mapping = mappings[i];
    if (!strcmp(mapping->name, kLinuxGateLibraryName)) {
      found_linux_gate = true;
      break;
    }
  }
  EXPECT_TRUE(found_linux_gate);
  EXPECT_EQ(linux_gate_loc, reinterpret_cast<void*>(mapping->start_addr));
  EXPECT_EQ(0, memcmp(linux_gate_loc, ELFMAG, SELFMAG));
}


TEST(LinuxPtraceDumperTest, LinuxGateMappingID) {
  LinuxPtraceDumper dumper(getpid());
  ASSERT_TRUE(dumper.Init());

  bool found_linux_gate = false;
  const wasteful_vector<MappingInfo*> mappings = dumper.mappings();
  unsigned index = 0;
  for (unsigned i = 0; i < mappings.size(); ++i) {
    if (!strcmp(mappings[i]->name, kLinuxGateLibraryName)) {
      found_linux_gate = true;
      index = i;
      break;
    }
  }
  ASSERT_TRUE(found_linux_gate);

  uint8_t identifier[sizeof(MDGUID)];
  ASSERT_TRUE(dumper.ElfFileIdentifierForMapping(*mappings[index],
                                                 true,
                                                 index,
                                                 identifier));
  uint8_t empty_identifier[sizeof(MDGUID)];
  memset(empty_identifier, 0, sizeof(empty_identifier));
  EXPECT_NE(0, memcmp(empty_identifier, identifier, sizeof(identifier)));
}



TEST(LinuxPtraceDumperTest, LinuxGateMappingIDChild) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    
    char b;
    IGNORE_RET(HANDLE_EINTR(read(fds[0], &b, sizeof(b))));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  LinuxPtraceDumper dumper(child);
  ASSERT_TRUE(dumper.Init());

  bool found_linux_gate = false;
  const wasteful_vector<MappingInfo*> mappings = dumper.mappings();
  unsigned index = 0;
  for (unsigned i = 0; i < mappings.size(); ++i) {
    if (!strcmp(mappings[i]->name, kLinuxGateLibraryName)) {
      found_linux_gate = true;
      index = i;
      break;
    }
  }
  ASSERT_TRUE(found_linux_gate);

  
  ASSERT_TRUE(dumper.ThreadsSuspend());
  uint8_t identifier[sizeof(MDGUID)];
  ASSERT_TRUE(dumper.ElfFileIdentifierForMapping(*mappings[index],
                                                 true,
                                                 index,
                                                 identifier));
  uint8_t empty_identifier[sizeof(MDGUID)];
  memset(empty_identifier, 0, sizeof(empty_identifier));
  EXPECT_NE(0, memcmp(empty_identifier, identifier, sizeof(identifier)));
  EXPECT_TRUE(dumper.ThreadsResume());
  close(fds[1]);
}
#endif

TEST(LinuxPtraceDumperTest, FileIDsMatch) {
  
  
  
  char exe_name[PATH_MAX];
  ASSERT_TRUE(SafeReadLink("/proc/self/exe", exe_name));

  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    
    char b;
    IGNORE_RET(HANDLE_EINTR(read(fds[0], &b, sizeof(b))));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  LinuxPtraceDumper dumper(child);
  ASSERT_TRUE(dumper.Init());
  const wasteful_vector<MappingInfo*> mappings = dumper.mappings();
  bool found_exe = false;
  unsigned i;
  for (i = 0; i < mappings.size(); ++i) {
    const MappingInfo* mapping = mappings[i];
    if (!strcmp(mapping->name, exe_name)) {
      found_exe = true;
      break;
    }
  }
  ASSERT_TRUE(found_exe);

  uint8_t identifier1[sizeof(MDGUID)];
  uint8_t identifier2[sizeof(MDGUID)];
  EXPECT_TRUE(dumper.ElfFileIdentifierForMapping(*mappings[i], true, i,
                                                 identifier1));
  FileID fileid(exe_name);
  EXPECT_TRUE(fileid.ElfFileIdentifier(identifier2));
  char identifier_string1[37];
  char identifier_string2[37];
  FileID::ConvertIdentifierToString(identifier1, identifier_string1,
                                    37);
  FileID::ConvertIdentifierToString(identifier2, identifier_string2,
                                    37);
  EXPECT_STREQ(identifier_string1, identifier_string2);
  close(fds[1]);
}
