




























#include <fcntl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include <ucontext.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/linux/handler/exception_handler.h"
#include "client/linux/minidump_writer/linux_dumper.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "client/linux/minidump_writer/minidump_writer_unittest_utils.h"
#include "common/linux/eintr_wrapper.h"
#include "common/linux/file_id.h"
#include "common/linux/ignore_ret.h"
#include "common/linux/safe_readlink.h"
#include "common/tests/auto_tempdir.h"
#include "common/tests/file_utils.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/minidump.h"
#include "processor/scoped_ptr.h"

using namespace google_breakpad;



const int kGUIDStringSize = 37;

namespace {
typedef testing::Test MinidumpWriterTest;
}

TEST(MinidumpWriterTest, SetupWithPath) {
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

  ExceptionHandler::CrashContext context;
  memset(&context, 0, sizeof(context));

  AutoTempDir temp_dir;
  string templ = temp_dir.path() + "/minidump-writer-unittest";
  
  context.tid = 1;
  ASSERT_TRUE(WriteMinidump(templ.c_str(), child, &context, sizeof(context)));
  struct stat st;
  ASSERT_EQ(stat(templ.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  close(fds[1]);
}

TEST(MinidumpWriterTest, SetupWithFD) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    char b;
    HANDLE_EINTR(read(fds[0], &b, sizeof(b)));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  ExceptionHandler::CrashContext context;
  memset(&context, 0, sizeof(context));

  AutoTempDir temp_dir;
  string templ = temp_dir.path() + "/minidump-writer-unittest";
  int fd = open(templ.c_str(), O_CREAT | O_WRONLY, S_IRWXU);
  
  context.tid = 1;
  ASSERT_TRUE(WriteMinidump(fd, child, &context, sizeof(context)));
  struct stat st;
  ASSERT_EQ(stat(templ.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  close(fds[1]);
}



TEST(MinidumpWriterTest, MappingInfo) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  
  const u_int32_t memory_size = sysconf(_SC_PAGESIZE);
  const char* kMemoryName = "a fake module";
  const u_int8_t kModuleGUID[sizeof(MDGUID)] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };
  char module_identifier_buffer[kGUIDStringSize];
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
                                 memory_size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANON,
                                 -1,
                                 0));
  const uintptr_t kMemoryAddress = reinterpret_cast<uintptr_t>(memory);
  ASSERT_TRUE(memory);

  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    char b;
    IGNORE_RET(HANDLE_EINTR(read(fds[0], &b, sizeof(b))));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  ExceptionHandler::CrashContext context;
  memset(&context, 0, sizeof(context));
  ASSERT_EQ(0, getcontext(&context.context));
  context.tid = child;

  AutoTempDir temp_dir;
  string templ = temp_dir.path() + "/minidump-writer-unittest";

  
  MappingInfo info;
  info.start_addr = kMemoryAddress;
  info.size = memory_size;
  info.offset = 0;
  strcpy(info.name, kMemoryName);

  MappingList mappings;
  AppMemoryList memory_list;
  MappingEntry mapping;
  mapping.first = info;
  memcpy(mapping.second, kModuleGUID, sizeof(MDGUID));
  mappings.push_back(mapping);
  ASSERT_TRUE(WriteMinidump(templ.c_str(), child, &context, sizeof(context),
                            mappings, memory_list));

  
  
  
  Minidump minidump(templ.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* module =
    module_list->GetModuleForAddress(kMemoryAddress);
  ASSERT_TRUE(module);

  EXPECT_EQ(kMemoryAddress, module->base_address());
  EXPECT_EQ(memory_size, module->size());
  EXPECT_EQ(kMemoryName, module->code_file());
  EXPECT_EQ(module_identifier, module->debug_identifier());

  u_int32_t len;
  
  EXPECT_TRUE(minidump.SeekToStreamType(MD_THREAD_LIST_STREAM, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_MEMORY_LIST_STREAM, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_EXCEPTION_STREAM, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_SYSTEM_INFO_STREAM, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_CPU_INFO, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_PROC_STATUS, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_CMD_LINE, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_ENVIRON, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_AUXV, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_MAPS, &len));
  EXPECT_TRUE(minidump.SeekToStreamType(MD_LINUX_DSO_DEBUG, &len));

  close(fds[1]);
}




TEST(MinidumpWriterTest, MappingInfoContained) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  
  const u_int32_t memory_size = sysconf(_SC_PAGESIZE);
  const char* kMemoryName = "a fake module";
  const u_int8_t kModuleGUID[sizeof(MDGUID)] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };
  char module_identifier_buffer[kGUIDStringSize];
  FileID::ConvertIdentifierToString(kModuleGUID,
                                    module_identifier_buffer,
                                    sizeof(module_identifier_buffer));
  string module_identifier(module_identifier_buffer);
  
  size_t pos;
  while ((pos = module_identifier.find('-')) != string::npos) {
    module_identifier.erase(pos, 1);
  }
  
  
  module_identifier += "0";

  
  AutoTempDir temp_dir;
  string tempfile = temp_dir.path() + "/minidump-writer-unittest-temp";
  int fd = open(tempfile.c_str(), O_RDWR | O_CREAT, 0);
  ASSERT_NE(-1, fd);
  unlink(tempfile.c_str());
  
  google_breakpad::scoped_array<char> buffer(new char[memory_size]);
  memset(buffer.get(), 0, memory_size);
  ASSERT_EQ(memory_size, write(fd, buffer.get(), memory_size));
  lseek(fd, 0, SEEK_SET);

  char* memory =
    reinterpret_cast<char*>(mmap(NULL,
                                 memory_size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE,
                                 fd,
                                 0));
  const uintptr_t kMemoryAddress = reinterpret_cast<uintptr_t>(memory);
  ASSERT_TRUE(memory);
  close(fd);

  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    char b;
    IGNORE_RET(HANDLE_EINTR(read(fds[0], &b, sizeof(b))));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  ExceptionHandler::CrashContext context;
  memset(&context, 0, sizeof(context));
  context.tid = 1;

  string dumpfile = temp_dir.path() + "/minidump-writer-unittest";

  
  
  MappingInfo info;
  info.start_addr = kMemoryAddress - memory_size;
  info.size = memory_size * 3;
  info.offset = 0;
  strcpy(info.name, kMemoryName);

  MappingList mappings;
  AppMemoryList memory_list;
  MappingEntry mapping;
  mapping.first = info;
  memcpy(mapping.second, kModuleGUID, sizeof(MDGUID));
  mappings.push_back(mapping);
  ASSERT_TRUE(WriteMinidump(dumpfile.c_str(), child, &context, sizeof(context),
                            mappings, memory_list));

  
  
  
  Minidump minidump(dumpfile.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* module =
    module_list->GetModuleForAddress(kMemoryAddress);
  ASSERT_TRUE(module);

  EXPECT_EQ(info.start_addr, module->base_address());
  EXPECT_EQ(info.size, module->size());
  EXPECT_EQ(kMemoryName, module->code_file());
  EXPECT_EQ(module_identifier, module->debug_identifier());

  close(fds[1]);
}

TEST(MinidumpWriterTest, DeletedBinary) {
  static const int kNumberOfThreadsInHelperProgram = 1;
  char kNumberOfThreadsArgument[2];
  sprintf(kNumberOfThreadsArgument, "%d", kNumberOfThreadsInHelperProgram);

  string helper_path(GetHelperBinary());
  if (helper_path.empty()) {
    FAIL() << "Couldn't find helper binary";
    exit(1);
  }

  
  AutoTempDir temp_dir;
  string binpath = temp_dir.path() + "/linux-dumper-unittest-helper";
  ASSERT_TRUE(CopyFile(helper_path.c_str(), binpath.c_str()))
      << "Failed to copy " << helper_path << " to " << binpath;
  ASSERT_EQ(0, chmod(binpath.c_str(), 0755));

  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  pid_t child_pid = fork();
  if (child_pid == 0) {
    
    close(fds[0]);

    
    char pipe_fd_string[8];
    sprintf(pipe_fd_string, "%d", fds[1]);
    execl(binpath.c_str(),
          binpath.c_str(),
          pipe_fd_string,
          kNumberOfThreadsArgument,
          NULL);
  }
  close(fds[1]);
  
  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 1000));
  ASSERT_EQ(1, r);
  ASSERT_TRUE(pfd.revents & POLLIN);
  uint8_t junk;
  const int nr = HANDLE_EINTR(read(fds[0], &junk, sizeof(junk)));
  ASSERT_EQ(sizeof(junk), nr);
  close(fds[0]);

  
  
  unlink(binpath.c_str());

  ExceptionHandler::CrashContext context;
  memset(&context, 0, sizeof(context));

  string templ = temp_dir.path() + "/minidump-writer-unittest";
  
  context.tid = 1;
  ASSERT_TRUE(WriteMinidump(templ.c_str(), child_pid, &context,
                            sizeof(context)));
  kill(child_pid, SIGKILL);

  struct stat st;
  ASSERT_EQ(stat(templ.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  Minidump minidump(templ.c_str());
  ASSERT_TRUE(minidump.Read());

  
  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* module = module_list->GetMainModule();
  EXPECT_STREQ(binpath.c_str(), module->code_file().c_str());
  
  FileID fileid(helper_path.c_str());
  uint8_t identifier[sizeof(MDGUID)];
  EXPECT_TRUE(fileid.ElfFileIdentifier(identifier));
  char identifier_string[kGUIDStringSize];
  FileID::ConvertIdentifierToString(identifier,
                                    identifier_string,
                                    kGUIDStringSize);
  string module_identifier(identifier_string);
  
  size_t pos;
  while ((pos = module_identifier.find('-')) != string::npos) {
    module_identifier.erase(pos, 1);
  }
  
  
  module_identifier += "0";
  EXPECT_EQ(module_identifier, module->debug_identifier());
}


TEST(MinidumpWriterTest, AdditionalMemory) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  
  const u_int32_t kMemorySize = sysconf(_SC_PAGESIZE);

  
  u_int8_t* memory = new u_int8_t[kMemorySize];
  const uintptr_t kMemoryAddress = reinterpret_cast<uintptr_t>(memory);
  ASSERT_TRUE(memory);

  
  for (u_int32_t i = 0; i < kMemorySize; ++i) {
    memory[i] = i % 255;
  }

  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    char b;
    HANDLE_EINTR(read(fds[0], &b, sizeof(b)));
    close(fds[0]);
    syscall(__NR_exit);
  }
  close(fds[0]);

  ExceptionHandler::CrashContext context;

  
  
  
  ASSERT_EQ(0, getcontext(&context.context));
  context.tid = child;

  AutoTempDir temp_dir;
  string templ = temp_dir.path() + "/minidump-writer-unittest";
  unlink(templ.c_str());

  MappingList mappings;
  AppMemoryList memory_list;

  
  AppMemory app_memory;
  app_memory.ptr = memory;
  app_memory.length = kMemorySize;
  memory_list.push_back(app_memory);
  ASSERT_TRUE(WriteMinidump(templ.c_str(), child, &context, sizeof(context),
                            mappings, memory_list));

  
  Minidump minidump(templ.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpMemoryList* dump_memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(dump_memory_list);
  const MinidumpMemoryRegion* region =
    dump_memory_list->GetMemoryRegionForAddress(kMemoryAddress);
  ASSERT_TRUE(region);

  EXPECT_EQ(kMemoryAddress, region->GetBase());
  EXPECT_EQ(kMemorySize, region->GetSize());

  
  EXPECT_EQ(0, memcmp(region->GetMemory(), memory, kMemorySize));

  delete[] memory;
  close(fds[1]);
}
