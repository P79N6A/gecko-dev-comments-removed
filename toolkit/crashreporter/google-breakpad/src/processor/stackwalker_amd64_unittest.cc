
































#include <string>
#include <string.h>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/test_assembler.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/stackwalker_unittest_utils.h"
#include "processor/stackwalker_amd64.h"

using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::StackFrame;
using google_breakpad::StackFrameAMD64;
using google_breakpad::StackwalkerAMD64;
using google_breakpad::SystemInfo;
using google_breakpad::test_assembler::kLittleEndian;
using google_breakpad::test_assembler::Label;
using google_breakpad::test_assembler::Section;
using std::string;
using std::vector;
using testing::_;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

class StackwalkerAMD64Fixture {
 public:
  StackwalkerAMD64Fixture()
    : stack_section(kLittleEndian),
      
      
      module1(0x40000000c0000000ULL, 0x10000, "module1", "version1"),
      module2(0x50000000b0000000ULL, 0x10000, "module2", "version2") {
    
    system_info.os = "Linux";
    system_info.os_short = "linux";
    system_info.os_version = "Horrendous Hippo";
    system_info.cpu = "x86";
    system_info.cpu_info = "";

    
    BrandContext(&raw_context);

    
    modules.Add(&module1);
    modules.Add(&module2);

    
    
    EXPECT_CALL(supplier, GetSymbolFile(_, _, _, _))
      .WillRepeatedly(Return(MockSymbolSupplier::NOT_FOUND));
  }

  
  
  void SetModuleSymbols(MockCodeModule *module, const string &info) {
    EXPECT_CALL(supplier, GetSymbolFile(module, &system_info, _, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<3>(info),
                            Return(MockSymbolSupplier::FOUND)));
  }

  
  
  void RegionFromSection() {
    string contents;
    ASSERT_TRUE(stack_section.GetContents(&contents));
    stack_region.Init(stack_section.start().Value(), contents);
  }

  
  void BrandContext(MDRawContextAMD64 *raw_context) {
    u_int8_t x = 173;
    for (size_t i = 0; i < sizeof(*raw_context); i++)
      reinterpret_cast<u_int8_t *>(raw_context)[i] = (x += 17);
  }
  
  SystemInfo system_info;
  MDRawContextAMD64 raw_context;
  Section stack_section;
  MockMemoryRegion stack_region;
  MockCodeModule module1;
  MockCodeModule module2;
  MockCodeModules modules;
  MockSymbolSupplier supplier;
  BasicSourceLineResolver resolver;
  CallStack call_stack;
  const vector<StackFrame *> *frames;
};

class GetContextFrame: public StackwalkerAMD64Fixture, public Test { };

TEST_F(GetContextFrame, Simple) {
  
  
  
  
  raw_context.rip = 0x40000000c0000200ULL;
  raw_context.rbp = 0x8000000080000000ULL;

  StackwalkerAMD64 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_GE(1U, frames->size());
  StackFrameAMD64 *frame = static_cast<StackFrameAMD64 *>(frames->at(0));
  
  
  EXPECT_TRUE(memcmp(&raw_context, &frame->context, sizeof(raw_context)) == 0);
}

struct CFIFixture: public StackwalkerAMD64Fixture {
  CFIFixture() {
    
    
    
    SetModuleSymbols(&module1,
                     
                     "FUNC 4000 1000 10 enchiridion\n"
                     
                     "STACK CFI INIT 4000 100 .cfa: $rsp 8 + .ra: .cfa 8 - ^\n"
                     
                     "STACK CFI 4001 .cfa: $rsp 16 + $rbx: .cfa 16 - ^\n"
                     
                     "STACK CFI 4002 $r12: $rbx\n"
                     
                     "STACK CFI 4003 .cfa: $rsp 40 + $r13: .cfa 32 - ^\n"
                     
                     "STACK CFI 4005 .ra: $r13\n"
                     
                     "STACK CFI 4006 .cfa: $rbp 16 + $rbp: .cfa 24 - ^\n"

                     
                     "FUNC 5000 1000 10 epictetus\n"
                     
                     "STACK CFI INIT 5000 1000 .cfa: $rsp .ra 0\n");

    
    expected.rsp = 0x8000000080000000ULL;
    expected.rip = 0x40000000c0005510ULL;
    expected.rbp = 0x68995b1de4700266ULL;
    expected.rbx = 0x5a5beeb38de23be8ULL;
    expected.r12 = 0xed1b02e8cc0fc79cULL;
    expected.r13 = 0x1d20ad8acacbe930ULL;
    expected.r14 = 0xe94cffc2f7adaa28ULL;
    expected.r15 = 0xb638d17d8da413b5ULL;

    
    raw_context = expected;
  }

  
  
  
  
  
  void CheckWalk() {
    RegionFromSection();
    raw_context.rsp = stack_section.start().Value();

    StackwalkerAMD64 walker(&system_info, &raw_context, &stack_region, &modules,
                          &supplier, &resolver);
    ASSERT_TRUE(walker.Walk(&call_stack));
    frames = call_stack.frames();
    ASSERT_EQ(2U, frames->size());

    StackFrameAMD64 *frame0 = static_cast<StackFrameAMD64 *>(frames->at(0));
    ASSERT_EQ(StackFrameAMD64::CONTEXT_VALID_ALL, frame0->context_validity);
    EXPECT_EQ("enchiridion", frame0->function_name);
    EXPECT_EQ(0x40000000c0004000ULL, frame0->function_base);

    StackFrameAMD64 *frame1 = static_cast<StackFrameAMD64 *>(frames->at(1));
    ASSERT_EQ((StackFrameAMD64::CONTEXT_VALID_RIP |
               StackFrameAMD64::CONTEXT_VALID_RSP |
               StackFrameAMD64::CONTEXT_VALID_RBP |
               StackFrameAMD64::CONTEXT_VALID_RBX |
               StackFrameAMD64::CONTEXT_VALID_R12 |
               StackFrameAMD64::CONTEXT_VALID_R13 |
               StackFrameAMD64::CONTEXT_VALID_R14 |
               StackFrameAMD64::CONTEXT_VALID_R15),
              frame1->context_validity);
    EXPECT_EQ(expected.rip, frame1->context.rip);
    EXPECT_EQ(expected.rsp, frame1->context.rsp);
    EXPECT_EQ(expected.rbp, frame1->context.rbp);
    EXPECT_EQ(expected.rbx, frame1->context.rbx);
    EXPECT_EQ(expected.r12, frame1->context.r12);
    EXPECT_EQ(expected.r13, frame1->context.r13);
    EXPECT_EQ(expected.r14, frame1->context.r14);
    EXPECT_EQ(expected.r15, frame1->context.r15);
    EXPECT_EQ("epictetus", frame1->function_name);
  }

  
  MDRawContextAMD64 expected;
};

class CFI: public CFIFixture, public Test { };

TEST_F(CFI, At4000) {
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x40000000c0005510ULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004000ULL;
  CheckWalk();
}

TEST_F(CFI, At4001) {
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x5a5beeb38de23be8ULL) 
    .D64(0x40000000c0005510ULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004001ULL;
  raw_context.rbx = 0xbe0487d2f9eafe29ULL; 
  CheckWalk();
}

TEST_F(CFI, At4002) {
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x5a5beeb38de23be8ULL) 
    .D64(0x40000000c0005510ULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004002ULL;
  raw_context.rbx = 0xed1b02e8cc0fc79cULL; 
  raw_context.r12 = 0xb0118de918a4bceaULL; 
  CheckWalk();
}

TEST_F(CFI, At4003) {
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x0e023828dffd4d81ULL) 
    .D64(0x1d20ad8acacbe930ULL) 
    .D64(0x319e68b49e3ace0fULL) 
    .D64(0x5a5beeb38de23be8ULL) 
    .D64(0x40000000c0005510ULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004003ULL;
  raw_context.rbx = 0xed1b02e8cc0fc79cULL; 
  raw_context.r12 = 0x89d04fa804c87a43ULL; 
  raw_context.r13 = 0x5118e02cbdb24b03ULL; 
  CheckWalk();
}


TEST_F(CFI, At4004) {
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x0e023828dffd4d81ULL) 
    .D64(0x1d20ad8acacbe930ULL) 
    .D64(0x319e68b49e3ace0fULL) 
    .D64(0x5a5beeb38de23be8ULL) 
    .D64(0x40000000c0005510ULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004004ULL;
  raw_context.rbx = 0xed1b02e8cc0fc79cULL; 
  raw_context.r12 = 0x89d04fa804c87a43ULL; 
  raw_context.r13 = 0x5118e02cbdb24b03ULL; 
  CheckWalk();
}

TEST_F(CFI, At4005) {
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x4b516dd035745953ULL) 
    .D64(0x1d20ad8acacbe930ULL) 
    .D64(0xa6d445e16ae3d872ULL) 
    .D64(0x5a5beeb38de23be8ULL) 
    .D64(0xaa95fa054aedfbaeULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004005ULL;
  raw_context.rbx = 0xed1b02e8cc0fc79cULL; 
  raw_context.r12 = 0x46b1b8868891b34aULL; 
  raw_context.r13 = 0x40000000c0005510ULL; 
  CheckWalk();
}

TEST_F(CFI, At4006) {
  Label frame0_rbp;
  Label frame1_rsp = expected.rsp;
  stack_section
    .D64(0x043c6dfceb91aa34ULL) 
    .D64(0x1d20ad8acacbe930ULL) 
    .D64(0x68995b1de4700266ULL) 
    .Mark(&frame0_rbp)          
    .D64(0x5a5beeb38de23be8ULL) 
    .D64(0xf015ee516ad89eabULL) 
    .Mark(&frame1_rsp);         
  raw_context.rip = 0x40000000c0004006ULL;
  raw_context.rbp = frame0_rbp.Value();
  raw_context.rbx = 0xed1b02e8cc0fc79cULL; 
  raw_context.r12 = 0x26e007b341acfebdULL; 
  raw_context.r13 = 0x40000000c0005510ULL; 
  CheckWalk();
}
