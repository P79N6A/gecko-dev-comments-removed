






































#include <stdio.h>

#include <sstream>
#include <string>

#include "breakpad_googletest_includes.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/memory_region.h"
#include "processor/logging.h"
#include "processor/module_serializer.h"
#include "processor/module_comparer.h"

namespace {

using google_breakpad::SourceLineResolverBase;
using google_breakpad::BasicSourceLineResolver;
using google_breakpad::FastSourceLineResolver;
using google_breakpad::ModuleSerializer;
using google_breakpad::ModuleComparer;
using google_breakpad::CFIFrameInfo;
using google_breakpad::CodeModule;
using google_breakpad::MemoryRegion;
using google_breakpad::StackFrame;
using google_breakpad::WindowsFrameInfo;
using google_breakpad::linked_ptr;
using google_breakpad::scoped_ptr;

class TestCodeModule : public CodeModule {
 public:
  explicit TestCodeModule(string code_file) : code_file_(code_file) {}
  virtual ~TestCodeModule() {}

  virtual u_int64_t base_address() const { return 0; }
  virtual u_int64_t size() const { return 0xb000; }
  virtual string code_file() const { return code_file_; }
  virtual string code_identifier() const { return ""; }
  virtual string debug_file() const { return ""; }
  virtual string debug_identifier() const { return ""; }
  virtual string version() const { return ""; }
  virtual const CodeModule* Copy() const {
    return new TestCodeModule(code_file_);
  }

 private:
  string code_file_;
};


class MockMemoryRegion: public MemoryRegion {
  u_int64_t GetBase() const { return 0x10000; }
  u_int32_t GetSize() const { return 0x01000; }
  bool GetMemoryAtAddress(u_int64_t address, u_int8_t *value) const {
    *value = address & 0xff;
    return true;
  }
  bool GetMemoryAtAddress(u_int64_t address, u_int16_t *value) const {
    *value = address & 0xffff;
    return true;
  }
  bool GetMemoryAtAddress(u_int64_t address, u_int32_t *value) const {
    switch (address) {
      case 0x10008: *value = 0x98ecadc3; break;  
      case 0x1000c: *value = 0x878f7524; break;  
      case 0x10010: *value = 0x6312f9a5; break;  
      case 0x10014: *value = 0x10038;    break;  
      case 0x10018: *value = 0xf6438648; break;  
      default: *value = 0xdeadbeef;      break;  
    }
    return true;
  }
  bool GetMemoryAtAddress(u_int64_t address, u_int64_t *value) const {
    *value = address;
    return true;
  }
};





static bool VerifyRegisters(
    const char *file, int line,
    const CFIFrameInfo::RegisterValueMap<u_int32_t> &expected,
    const CFIFrameInfo::RegisterValueMap<u_int32_t> &actual) {
  CFIFrameInfo::RegisterValueMap<u_int32_t>::const_iterator a;
  a = actual.find(".cfa");
  if (a == actual.end())
    return false;
  a = actual.find(".ra");
  if (a == actual.end())
    return false;
  for (a = actual.begin(); a != actual.end(); a++) {
    CFIFrameInfo::RegisterValueMap<u_int32_t>::const_iterator e =
      expected.find(a->first);
    if (e == expected.end()) {
      fprintf(stderr, "%s:%d: unexpected register '%s' recovered, value 0x%x\n",
              file, line, a->first.c_str(), a->second);
      return false;
    }
    if (e->second != a->second) {
      fprintf(stderr,
              "%s:%d: register '%s' recovered value was 0x%x, expected 0x%x\n",
              file, line, a->first.c_str(), a->second, e->second);
      return false;
    }
    
    
    
  }
  return true;
}

static bool VerifyEmpty(const StackFrame &frame) {
  if (frame.function_name.empty() &&
      frame.source_file_name.empty() &&
      frame.source_line == 0)
    return true;
  return false;
}

static void ClearSourceLineInfo(StackFrame *frame) {
  frame->function_name.clear();
  frame->module = NULL;
  frame->source_file_name.clear();
  frame->source_line = 0;
}

class TestFastSourceLineResolver : public ::testing::Test {
 public:
  void SetUp() {
    testdata_dir = string(getenv("srcdir") ? getenv("srcdir") : ".") +
                         "/src/processor/testdata";
  }

  string symbol_file(int file_index) {
    std::stringstream ss;
    ss << testdata_dir << "/module" << file_index << ".out";
    return ss.str();
  }

  ModuleSerializer serializer;
  BasicSourceLineResolver basic_resolver;
  FastSourceLineResolver fast_resolver;
  ModuleComparer module_comparer;

  string testdata_dir;
};


TEST_F(TestFastSourceLineResolver, TestLoadAndResolve) {
  TestCodeModule module1("module1");
  ASSERT_TRUE(basic_resolver.LoadModule(&module1, symbol_file(1)));
  ASSERT_TRUE(basic_resolver.HasModule(&module1));
  
  ASSERT_TRUE(serializer.ConvertOneModule(
      module1.code_file(), &basic_resolver, &fast_resolver));
  ASSERT_TRUE(fast_resolver.HasModule(&module1));

  TestCodeModule module2("module2");
  ASSERT_TRUE(basic_resolver.LoadModule(&module2, symbol_file(2)));
  ASSERT_TRUE(basic_resolver.HasModule(&module2));
  
  ASSERT_TRUE(serializer.ConvertOneModule(
      module2.code_file(), &basic_resolver, &fast_resolver));
  ASSERT_TRUE(fast_resolver.HasModule(&module2));

  StackFrame frame;
  scoped_ptr<WindowsFrameInfo> windows_frame_info;
  scoped_ptr<CFIFrameInfo> cfi_frame_info;
  frame.instruction = 0x1000;
  frame.module = NULL;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_FALSE(frame.module);
  ASSERT_TRUE(frame.function_name.empty());
  ASSERT_EQ(frame.function_base, 0);
  ASSERT_TRUE(frame.source_file_name.empty());
  ASSERT_EQ(frame.source_line, 0);
  ASSERT_EQ(frame.source_line_base, 0);

  frame.module = &module1;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, "Function1_1");
  ASSERT_TRUE(frame.module);
  ASSERT_EQ(frame.module->code_file(), "module1");
  ASSERT_EQ(frame.function_base, 0x1000);
  ASSERT_EQ(frame.source_file_name, "file1_1.cc");
  ASSERT_EQ(frame.source_line, 44);
  ASSERT_EQ(frame.source_line_base, 0x1000);
  windows_frame_info.reset(fast_resolver.FindWindowsFrameInfo(&frame));
  ASSERT_TRUE(windows_frame_info.get());
  ASSERT_FALSE(windows_frame_info->allocates_base_pointer);
  ASSERT_EQ(windows_frame_info->program_string,
            "$eip 4 + ^ = $esp $ebp 8 + = $ebp $ebp ^ =");

  ClearSourceLineInfo(&frame);
  frame.instruction = 0x800;
  frame.module = &module1;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_TRUE(VerifyEmpty(frame));
  windows_frame_info.reset(fast_resolver.FindWindowsFrameInfo(&frame));
  ASSERT_FALSE(windows_frame_info.get());

  frame.instruction = 0x1280;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, "Function1_3");
  ASSERT_TRUE(frame.source_file_name.empty());
  ASSERT_EQ(frame.source_line, 0);
  windows_frame_info.reset(fast_resolver.FindWindowsFrameInfo(&frame));
  ASSERT_TRUE(windows_frame_info.get());
  ASSERT_EQ(windows_frame_info->type_, WindowsFrameInfo::STACK_INFO_UNKNOWN);
  ASSERT_FALSE(windows_frame_info->allocates_base_pointer);
  ASSERT_TRUE(windows_frame_info->program_string.empty());

  frame.instruction = 0x1380;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, "Function1_4");
  ASSERT_TRUE(frame.source_file_name.empty());
  ASSERT_EQ(frame.source_line, 0);
  windows_frame_info.reset(fast_resolver.FindWindowsFrameInfo(&frame));
  ASSERT_TRUE(windows_frame_info.get());
  ASSERT_EQ(windows_frame_info->type_, WindowsFrameInfo::STACK_INFO_FRAME_DATA);
  ASSERT_FALSE(windows_frame_info->allocates_base_pointer);
  ASSERT_FALSE(windows_frame_info->program_string.empty());

  frame.instruction = 0x2000;
  windows_frame_info.reset(fast_resolver.FindWindowsFrameInfo(&frame));
  ASSERT_FALSE(windows_frame_info.get());

  
  
  
  frame.instruction = 0x3d3f;
  frame.module = &module1;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_FALSE(cfi_frame_info.get());

  frame.instruction = 0x3e9f;
  frame.module = &module1;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_FALSE(cfi_frame_info.get());

  CFIFrameInfo::RegisterValueMap<u_int32_t> current_registers;
  CFIFrameInfo::RegisterValueMap<u_int32_t> caller_registers;
  CFIFrameInfo::RegisterValueMap<u_int32_t> expected_caller_registers;
  MockMemoryRegion memory;

  
  
  expected_caller_registers[".cfa"] = 0x1001c;
  expected_caller_registers[".ra"]  = 0xf6438648;
  expected_caller_registers["$ebp"] = 0x10038;
  expected_caller_registers["$ebx"] = 0x98ecadc3;
  expected_caller_registers["$esi"] = 0x878f7524;
  expected_caller_registers["$edi"] = 0x6312f9a5;

  frame.instruction = 0x3d40;
  frame.module = &module1;
  current_registers.clear();
  current_registers["$esp"] = 0x10018;
  current_registers["$ebp"] = 0x10038;
  current_registers["$ebx"] = 0x98ecadc3;
  current_registers["$esi"] = 0x878f7524;
  current_registers["$edi"] = 0x6312f9a5;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_TRUE(cfi_frame_info.get());
  ASSERT_TRUE(cfi_frame_info.get()
              ->FindCallerRegs<u_int32_t>(current_registers, memory,
                                          &caller_registers));
  ASSERT_TRUE(VerifyRegisters(__FILE__, __LINE__,
                              expected_caller_registers, caller_registers));

  frame.instruction = 0x3d41;
  current_registers["$esp"] = 0x10014;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_TRUE(cfi_frame_info.get());
  ASSERT_TRUE(cfi_frame_info.get()
              ->FindCallerRegs<u_int32_t>(current_registers, memory,
                                          &caller_registers));
  ASSERT_TRUE(VerifyRegisters(__FILE__, __LINE__,
                              expected_caller_registers, caller_registers));

  frame.instruction = 0x3d43;
  current_registers["$ebp"] = 0x10014;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_TRUE(cfi_frame_info.get());
  ASSERT_TRUE(cfi_frame_info.get()
              ->FindCallerRegs<u_int32_t>(current_registers, memory,
                                          &caller_registers));
  VerifyRegisters(__FILE__, __LINE__,
                  expected_caller_registers, caller_registers);

  frame.instruction = 0x3d54;
  current_registers["$ebx"] = 0x6864f054U;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_TRUE(cfi_frame_info.get());
  ASSERT_TRUE(cfi_frame_info.get()
              ->FindCallerRegs<u_int32_t>(current_registers, memory,
                                          &caller_registers));
  VerifyRegisters(__FILE__, __LINE__,
                  expected_caller_registers, caller_registers);

  frame.instruction = 0x3d5a;
  current_registers["$esi"] = 0x6285f79aU;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_TRUE(cfi_frame_info.get());
  ASSERT_TRUE(cfi_frame_info.get()
              ->FindCallerRegs<u_int32_t>(current_registers, memory,
                                          &caller_registers));
  VerifyRegisters(__FILE__, __LINE__,
                  expected_caller_registers, caller_registers);

  frame.instruction = 0x3d84;
  current_registers["$edi"] = 0x64061449U;
  cfi_frame_info.reset(fast_resolver.FindCFIFrameInfo(&frame));
  ASSERT_TRUE(cfi_frame_info.get());
  ASSERT_TRUE(cfi_frame_info.get()
              ->FindCallerRegs<u_int32_t>(current_registers, memory,
                                          &caller_registers));
  VerifyRegisters(__FILE__, __LINE__,
                  expected_caller_registers, caller_registers);

  frame.instruction = 0x2900;
  frame.module = &module1;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, string("PublicSymbol"));

  frame.instruction = 0x4000;
  frame.module = &module1;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, string("LargeFunction"));

  frame.instruction = 0x2181;
  frame.module = &module2;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, "Function2_2");
  ASSERT_EQ(frame.function_base, 0x2170);
  ASSERT_TRUE(frame.module);
  ASSERT_EQ(frame.module->code_file(), "module2");
  ASSERT_EQ(frame.source_file_name, "file2_2.cc");
  ASSERT_EQ(frame.source_line, 21);
  ASSERT_EQ(frame.source_line_base, 0x2180);
  windows_frame_info.reset(fast_resolver.FindWindowsFrameInfo(&frame));
  ASSERT_TRUE(windows_frame_info.get());
  ASSERT_EQ(windows_frame_info->type_, WindowsFrameInfo::STACK_INFO_FRAME_DATA);
  ASSERT_EQ(windows_frame_info->prolog_size, 1);

  frame.instruction = 0x216f;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, "Public2_1");

  ClearSourceLineInfo(&frame);
  frame.instruction = 0x219f;
  frame.module = &module2;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_TRUE(frame.function_name.empty());

  frame.instruction = 0x21a0;
  frame.module = &module2;
  fast_resolver.FillSourceLineInfo(&frame);
  ASSERT_EQ(frame.function_name, "Public2_2");
}

TEST_F(TestFastSourceLineResolver, TestInvalidLoads) {
  TestCodeModule module3("module3");
  ASSERT_FALSE(basic_resolver.LoadModule(&module3,
                                         testdata_dir + "/module3_bad.out"));
  ASSERT_FALSE(basic_resolver.HasModule(&module3));
  
  ASSERT_FALSE(serializer.ConvertOneModule(module3.code_file(),
                                           &basic_resolver,
                                           &fast_resolver));
  ASSERT_FALSE(fast_resolver.HasModule(&module3));

  TestCodeModule module4("module4");
  ASSERT_FALSE(basic_resolver.LoadModule(&module4,
                                         testdata_dir + "/module4_bad.out"));
  ASSERT_FALSE(basic_resolver.HasModule(&module4));
  
  ASSERT_FALSE(serializer.ConvertOneModule(module4.code_file(),
                                           &basic_resolver,
                                           &fast_resolver));
  ASSERT_FALSE(fast_resolver.HasModule(&module4));

  TestCodeModule module5("module5");
  ASSERT_FALSE(fast_resolver.LoadModule(&module5,
                                         testdata_dir + "/invalid-filename"));
  ASSERT_FALSE(fast_resolver.HasModule(&module5));

  TestCodeModule invalidmodule("invalid-module");
  ASSERT_FALSE(fast_resolver.HasModule(&invalidmodule));
}

TEST_F(TestFastSourceLineResolver, TestUnload) {
  TestCodeModule module1("module1");
  ASSERT_FALSE(basic_resolver.HasModule(&module1));

  ASSERT_TRUE(basic_resolver.LoadModule(&module1, symbol_file(1)));
  ASSERT_TRUE(basic_resolver.HasModule(&module1));
  
  ASSERT_TRUE(serializer.ConvertOneModule(module1.code_file(),
                                          &basic_resolver,
                                          &fast_resolver));
  ASSERT_TRUE(fast_resolver.HasModule(&module1));
  basic_resolver.UnloadModule(&module1);
  fast_resolver.UnloadModule(&module1);
  ASSERT_FALSE(fast_resolver.HasModule(&module1));

  ASSERT_TRUE(basic_resolver.LoadModule(&module1, symbol_file(1)));
  ASSERT_TRUE(basic_resolver.HasModule(&module1));
  
  ASSERT_TRUE(serializer.ConvertOneModule(module1.code_file(),
                                          &basic_resolver,
                                          &fast_resolver));
  ASSERT_TRUE(fast_resolver.HasModule(&module1));
}

TEST_F(TestFastSourceLineResolver, CompareModule) {
  char *symbol_data;
  string symbol_data_string;
  string filename;

  for (int module_index = 0; module_index < 3; ++module_index) {
    std::stringstream ss;
    ss << testdata_dir << "/module" << module_index << ".out";
    filename = ss.str();
    ASSERT_TRUE(SourceLineResolverBase::ReadSymbolFile(
        &symbol_data, symbol_file(module_index)));
    symbol_data_string = symbol_data;
    delete [] symbol_data;
    ASSERT_TRUE(module_comparer.Compare(symbol_data_string));
  }
}

}  

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
