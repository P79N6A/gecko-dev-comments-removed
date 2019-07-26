




























#include <string>

#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/linux/eintr_wrapper.h"
#include "common/linux/linux_libc_support.h"
#include "common/linux/linux_syscall_support.h"
#include "google_breakpad/processor/minidump.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;

static void sigchld_handler(int signo) { }

class ExceptionHandlerTest : public ::testing::Test {
 protected:
  void SetUp() {
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    ASSERT_NE(sigaction(SIGCHLD, &sa, &old_action), -1);
  }

  void TearDown() {
    sigaction(SIGCHLD, &old_action, NULL);
  }

  struct sigaction old_action;
};

TEST(ExceptionHandlerTest, Simple) {
  ExceptionHandler handler("/tmp", NULL, NULL, NULL, true);
}

static bool DoneCallback(const char* dump_path,
                         const char* minidump_id,
                         void* context,
                         bool succeeded) {
  if (!succeeded)
    return succeeded;

  int fd = (intptr_t) context;
  uint32_t len = my_strlen(minidump_id);
  HANDLE_EINTR(sys_write(fd, &len, sizeof(len)));
  HANDLE_EINTR(sys_write(fd, minidump_id, len));
  sys_close(fd);

  return true;
}

TEST(ExceptionHandlerTest, ChildCrash) {
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp", NULL, DoneCallback, (void*) fds[1],
                             true);
    *reinterpret_cast<int*>(NULL) = 0;
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGSEGV);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = std::string("/tmp/") + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);
  unlink(minidump_filename.c_str());
  free(filename);
}



TEST(ExceptionHandlerTest, InstructionPointerMemory) {
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  
  
  const u_int32_t kMemorySize = 256;  
  const int kOffset = kMemorySize / 2;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp", NULL, DoneCallback, (void*) fds[1],
                             true);
    
    char* memory =
      reinterpret_cast<char*>(mmap(NULL,
                                   kMemorySize,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON,
                                   -1,
                                   0));
    if (!memory)
      exit(0);

    
    
    
    memcpy(memory + kOffset, instructions, sizeof(instructions));
    
    
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
    memory_function();
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGILL);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = std::string("/tmp/") + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  
  
  
  
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_LT(0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  u_int64_t instruction_pointer;
  switch (context->GetContextCPU()) {
  case MD_CONTEXT_X86:
    instruction_pointer = context->GetContextX86()->eip;
    break;
  case MD_CONTEXT_AMD64:
    instruction_pointer = context->GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    instruction_pointer = context->GetContextARM()->iregs[15];
    break;
  default:
    FAIL() << "Unknown context CPU: " << context->GetContextCPU();
    break;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  ASSERT_TRUE(region);

  EXPECT_EQ(kMemorySize, region->GetSize());
  const u_int8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  u_int8_t prefix_bytes[kOffset];
  u_int8_t suffix_bytes[kMemorySize - kOffset - sizeof(instructions)];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);

  unlink(minidump_filename.c_str());
  free(filename);
}



TEST(ExceptionHandlerTest, InstructionPointerMemoryMinBound) {
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  
  
  const u_int32_t kMemorySize = 256;  
  const int kOffset = 0;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp", NULL, DoneCallback, (void*) fds[1],
                             true);
    
    char* memory =
      reinterpret_cast<char*>(mmap(NULL,
                                   kMemorySize,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON,
                                   -1,
                                   0));
    if (!memory)
      exit(0);

    
    
    
    memcpy(memory + kOffset, instructions, sizeof(instructions));
    
    
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
    memory_function();
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGILL);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = std::string("/tmp/") + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  
  
  
  
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_LT(0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  u_int64_t instruction_pointer;
  switch (context->GetContextCPU()) {
  case MD_CONTEXT_X86:
    instruction_pointer = context->GetContextX86()->eip;
    break;
  case MD_CONTEXT_AMD64:
    instruction_pointer = context->GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    instruction_pointer = context->GetContextARM()->iregs[15];
    break;
  default:
    FAIL() << "Unknown context CPU: " << context->GetContextCPU();
    break;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  ASSERT_TRUE(region);

  EXPECT_EQ(kMemorySize / 2, region->GetSize());
  const u_int8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  u_int8_t suffix_bytes[kMemorySize / 2 - sizeof(instructions)];
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);

  unlink(minidump_filename.c_str());
  free(filename);
}



TEST(ExceptionHandlerTest, InstructionPointerMemoryMaxBound) {
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  
  
  
  
  
  const u_int32_t kMemorySize = 4096;  
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  const int kOffset = kMemorySize - sizeof(instructions);

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp", NULL, DoneCallback, (void*) fds[1],
                             true);
    
    char* memory =
      reinterpret_cast<char*>(mmap(NULL,
                                   kMemorySize,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON,
                                   -1,
                                   0));
    if (!memory)
      exit(0);

    
    
    
    memcpy(memory + kOffset, instructions, sizeof(instructions));
    
    
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
    memory_function();
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGILL);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = std::string("/tmp/") + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  
  
  
  
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_LT(0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  u_int64_t instruction_pointer;
  switch (context->GetContextCPU()) {
  case MD_CONTEXT_X86:
    instruction_pointer = context->GetContextX86()->eip;
    break;
  case MD_CONTEXT_AMD64:
    instruction_pointer = context->GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    instruction_pointer = context->GetContextARM()->iregs[15];
    break;
  default:
    FAIL() << "Unknown context CPU: " << context->GetContextCPU();
    break;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  ASSERT_TRUE(region);

  const size_t kPrefixSize = 128;  
  EXPECT_EQ(kPrefixSize + sizeof(instructions), region->GetSize());
  const u_int8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  u_int8_t prefix_bytes[kPrefixSize];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kPrefixSize,
                     instructions, sizeof(instructions)) == 0);

  unlink(minidump_filename.c_str());
  free(filename);
}



TEST(ExceptionHandlerTest, InstructionPointerMemoryNullPointer) {
  int fds[2];
  ASSERT_NE(pipe(fds), -1);


  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp", NULL, DoneCallback, (void*) fds[1],
                             true);
    
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(NULL);
    memory_function();
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGSEGV);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = std::string("/tmp/") + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  
  
  
  
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_EQ((unsigned int)1, memory_list->region_count());

  unlink(minidump_filename.c_str());
  free(filename);
}

static bool SimpleCallback(const char* dump_path,
                           const char* minidump_id,
                           void* context,
                           bool succeeded) {
  if (!succeeded)
    return succeeded;

  string* minidump_file = reinterpret_cast<string*>(context);
  minidump_file->append(dump_path);
  minidump_file->append("/");
  minidump_file->append(minidump_id);
  minidump_file->append(".dmp");
  return true;
}


TEST(ExceptionHandlerTest, ModuleInfo) {
  
  
  const u_int32_t kMemorySize = sysconf(_SC_PAGESIZE);
  const char* kMemoryName = "a fake module";
  const u_int8_t kModuleGUID[sizeof(MDGUID)] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };
  char module_identifier_buffer[37];
  FileID::ConvertIdentifierToString(kModuleGUID,
                                    module_identifier_buffer,
                                    sizeof(module_identifier_buffer));
  string module_identifier(module_identifier_buffer);
  
  size_t pos;
  while ((pos = module_identifier.find('-')) != string::npos) {
    module_identifier.erase(pos, 1);
  }
  
  
  module_identifier += "0";

  
  char* memory =
    reinterpret_cast<char*>(mmap(NULL,
                                 kMemorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANON,
                                 -1,
                                 0));
  const u_int64_t kMemoryAddress = reinterpret_cast<u_int64_t>(memory);
  ASSERT_TRUE(memory);

  string minidump_filename;
  ExceptionHandler handler(TEMPDIR, NULL, SimpleCallback,
                           (void*)&minidump_filename, true);
  
  handler.AddMappingInfo(kMemoryName,
                         kModuleGUID,
                         kMemoryAddress,
                         kMemorySize,
                         0);
  handler.WriteMinidump();

  
  
  
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* module =
    module_list->GetModuleForAddress(kMemoryAddress);
  ASSERT_TRUE(module);

  EXPECT_EQ(kMemoryAddress, module->base_address());
  EXPECT_EQ(kMemorySize, module->size());
  EXPECT_EQ(kMemoryName, module->code_file());
  EXPECT_EQ(module_identifier, module->debug_identifier());

  unlink(minidump_filename.c_str());
}

static const unsigned kControlMsgSize =
    CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct ucred));

static bool
CrashHandler(const void* crash_context, size_t crash_context_size,
             void* context) {
  const int fd = (intptr_t) context;
  int fds[2];
  pipe(fds);
  struct kernel_msghdr msg = {0};
  struct kernel_iovec iov;
  iov.iov_base = const_cast<void*>(crash_context);
  iov.iov_len = crash_context_size;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  char cmsg[kControlMsgSize];
  memset(cmsg, 0, kControlMsgSize);
  msg.msg_control = cmsg;
  msg.msg_controllen = sizeof(cmsg);

  struct cmsghdr *hdr = CMSG_FIRSTHDR(&msg);
  hdr->cmsg_level = SOL_SOCKET;
  hdr->cmsg_type = SCM_RIGHTS;
  hdr->cmsg_len = CMSG_LEN(sizeof(int));
  *((int*) CMSG_DATA(hdr)) = fds[1];
  hdr = CMSG_NXTHDR((struct msghdr*) &msg, hdr);
  hdr->cmsg_level = SOL_SOCKET;
  hdr->cmsg_type = SCM_CREDENTIALS;
  hdr->cmsg_len = CMSG_LEN(sizeof(struct ucred));
  struct ucred *cred = reinterpret_cast<struct ucred*>(CMSG_DATA(hdr));
  cred->uid = getuid();
  cred->gid = getgid();
  cred->pid = getpid();

  HANDLE_EINTR(sys_sendmsg(fd, &msg, 0));
  sys_close(fds[1]);

  char b;
  HANDLE_EINTR(sys_read(fds[0], &b, 1));

  return true;
}

TEST(ExceptionHandlerTest, ExternalDumper) {
  int fds[2];
  ASSERT_NE(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds), -1);
  static const int on = 1;
  setsockopt(fds[0], SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));
  setsockopt(fds[1], SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp1", NULL, NULL, (void*) fds[1], true);
    handler.set_crash_handler(CrashHandler);
    *reinterpret_cast<int*>(NULL) = 0;
  }
  close(fds[1]);
  struct msghdr msg = {0};
  struct iovec iov;
  static const unsigned kCrashContextSize =
      sizeof(ExceptionHandler::CrashContext);
  char context[kCrashContextSize];
  char control[kControlMsgSize];
  iov.iov_base = context;
  iov.iov_len = kCrashContextSize;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = kControlMsgSize;

  const ssize_t n = HANDLE_EINTR(recvmsg(fds[0], &msg, 0));
  ASSERT_EQ(n, kCrashContextSize);
  ASSERT_EQ(msg.msg_controllen, kControlMsgSize);
  ASSERT_EQ(msg.msg_flags, 0);

  pid_t crashing_pid = -1;
  int signal_fd = -1;
  for (struct cmsghdr *hdr = CMSG_FIRSTHDR(&msg); hdr;
       hdr = CMSG_NXTHDR(&msg, hdr)) {
    if (hdr->cmsg_level != SOL_SOCKET)
      continue;
    if (hdr->cmsg_type == SCM_RIGHTS) {
      const unsigned len = hdr->cmsg_len -
          (((uint8_t*)CMSG_DATA(hdr)) - (uint8_t*)hdr);
      ASSERT_EQ(len, sizeof(int));
      signal_fd = *((int *) CMSG_DATA(hdr));
    } else if (hdr->cmsg_type == SCM_CREDENTIALS) {
      const struct ucred *cred =
          reinterpret_cast<struct ucred*>(CMSG_DATA(hdr));
      crashing_pid = cred->pid;
    }
  }

  ASSERT_NE(crashing_pid, -1);
  ASSERT_NE(signal_fd, -1);

  char templ[] = "/tmp/exception-handler-unittest-XXXXXX";
  mktemp(templ);
  ASSERT_TRUE(WriteMinidump(templ, crashing_pid, context,
                            kCrashContextSize));
  static const char b = 0;
  HANDLE_EINTR(write(signal_fd, &b, 1));

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGSEGV);

  struct stat st;
  ASSERT_EQ(stat(templ, &st), 0);
  ASSERT_GT(st.st_size, 0u);
  unlink(templ);
}
