




































#ifndef GOOGLE_PROTOBUF_UNKNOWN_FIELD_SET_H__
#define GOOGLE_PROTOBUF_UNKNOWN_FIELD_SET_H__

#include <assert.h>
#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
  namespace io {
    class CodedInputStream;         
    class CodedOutputStream;        
    class ZeroCopyInputStream;      
  }
  namespace internal {
    class WireFormat;               
    class MessageSetFieldSkipperUsingCord;
                                    
  }

class Message;                      
class UnknownField;                 













class LIBPROTOBUF_EXPORT UnknownFieldSet {
 public:
  UnknownFieldSet();
  ~UnknownFieldSet();

  
  inline void Clear();

  
  void ClearAndFreeMemory();

  
  inline bool empty() const;

  
  void MergeFrom(const UnknownFieldSet& other);

  
  inline void Swap(UnknownFieldSet* x);

  
  
  
  int SpaceUsedExcludingSelf() const;

  
  int SpaceUsed() const;

  
  inline int field_count() const;
  
  
  inline const UnknownField& field(int index) const;
  
  
  
  inline UnknownField* mutable_field(int index);

  

  void AddVarint(int number, uint64 value);
  void AddFixed32(int number, uint32 value);
  void AddFixed64(int number, uint64 value);
  void AddLengthDelimited(int number, const string& value);
  string* AddLengthDelimited(int number);
  UnknownFieldSet* AddGroup(int number);

  
  void AddField(const UnknownField& field);

  
  
  void DeleteSubrange(int start, int num);

  
  
  
  void DeleteByNumber(int number);

  
  

  bool MergeFromCodedStream(io::CodedInputStream* input);
  bool ParseFromCodedStream(io::CodedInputStream* input);
  bool ParseFromZeroCopyStream(io::ZeroCopyInputStream* input);
  bool ParseFromArray(const void* data, int size);
  inline bool ParseFromString(const string& data) {
    return ParseFromArray(data.data(), static_cast<int>(data.size()));
  }

 private:

  void ClearFallback();

  vector<UnknownField>* fields_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(UnknownFieldSet);
};


class LIBPROTOBUF_EXPORT UnknownField {
 public:
  enum Type {
    TYPE_VARINT,
    TYPE_FIXED32,
    TYPE_FIXED64,
    TYPE_LENGTH_DELIMITED,
    TYPE_GROUP
  };

  
  inline int number() const;

  
  inline Type type() const;

  
  

  inline uint64 varint() const;
  inline uint32 fixed32() const;
  inline uint64 fixed64() const;
  inline const string& length_delimited() const;
  inline const UnknownFieldSet& group() const;

  inline void set_varint(uint64 value);
  inline void set_fixed32(uint32 value);
  inline void set_fixed64(uint64 value);
  inline void set_length_delimited(const string& value);
  inline string* mutable_length_delimited();
  inline UnknownFieldSet* mutable_group();

  
  
  
  
  void SerializeLengthDelimitedNoTag(io::CodedOutputStream* output) const;
  uint8* SerializeLengthDelimitedNoTagToArray(uint8* target) const;

  inline int GetLengthDelimitedSize() const;

 private:
  friend class UnknownFieldSet;

  
  void Delete();

  
  void DeepCopy();

  
  
  inline void SetType(Type type);

  uint32 number_;
  uint32 type_;
  union {
    uint64 varint_;
    uint32 fixed32_;
    uint64 fixed64_;
    mutable union {
      string* string_value_;
    } length_delimited_;
    UnknownFieldSet* group_;
  };
};




inline void UnknownFieldSet::Clear() {
  if (fields_ != NULL) {
    ClearFallback();
  }
}

inline bool UnknownFieldSet::empty() const {
  return fields_ == NULL || fields_->empty();
}

inline void UnknownFieldSet::Swap(UnknownFieldSet* x) {
  std::swap(fields_, x->fields_);
}

inline int UnknownFieldSet::field_count() const {
  return (fields_ == NULL) ? 0 : static_cast<int>(fields_->size());
}
inline const UnknownField& UnknownFieldSet::field(int index) const {
  return (*fields_)[index];
}
inline UnknownField* UnknownFieldSet::mutable_field(int index) {
  return &(*fields_)[index];
}

inline void UnknownFieldSet::AddLengthDelimited(
    int number, const string& value) {
  AddLengthDelimited(number)->assign(value);
}


inline int UnknownField::number() const { return number_; }
inline UnknownField::Type UnknownField::type() const {
  return static_cast<Type>(type_);
}

inline uint64 UnknownField::varint() const {
  assert(type() == TYPE_VARINT);
  return varint_;
}
inline uint32 UnknownField::fixed32() const {
  assert(type() == TYPE_FIXED32);
  return fixed32_;
}
inline uint64 UnknownField::fixed64() const {
  assert(type() == TYPE_FIXED64);
  return fixed64_;
}
inline const string& UnknownField::length_delimited() const {
  assert(type() == TYPE_LENGTH_DELIMITED);
  return *length_delimited_.string_value_;
}
inline const UnknownFieldSet& UnknownField::group() const {
  assert(type() == TYPE_GROUP);
  return *group_;
}

inline void UnknownField::set_varint(uint64 value) {
  assert(type() == TYPE_VARINT);
  varint_ = value;
}
inline void UnknownField::set_fixed32(uint32 value) {
  assert(type() == TYPE_FIXED32);
  fixed32_ = value;
}
inline void UnknownField::set_fixed64(uint64 value) {
  assert(type() == TYPE_FIXED64);
  fixed64_ = value;
}
inline void UnknownField::set_length_delimited(const string& value) {
  assert(type() == TYPE_LENGTH_DELIMITED);
  length_delimited_.string_value_->assign(value);
}
inline string* UnknownField::mutable_length_delimited() {
  assert(type() == TYPE_LENGTH_DELIMITED);
  return length_delimited_.string_value_;
}
inline UnknownFieldSet* UnknownField::mutable_group() {
  assert(type() == TYPE_GROUP);
  return group_;
}

inline int UnknownField::GetLengthDelimitedSize() const {
  GOOGLE_DCHECK_EQ(TYPE_LENGTH_DELIMITED, type());
  return static_cast<int>(length_delimited_.string_value_->size());
}

inline void UnknownField::SetType(Type type) {
  type_ = type;
}


}  

}  
#endif  
