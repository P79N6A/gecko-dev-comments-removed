































#include <string>
#include "google/call_stack.h"
#include "google/minidump_processor.h"
#include "google/process_state.h"
#include "google/stack_frame.h"
#include "google/symbol_supplier.h"
#include "processor/minidump.h"
#include "processor/scoped_ptr.h"

using std::string;
using google_airbag::CallStack;
using google_airbag::MinidumpProcessor;
using google_airbag::ProcessState;
using google_airbag::scoped_ptr;

#define ASSERT_TRUE(cond) \
  if (!(cond)) {                                                        \
    fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    return false; \
  }

#define ASSERT_EQ(e1, e2) ASSERT_TRUE((e1) == (e2))

namespace google_airbag {

class TestSymbolSupplier : public SymbolSupplier {
 public:
  virtual string GetSymbolFile(MinidumpModule *module);
};

string TestSymbolSupplier::GetSymbolFile(MinidumpModule *module) {
  if (*(module->GetName()) == "c:\\test_app.exe") {
    return string(getenv("srcdir") ? getenv("srcdir") : ".") +
      "/src/processor/testdata/minidump2.sym";
  }

  return "";
}

}  

using google_airbag::TestSymbolSupplier;

static bool RunTests() {
  TestSymbolSupplier supplier;
  MinidumpProcessor processor(&supplier);

  string minidump_file = string(getenv("srcdir") ? getenv("srcdir") : ".") +
                         "/src/processor/testdata/minidump2.dmp";

  scoped_ptr<ProcessState> state(processor.Process(minidump_file));
  ASSERT_TRUE(state.get());
  ASSERT_EQ(state->cpu(), "x86");
  ASSERT_EQ(state->cpu_info(), "GenuineIntel family 6 model 13 stepping 8");
  ASSERT_EQ(state->os(), "Windows NT");
  ASSERT_EQ(state->os_version(), "5.1.2600 Service Pack 2");
  ASSERT_TRUE(state->crashed());
  ASSERT_EQ(state->crash_reason(), "EXCEPTION_ACCESS_VIOLATION");
  ASSERT_EQ(state->crash_address(), 0);
  ASSERT_EQ(state->threads()->size(), 1);
  ASSERT_EQ(state->crash_thread(), 0);
  CallStack *stack = state->threads()->at(0);
  ASSERT_TRUE(stack);
  ASSERT_EQ(stack->frames()->size(), 4);

  ASSERT_EQ(stack->frames()->at(0)->module_base, 0x400000);
  ASSERT_EQ(stack->frames()->at(0)->module_name, "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(0)->function_name, "CrashFunction()");
  ASSERT_EQ(stack->frames()->at(0)->source_file_name, "c:\\test_app.cc");
  ASSERT_EQ(stack->frames()->at(0)->source_line, 65);

  ASSERT_EQ(stack->frames()->at(1)->module_base, 0x400000);
  ASSERT_EQ(stack->frames()->at(1)->module_name, "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(1)->function_name, "main");
  ASSERT_EQ(stack->frames()->at(1)->source_file_name, "c:\\test_app.cc");
  ASSERT_EQ(stack->frames()->at(1)->source_line, 70);

  
  ASSERT_EQ(stack->frames()->at(2)->module_base, 0x400000);
  ASSERT_EQ(stack->frames()->at(2)->module_name, "c:\\test_app.exe");
  ASSERT_EQ(stack->frames()->at(2)->function_name, "__tmainCRTStartup");
  ASSERT_EQ(stack->frames()->at(2)->source_file_name,
            "f:\\rtm\\vctools\\crt_bld\\self_x86\\crt\\src\\crt0.c");
  ASSERT_EQ(stack->frames()->at(2)->source_line, 318);

  
  ASSERT_EQ(stack->frames()->at(3)->module_base, 0x7c800000);
  ASSERT_EQ(stack->frames()->at(3)->module_name,
            "C:\\WINDOWS\\system32\\kernel32.dll");
  ASSERT_TRUE(stack->frames()->at(3)->function_name.empty());
  ASSERT_TRUE(stack->frames()->at(3)->source_file_name.empty());
  ASSERT_EQ(stack->frames()->at(3)->source_line, 0);

  return true;
}

int main(int argc, char *argv[]) {
  if (!RunTests()) {
    return 1;
  }

  return 0;
}
