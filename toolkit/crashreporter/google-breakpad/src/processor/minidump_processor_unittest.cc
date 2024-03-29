































#include <stdlib.h>

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <utility>

#include "breakpad_googletest_includes.h"
#include "common/scoped_ptr.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "processor/logging.h"
#include "processor/stackwalker_unittest_utils.h"

using std::map;

namespace google_breakpad {
class MockMinidump : public Minidump {
 public:
  MockMinidump() : Minidump("") {
  }

  MOCK_METHOD0(Read, bool());
  MOCK_CONST_METHOD0(path, string());
  MOCK_CONST_METHOD0(header, const MDRawHeader*());
  MOCK_METHOD0(GetThreadList, MinidumpThreadList*());
  MOCK_METHOD0(GetSystemInfo, MinidumpSystemInfo*());
  MOCK_METHOD0(GetBreakpadInfo, MinidumpBreakpadInfo*());
  MOCK_METHOD0(GetException, MinidumpException*());
  MOCK_METHOD0(GetAssertion, MinidumpAssertion*());
  MOCK_METHOD0(GetModuleList, MinidumpModuleList*());
};

class MockMinidumpThreadList : public MinidumpThreadList {
 public:
  MockMinidumpThreadList() : MinidumpThreadList(NULL) {}

  MOCK_CONST_METHOD0(thread_count, unsigned int());
  MOCK_CONST_METHOD1(GetThreadAtIndex, MinidumpThread*(unsigned int));
};

class MockMinidumpThread : public MinidumpThread {
 public:
  MockMinidumpThread() : MinidumpThread(NULL) {}

  MOCK_CONST_METHOD1(GetThreadID, bool(uint32_t*));
  MOCK_METHOD0(GetContext, MinidumpContext*());
  MOCK_METHOD0(GetMemory, MinidumpMemoryRegion*());
};



class MockMinidumpMemoryRegion : public MinidumpMemoryRegion {
 public:
  MockMinidumpMemoryRegion(uint64_t base, const string& contents) :
      MinidumpMemoryRegion(NULL) {
    region_.Init(base, contents);
  }

  uint64_t GetBase() const { return region_.GetBase(); }
  uint32_t GetSize() const { return region_.GetSize(); }

  bool GetMemoryAtAddress(uint64_t address, uint8_t  *value) const {
    return region_.GetMemoryAtAddress(address, value);
  }
  bool GetMemoryAtAddress(uint64_t address, uint16_t *value) const {
    return region_.GetMemoryAtAddress(address, value);
  }
  bool GetMemoryAtAddress(uint64_t address, uint32_t *value) const {
    return region_.GetMemoryAtAddress(address, value);
  }
  bool GetMemoryAtAddress(uint64_t address, uint64_t *value) const {
    return region_.GetMemoryAtAddress(address, value);
  }

  MockMemoryRegion region_;
};

}  

namespace {

using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::CodeModule;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpMemoryRegion;
using google_breakpad::MinidumpProcessor;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpThreadList;
using google_breakpad::MinidumpThread;
using google_breakpad::MockMinidump;
using google_breakpad::MockMinidumpMemoryRegion;
using google_breakpad::MockMinidumpThread;
using google_breakpad::MockMinidumpThreadList;
using google_breakpad::ProcessState;
using google_breakpad::scoped_ptr;
using google_breakpad::SymbolSupplier;
using google_breakpad::SystemInfo;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Mock;
using ::testing::Ne;
using ::testing::Property;
using ::testing::Return;
using ::testing::SetArgumentPointee;

static const char *kSystemInfoOS = "Windows NT";
static const char *kSystemInfoOSShort = "windows";
static const char *kSystemInfoOSVersion = "5.1.2600 Service Pack 2";
static const char *kSystemInfoCPU = "x86";
static const char *kSystemInfoCPUInfo =
    "GenuineIntel family 6 model 13 stepping 8";

#define ASSERT_TRUE_ABORT(cond) \
  if (!(cond)) {                                                        \
    fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    abort(); \
  }

#define ASSERT_EQ_ABORT(e1, e2) ASSERT_TRUE_ABORT((e1) == (e2))

class TestSymbolSupplier : public SymbolSupplier {
 public:
  TestSymbolSupplier() : interrupt_(false) {}

  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file);

  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file,
                                     string *symbol_data);

  virtual SymbolResult GetCStringSymbolData(const CodeModule *module,
                                            const SystemInfo *system_info,
                                            string *symbol_file,
                                            char **symbol_data);

  virtual void FreeSymbolData(const CodeModule *module);

  
  void set_interrupt(bool interrupt) { interrupt_ = interrupt; }

 private:
  bool interrupt_;
  map<string, char *> memory_buffers_;
};

SymbolSupplier::SymbolResult TestSymbolSupplier::GetSymbolFile(
    const CodeModule *module,
    const SystemInfo *system_info,
    string *symbol_file) {
  ASSERT_TRUE_ABORT(module);
  ASSERT_TRUE_ABORT(system_info);
  ASSERT_EQ_ABORT(system_info->cpu, kSystemInfoCPU);
  ASSERT_EQ_ABORT(system_info->cpu_info, kSystemInfoCPUInfo);
  ASSERT_EQ_ABORT(system_info->os, kSystemInfoOS);
  ASSERT_EQ_ABORT(system_info->os_short, kSystemInfoOSShort);
  ASSERT_EQ_ABORT(system_info->os_version, kSystemInfoOSVersion);

  if (interrupt_) {
    return INTERRUPT;
  }

  if (module && module->code_file() == "c:\\test_app.exe") {
      *symbol_file = string(getenv("srcdir") ? getenv("srcdir") : ".") +
                     "/src/processor/testdata/symbols/test_app.pdb/" +
                     module->debug_identifier() +
                     "/test_app.sym";
    return FOUND;
  }

  return NOT_FOUND;
}

SymbolSupplier::SymbolResult TestSymbolSupplier::GetSymbolFile(
    const CodeModule *module,
    const SystemInfo *system_info,
    string *symbol_file,
    string *symbol_data) {
  SymbolSupplier::SymbolResult s = GetSymbolFile(module, system_info,
                                                 symbol_file);
  if (s == FOUND) {
    std::ifstream in(symbol_file->c_str());
    std::getline(in, *symbol_data, string::traits_type::to_char_type(
                     string::traits_type::eof()));
    in.close();
  }

  return s;
}

SymbolSupplier::SymbolResult TestSymbolSupplier::GetCStringSymbolData(
    const CodeModule *module,
    const SystemInfo *system_info,
    string *symbol_file,
    char **symbol_data) {
  string symbol_data_string;
  SymbolSupplier::SymbolResult s = GetSymbolFile(module,
                                                 system_info,
                                                 symbol_file,
                                                 &symbol_data_string);
  if (s == FOUND) {
    unsigned int size = symbol_data_string.size() + 1;
    *symbol_data = new char[size];
    if (*symbol_data == NULL) {
      BPLOG(ERROR) << "Memory allocation failed for module: "
                   << module->code_file() << " size: " << size;
      return INTERRUPT;
    }
    strcpy(*symbol_data, symbol_data_string.c_str());
    memory_buffers_.insert(make_pair(module->code_file(), *symbol_data));
  }

  return s;
}

void TestSymbolSupplier::FreeSymbolData(const CodeModule *module) {
  map<string, char *>::iterator it = memory_buffers_.find(module->code_file());
  if (it != memory_buffers_.end()) {
    delete [] it->second;
    memory_buffers_.erase(it);
  }
}



class TestMinidumpSystemInfo : public MinidumpSystemInfo {
 public:
  TestMinidumpSystemInfo(MDRawSystemInfo info) :
      MinidumpSystemInfo(NULL) {
    valid_ = true;
    system_info_ = info;
    csd_version_ = new string("");
  }
};



class TestMinidumpContext : public MinidumpContext {
public:
  TestMinidumpContext(const MDRawContextX86& context) : MinidumpContext(NULL) {
    valid_ = true;
    context_.x86 = new MDRawContextX86(context);
    context_flags_ = MD_CONTEXT_X86;
  }
};

class MinidumpProcessorTest : public ::testing::Test {
};

TEST_F(MinidumpProcessorTest, TestCorruptMinidumps) {
  MockMinidump dump;
  TestSymbolSupplier supplier;
  BasicSourceLineResolver resolver;
  MinidumpProcessor processor(&supplier, &resolver);
  ProcessState state;

  EXPECT_EQ(processor.Process("nonexistent minidump", &state),
            google_breakpad::PROCESS_ERROR_MINIDUMP_NOT_FOUND);

  EXPECT_CALL(dump, path()).WillRepeatedly(Return("mock minidump"));
  EXPECT_CALL(dump, Read()).WillRepeatedly(Return(true));

  MDRawHeader fakeHeader;
  fakeHeader.time_date_stamp = 0;
  EXPECT_CALL(dump, header()).WillOnce(Return((MDRawHeader*)NULL)).
      WillRepeatedly(Return(&fakeHeader));

  EXPECT_EQ(processor.Process(&dump, &state),
            google_breakpad::PROCESS_ERROR_NO_MINIDUMP_HEADER);

  EXPECT_CALL(dump, GetThreadList()).
      WillOnce(Return((MinidumpThreadList*)NULL));
  EXPECT_CALL(dump, GetSystemInfo()).
    WillRepeatedly(Return((MinidumpSystemInfo*)NULL));

  EXPECT_EQ(processor.Process(&dump, &state),
            google_breakpad::PROCESS_ERROR_NO_THREAD_LIST);
}



TEST_F(MinidumpProcessorTest, TestSymbolSupplierLookupCounts) {
  MockSymbolSupplier supplier;
  BasicSourceLineResolver resolver;
  MinidumpProcessor processor(&supplier, &resolver);

  string minidump_file = string(getenv("srcdir") ? getenv("srcdir") : ".") +
                         "/src/processor/testdata/minidump2.dmp";
  ProcessState state;
  EXPECT_CALL(supplier, GetCStringSymbolData(
      Property(&google_breakpad::CodeModule::code_file,
               "c:\\test_app.exe"),
      _, _, _)).WillOnce(Return(SymbolSupplier::NOT_FOUND));
  EXPECT_CALL(supplier, GetCStringSymbolData(
      Property(&google_breakpad::CodeModule::code_file,
               Ne("c:\\test_app.exe")),
      _, _, _)).WillRepeatedly(Return(SymbolSupplier::NOT_FOUND));
  ASSERT_EQ(processor.Process(minidump_file, &state),
            google_breakpad::PROCESS_OK);

  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&supplier));

  
  
  EXPECT_CALL(supplier, GetCStringSymbolData(
      Property(&google_breakpad::CodeModule::code_file,
               "c:\\test_app.exe"),
      _, _, _)).WillOnce(Return(SymbolSupplier::NOT_FOUND));
  EXPECT_CALL(supplier, GetCStringSymbolData(
      Property(&google_breakpad::CodeModule::code_file,
               Ne("c:\\test_app.exe")),
      _, _, _)).WillRepeatedly(Return(SymbolSupplier::NOT_FOUND));
  ASSERT_EQ(processor.Process(minidump_file, &state),
            google_breakpad::PROCESS_OK);
}

TEST_F(MinidumpProcessorTest, TestBasicProcessing) {
  TestSymbolSupplier supplier;
  BasicSourceLineResolver resolver;
  MinidumpProcessor processor(&supplier, &resolver);

  string minidump_file = string(getenv("srcdir") ? getenv("srcdir") : ".") +
                         "/src/processor/testdata/minidump2.dmp";

  ProcessState state;
  ASSERT_EQ(processor.Process(minidump_file, &state),
            google_breakpad::PROCESS_OK);
  ASSERT_EQ(state.system_info()->os, kSystemInfoOS);
  ASSERT_EQ(state.system_info()->os_short, kSystemInfoOSShort);
  ASSERT_EQ(state.system_info()->os_version, kSystemInfoOSVersion);
  ASSERT_EQ(state.system_info()->cpu, kSystemInfoCPU);
  ASSERT_EQ(state.system_info()->cpu_info, kSystemInfoCPUInfo);
  ASSERT_TRUE(state.crashed());
  ASSERT_EQ(state.crash_reason(), "EXCEPTION_ACCESS_VIOLATION_WRITE");
  ASSERT_EQ(state.crash_address(), 0x45U);
  ASSERT_EQ(state.threads()->size(), size_t(1));
  ASSERT_EQ(state.requesting_thread(), 0);

  CallStack *stack = state.threads()->at(0);
  ASSERT_TRUE(stack);
  ASSERT_EQ(stack->frames()->size(), 4U);

  ASSERT_TRUE(stack->frames()->at(0)->module);
  ASSERT_EQ(stack->frames()->at(0)->module->base_address(), 0x400000U);
  ASSERT_EQ(stack->frames()->at(0)->module->code_file(), "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(0)->function_name,
            "`anonymous namespace'::CrashFunction");
  ASSERT_EQ(stack->frames()->at(0)->source_file_name, "c:\\test_app.cc");
  ASSERT_EQ(stack->frames()->at(0)->source_line, 58);

  ASSERT_TRUE(stack->frames()->at(1)->module);
  ASSERT_EQ(stack->frames()->at(1)->module->base_address(), 0x400000U);
  ASSERT_EQ(stack->frames()->at(1)->module->code_file(), "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(1)->function_name, "main");
  ASSERT_EQ(stack->frames()->at(1)->source_file_name, "c:\\test_app.cc");
  ASSERT_EQ(stack->frames()->at(1)->source_line, 65);

  
  ASSERT_TRUE(stack->frames()->at(2)->module);
  ASSERT_EQ(stack->frames()->at(2)->module->base_address(), 0x400000U);
  ASSERT_EQ(stack->frames()->at(2)->module->code_file(), "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(2)->function_name, "__tmainCRTStartup");
  ASSERT_EQ(stack->frames()->at(2)->source_file_name,
            "f:\\sp\\vctools\\crt_bld\\self_x86\\crt\\src\\crt0.c");
  ASSERT_EQ(stack->frames()->at(2)->source_line, 327);

  
  ASSERT_TRUE(stack->frames()->at(3)->module);
  ASSERT_EQ(stack->frames()->at(3)->module->base_address(), 0x7c800000U);
  ASSERT_EQ(stack->frames()->at(3)->module->code_file(),
            "C:\\WINDOWS\\system32\\kernel32.dll");
  ASSERT_TRUE(stack->frames()->at(3)->function_name.empty());
  ASSERT_TRUE(stack->frames()->at(3)->source_file_name.empty());
  ASSERT_EQ(stack->frames()->at(3)->source_line, 0);

  ASSERT_EQ(state.modules()->module_count(), 13U);
  ASSERT_TRUE(state.modules()->GetMainModule());
  ASSERT_EQ(state.modules()->GetMainModule()->code_file(), "c:\\test_app.exe");
  ASSERT_FALSE(state.modules()->GetModuleForAddress(0));
  ASSERT_EQ(state.modules()->GetMainModule(),
            state.modules()->GetModuleForAddress(0x400000));
  ASSERT_EQ(state.modules()->GetModuleForAddress(0x7c801234)->debug_file(),
            "kernel32.pdb");
  ASSERT_EQ(state.modules()->GetModuleForAddress(0x77d43210)->version(),
            "5.1.2600.2622");

  
  
  ASSERT_EQ(google_breakpad::EXPLOITABILITY_NOT_ANALYZED,
            state.exploitability());

  
  state.Clear();
  supplier.set_interrupt(true);
  ASSERT_EQ(processor.Process(minidump_file, &state),
            google_breakpad::PROCESS_SYMBOL_SUPPLIER_INTERRUPTED);
}

TEST_F(MinidumpProcessorTest, TestThreadMissingMemory) {
  MockMinidump dump;
  EXPECT_CALL(dump, path()).WillRepeatedly(Return("mock minidump"));
  EXPECT_CALL(dump, Read()).WillRepeatedly(Return(true));

  MDRawHeader fake_header;
  fake_header.time_date_stamp = 0;
  EXPECT_CALL(dump, header()).WillRepeatedly(Return(&fake_header));

  MDRawSystemInfo raw_system_info;
  memset(&raw_system_info, 0, sizeof(raw_system_info));
  raw_system_info.processor_architecture = MD_CPU_ARCHITECTURE_X86;
  raw_system_info.platform_id = MD_OS_WIN32_NT;
  TestMinidumpSystemInfo dump_system_info(raw_system_info);

  EXPECT_CALL(dump, GetSystemInfo()).
      WillRepeatedly(Return(&dump_system_info));

  MockMinidumpThreadList thread_list;
  EXPECT_CALL(dump, GetThreadList()).
      WillOnce(Return(&thread_list));

  
  MockMinidumpThread no_memory_thread;
  EXPECT_CALL(no_memory_thread, GetThreadID(_)).
    WillRepeatedly(DoAll(SetArgumentPointee<0>(1),
                         Return(true)));
  EXPECT_CALL(no_memory_thread, GetMemory()).
    WillRepeatedly(Return((MinidumpMemoryRegion*)NULL));

  MDRawContextX86 no_memory_thread_raw_context;
  memset(&no_memory_thread_raw_context, 0,
         sizeof(no_memory_thread_raw_context));
  no_memory_thread_raw_context.context_flags = MD_CONTEXT_X86_FULL;
  const uint32_t kExpectedEIP = 0xabcd1234;
  no_memory_thread_raw_context.eip = kExpectedEIP;
  TestMinidumpContext no_memory_thread_context(no_memory_thread_raw_context);
  EXPECT_CALL(no_memory_thread, GetContext()).
    WillRepeatedly(Return(&no_memory_thread_context));

  EXPECT_CALL(thread_list, thread_count()).
    WillRepeatedly(Return(1));
  EXPECT_CALL(thread_list, GetThreadAtIndex(0)).
    WillOnce(Return(&no_memory_thread));

  MinidumpProcessor processor((SymbolSupplier*)NULL, NULL);
  ProcessState state;
  EXPECT_EQ(processor.Process(&dump, &state),
            google_breakpad::PROCESS_OK);

  
  ASSERT_EQ(1U, state.threads()->size());
  ASSERT_EQ(1U, state.threads()->at(0)->frames()->size());
  ASSERT_EQ(kExpectedEIP, state.threads()->at(0)->frames()->at(0)->instruction);
}

TEST_F(MinidumpProcessorTest, TestThreadMissingContext) {
  MockMinidump dump;
  EXPECT_CALL(dump, path()).WillRepeatedly(Return("mock minidump"));
  EXPECT_CALL(dump, Read()).WillRepeatedly(Return(true));

  MDRawHeader fake_header;
  fake_header.time_date_stamp = 0;
  EXPECT_CALL(dump, header()).WillRepeatedly(Return(&fake_header));

  MDRawSystemInfo raw_system_info;
  memset(&raw_system_info, 0, sizeof(raw_system_info));
  raw_system_info.processor_architecture = MD_CPU_ARCHITECTURE_X86;
  raw_system_info.platform_id = MD_OS_WIN32_NT;
  TestMinidumpSystemInfo dump_system_info(raw_system_info);

  EXPECT_CALL(dump, GetSystemInfo()).
      WillRepeatedly(Return(&dump_system_info));

  MockMinidumpThreadList thread_list;
  EXPECT_CALL(dump, GetThreadList()).
      WillOnce(Return(&thread_list));

  
  MockMinidumpThread no_context_thread;
  EXPECT_CALL(no_context_thread, GetThreadID(_)).
    WillRepeatedly(DoAll(SetArgumentPointee<0>(1),
                         Return(true)));
  EXPECT_CALL(no_context_thread, GetContext()).
    WillRepeatedly(Return((MinidumpContext*)NULL));

  
  MockMinidumpMemoryRegion no_context_thread_memory(0x1234, "xxx");
  EXPECT_CALL(no_context_thread, GetMemory()).
    WillRepeatedly(Return(&no_context_thread_memory));

  EXPECT_CALL(thread_list, thread_count()).
    WillRepeatedly(Return(1));
  EXPECT_CALL(thread_list, GetThreadAtIndex(0)).
    WillOnce(Return(&no_context_thread));

  MinidumpProcessor processor((SymbolSupplier*)NULL, NULL);
  ProcessState state;
  EXPECT_EQ(processor.Process(&dump, &state),
            google_breakpad::PROCESS_OK);

  
  ASSERT_EQ(1U, state.threads()->size());
  ASSERT_EQ(0U, state.threads()->at(0)->frames()->size());
}

}  

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
