



































#ifndef PROCESSOR_CFI_ASSEMBLER_H_
#define PROCESSOR_CFI_ASSEMBLER_H_

#include <string>

#include "common/dwarf/dwarf2enums.h"
#include "common/test_assembler.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using dwarf2reader::DwarfPointerEncoding;
using google_breakpad::test_assembler::Endianness;
using google_breakpad::test_assembler::Label;
using google_breakpad::test_assembler::Section;
using std::string;

class CFISection: public Section {
 public:

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  struct EncodedPointerBases {
    EncodedPointerBases() : cfi(), text(), data() { }

    
    
    
    u_int64_t cfi;

    
    u_int64_t text;

    
    
    u_int64_t data;
  };

  
  
  
  
  
  CFISection(Endianness endianness, size_t address_size,
             bool eh_frame = false)
      : Section(endianness), address_size_(address_size), eh_frame_(eh_frame),
        pointer_encoding_(dwarf2reader::DW_EH_PE_absptr),
        encoded_pointer_bases_(), entry_length_(NULL), in_fde_(false) {
    
    
    start() = 0;
  }

  
  size_t AddressSize() const { return address_size_; }

  
  
  bool ContainsEHFrame() const { return eh_frame_; }

  
  void SetPointerEncoding(DwarfPointerEncoding encoding) {
    pointer_encoding_ = encoding;
  }

  
  
  
  void SetEncodedPointerBases(const EncodedPointerBases &bases) {
    encoded_pointer_bases_ = bases;
  }

  
  
  
  
  
  
  
  
  
  CFISection &CIEHeader(u_int64_t code_alignment_factor,
                        int data_alignment_factor,
                        unsigned return_address_register,
                        u_int8_t version = 3,
                        const string &augmentation = "",
                        bool dwarf64 = false);

  
  
  
  
  
  
  
  
  
  
  CFISection &FDEHeader(Label cie_pointer,
                        u_int64_t initial_location,
                        u_int64_t address_range,
                        bool dwarf64 = false);

  
  
  
  
  CFISection &FinishEntry();

  
  
  CFISection &Block(const string &block) {
    ULEB128(block.size());
    Append(block);
    return *this;
  }

  
  
  CFISection &Address(u_int64_t address) {
    Section::Append(endianness(), address_size_, address);
    return *this;
  }
  CFISection &Address(Label address) {
    Section::Append(endianness(), address_size_, address);
    return *this;
  }

  
  
  
  
  
  
  
  
  
  CFISection &EncodedPointer(u_int64_t address) {
    return EncodedPointer(address, pointer_encoding_, encoded_pointer_bases_);
  }
  CFISection &EncodedPointer(u_int64_t address, DwarfPointerEncoding encoding) {
    return EncodedPointer(address, encoding, encoded_pointer_bases_);
  }
  CFISection &EncodedPointer(u_int64_t address, DwarfPointerEncoding encoding,
                             const EncodedPointerBases &bases);

  
  CFISection &Mark(Label *label)   { Section::Mark(label); return *this; }
  CFISection &D8(u_int8_t v)       { Section::D8(v);       return *this; }
  CFISection &D16(u_int16_t v)     { Section::D16(v);      return *this; }
  CFISection &D16(Label v)         { Section::D16(v);      return *this; }
  CFISection &D32(u_int32_t v)     { Section::D32(v);      return *this; }
  CFISection &D32(const Label &v)  { Section::D32(v);      return *this; }
  CFISection &D64(u_int64_t v)     { Section::D64(v);      return *this; }
  CFISection &D64(const Label &v)  { Section::D64(v);      return *this; }
  CFISection &LEB128(long long v)  { Section::LEB128(v);   return *this; }
  CFISection &ULEB128(u_int64_t v) { Section::ULEB128(v);  return *this; }

 private:
  
  
  
  struct PendingLength {
    Label length;
    Label start;
  };

  

  
  
  
  static const u_int32_t kDwarf64InitialLengthMarker = 0xffffffffU;

  
  static const u_int32_t kDwarf32CIEIdentifier = ~(u_int32_t)0;
  static const u_int64_t kDwarf64CIEIdentifier = ~(u_int64_t)0;
  static const u_int32_t kEHFrame32CIEIdentifier = 0;
  static const u_int64_t kEHFrame64CIEIdentifier = 0;

  
  size_t address_size_;

  
  
  bool eh_frame_;

  
  DwarfPointerEncoding pointer_encoding_;

  
  EncodedPointerBases encoded_pointer_bases_;

  
  
  
  
  
  
  
  
  PendingLength *entry_length_;

  
  
  bool in_fde_;

  
  
  u_int64_t fde_start_address_;
};

}  

#endif  
