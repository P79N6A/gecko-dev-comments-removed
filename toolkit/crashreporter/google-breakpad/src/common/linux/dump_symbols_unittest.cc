

































#include <elf.h>
#include <link.h>
#include <stdio.h>

#include <sstream>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/linux/synth_elf.h"
#include "common/module.h"
#include "common/using_std_string.h"

namespace google_breakpad {
bool ReadSymbolDataInternal(const uint8_t* obj_file,
                            const string& obj_filename,
                            const std::vector<string>& debug_dir,
                            SymbolData symbol_data,
                            Module** module);
}

using google_breakpad::synth_elf::ELF;
using google_breakpad::synth_elf::StringTable;
using google_breakpad::synth_elf::SymbolTable;
using google_breakpad::test_assembler::kLittleEndian;
using google_breakpad::test_assembler::Section;
using google_breakpad::Module;
using google_breakpad::ReadSymbolDataInternal;
using std::stringstream;
using std::vector;
using ::testing::Test;

class DumpSymbols : public Test {
 public:
  void GetElfContents(ELF& elf) {
    string contents;
    ASSERT_TRUE(elf.GetContents(&contents));
    ASSERT_LT(0U, contents.size());

    elfdata_v.clear();
    elfdata_v.insert(elfdata_v.begin(), contents.begin(), contents.end());
    elfdata = &elfdata_v[0];
  }

  vector<uint8_t> elfdata_v;
  uint8_t* elfdata;
};

TEST_F(DumpSymbols, Invalid) {
  Elf32_Ehdr header;
  memset(&header, 0, sizeof(header));
  Module* module;
  EXPECT_FALSE(ReadSymbolDataInternal(reinterpret_cast<uint8_t*>(&header),
                                      "foo",
                                      vector<string>(),
                                      ALL_SYMBOL_DATA,
                                      &module));
}

TEST_F(DumpSymbols, SimplePublic32) {
  ELF elf(EM_386, ELFCLASS32, kLittleEndian);
  
  Section text(kLittleEndian);
  text.Append(4096, 0);
  elf.AddSection(".text", text, SHT_PROGBITS);

  
  StringTable table(kLittleEndian);
  SymbolTable syms(kLittleEndian, 4, table);
  syms.AddSymbol("superfunc", (uint32_t)0x1000, (uint32_t)0x10,
                 ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
                 SHN_UNDEF + 1);
  int index = elf.AddSection(".dynstr", table, SHT_STRTAB);
  elf.AddSection(".dynsym", syms,
                 SHT_DYNSYM,          
                 SHF_ALLOC,           
                 0,                   
                 index,               
                 sizeof(Elf32_Sym));  

  elf.Finish();
  GetElfContents(elf);

  Module* module;
  EXPECT_TRUE(ReadSymbolDataInternal(elfdata,
                                     "foo",
                                     vector<string>(),
                                     ALL_SYMBOL_DATA,
                                     &module));

  stringstream s;
  module->Write(s, ALL_SYMBOL_DATA);
  EXPECT_EQ("MODULE Linux x86 000000000000000000000000000000000 foo\n"
            "PUBLIC 1000 0 superfunc\n",
            s.str());
  delete module;
}

TEST_F(DumpSymbols, SimplePublic64) {
  ELF elf(EM_X86_64, ELFCLASS64, kLittleEndian);
  
  Section text(kLittleEndian);
  text.Append(4096, 0);
  elf.AddSection(".text", text, SHT_PROGBITS);

  
  StringTable table(kLittleEndian);
  SymbolTable syms(kLittleEndian, 8, table);
  syms.AddSymbol("superfunc", (uint64_t)0x1000, (uint64_t)0x10,
                 ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
                 SHN_UNDEF + 1);
  int index = elf.AddSection(".dynstr", table, SHT_STRTAB);
  elf.AddSection(".dynsym", syms,
                 SHT_DYNSYM,          
                 SHF_ALLOC,           
                 0,                   
                 index,               
                 sizeof(Elf64_Sym));  

  elf.Finish();
  GetElfContents(elf);

  Module* module;
  EXPECT_TRUE(ReadSymbolDataInternal(elfdata,
                                     "foo",
                                     vector<string>(),
                                     ALL_SYMBOL_DATA,
                                     &module));

  stringstream s;
  module->Write(s, ALL_SYMBOL_DATA);
  EXPECT_EQ("MODULE Linux x86_64 000000000000000000000000000000000 foo\n"
            "PUBLIC 1000 0 superfunc\n",
            s.str());
}
