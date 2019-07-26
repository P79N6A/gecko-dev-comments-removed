






















































#ifndef PROCESSOR_TEST_ASSEMBLER_H_
#define PROCESSOR_TEST_ASSEMBLER_H_

#include <list>
#include <vector>
#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::list;
using std::vector;

namespace test_assembler {






































class Label {
 public:
  Label();                      
  Label(uint64_t value);       
  Label(const Label &value);    
  ~Label();

  
  
  
  
  
  
  
  
  uint64_t Value() const;

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
    Binding(uint64_t addend);
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

inline Label operator+(uint64_t a, const Label &l) { return l + a; }




enum Endianness {
  kBigEndian,        
  kLittleEndian,     
  kUnsetEndian,      
};
 























class Section {
 public:
  Section(Endianness endianness = kUnsetEndian)
      : endianness_(endianness) { };

  
  
  virtual ~Section() { };

  
  
  
  void set_endianness(Endianness endianness) {
    endianness_ = endianness;
  }

  
  Endianness endianness() const { return endianness_; }

  
  
  Section &Append(const uint8_t *data, size_t size) {
    contents_.append(reinterpret_cast<const char *>(data), size);
    return *this;
  };
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

  
  
  Section &AppendCString(const string &data, size_t size) {
    contents_.append(data, 0, size);
    if (data.size() < size)
      Append(size - data.size(), 0);
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

  
  void Clear();

  
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

#endif  
