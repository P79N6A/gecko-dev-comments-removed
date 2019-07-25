




























#include <unistd.h>
#include <sys/syscall.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/linux/eintr_wrapper.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;

namespace {
typedef testing::Test MinidumpWriterTest;
}

TEST(MinidumpWriterTest, Setup) {
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

  char templ[] = "/tmp/minidump-writer-unittest-XXXXXX";
  mktemp(templ);
  ASSERT_TRUE(WriteMinidump(templ, child, &context, sizeof(context)));
  struct stat st;
  ASSERT_EQ(stat(templ, &st), 0);
  ASSERT_GT(st.st_size, 0u);
  unlink(templ);

  close(fds[1]);
}



TEST(MinidumpWriterTest, MappingInfo) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  
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

  char templ[] = TEMPDIR "/minidump-writer-unittest-XXXXXX";
  mktemp(templ);

  
  MappingInfo info;
  info.start_addr = kMemoryAddress;
  info.size = kMemorySize;
  info.offset = 0;
  strcpy(info.name, kMemoryName);
  
  MappingList mappings;
  std::pair<MappingInfo, u_int8_t[sizeof(MDGUID)]> mapping;
  mapping.first = info;
  memcpy(mapping.second, kModuleGUID, sizeof(MDGUID));
  mappings.push_back(mapping);
  ASSERT_TRUE(WriteMinidump(templ, child, &context, sizeof(context), mappings));

  
  
  
  Minidump minidump(templ);
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

  unlink(templ);
  close(fds[1]);
}




TEST(MinidumpWriterTest, MappingInfoContained) {
  int fds[2];
  ASSERT_NE(-1, pipe(fds));

  
  
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
  
  
  char tempfile[] = TEMPDIR "/minidump-writer-unittest-temp-XXXXXX";
  mktemp(tempfile);
  printf("tempfile: %s\n", tempfile);
  int fd = open(tempfile, O_RDWR | O_CREAT, 0);
  ASSERT_NE(-1, fd);
  unlink(tempfile);
  
  char buffer[kMemorySize];
  memset(buffer, 0, kMemorySize);
  ASSERT_EQ(kMemorySize, write(fd, buffer, kMemorySize));
  lseek(fd, 0, SEEK_SET);

  char* memory =
    reinterpret_cast<char*>(mmap(NULL,
                                 kMemorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE,
                                 fd,
                                 0));
  const u_int64_t kMemoryAddress = reinterpret_cast<u_int64_t>(memory);
  ASSERT_TRUE(memory);
  close(fd);

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

  char dumpfile[] = TEMPDIR "/minidump-writer-unittest-XXXXXX";
  mktemp(dumpfile);

  
  
  MappingInfo info;
  info.start_addr = kMemoryAddress - kMemorySize;
  info.size = kMemorySize * 3;
  info.offset = 0;
  strcpy(info.name, kMemoryName);

  MappingList mappings;
  std::pair<MappingInfo, u_int8_t[sizeof(MDGUID)]> mapping;
  mapping.first = info;
  memcpy(mapping.second, kModuleGUID, sizeof(MDGUID));
  mappings.push_back(mapping);
  ASSERT_TRUE(WriteMinidump(dumpfile, child, &context, sizeof(context), mappings));

  
  
  
  Minidump minidump(dumpfile);
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

  unlink(dumpfile);
  close(fds[1]);
}
