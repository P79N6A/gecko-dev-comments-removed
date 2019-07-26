
































#include <string>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/dwarf_cfi_to_module.h"
#include "common/using_std_string.h"

using std::vector;

using google_breakpad::Module;
using google_breakpad::DwarfCFIToModule;
using testing::ContainerEq;
using testing::Test;
using testing::_;

struct MockCFIReporter: public DwarfCFIToModule::Reporter {
  MockCFIReporter(const string &file, const string &section)
      : Reporter(file, section) { }
  MOCK_METHOD2(UnnamedRegister, void(size_t offset, int reg));
  MOCK_METHOD2(UndefinedNotSupported, void(size_t offset,
                                           const UniqueString* reg));
  MOCK_METHOD2(ExpressionsNotSupported, void(size_t offset,
                                             const UniqueString* reg));
};

struct DwarfCFIToModuleFixture {
  DwarfCFIToModuleFixture()
      : module("module name", "module os", "module arch", "module id"),
        reporter("reporter file", "reporter section"),
        handler(&module, register_names, &reporter) {
    register_names.push_back(toUniqueString("reg0"));
    register_names.push_back(toUniqueString("reg1"));
    register_names.push_back(toUniqueString("reg2"));
    register_names.push_back(toUniqueString("reg3"));
    register_names.push_back(toUniqueString("reg4"));
    register_names.push_back(toUniqueString("reg5"));
    register_names.push_back(toUniqueString("reg6"));
    register_names.push_back(toUniqueString("reg7"));
    register_names.push_back(toUniqueString("sp"));
    register_names.push_back(toUniqueString("pc"));
    register_names.push_back(ustr__empty());

    EXPECT_CALL(reporter, UnnamedRegister(_, _)).Times(0);
    EXPECT_CALL(reporter, UndefinedNotSupported(_, _)).Times(0);
    EXPECT_CALL(reporter, ExpressionsNotSupported(_, _)).Times(0);
  }

  Module module;
  vector<const UniqueString*> register_names;
  MockCFIReporter reporter;
  DwarfCFIToModule handler;
  vector<Module::StackFrameEntry *> entries;
};

class Entry: public DwarfCFIToModuleFixture, public Test { };

TEST_F(Entry, Accept) {
  ASSERT_TRUE(handler.Entry(0x3b8961b8, 0xa21069698096fc98ULL,
                            0xb440ce248169c8d6ULL, 3, "", 0xea93c106));
  ASSERT_TRUE(handler.End());
  module.GetStackFrameEntries(&entries);
  EXPECT_EQ(1U, entries.size());
  EXPECT_EQ(0xa21069698096fc98ULL, entries[0]->address);
  EXPECT_EQ(0xb440ce248169c8d6ULL, entries[0]->size);
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Entry, AcceptOldVersion) {
  ASSERT_TRUE(handler.Entry(0xeb60e0fc, 0x75b8806bb09eab78ULL,
                            0xc771f44958d40bbcULL, 1, "", 0x093c945e));
  ASSERT_TRUE(handler.End());
  module.GetStackFrameEntries(&entries);
  EXPECT_EQ(1U, entries.size());
  EXPECT_EQ(0x75b8806bb09eab78ULL, entries[0]->address);
  EXPECT_EQ(0xc771f44958d40bbcULL, entries[0]->size);
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

struct RuleFixture: public DwarfCFIToModuleFixture {
  RuleFixture() : DwarfCFIToModuleFixture() {
    entry_address = 0x89327ebf86b47492ULL;
    entry_size    = 0x2f8cd573072fe02aULL;
    return_reg    = 0x7886a346;
  }
  void StartEntry() {
    ASSERT_TRUE(handler.Entry(0x4445c05c, entry_address, entry_size,
                              3, "", return_reg));
  }
  void CheckEntry() {
    module.GetStackFrameEntries(&entries);
    EXPECT_EQ(1U, entries.size());
    EXPECT_EQ(entry_address, entries[0]->address);
    EXPECT_EQ(entry_size, entries[0]->size);
  }
  uint64 entry_address, entry_size;
  unsigned return_reg;
};

class Rule: public RuleFixture, public Test { };

TEST_F(Rule, UndefinedRule) {
  EXPECT_CALL(reporter, UndefinedNotSupported(_, toUniqueString("reg7")));
  StartEntry();
  ASSERT_TRUE(handler.UndefinedRule(entry_address, 7));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, RegisterWithEmptyName) {
  EXPECT_CALL(reporter, UnnamedRegister(_, 10));
  EXPECT_CALL(reporter,
              UndefinedNotSupported(_, toUniqueString("unnamed_register10")));
  StartEntry();
  ASSERT_TRUE(handler.UndefinedRule(entry_address, 10));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, SameValueRule) {
  StartEntry();
  ASSERT_TRUE(handler.SameValueRule(entry_address, 6));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  Module::RuleMap expected_initial;
  const UniqueString* reg6 = toUniqueString("reg6");
  expected_initial[reg6] = Module::Expr(reg6, 0, false);
  EXPECT_THAT(entries[0]->initial_rules, ContainerEq(expected_initial));
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, OffsetRule) {
  StartEntry();
  ASSERT_TRUE(handler.OffsetRule(entry_address + 1, return_reg,
                                 DwarfCFIToModule::kCFARegister,
                                 16927065));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  Module::RuleChangeMap expected_changes;
  expected_changes[entry_address + 1][ustr__ZDra()] =
      Module::Expr(ustr__ZDcfa(), 16927065, true);
  EXPECT_THAT(entries[0]->rule_changes, ContainerEq(expected_changes));
}

TEST_F(Rule, OffsetRuleNegative) {
  StartEntry();
  ASSERT_TRUE(handler.OffsetRule(entry_address + 1,
                                 DwarfCFIToModule::kCFARegister, 4, -34530721));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  Module::RuleChangeMap expected_changes;
  expected_changes[entry_address + 1][ustr__ZDcfa()] =
      Module::Expr(toUniqueString("reg4"), -34530721, true);
  EXPECT_THAT(entries[0]->rule_changes, ContainerEq(expected_changes));
}

TEST_F(Rule, ValOffsetRule) {
  
  EXPECT_CALL(reporter, UnnamedRegister(_, 11));
  StartEntry();
  ASSERT_TRUE(handler.ValOffsetRule(entry_address + 0x5ab7,
                                    DwarfCFIToModule::kCFARegister,
                                    11, 61812979));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  Module::RuleChangeMap expected_changes;
  expected_changes[entry_address + 0x5ab7][ustr__ZDcfa()] =
      Module::Expr(toUniqueString("unnamed_register11"), 61812979, false);
  EXPECT_THAT(entries[0]->rule_changes, ContainerEq(expected_changes));
}

TEST_F(Rule, RegisterRule) {
  StartEntry();
  ASSERT_TRUE(handler.RegisterRule(entry_address, return_reg, 3));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  Module::RuleMap expected_initial;
  expected_initial[ustr__ZDra()] =
      Module::Expr(toUniqueString("reg3"), 0, false);
  EXPECT_THAT(entries[0]->initial_rules, ContainerEq(expected_initial));
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, ExpressionRule) {
  EXPECT_CALL(reporter, ExpressionsNotSupported(_, toUniqueString("reg2")));
  StartEntry();
  ASSERT_TRUE(handler.ExpressionRule(entry_address + 0xf326, 2,
                                     "it takes two to tango"));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, ValExpressionRule) {
  EXPECT_CALL(reporter, ExpressionsNotSupported(_, toUniqueString("reg0")));
  StartEntry();
  ASSERT_TRUE(handler.ValExpressionRule(entry_address + 0x6367, 0,
                                        "bit off more than he could chew"));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  EXPECT_EQ(0U, entries[0]->initial_rules.size());
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, DefaultReturnAddressRule) {
  return_reg = 2;
  StartEntry();
  ASSERT_TRUE(handler.RegisterRule(entry_address, 0, 1));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  Module::RuleMap expected_initial;
  expected_initial[ustr__ZDra()] =
      Module::Expr(toUniqueString("reg2"), 0, false);
  expected_initial[toUniqueString("reg0")] =
      Module::Expr(toUniqueString("reg1"), 0, false);
  EXPECT_THAT(entries[0]->initial_rules, ContainerEq(expected_initial));
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, DefaultReturnAddressRuleOverride) {
  return_reg = 2;
  StartEntry();
  ASSERT_TRUE(handler.RegisterRule(entry_address, return_reg, 1));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  Module::RuleMap expected_initial;
  expected_initial[ustr__ZDra()] =
      Module::Expr(toUniqueString("reg1"), 0, false);
  EXPECT_THAT(entries[0]->initial_rules, ContainerEq(expected_initial));
  EXPECT_EQ(0U, entries[0]->rule_changes.size());
}

TEST_F(Rule, DefaultReturnAddressRuleLater) {
  return_reg = 2;
  StartEntry();
  ASSERT_TRUE(handler.RegisterRule(entry_address + 1, return_reg, 1));
  ASSERT_TRUE(handler.End());
  CheckEntry();
  Module::RuleMap expected_initial;
  expected_initial[ustr__ZDra()] =
      Module::Expr(toUniqueString("reg2"), 0, false);
  EXPECT_THAT(entries[0]->initial_rules, ContainerEq(expected_initial));
  Module::RuleChangeMap expected_changes;
  expected_changes[entry_address + 1][ustr__ZDra()] =
      Module::Expr(toUniqueString("reg1"), 0, false);
  EXPECT_THAT(entries[0]->rule_changes, ContainerEq(expected_changes));
}

TEST(RegisterNames, I386) {
  vector<const UniqueString*> names = DwarfCFIToModule::RegisterNames::I386();

  EXPECT_EQ(toUniqueString("$eax"), names[0]);
  EXPECT_EQ(toUniqueString("$ecx"), names[1]);
  EXPECT_EQ(toUniqueString("$esp"), names[4]);
  EXPECT_EQ(toUniqueString("$eip"), names[8]);
}

TEST(RegisterNames, ARM) {
  vector<const UniqueString*> names = DwarfCFIToModule::RegisterNames::ARM();

  EXPECT_EQ(toUniqueString("r0"), names[0]);
  EXPECT_EQ(toUniqueString("r10"), names[10]);
  EXPECT_EQ(toUniqueString("sp"), names[13]);
  EXPECT_EQ(toUniqueString("lr"), names[14]);
  EXPECT_EQ(toUniqueString("pc"), names[15]);
}

TEST(RegisterNames, X86_64) {
  vector<const UniqueString*> names = DwarfCFIToModule::RegisterNames::X86_64();

  EXPECT_EQ(toUniqueString("$rax"), names[0]);
  EXPECT_EQ(toUniqueString("$rdx"), names[1]);
  EXPECT_EQ(toUniqueString("$rbp"), names[6]);
  EXPECT_EQ(toUniqueString("$rsp"), names[7]);
  EXPECT_EQ(toUniqueString("$rip"), names[16]);
}
