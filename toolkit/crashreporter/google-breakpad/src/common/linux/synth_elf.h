


































#ifndef COMMON_LINUX_SYNTH_ELF_H_
#define COMMON_LINUX_SYNTH_ELF_H_

#include "common/test_assembler.h"

#include <list>
#include <map>
#include <string>
#include <utility>

#include "common/using_std_string.h"

namespace google_breakpad {
namespace synth_elf {

using std::list;
using std::map;
using std::pair;
using test_assembler::Endianness;
using test_assembler::kLittleEndian;
using test_assembler::kUnsetEndian;
using test_assembler::Label;
using test_assembler::Section;



class StringTable : public Section {
public:
  StringTable(Endianness endianness = kUnsetEndian)
  : Section(endianness) {
    start() = 0;
    empty_string = Add("");
  }

  
  
  
  Label Add(const string& s) {
    if (strings_.find(s) != strings_.end())
      return strings_[s];

    Label string_label(Here());
    AppendCString(s);
    strings_[s] = string_label;
    return string_label;
  }

  
  
  Label empty_string;

  
  map<string,Label> strings_;
};


class ELF : public Section {
 public:
  ELF(uint16_t machine,    
      uint8_t file_class,  
      Endianness endianness = kLittleEndian);

  
  
  
  int AddSection(const string& name, const Section& section,
                 uint32_t type, uint32_t flags = 0, uint64_t addr = 0,
                 uint32_t link = 0, uint64_t entsize = 0, uint64_t offset = 0);
                  
  
  void Finish();

 private:
  
  const size_t addr_size_;

  
  Label program_header_label_;
  
  int program_count_;
  Label program_count_label_;

  
  Label section_header_label_;
  
  int section_count_;
  Label section_count_label_;
  
  Section section_header_table_;

  
  
  Label section_header_string_index_;
  
  StringTable section_header_strings_;
};


class SymbolTable : public Section {
 public:
  
  
  
  SymbolTable(Endianness endianness, size_t addr_size, StringTable& table);

  
  void AddSymbol(const string& name, uint32_t value,
                 uint32_t size, unsigned info, uint16_t shndx);
  
  void AddSymbol(const string& name, uint64_t value,
                 uint64_t size, unsigned info, uint16_t shndx);

 private:
  size_t addr_size_;
  StringTable& table_;
};


class BuildIDNote : public Section {
public:
  BuildIDNote(const uint8_t* id_bytes, size_t id_size, Endianness endianness);

  
  static void AppendSection(ELF& elf, const uint8_t* id_bytes, size_t id_size);
};

}  
}  

#endif  
