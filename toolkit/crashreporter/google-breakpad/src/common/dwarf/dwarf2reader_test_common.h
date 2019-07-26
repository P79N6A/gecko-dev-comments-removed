




































#ifndef COMMON_DWARF_DWARF2READER_TEST_COMMON_H__
#define COMMON_DWARF_DWARF2READER_TEST_COMMON_H__

#include "common/test_assembler.h"
#include "common/dwarf/dwarf2enums.h"



class TestCompilationUnit: public google_breakpad::test_assembler::Section {
 public:
  typedef dwarf2reader::DwarfTag DwarfTag;
  typedef dwarf2reader::DwarfAttribute DwarfAttribute;
  typedef dwarf2reader::DwarfForm DwarfForm;
  typedef google_breakpad::test_assembler::Label Label;

  
  
  
  void set_format_size(size_t format_size) {
    assert(format_size == 4 || format_size == 8);
    format_size_ = format_size;
  }
    
  
  
  template<typename T>
  void SectionOffset(T offset) {
    if (format_size_ == 4)
      D32(offset);
    else
      D64(offset);
  }

  
  
  TestCompilationUnit &Header(int version, const Label &abbrev_offset,
                              size_t address_size) {
    if (format_size_ == 4) {
      D32(length_);
    } else {
      D32(0xffffffff);
      D64(length_);
    }
    post_length_offset_ = Size();
    D16(version);
    SectionOffset(abbrev_offset);
    D8(address_size);
    return *this;
  }

  
  TestCompilationUnit &Finish() {
    length_ = Size() - post_length_offset_;
    return *this;
  }

 private:
  
  size_t format_size_;

  
  
  uint64_t post_length_offset_;

  
  Label length_;
};



class TestAbbrevTable: public google_breakpad::test_assembler::Section {
 public:
  typedef dwarf2reader::DwarfTag DwarfTag;
  typedef dwarf2reader::DwarfAttribute DwarfAttribute;
  typedef dwarf2reader::DwarfForm DwarfForm;
  typedef dwarf2reader::DwarfHasChild DwarfHasChild;
  typedef google_breakpad::test_assembler::Label Label;

  
  
  
  TestAbbrevTable &Abbrev(int code, DwarfTag tag, DwarfHasChild has_children) {
    assert(code != 0);
    ULEB128(code);
    ULEB128(static_cast<unsigned>(tag));
    D8(static_cast<unsigned>(has_children));
    return *this;
  };

  
  
  TestAbbrevTable &Attribute(DwarfAttribute name, DwarfForm form) {
    ULEB128(static_cast<unsigned>(name));
    ULEB128(static_cast<unsigned>(form));
    return *this;
  }

  
  TestAbbrevTable &EndAbbrev() {
    ULEB128(0);
    ULEB128(0);
    return *this;
  }

  
  TestAbbrevTable &EndTable() {
    ULEB128(0);
    return *this;
  }
};

#endif 
