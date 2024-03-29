

































#include "common/test_assembler.h"

#include <assert.h>
#include <stdio.h>

#include <iterator>

namespace google_breakpad {
namespace test_assembler {

using std::back_insert_iterator;

Label::Label() : value_(new Binding()) { }
Label::Label(uint64_t value) : value_(new Binding(value)) { }
Label::Label(const Label &label) {
  value_ = label.value_;
  value_->Acquire();
}
Label::~Label() {
  if (value_->Release()) delete value_;
}

Label &Label::operator=(uint64_t value) {
  value_->Set(NULL, value);
  return *this;
}

Label &Label::operator=(const Label &label) {
  value_->Set(label.value_, 0);
  return *this;
}

Label Label::operator+(uint64_t addend) const {
  Label l;
  l.value_->Set(this->value_, addend);
  return l;
}

Label Label::operator-(uint64_t subtrahend) const {
  Label l;
  l.value_->Set(this->value_, -subtrahend);
  return l;
}








#ifdef NDEBUG
#define ALWAYS_EVALUATE_AND_ASSERT(x) x
#else
#define ALWAYS_EVALUATE_AND_ASSERT(x) assert(x)
#endif

uint64_t Label::operator-(const Label &label) const {
  uint64_t offset;
  ALWAYS_EVALUATE_AND_ASSERT(IsKnownOffsetFrom(label, &offset));
  return offset;
}

uint64_t Label::Value() const {
  uint64_t v = 0;
  ALWAYS_EVALUATE_AND_ASSERT(IsKnownConstant(&v));
  return v;
};

bool Label::IsKnownConstant(uint64_t *value_p) const {
  Binding *base;
  uint64_t addend;
  value_->Get(&base, &addend);
  if (base != NULL) return false;
  if (value_p) *value_p = addend;
  return true;
}

bool Label::IsKnownOffsetFrom(const Label &label, uint64_t *offset_p) const
{
  Binding *label_base, *this_base;
  uint64_t label_addend, this_addend;
  label.value_->Get(&label_base, &label_addend);
  value_->Get(&this_base, &this_addend);
  
  
  
  if (this_base != label_base) return false;
  if (offset_p) *offset_p = this_addend - label_addend;
  return true;
}

Label::Binding::Binding() : base_(this), addend_(), reference_count_(1) { }

Label::Binding::Binding(uint64_t addend)
    : base_(NULL), addend_(addend), reference_count_(1) { }

Label::Binding::~Binding() {
  assert(reference_count_ == 0);
  if (base_ && base_ != this && base_->Release())
    delete base_;
}

void Label::Binding::Set(Binding *binding, uint64_t addend) {
  if (!base_ && !binding) {
    
    assert(addend_ == addend);
  } else if (!base_) {
    
    
    binding->Set(NULL, addend_ - addend);
  } else {
    if (binding) {
      
      
      
      
      
      uint64_t binding_addend;
      binding->Get(&binding, &binding_addend);
      addend += binding_addend;
    }

    
    
    
    assert(binding != this);

    if (base_ != this) {
      
      
      
      
      
      base_->Set(binding, addend - addend_);
      
      if (base_->Release()) delete base_;
    }
    
    
    
    
    
    
    
    if (binding) binding->Acquire();
    base_ = binding;
    addend_ = addend;
  }
}

void Label::Binding::Get(Binding **base, uint64_t *addend) {
  if (base_ && base_ != this) {
    
    
    
    
    Binding *final_base;
    uint64_t final_addend;
    base_->Get(&final_base, &final_addend);
    if (final_base) final_base->Acquire();
    if (base_->Release()) delete base_;
    base_ = final_base;
    addend_ += final_addend;
  }
  *base = base_;
  *addend = addend_;
}

template<typename Inserter>
static inline void InsertEndian(test_assembler::Endianness endianness,
                                size_t size, uint64_t number, Inserter dest) {
  assert(size > 0);
  if (endianness == kLittleEndian) {
    for (size_t i = 0; i < size; i++) {
      *dest++ = (char) (number & 0xff);
      number >>= 8;
    }
  } else {
    assert(endianness == kBigEndian);
    
    for (size_t i = size - 1; i < size; i--)
      *dest++ = (char) ((number >> (i * 8)) & 0xff);
  }
}

Section &Section::Append(Endianness endianness, size_t size, uint64_t number) {
  InsertEndian(endianness, size, number,
               back_insert_iterator<string>(contents_));
  return *this;
}

Section &Section::Append(Endianness endianness, size_t size,
                         const Label &label) {
  
  
  uint64_t value;
  if (label.IsKnownConstant(&value))
    return Append(endianness, size, value);

  
  
  assert(endianness != kUnsetEndian);

  references_.push_back(Reference(contents_.size(), endianness, size, label));
  contents_.append(size, 0);
  return *this;
}

#define ENDIANNESS_L kLittleEndian
#define ENDIANNESS_B kBigEndian
#define ENDIANNESS(e) ENDIANNESS_ ## e

#define DEFINE_SHORT_APPEND_NUMBER_ENDIAN(e, bits)                      \
  Section &Section::e ## bits(uint ## bits ## _t v) {                  \
    InsertEndian(ENDIANNESS(e), bits / 8, v,                            \
                 back_insert_iterator<string>(contents_));              \
    return *this;                                                       \
  }

#define DEFINE_SHORT_APPEND_LABEL_ENDIAN(e, bits)                       \
  Section &Section::e ## bits(const Label &v) {                         \
    return Append(ENDIANNESS(e), bits / 8, v);                          \
  }


#define DEFINE_SHORT_APPEND_ENDIAN(e, bits)                             \
  DEFINE_SHORT_APPEND_NUMBER_ENDIAN(e, bits)                            \
  DEFINE_SHORT_APPEND_LABEL_ENDIAN(e, bits)

DEFINE_SHORT_APPEND_LABEL_ENDIAN(L, 8);
DEFINE_SHORT_APPEND_LABEL_ENDIAN(B, 8);
DEFINE_SHORT_APPEND_ENDIAN(L, 16);
DEFINE_SHORT_APPEND_ENDIAN(L, 32);
DEFINE_SHORT_APPEND_ENDIAN(L, 64);
DEFINE_SHORT_APPEND_ENDIAN(B, 16);
DEFINE_SHORT_APPEND_ENDIAN(B, 32);
DEFINE_SHORT_APPEND_ENDIAN(B, 64);

#define DEFINE_SHORT_APPEND_NUMBER_DEFAULT(bits)                        \
  Section &Section::D ## bits(uint ## bits ## _t v) {                  \
    InsertEndian(endianness_, bits / 8, v,                              \
                 back_insert_iterator<string>(contents_));              \
    return *this;                                                       \
  }
#define DEFINE_SHORT_APPEND_LABEL_DEFAULT(bits)                         \
  Section &Section::D ## bits(const Label &v) {                         \
    return Append(endianness_, bits / 8, v);                            \
  }
#define DEFINE_SHORT_APPEND_DEFAULT(bits)                               \
  DEFINE_SHORT_APPEND_NUMBER_DEFAULT(bits)                              \
  DEFINE_SHORT_APPEND_LABEL_DEFAULT(bits)

DEFINE_SHORT_APPEND_LABEL_DEFAULT(8)
DEFINE_SHORT_APPEND_DEFAULT(16);
DEFINE_SHORT_APPEND_DEFAULT(32);
DEFINE_SHORT_APPEND_DEFAULT(64);

Section &Section::Append(const Section &section) {
  size_t base = contents_.size();
  contents_.append(section.contents_);
  for (vector<Reference>::const_iterator it = section.references_.begin();
       it != section.references_.end(); it++)
    references_.push_back(Reference(base + it->offset, it->endianness,
                                    it->size, it->label));
  return *this;
}

Section &Section::LEB128(long long value) {
  while (value < -0x40 || 0x3f < value) {
    contents_ += (value & 0x7f) | 0x80;
    if (value < 0)
      value = (value >> 7) | ~(((unsigned long long) -1) >> 7);
    else
      value = (value >> 7);
  }
  contents_ += value & 0x7f;
  return *this;
}

Section &Section::ULEB128(uint64_t value) {
  while (value > 0x7f) {
    contents_ += (value & 0x7f) | 0x80;
    value = (value >> 7);
  }
  contents_ += value;
  return *this;
}

Section &Section::Align(size_t alignment, uint8_t pad_byte) {
  
  assert(((alignment - 1) & alignment) == 0);
  size_t new_size = (contents_.size() + alignment - 1) & ~(alignment - 1);
  contents_.append(new_size - contents_.size(), pad_byte);
  assert((contents_.size() & (alignment - 1)) == 0);
  return *this;
}

void Section::Clear() {
  contents_.clear();
  references_.clear();
}

bool Section::GetContents(string *contents) {
  
  
  for (size_t i = 0; i < references_.size(); i++) {
    Reference &r = references_[i];
    uint64_t value;
    if (!r.label.IsKnownConstant(&value)) {
      fprintf(stderr, "Undefined label #%zu at offset 0x%zx\n", i, r.offset);
      return false;
    }
    assert(r.offset < contents_.size());
    assert(contents_.size() - r.offset >= r.size);
    InsertEndian(r.endianness, r.size, value, contents_.begin() + r.offset);
  }
  contents->clear();
  std::swap(contents_, *contents);
  references_.clear();
  return true;
}

}  
}  
