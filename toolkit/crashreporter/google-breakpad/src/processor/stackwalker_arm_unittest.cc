
































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
#include "processor/stackwalker_arm.h"
#include "processor/windows_frame_info.h"

using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::StackFrame;
using google_breakpad::StackFrameARM;
using google_breakpad::StackwalkerARM;
using google_breakpad::SystemInfo;
using google_breakpad::WindowsFrameInfo;
using google_breakpad::test_assembler::kLittleEndian;
using google_breakpad::test_assembler::Label;
using google_breakpad::test_assembler::Section;
using std::string;
using std::vector;
using testing::_;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

class StackwalkerARMFixture {
 public:
  StackwalkerARMFixture()
    : stack_section(kLittleEndian),
      
      
      module1(0x40000000, 0x10000, "module1", "version1"),
      module2(0x50000000, 0x10000, "module2", "version2") {
    
    system_info.os = "Linux";
    system_info.os_short = "linux";
    system_info.os_version = "Lugubrious Labrador";
    system_info.cpu = "arm";
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

  
  void BrandContext(MDRawContextARM *raw_context) {
    u_int8_t x = 173;
    for (size_t i = 0; i < sizeof(*raw_context); i++)
      reinterpret_cast<u_int8_t *>(raw_context)[i] = (x += 17);
  }
  
  SystemInfo system_info;
  MDRawContextARM raw_context;
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

class GetContextFrame: public StackwalkerARMFixture, public Test { };

TEST_F(GetContextFrame, Simple) {
  
  
  
  StackwalkerARM walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(1U, frames->size());
  StackFrameARM *frame = static_cast<StackFrameARM *>(frames->at(0));
  
  
  EXPECT_TRUE(memcmp(&raw_context, &frame->context, sizeof(raw_context)) == 0);
}

struct CFIFixture: public StackwalkerARMFixture {
  CFIFixture() {
    
    
    
    SetModuleSymbols(&module1,
                     
                     "FUNC 4000 1000 10 enchiridion\n"
                     
                     
                     "STACK CFI INIT 4000 100 .cfa: sp .ra: lr\n"
                     
                     "STACK CFI 4001 .cfa: sp 12 + r4: .cfa 12 - ^"
                     " r11: .cfa 8 - ^ .ra: .cfa 4 - ^\n"
                     
                     
                     "STACK CFI 4002 r4: r0 r5: r1 r6: r2 r7: r3\n"
                     
                     "STACK CFI 4003 .cfa: sp 16 + r1: .cfa 16 - ^"
                     " r4: r4 r5: r5 r6: r6 r7: r7\n"
                     
                     
                     "STACK CFI 4005 .cfa: sp 12 + r1: .cfa 12 - ^"
                     " r11: .cfa 4 - ^ .ra: .cfa ^ sp: .cfa 4 +\n"
                     
                     
                     "STACK CFI 4006 .cfa: sp 16 + pc: .cfa 16 - ^\n"

                     
                     "FUNC 5000 1000 10 epictetus\n"
                     
                     "STACK CFI INIT 5000 1000 .cfa: 0 .ra: 0\n"

                     
                     
                     "FUNC 6000 1000 20 palinal\n"
                     "STACK CFI INIT 6000 1000 .cfa: sp 4 - .ra: lr\n"

                     
                     
                     "FUNC 7000 1000 20 rhetorical\n"
                     "STACK CFI INIT 7000 1000 .cfa: moot .ra: ambiguous\n");

    
    expected.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40005510;
    expected.iregs[MD_CONTEXT_ARM_REG_SP] = 0x80000000;
    expected.iregs[4] = 0xb5d55e68;
    expected.iregs[5] = 0xebd134f3;
    expected.iregs[6] = 0xa31e74bc;
    expected.iregs[7] = 0x2dcb16b3;
    expected.iregs[8] = 0x2ada2137;
    expected.iregs[9] = 0xbbbb557d;
    expected.iregs[10] = 0x48bf8ca7;
    expected.iregs[MD_CONTEXT_ARM_REG_FP] = 0x8112e110;

    
    
    
    
    expected_validity = (StackFrameARM::CONTEXT_VALID_PC |
                         StackFrameARM::CONTEXT_VALID_SP |
                         StackFrameARM::CONTEXT_VALID_R4 |
                         StackFrameARM::CONTEXT_VALID_R5 |
                         StackFrameARM::CONTEXT_VALID_R6 |
                         StackFrameARM::CONTEXT_VALID_R7 |
                         StackFrameARM::CONTEXT_VALID_R8 |
                         StackFrameARM::CONTEXT_VALID_R9 |
                         StackFrameARM::CONTEXT_VALID_R10 |
                         StackFrameARM::CONTEXT_VALID_FP);

    
    context_frame_validity = StackFrameARM::CONTEXT_VALID_ALL;

    
    raw_context = expected;
  }

  
  
  
  
  
  void CheckWalk() {
    RegionFromSection();
    raw_context.iregs[MD_CONTEXT_ARM_REG_SP] = stack_section.start().Value();

    StackwalkerARM walker(&system_info, &raw_context, &stack_region, &modules,
                          &supplier, &resolver);
    walker.SetContextFrameValidity(context_frame_validity);
    ASSERT_TRUE(walker.Walk(&call_stack));
    frames = call_stack.frames();
    ASSERT_EQ(2U, frames->size());

    StackFrameARM *frame0 = static_cast<StackFrameARM *>(frames->at(0));
    ASSERT_EQ(context_frame_validity, frame0->context_validity);
    EXPECT_EQ("enchiridion", frame0->function_name);
    EXPECT_EQ(0x40004000U, frame0->function_base);

    StackFrameARM *frame1 = static_cast<StackFrameARM *>(frames->at(1));
    ASSERT_EQ(expected_validity, frame1->context_validity);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R1)
      EXPECT_EQ(expected.iregs[1], frame1->context.iregs[1]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R4)
      EXPECT_EQ(expected.iregs[4], frame1->context.iregs[4]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R5)
      EXPECT_EQ(expected.iregs[5], frame1->context.iregs[5]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R6)
      EXPECT_EQ(expected.iregs[6], frame1->context.iregs[6]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R7)
      EXPECT_EQ(expected.iregs[7], frame1->context.iregs[7]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R8)
      EXPECT_EQ(expected.iregs[8], frame1->context.iregs[8]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R9)
      EXPECT_EQ(expected.iregs[9], frame1->context.iregs[9]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_R10)
      EXPECT_EQ(expected.iregs[10], frame1->context.iregs[10]);
    if (expected_validity & StackFrameARM::CONTEXT_VALID_FP)
      EXPECT_EQ(expected.iregs[MD_CONTEXT_ARM_REG_FP],
                frame1->context.iregs[MD_CONTEXT_ARM_REG_FP]);

    
    
    EXPECT_EQ(expected.iregs[MD_CONTEXT_ARM_REG_SP],
              frame1->context.iregs[MD_CONTEXT_ARM_REG_SP]);
    EXPECT_EQ(expected.iregs[MD_CONTEXT_ARM_REG_PC],
              frame1->context.iregs[MD_CONTEXT_ARM_REG_PC]);
    EXPECT_EQ(expected.iregs[MD_CONTEXT_ARM_REG_PC],
              frame1->instruction + 1);
    EXPECT_EQ("epictetus", frame1->function_name);
  }

  
  MDRawContextARM expected;

  
  int expected_validity;

  
  int context_frame_validity;
};

class CFI: public CFIFixture, public Test { };

TEST_F(CFI, At4000) {
  stack_section.start() = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004000;
  raw_context.iregs[MD_CONTEXT_ARM_REG_LR] = 0x40005510;
  CheckWalk();
}

TEST_F(CFI, At4001) {
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0xb5d55e68)            
    .D32(0x8112e110)            
    .D32(0x40005510)            
    .Mark(&frame1_sp);          
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004001;
  raw_context.iregs[4] = 0x635adc9f;                     
  raw_context.iregs[MD_CONTEXT_ARM_REG_FP] = 0xbe145fc4; 
  CheckWalk();
}


TEST_F(CFI, At4001LimitedValidity) {
  context_frame_validity =
    StackFrameARM::CONTEXT_VALID_PC | StackFrameARM::CONTEXT_VALID_SP;
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004001;
  raw_context.iregs[MD_CONTEXT_ARM_REG_FP] = 0xbe145fc4; 
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0xb5d55e68)            
    .D32(0x8112e110)            
    .D32(0x40005510)            
    .Mark(&frame1_sp);          
  expected_validity = (StackFrameARM::CONTEXT_VALID_PC
                       | StackFrameARM::CONTEXT_VALID_SP
                       | StackFrameARM::CONTEXT_VALID_FP
                       | StackFrameARM::CONTEXT_VALID_R4);
  CheckWalk();
}

TEST_F(CFI, At4002) {
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0xfb81ff3d)            
    .D32(0x8112e110)            
    .D32(0x40005510)            
    .Mark(&frame1_sp);          
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004002;
  raw_context.iregs[0] = 0xb5d55e68;  
  raw_context.iregs[1] = 0xebd134f3;  
  raw_context.iregs[2] = 0xa31e74bc;  
  raw_context.iregs[3] = 0x2dcb16b3;  
  raw_context.iregs[4] = 0xfdd35466;  
  raw_context.iregs[5] = 0xf18c946c;  
  raw_context.iregs[6] = 0xac2079e8;  
  raw_context.iregs[7] = 0xa449829f;  
  raw_context.iregs[MD_CONTEXT_ARM_REG_FP] = 0xbe145fc4; 
  CheckWalk();
}

TEST_F(CFI, At4003) {
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0x48c8dd5a)            
    .D32(0xcb78040e)            
    .D32(0x8112e110)            
    .D32(0x40005510)            
    .Mark(&frame1_sp);          
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004003;
  raw_context.iregs[1] = 0xfb756319;                     
  raw_context.iregs[MD_CONTEXT_ARM_REG_FP] = 0x0a2857ea; 
  expected.iregs[1] = 0x48c8dd5a;    
  expected_validity |= StackFrameARM::CONTEXT_VALID_R1;
  CheckWalk();
}



TEST_F(CFI, At4004) {
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0x48c8dd5a)            
    .D32(0xcb78040e)            
    .D32(0x8112e110)            
    .D32(0x40005510)            
    .Mark(&frame1_sp);          
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004004;
  raw_context.iregs[1] = 0xfb756319; 
  expected.iregs[1] = 0x48c8dd5a; 
  expected_validity |= StackFrameARM::CONTEXT_VALID_R1;
  CheckWalk();
}



TEST_F(CFI, At4005) {
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0x48c8dd5a)            
    .D32(0xf013f841)            
    .D32(0x8112e110)            
    .D32(0x40005510)            
    .Mark(&frame1_sp);          
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004005;
  raw_context.iregs[1] = 0xfb756319; 
  expected.iregs[1] = 0x48c8dd5a; 
  expected_validity |= StackFrameARM::CONTEXT_VALID_R1;
  CheckWalk();
}



TEST_F(CFI, At4006) {
  Label frame1_sp = expected.iregs[MD_CONTEXT_ARM_REG_SP];
  stack_section
    .D32(0x40005510)            
    .D32(0x48c8dd5a)            
    .D32(0xf013f841)            
    .D32(0x8112e110)            
    .D32(0xf8d15783)            
    .Mark(&frame1_sp);          
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40004006;
  raw_context.iregs[1] = 0xfb756319; 
  expected.iregs[1] = 0x48c8dd5a; 
  expected_validity |= StackFrameARM::CONTEXT_VALID_R1;
  CheckWalk();
}



TEST_F(CFI, RejectBackwards) {
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40006000;
  raw_context.iregs[MD_CONTEXT_ARM_REG_SP] = 0x80000000;  
  raw_context.iregs[MD_CONTEXT_ARM_REG_LR] = 0x40005510;
  StackwalkerARM walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(1U, frames->size());
}


TEST_F(CFI, RejectBadExpressions) {
  raw_context.iregs[MD_CONTEXT_ARM_REG_PC] = 0x40007000;
  raw_context.iregs[MD_CONTEXT_ARM_REG_SP] = 0x80000000;  
  StackwalkerARM walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(1U, frames->size());
}

