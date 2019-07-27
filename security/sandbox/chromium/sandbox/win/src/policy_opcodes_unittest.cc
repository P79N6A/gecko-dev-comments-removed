



#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/sandbox_nt_types.h"
#include "sandbox/win/src/policy_engine_params.h"
#include "sandbox/win/src/policy_engine_opcodes.h"
#include "testing/gtest/include/gtest/gtest.h"


#define INIT_GLOBAL_RTL(member) \
  g_nt.member = reinterpret_cast<member##Function>( \
  ::GetProcAddress(ntdll, #member)); \
  if (NULL == g_nt.member) \
  return false

namespace sandbox {

SANDBOX_INTERCEPT NtExports g_nt;

bool SetupNtdllImports() {
    HMODULE ntdll = ::GetModuleHandle(kNtdllName);

    INIT_GLOBAL_RTL(RtlAllocateHeap);
    INIT_GLOBAL_RTL(RtlAnsiStringToUnicodeString);
    INIT_GLOBAL_RTL(RtlCompareUnicodeString);
    INIT_GLOBAL_RTL(RtlCreateHeap);
    INIT_GLOBAL_RTL(RtlDestroyHeap);
    INIT_GLOBAL_RTL(RtlFreeHeap);
    INIT_GLOBAL_RTL(_strnicmp);
    INIT_GLOBAL_RTL(strlen);
    INIT_GLOBAL_RTL(wcslen);

  return true;
}

TEST(PolicyEngineTest, ParameterSetTest) {
  void* pv1 = reinterpret_cast<void*>(0x477EAA5);
  const void* pv2 = reinterpret_cast<void*>(0x987654);
  ParameterSet pset1 = ParamPickerMake(pv1);
  ParameterSet pset2 = ParamPickerMake(pv2);

  
  const void* result1 =0;
  unsigned long result2 = 0;
  EXPECT_TRUE(pset1.Get(&result1));
  EXPECT_TRUE(pv1 == result1);
  EXPECT_FALSE(pset1.Get(&result2));
  EXPECT_TRUE(pset2.Get(&result1));
  EXPECT_TRUE(pv2 == result1);
  EXPECT_FALSE(pset2.Get(&result2));

  
  unsigned long number = 12747;
  ParameterSet pset3 = ParamPickerMake(number);
  EXPECT_FALSE(pset3.Get(&result1));
  EXPECT_TRUE(pset3.Get(&result2));
  EXPECT_EQ(number, result2);

  
  const wchar_t* txt = L"S231L";
  ParameterSet pset4 = ParamPickerMake(txt);
  const wchar_t* result3 = NULL;
  EXPECT_TRUE(pset4.Get(&result3));
  EXPECT_EQ(0, wcscmp(txt, result3));
}

TEST(PolicyEngineTest, OpcodeConstraints) {
  
  
  
  EXPECT_FALSE(__is_polymorphic(PolicyOpcode));
  
  
  EXPECT_TRUE(__has_trivial_destructor(PolicyOpcode));
  EXPECT_TRUE(__has_trivial_constructor(PolicyOpcode));
  EXPECT_TRUE(__has_trivial_copy(PolicyOpcode));
}

TEST(PolicyEngineTest, TrueFalseOpcodes) {
  void* dummy = NULL;
  ParameterSet ppb1 = ParamPickerMake(dummy);
  char memory[1024];
  OpcodeFactory opcode_maker(memory, sizeof(memory));

  
  PolicyOpcode* op1 = opcode_maker.MakeOpAlwaysFalse(kPolNone);
  EXPECT_EQ(EVAL_FALSE, op1->Evaluate(&ppb1, 1, NULL));
  EXPECT_FALSE(op1->IsAction());

  
  PolicyOpcode* op2 = opcode_maker.MakeOpAlwaysTrue(kPolNone);
  EXPECT_EQ(EVAL_TRUE, op2->Evaluate(&ppb1, 1, NULL));

  
  EXPECT_EQ(EVAL_ERROR, op2->Evaluate(NULL, 0, NULL));
  EXPECT_EQ(EVAL_ERROR, op2->Evaluate(NULL, 1, NULL));

  
  EXPECT_EQ(EVAL_TRUE, op2->Evaluate(&ppb1, 0, NULL));
  EXPECT_EQ(EVAL_TRUE, op2->Evaluate(&ppb1, 1, NULL));

  
  
  
  PolicyOpcode* op3 = opcode_maker.MakeOpAlwaysFalse(kPolNegateEval);
  EXPECT_EQ(EVAL_TRUE, op3->Evaluate(&ppb1, 1, NULL));
  PolicyOpcode* op4 = opcode_maker.MakeOpAlwaysTrue(kPolNegateEval);
  EXPECT_EQ(EVAL_FALSE, op4->Evaluate(&ppb1, 1, NULL));

  
  PolicyOpcode* op5 = opcode_maker.MakeOpAlwaysTrue(kPolClearContext);
  MatchContext context;
  context.position = 1;
  context.options = kPolUseOREval;
  EXPECT_EQ(EVAL_TRUE, op5->Evaluate(&ppb1, 1, &context));
  EXPECT_EQ(0, context.position);
  MatchContext context2;
  EXPECT_EQ(context2.options, context.options);
}

TEST(PolicyEngineTest, OpcodeMakerCase1) {
  
  
  void* dummy = NULL;
  ParameterSet ppb1 = ParamPickerMake(dummy);

  char memory[256];
  OpcodeFactory opcode_maker(memory, sizeof(memory));
  size_t count = sizeof(memory) / sizeof(PolicyOpcode);

  for (size_t ix =0; ix != count; ++ix) {
     PolicyOpcode* op = opcode_maker.MakeOpAlwaysFalse(kPolNone);
     ASSERT_TRUE(NULL != op);
     EXPECT_EQ(EVAL_FALSE, op->Evaluate(&ppb1, 1, NULL));
  }
  
  PolicyOpcode* op1 = opcode_maker.MakeOpAlwaysFalse(kPolNone);
  ASSERT_TRUE(NULL == op1);
}

TEST(PolicyEngineTest, OpcodeMakerCase2) {
  SetupNtdllImports();
  
  
  
  
  const wchar_t* txt1 = L"1234";
  const wchar_t txt2[] = L"123";

  ParameterSet ppb1 = ParamPickerMake(txt1);
  MatchContext mc1;

  char memory[256];
  OpcodeFactory opcode_maker(memory, sizeof(memory));
  size_t count = sizeof(memory) / (sizeof(PolicyOpcode) + sizeof(txt2));

  
  for (size_t ix =0; ix != count; ++ix) {
    PolicyOpcode* op = opcode_maker.MakeOpWStringMatch(0, txt2, 0,
                                                       CASE_SENSITIVE,
                                                       kPolClearContext);
    ASSERT_TRUE(NULL != op);
    EXPECT_EQ(EVAL_TRUE, op->Evaluate(&ppb1, 1, &mc1));
  }

  
  PolicyOpcode* op1 = opcode_maker.MakeOpWStringMatch(0, txt2, 0,
                                                      CASE_SENSITIVE,
                                                      kPolNone);
  ASSERT_TRUE(NULL == op1);
}

TEST(PolicyEngineTest, IntegerOpcodes) {
  const wchar_t* txt = L"abcdef";
  unsigned long num1 = 42;
  unsigned long num2 = 113377;

  ParameterSet pp_wrong1 = ParamPickerMake(txt);
  ParameterSet pp_num1 = ParamPickerMake(num1);
  ParameterSet pp_num2 = ParamPickerMake(num2);

  char memory[128];
  OpcodeFactory opcode_maker(memory, sizeof(memory));

  
  PolicyOpcode* op_m42 = opcode_maker.MakeOpNumberMatch(0, 42UL, kPolNone);
  EXPECT_EQ(EVAL_TRUE, op_m42->Evaluate(&pp_num1, 1, NULL));
  EXPECT_EQ(EVAL_FALSE, op_m42->Evaluate(&pp_num2, 1, NULL));
  EXPECT_EQ(EVAL_ERROR, op_m42->Evaluate(&pp_wrong1, 1, NULL));

  
  const void* vp = NULL;
  ParameterSet pp_num3 = ParamPickerMake(vp);
  PolicyOpcode* op_vp_null = opcode_maker.MakeOpVoidPtrMatch(0, NULL,
                                                             kPolNone);
  EXPECT_EQ(EVAL_TRUE, op_vp_null->Evaluate(&pp_num3, 1, NULL));
  EXPECT_EQ(EVAL_FALSE, op_vp_null->Evaluate(&pp_num1, 1, NULL));
  EXPECT_EQ(EVAL_ERROR, op_vp_null->Evaluate(&pp_wrong1, 1, NULL));

  
  PolicyOpcode* op_range1 = opcode_maker.MakeOpUlongMatchRange(0, 41, 43,
                                                               kPolNone);
  EXPECT_EQ(EVAL_TRUE, op_range1->Evaluate(&pp_num1, 1, NULL));
  EXPECT_EQ(EVAL_FALSE, op_range1->Evaluate(&pp_num2, 1, NULL));
  EXPECT_EQ(EVAL_ERROR, op_range1->Evaluate(&pp_wrong1, 1, NULL));
}

TEST(PolicyEngineTest, LogicalOpcodes) {
  char memory[128];
  OpcodeFactory opcode_maker(memory, sizeof(memory));

  unsigned long num1 = 0x10100702;
  ParameterSet pp_num1 = ParamPickerMake(num1);

  PolicyOpcode* op_and1 = opcode_maker.MakeOpUlongAndMatch(0, 0x00100000,
                                                           kPolNone);
  EXPECT_EQ(EVAL_TRUE, op_and1->Evaluate(&pp_num1, 1, NULL));
  PolicyOpcode* op_and2 = opcode_maker.MakeOpUlongAndMatch(0, 0x00000001,
                                                           kPolNone);
  EXPECT_EQ(EVAL_FALSE, op_and2->Evaluate(&pp_num1, 1, NULL));
}

TEST(PolicyEngineTest, WCharOpcodes1) {
  SetupNtdllImports();

  const wchar_t* txt1 = L"the quick fox jumps over the lazy dog";
  const wchar_t txt2[] = L"the quick";
  const wchar_t txt3[] = L" fox jumps";
  const wchar_t txt4[] = L"the lazy dog";
  const wchar_t txt5[] = L"jumps over";
  const wchar_t txt6[] = L"g";

  ParameterSet pp_tc1 = ParamPickerMake(txt1);
  char memory[512];
  OpcodeFactory opcode_maker(memory, sizeof(memory));

  PolicyOpcode* op1 = opcode_maker.MakeOpWStringMatch(0, txt2, 0,
                                                      CASE_SENSITIVE,
                                                      kPolNone);

  
  
  MatchContext mc1;
  EXPECT_EQ(EVAL_TRUE, op1->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_TRUE(_countof(txt2) == mc1.position + 1);

  
  EXPECT_EQ(EVAL_FALSE, op1->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_TRUE(_countof(txt2) == mc1.position + 1);

  
  
  PolicyOpcode* op3 = opcode_maker.MakeOpWStringMatch(0, txt3, 0,
                                                      CASE_SENSITIVE,
                                                      kPolNone);
  EXPECT_EQ(EVAL_TRUE, op3->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_TRUE(_countof(txt3) + _countof(txt2) == mc1.position + 2);

  
  
  
  PolicyOpcode* op4 = opcode_maker.MakeOpWStringMatch(0, txt4, 6,
                                                      CASE_SENSITIVE,
                                                      kPolClearContext);
  EXPECT_EQ(EVAL_TRUE, op4->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_EQ(0, mc1.position);

  
  PolicyOpcode* op4b = opcode_maker.MakeOpWStringMatch(0, txt4, kSeekToEnd,
                                                       CASE_SENSITIVE,
                                                       kPolClearContext);
  EXPECT_EQ(EVAL_TRUE, op4b->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_EQ(0, mc1.position);

  
  
  PolicyOpcode* op5 = opcode_maker.MakeOpWStringMatch(0, txt5, kSeekForward,
                                                      CASE_SENSITIVE, kPolNone);
  EXPECT_EQ(EVAL_TRUE, op5->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_EQ(24, mc1.position);

  
  PolicyOpcode* op5b = opcode_maker.MakeOpWStringMatch(0, txt5, kSeekToEnd,
                                                       CASE_SENSITIVE,
                                                       kPolNone);
  EXPECT_EQ(EVAL_FALSE, op5b->Evaluate(&pp_tc1, 1, &mc1));

  
  
  PolicyOpcode* op6 = opcode_maker.MakeOpWStringMatch(0, txt4, 2,
                                                      CASE_SENSITIVE, kPolNone);
  EXPECT_EQ(24, mc1.position);

  
  MatchContext mc2;
  PolicyOpcode* op7 = opcode_maker.MakeOpWStringMatch(0, txt6, kSeekForward,
                                                      CASE_SENSITIVE, kPolNone);
  EXPECT_EQ(EVAL_TRUE, op7->Evaluate(&pp_tc1, 1, &mc2));

  
  
  EXPECT_EQ(EVAL_FALSE, op7->Evaluate(&pp_tc1, 1, &mc2));
}

TEST(PolicyEngineTest, WCharOpcodes2) {
  SetupNtdllImports();

  const wchar_t* path1 = L"c:\\documents and settings\\Microsoft\\BLAH.txt";
  const wchar_t txt1[] = L"Settings\\microsoft";
  ParameterSet pp_tc1 = ParamPickerMake(path1);

  char memory[256];
  OpcodeFactory opcode_maker(memory, sizeof(memory));
  MatchContext mc1;

  
  
  
  PolicyOpcode* op1s = opcode_maker.MakeOpWStringMatch(0, txt1, kSeekForward,
                                                      CASE_SENSITIVE, kPolNone);
  PolicyOpcode* op1i = opcode_maker.MakeOpWStringMatch(0, txt1, kSeekForward,
                                                       CASE_INSENSITIVE,
                                                       kPolNone);
  EXPECT_EQ(EVAL_FALSE, op1s->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_EQ(EVAL_TRUE, op1i->Evaluate(&pp_tc1, 1, &mc1));
  EXPECT_EQ(35, mc1.position);
}

TEST(PolicyEngineTest, ActionOpcodes) {
  char memory[256];
  OpcodeFactory opcode_maker(memory, sizeof(memory));
  MatchContext mc1;
  void* dummy = NULL;
  ParameterSet ppb1 = ParamPickerMake(dummy);

  PolicyOpcode* op1 = opcode_maker.MakeOpAction(ASK_BROKER, kPolNone);
  EXPECT_TRUE(op1->IsAction());
  EXPECT_EQ(ASK_BROKER, op1->Evaluate(&ppb1, 1, &mc1));
}

}  
