

































#include <elf.h>

#include <string>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/linux/elf_symbols_to_module.h"
#include "common/linux/synth_elf.h"
#include "common/module.h"
#include "common/test_assembler.h"
#include "common/using_std_string.h"

using google_breakpad::Module;
using google_breakpad::synth_elf::StringTable;
using google_breakpad::test_assembler::Endianness;
using google_breakpad::test_assembler::kBigEndian;
using google_breakpad::test_assembler::kLittleEndian;
using google_breakpad::test_assembler::Label;
using google_breakpad::test_assembler::Section;
using ::testing::Test;
using ::testing::TestWithParam;
using std::vector;

class ELFSymbolsToModuleTestFixture {
public:
  ELFSymbolsToModuleTestFixture(Endianness endianness,
                                size_t value_size) : module("a", "b", "c", "d"),
                                                     section(endianness),
                                                     table(endianness),
                                                     value_size(value_size) {}

  bool ProcessSection() {
    string section_contents, table_contents;
    section.GetContents(&section_contents);
    table.GetContents(&table_contents);

    bool ret = ELFSymbolsToModule(reinterpret_cast<const uint8_t*>(section_contents.data()),
                                  section_contents.size(),
                                  reinterpret_cast<const uint8_t*>(table_contents.data()),
                                  table_contents.size(),
                                  section.endianness() == kBigEndian,
                                  value_size,
                                  &module);
    module.GetExterns(&externs, externs.end());
    return ret;
  }

  Module module;
  Section section;
  StringTable table;
  string section_contents;
  
  size_t value_size;

  vector<Module::Extern *> externs;
};

class ELFSymbolsToModuleTest32 : public ELFSymbolsToModuleTestFixture,
                                 public TestWithParam<Endianness>  {
public:
  ELFSymbolsToModuleTest32() : ELFSymbolsToModuleTestFixture(GetParam(), 4) {}

  void AddElf32Sym(const string& name, uint32_t value,
                   uint32_t size, unsigned info, uint16_t shndx) {
    section
      .D32(table.Add(name))
      .D32(value)
      .D32(size)
      .D8(info)
      .D8(0) 
      .D16(shndx);
  }
};

TEST_P(ELFSymbolsToModuleTest32, NoFuncs) {
  ProcessSection();

  ASSERT_EQ((size_t)0, externs.size());
}

TEST_P(ELFSymbolsToModuleTest32, OneFunc) {
  const string kFuncName = "superfunc";
  const uint32_t kFuncAddr = 0x1000;
  const uint32_t kFuncSize = 0x10;

  AddElf32Sym(kFuncName, kFuncAddr, kFuncSize,
              ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
              
              SHN_UNDEF + 1);

  ProcessSection();

  ASSERT_EQ((size_t)1, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr, extern1->address);
}

TEST_P(ELFSymbolsToModuleTest32, NameOutOfBounds) {
  const string kFuncName = "";
  const uint32_t kFuncAddr = 0x1000;
  const uint32_t kFuncSize = 0x10;

  table.Add("Foo");
  table.Add("Bar");
  
  section
    .D32((uint32_t)table.Here().Value() + 1)
    .D32(kFuncAddr)
    .D32(kFuncSize)
    .D8(ELF32_ST_INFO(STB_GLOBAL, STT_FUNC))
    .D8(0) 
    .D16(SHN_UNDEF + 1);

  ProcessSection();

  ASSERT_EQ((size_t)1, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr, extern1->address);
}

TEST_P(ELFSymbolsToModuleTest32, NonTerminatedStringTable) {
  const string kFuncName = "";
  const uint32_t kFuncAddr = 0x1000;
  const uint32_t kFuncSize = 0x10;

  table.Add("Foo");
  table.Add("Bar");
  
  Label l;
  table
    .Mark(&l)
    .Append("Unterminated");
  
  section
    .D32((uint32_t)l.Value())
    .D32(kFuncAddr)
    .D32(kFuncSize)
    .D8(ELF32_ST_INFO(STB_GLOBAL, STT_FUNC))
    .D8(0) 
    .D16(SHN_UNDEF + 1);

  ProcessSection();

  ASSERT_EQ((size_t)1, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr, extern1->address);
}

TEST_P(ELFSymbolsToModuleTest32, MultipleFuncs) {
  const string kFuncName1 = "superfunc";
  const uint32_t kFuncAddr1 = 0x10001000;
  const uint32_t kFuncSize1 = 0x10;
  const string kFuncName2 = "awesomefunc";
  const uint32_t kFuncAddr2 = 0x20002000;
  const uint32_t kFuncSize2 = 0x2f;
  const string kFuncName3 = "megafunc";
  const uint32_t kFuncAddr3 = 0x30003000;
  const uint32_t kFuncSize3 = 0x3c;

  AddElf32Sym(kFuncName1, kFuncAddr1, kFuncSize1,
              ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
              
              SHN_UNDEF + 1);
  AddElf32Sym(kFuncName2, kFuncAddr2, kFuncSize2,
              ELF32_ST_INFO(STB_LOCAL, STT_FUNC),
              
              SHN_UNDEF + 2);
  AddElf32Sym(kFuncName3, kFuncAddr3, kFuncSize3,
              ELF32_ST_INFO(STB_LOCAL, STT_FUNC),
              
              SHN_UNDEF + 3);

  ProcessSection();

  ASSERT_EQ((size_t)3, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName1, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr1, extern1->address);
  Module::Extern *extern2 = externs[1];
  EXPECT_EQ(kFuncName2, extern2->name);
  EXPECT_EQ((Module::Address)kFuncAddr2, extern2->address);
  Module::Extern *extern3 = externs[2];
  EXPECT_EQ(kFuncName3, extern3->name);
  EXPECT_EQ((Module::Address)kFuncAddr3, extern3->address);
}

TEST_P(ELFSymbolsToModuleTest32, SkipStuff) {
  const string kFuncName = "superfunc";
  const uint32_t kFuncAddr = 0x1000;
  const uint32_t kFuncSize = 0x10;

  
  AddElf32Sym("skipme", 0xFFFF, 0x10,
              ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
              SHN_UNDEF);
  AddElf32Sym(kFuncName, kFuncAddr, kFuncSize,
              ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
              
              SHN_UNDEF + 1);
  
  AddElf32Sym("skipmetoo", 0xAAAA, 0x10,
              ELF32_ST_INFO(STB_GLOBAL, STT_FILE),
              SHN_UNDEF + 1);

  ProcessSection();

  ASSERT_EQ((size_t)1, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr, extern1->address);
}


INSTANTIATE_TEST_CASE_P(Endian,
                        ELFSymbolsToModuleTest32,
                        ::testing::Values(kLittleEndian, kBigEndian));








class ELFSymbolsToModuleTest64 : public ELFSymbolsToModuleTestFixture,
                                 public TestWithParam<Endianness>  {
public:
  ELFSymbolsToModuleTest64() : ELFSymbolsToModuleTestFixture(GetParam(), 8) {}

  void AddElf64Sym(const string& name, uint64_t value,
                   uint64_t size, unsigned info, uint16_t shndx) {
    section
      .D32(table.Add(name))
      .D8(info)
      .D8(0) 
      .D16(shndx)
      .D64(value)
      .D64(size);
  }
};

TEST_P(ELFSymbolsToModuleTest64, NoFuncs) {
  ProcessSection();

  ASSERT_EQ((size_t)0, externs.size());
}

TEST_P(ELFSymbolsToModuleTest64, OneFunc) {
  const string kFuncName = "superfunc";
  const uint64_t kFuncAddr = 0x1000200030004000ULL;
  const uint64_t kFuncSize = 0x1000;

  AddElf64Sym(kFuncName, kFuncAddr, kFuncSize,
              ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
              
              SHN_UNDEF + 1);

  ProcessSection();

  ASSERT_EQ((size_t)1, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr, extern1->address);
}

TEST_P(ELFSymbolsToModuleTest64, MultipleFuncs) {
  const string kFuncName1 = "superfunc";
  const uint64_t kFuncAddr1 = 0x1000100010001000ULL;
  const uint64_t kFuncSize1 = 0x1000;
  const string kFuncName2 = "awesomefunc";
  const uint64_t kFuncAddr2 = 0x2000200020002000ULL;
  const uint64_t kFuncSize2 = 0x2f00;
  const string kFuncName3 = "megafunc";
  const uint64_t kFuncAddr3 = 0x3000300030003000ULL;
  const uint64_t kFuncSize3 = 0x3c00;

  AddElf64Sym(kFuncName1, kFuncAddr1, kFuncSize1,
              ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
              
              SHN_UNDEF + 1);
  AddElf64Sym(kFuncName2, kFuncAddr2, kFuncSize2,
              ELF64_ST_INFO(STB_LOCAL, STT_FUNC),
              
              SHN_UNDEF + 2);
  AddElf64Sym(kFuncName3, kFuncAddr3, kFuncSize3,
              ELF64_ST_INFO(STB_LOCAL, STT_FUNC),
              
              SHN_UNDEF + 3);

  ProcessSection();

  ASSERT_EQ((size_t)3, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName1, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr1, extern1->address);
  Module::Extern *extern2 = externs[1];
  EXPECT_EQ(kFuncName2, extern2->name);
  EXPECT_EQ((Module::Address)kFuncAddr2, extern2->address);
  Module::Extern *extern3 = externs[2];
  EXPECT_EQ(kFuncName3, extern3->name);
  EXPECT_EQ((Module::Address)kFuncAddr3, extern3->address);
}

TEST_P(ELFSymbolsToModuleTest64, SkipStuff) {
  const string kFuncName = "superfunc";
  const uint64_t kFuncAddr = 0x1000100010001000ULL;
  const uint64_t kFuncSize = 0x1000;

  
  AddElf64Sym("skipme", 0xFFFF, 0x10,
              ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
              SHN_UNDEF);
  AddElf64Sym(kFuncName, kFuncAddr, kFuncSize,
              ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
              
              SHN_UNDEF + 1);
  
  AddElf64Sym("skipmetoo", 0xAAAA, 0x10,
              ELF64_ST_INFO(STB_GLOBAL, STT_FILE),
              SHN_UNDEF + 1);

  ProcessSection();

  ASSERT_EQ((size_t)1, externs.size());
  Module::Extern *extern1 = externs[0];
  EXPECT_EQ(kFuncName, extern1->name);
  EXPECT_EQ((Module::Address)kFuncAddr, extern1->address);
}


INSTANTIATE_TEST_CASE_P(Endian,
                        ELFSymbolsToModuleTest64,
                        ::testing::Values(kLittleEndian, kBigEndian));
