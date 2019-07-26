






























#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "breakpad_googletest_includes.h"
#include "client/mac/handler/exception_handler.h"
#include "common/mac/MachIPC.h"
#include "common/tests/auto_tempdir.h"
#include "google_breakpad/processor/minidump.h"

namespace google_breakpad {



std::ostringstream info_log;
}

namespace {
using std::string;
using google_breakpad::AutoTempDir;
using google_breakpad::ExceptionHandler;
using google_breakpad::MachPortSender;
using google_breakpad::MachReceiveMessage;
using google_breakpad::MachSendMessage;
using google_breakpad::Minidump;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpMemoryRegion;
using google_breakpad::ReceivePort;
using testing::Test;

class ExceptionHandlerTest : public Test {
 public:
  void InProcessCrash(bool aborting);
  AutoTempDir tempDir;
  string lastDumpName;
};

static void Crasher() {
  int *a = (int*)0x42;

  fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}

static void AbortCrasher() {
  fprintf(stdout, "Going to crash...\n");
  abort();
}

static void SoonToCrash(void(*crasher)()) {
  crasher();
}

static bool MDCallback(const char *dump_dir, const char *file_name,
                       void *context, bool success) {
  string path(dump_dir);
  path.append("/");
  path.append(file_name);
  path.append(".dmp");

  int fd = *reinterpret_cast<int*>(context);
  (void)write(fd, path.c_str(), path.length() + 1);
  close(fd);
  exit(0);
  
  return true;
}

void ExceptionHandlerTest::InProcessCrash(bool aborting) {
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));
  
  pid_t pid = fork();
  if (pid == 0) {
    
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    
    SoonToCrash(aborting ? &AbortCrasher : &Crasher);
    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);

  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  ASSERT_TRUE(exception);

  const MDRawExceptionStream* raw_exception = exception->exception();
  ASSERT_TRUE(raw_exception);

  if (aborting) {
    EXPECT_EQ(MD_EXCEPTION_MAC_SOFTWARE,
              raw_exception->exception_record.exception_code);
    EXPECT_EQ(MD_EXCEPTION_CODE_MAC_ABORT,
              raw_exception->exception_record.exception_flags);
  } else {
    EXPECT_EQ(MD_EXCEPTION_MAC_BAD_ACCESS,
              raw_exception->exception_record.exception_code);
#if defined(__x86_64__)
    EXPECT_EQ(MD_EXCEPTION_CODE_MAC_INVALID_ADDRESS,
              raw_exception->exception_record.exception_flags);
#elif defined(__i386__)
    EXPECT_EQ(MD_EXCEPTION_CODE_MAC_PROTECTION_FAILURE,
              raw_exception->exception_record.exception_flags);
#endif
  }

  const MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  
  
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(memory_list);
  MinidumpMemoryRegion* region =
      memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
}

TEST_F(ExceptionHandlerTest, InProcess) {
  InProcessCrash(false);
}

TEST_F(ExceptionHandlerTest, InProcessAbort) {
  InProcessCrash(true);
}

static bool DumpNameMDCallback(const char *dump_dir, const char *file_name,
                               void *context, bool success) {
  ExceptionHandlerTest *self = reinterpret_cast<ExceptionHandlerTest*>(context);
  if (dump_dir && file_name) {
    self->lastDumpName = dump_dir;
    self->lastDumpName += "/";
    self->lastDumpName += file_name;
    self->lastDumpName += ".dmp";
  }
  return true;
}

TEST_F(ExceptionHandlerTest, WriteMinidump) {
  ExceptionHandler eh(tempDir.path(), NULL, DumpNameMDCallback, this, true,
                      NULL);
  ASSERT_TRUE(eh.WriteMinidump());

  
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  
  Minidump minidump(lastDumpName);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  EXPECT_FALSE(exception);
}

TEST_F(ExceptionHandlerTest, WriteMinidumpWithException) {
  ExceptionHandler eh(tempDir.path(), NULL, DumpNameMDCallback, this, true,
                      NULL);
  ASSERT_TRUE(eh.WriteMinidump(true));

  
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  
  Minidump minidump(lastDumpName);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  ASSERT_TRUE(exception);
  const MDRawExceptionStream* raw_exception = exception->exception();
  ASSERT_TRUE(raw_exception);

  EXPECT_EQ(MD_EXCEPTION_MAC_BREAKPOINT,
            raw_exception->exception_record.exception_code);
}

TEST_F(ExceptionHandlerTest, DumpChildProcess) {
  const int kTimeoutMs = 2000;
  
  char machPortName[128];
  sprintf(machPortName, "ExceptionHandlerTest.%d", getpid());
  ReceivePort parent_recv_port(machPortName);

  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  
  pid_t pid = fork();
  if (pid == 0) {
    
    close(fds[1]);

    
    MachSendMessage child_message(0);
    child_message.AddDescriptor(mach_task_self());
    child_message.AddDescriptor(mach_thread_self());

    MachPortSender child_sender(machPortName);
    if (child_sender.SendMessage(child_message, kTimeoutMs) != KERN_SUCCESS)
      exit(1);

    
    uint8_t data;
    read(fds[0], &data, 1);
    exit(0);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[0]);

  
  MachReceiveMessage child_message;
  ASSERT_EQ(KERN_SUCCESS,
	    parent_recv_port.WaitForMessage(&child_message, kTimeoutMs));
  mach_port_t child_task = child_message.GetTranslatedPort(0);
  mach_port_t child_thread = child_message.GetTranslatedPort(1);
  ASSERT_NE((mach_port_t)MACH_PORT_NULL, child_task);
  ASSERT_NE((mach_port_t)MACH_PORT_NULL, child_thread);

  
  bool result = ExceptionHandler::WriteMinidumpForChild(child_task,
                                                        child_thread,
                                                        tempDir.path(),
                                                        DumpNameMDCallback,
                                                        this);
  ASSERT_EQ(true, result);

  
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  
  uint8_t data = 1;
  (void)write(fds[1], &data, 1);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
}



TEST_F(ExceptionHandlerTest, InstructionPointerMemory) {
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  
  
  const uint32_t kMemorySize = 256;  
  const int kOffset = kMemorySize / 2;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    
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
    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[1]);

  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  
  
  
  
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_NE((unsigned int)0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  EXPECT_EQ(kMemorySize, region->GetSize());
  const uint8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  uint8_t prefix_bytes[kOffset];
  uint8_t suffix_bytes[kMemorySize - kOffset - sizeof(instructions)];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);
}



TEST_F(ExceptionHandlerTest, InstructionPointerMemoryMinBound) {
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  
  
  const uint32_t kMemorySize = 256;  
  const int kOffset = 0;
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    
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
    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[1]);

  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  
  
  
  
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_NE((unsigned int)0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  EXPECT_EQ(kMemorySize / 2, region->GetSize());
  const uint8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  uint8_t suffix_bytes[kMemorySize / 2 - sizeof(instructions)];
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);
}



TEST_F(ExceptionHandlerTest, InstructionPointerMemoryMaxBound) {
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  
  
  
  
  
  const uint32_t kMemorySize = 4096;  
  
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  const int kOffset = kMemorySize - sizeof(instructions);

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    
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
    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[1]);

  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  
  
  
  
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_NE((unsigned int)0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  const size_t kPrefixSize = 128;  
  EXPECT_EQ(kPrefixSize + sizeof(instructions), region->GetSize());
  const uint8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  uint8_t prefix_bytes[kPrefixSize];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kPrefixSize,
                     instructions, sizeof(instructions)) == 0);
}



TEST_F(ExceptionHandlerTest, InstructionPointerMemoryNullPointer) {
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(NULL);
    memory_function();
    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[1]);

  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  
  
  
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_EQ((unsigned int)1, memory_list->region_count());
}

static void *Junk(void *) {
  sleep(1000000);
  return NULL;
}



TEST_F(ExceptionHandlerTest, MemoryListMultipleThreads) {
  
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);

    
    pthread_t junk_thread;
    if (pthread_create(&junk_thread, NULL, Junk, NULL) == 0)
      pthread_detach(junk_thread);

    
    Crasher();

    
    exit(1);
  }
  
  ASSERT_NE(-1, pid);
  close(fds[1]);

  
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(memory_list);
  
  
  ASSERT_EQ((unsigned int)3, memory_list->region_count());
}

}
