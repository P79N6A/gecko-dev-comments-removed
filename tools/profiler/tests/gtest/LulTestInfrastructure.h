



























































#ifndef LUL_TEST_INFRASTRUCTURE_H
#define LUL_TEST_INFRASTRUCTURE_H

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace lul_test {
namespace test_assembler {






































class Label {
 public:
  Label();                               
  explicit Label(uint64_t value);        
  Label(const Label &value);             
  ~Label();

  Label &operator=(uint64_t value);
  Label &operator=(const Label &value);
  Label operator+(uint64_t addend) const;
  Label operator-(uint64_t subtrahend) const;
  uint64_t operator-(const Label &subtrahend) const;

  
  

  
  
  bool IsKnownConstant(uint64_t *value_p = NULL) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool IsKnownOffsetFrom(const Label &label, uint64_t *offset_p = NULL) const;

 private:
  
  
  
  
  
  
  
  
  
  
  
  
  class Binding {
   public:
    Binding();
    explicit Binding(uint64_t addend);
    ~Binding();

    
    void Acquire() { reference_count_++; };
    
    bool Release() { return --reference_count_ == 0; }

    
    
    
    
    
    void Set(Binding *binding, uint64_t value);

    
    
    
    
    
    
    
    
    
    
    void Get(Binding **base, uint64_t *addend);

   private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    Binding *base_;
    uint64_t addend_;

    
    
    
    int reference_count_;
  };

  
  Binding *value_;
};


enum Endianness {
  kBigEndian,        
  kLittleEndian,     
  kUnsetEndian,      
};
 























class Section {
 public:
  explicit Section(Endianness endianness = kUnsetEndian)
      : endianness_(endianness) { };

  
  
  virtual ~Section() { };

  
  Endianness endianness() const { return endianness_; }

  
  
  Section &Append(const string &data) {
    contents_.append(data);
    return *this;
  };

  
  
  Section &Append(size_t size, uint8_t byte) {
    contents_.append(size, (char) byte);
    return *this;
  }
      
  
  
  
  Section &Append(Endianness endianness, size_t size, uint64_t number);
  Section &Append(Endianness endianness, size_t size, const Label &label);

  
  
  
  
  
  
  
  Section &Append(const Section &section);

  
  
  Section &AppendCString(const string &data) {
    Append(data);
    contents_ += '\0';
    return *this;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Section &L8(uint8_t value) { contents_ += value; return *this; }
  Section &B8(uint8_t value) { contents_ += value; return *this; }
  Section &D8(uint8_t value) { contents_ += value; return *this; }
  Section &L16(uint16_t), &L32(uint32_t), &L64(uint64_t),
          &B16(uint16_t), &B32(uint32_t), &B64(uint64_t),
          &D16(uint16_t), &D32(uint32_t), &D64(uint64_t);
  Section &L8(const Label &label),  &L16(const Label &label),
          &L32(const Label &label), &L64(const Label &label),
          &B8(const Label &label),  &B16(const Label &label),
          &B32(const Label &label), &B64(const Label &label),
          &D8(const Label &label),  &D16(const Label &label),
          &D32(const Label &label), &D64(const Label &label);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Section &LEB128(long long value);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Section &ULEB128(uint64_t value);

  
  
  
  
  Section &Align(size_t alignment, uint8_t pad_byte = 0);

  
  size_t Size() const { return contents_.size(); }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Label start() const { return start_; }

  
  
  Label Here() const { return start_ + Size(); }

  
  Section &Mark(Label *label) { *label = Here(); return *this; }

  
  
  
  
  bool GetContents(string *contents);

 private:
  
  struct Reference {
    Reference(size_t set_offset, Endianness set_endianness,  size_t set_size,
              const Label &set_label)
        : offset(set_offset), endianness(set_endianness), size(set_size),
          label(set_label) { }
      
    
    size_t offset;

    
    Endianness endianness;

    
    size_t size;

    
    Label label;
  };

  
  Endianness endianness_;

  
  string contents_;
  
  
  vector<Reference> references_;

  
  Label start_;
};

}  
}  


namespace lul_test {

using lul::DwarfPointerEncoding;
using lul_test::test_assembler::Endianness;
using lul_test::test_assembler::Label;
using lul_test::test_assembler::Section;

class CFISection: public Section {
 public:

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  struct EncodedPointerBases {
    EncodedPointerBases() : cfi(), text(), data() { }

    
    
    
    uint64_t cfi;

    
    uint64_t text;

    
    
    uint64_t data;
  };

  
  
  
  
  
  CFISection(Endianness endianness, size_t address_size,
             bool eh_frame = false)
      : Section(endianness), address_size_(address_size), eh_frame_(eh_frame),
        pointer_encoding_(lul::DW_EH_PE_absptr),
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

  
  
  
  
  
  
  
  
  
  CFISection &CIEHeader(uint64_t code_alignment_factor,
                        int data_alignment_factor,
                        unsigned return_address_register,
                        uint8_t version = 3,
                        const string &augmentation = "",
                        bool dwarf64 = false);

  
  
  
  
  
  
  
  
  
  
  CFISection &FDEHeader(Label cie_pointer,
                        uint64_t initial_location,
                        uint64_t address_range,
                        bool dwarf64 = false);

  
  
  
  
  CFISection &FinishEntry();

  
  
  CFISection &Block(const string &block) {
    ULEB128(block.size());
    Append(block);
    return *this;
  }

  
  
  CFISection &Address(uint64_t address) {
    Section::Append(endianness(), address_size_, address);
    return *this;
  }

  
  
  
  
  
  
  
  
  
  CFISection &EncodedPointer(uint64_t address) {
    return EncodedPointer(address, pointer_encoding_, encoded_pointer_bases_);
  }
  CFISection &EncodedPointer(uint64_t address, DwarfPointerEncoding encoding) {
    return EncodedPointer(address, encoding, encoded_pointer_bases_);
  }
  CFISection &EncodedPointer(uint64_t address, DwarfPointerEncoding encoding,
                             const EncodedPointerBases &bases);

  
  CFISection &Mark(Label *label)   { Section::Mark(label); return *this; }
  CFISection &D8(uint8_t v)       { Section::D8(v);       return *this; }
  CFISection &D16(uint16_t v)     { Section::D16(v);      return *this; }
  CFISection &D16(Label v)         { Section::D16(v);      return *this; }
  CFISection &D32(uint32_t v)     { Section::D32(v);      return *this; }
  CFISection &D32(const Label &v)  { Section::D32(v);      return *this; }
  CFISection &D64(uint64_t v)     { Section::D64(v);      return *this; }
  CFISection &D64(const Label &v)  { Section::D64(v);      return *this; }
  CFISection &LEB128(long long v)  { Section::LEB128(v);   return *this; }
  CFISection &ULEB128(uint64_t v) { Section::ULEB128(v);  return *this; }

 private:
  
  
  
  struct PendingLength {
    Label length;
    Label start;
  };

  

  
  
  
  static const uint32_t kDwarf64InitialLengthMarker = 0xffffffffU;

  
  static const uint32_t kDwarf32CIEIdentifier = ~(uint32_t)0;
  static const uint64_t kDwarf64CIEIdentifier = ~(uint64_t)0;
  static const uint32_t kEHFrame32CIEIdentifier = 0;
  static const uint64_t kEHFrame64CIEIdentifier = 0;

  
  size_t address_size_;

  
  
  bool eh_frame_;

  
  DwarfPointerEncoding pointer_encoding_;

  
  EncodedPointerBases encoded_pointer_bases_;

  
  
  
  
  
  
  
  
  PendingLength *entry_length_;

  
  
  bool in_fde_;

  
  
  uint64_t fde_start_address_;
};

}  

#endif 
