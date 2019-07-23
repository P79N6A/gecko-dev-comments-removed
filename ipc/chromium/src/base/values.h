



















#ifndef BASE_VALUES_H_
#define BASE_VALUES_H_

#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"

class Value;
class FundamentalValue;
class StringValue;
class BinaryValue;
class DictionaryValue;
class ListValue;

typedef std::vector<Value*> ValueVector;
typedef std::map<std::wstring, Value*> ValueMap;




class Value {
 public:
  virtual ~Value();

  
  
  
  static Value* CreateNullValue();
  static Value* CreateBooleanValue(bool in_value);
  static Value* CreateIntegerValue(int in_value);
  static Value* CreateRealValue(double in_value);
  static Value* CreateStringValue(const std::string& in_value);
  static Value* CreateStringValue(const std::wstring& in_value);

  
  
  static BinaryValue* CreateBinaryValue(char* buffer, size_t size);

  typedef enum {
    TYPE_NULL = 0,
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_BINARY,
    TYPE_DICTIONARY,
    TYPE_LIST
  } ValueType;

  
  
  
  
  
  ValueType GetType() const { return type_; }

  
  bool IsType(ValueType type) const { return type == type_; }

  
  
  
  
  virtual bool GetAsBoolean(bool* out_value) const;
  virtual bool GetAsInteger(int* out_value) const;
  virtual bool GetAsReal(double* out_value) const;
  virtual bool GetAsString(std::string* out_value) const;
  virtual bool GetAsString(std::wstring* out_value) const;

  
  
  virtual Value* DeepCopy() const;

  
  virtual bool Equals(const Value* other) const;

 protected:
  
  
  Value(ValueType type) : type_(type) {}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Value);
  Value();

  ValueType type_;
};


class FundamentalValue : public Value {
 public:
  FundamentalValue(bool in_value)
    : Value(TYPE_BOOLEAN), boolean_value_(in_value) {}
  FundamentalValue(int in_value)
    : Value(TYPE_INTEGER), integer_value_(in_value) {}
  FundamentalValue(double in_value)
    : Value(TYPE_REAL), real_value_(in_value) {}
  ~FundamentalValue();

  
  virtual bool GetAsBoolean(bool* out_value) const;
  virtual bool GetAsInteger(int* out_value) const;
  virtual bool GetAsReal(double* out_value) const;
  virtual Value* DeepCopy() const;
  virtual bool Equals(const Value* other) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FundamentalValue);

  union {
    bool boolean_value_;
    int integer_value_;
    double real_value_;
  };
};

class StringValue : public Value {
 public:
  
  StringValue(const std::string& in_value);

  
  StringValue(const std::wstring& in_value);

  ~StringValue();

  
  bool GetAsString(std::string* out_value) const;
  bool GetAsString(std::wstring* out_value) const;
  Value* DeepCopy() const;
  virtual bool Equals(const Value* other) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StringValue);

  std::string value_;
};

class BinaryValue: public Value {
public:
  
  
  
  static BinaryValue* Create(char* buffer, size_t size);

  
  
  
  
  static BinaryValue* CreateWithCopiedBuffer(char* buffer, size_t size);

  ~BinaryValue();

  
  Value* DeepCopy() const;
  virtual bool Equals(const Value* other) const;

  size_t GetSize() const { return size_; }
  char* GetBuffer() { return buffer_; }

private:
  DISALLOW_EVIL_CONSTRUCTORS(BinaryValue);

  
  
  BinaryValue(char* buffer, size_t size);

  char* buffer_;
  size_t size_;
};

class DictionaryValue : public Value {
 public:
  DictionaryValue() : Value(TYPE_DICTIONARY) {}
  ~DictionaryValue();

  
  Value* DeepCopy() const;
  virtual bool Equals(const Value* other) const;

  
  bool HasKey(const std::wstring& key) const;

  
  size_t GetSize() const { return dictionary_.size(); }

  
  void Clear();

  
  
  
  
  
  
  
  
  
  bool Set(const std::wstring& path, Value* in_value);

  
  
  bool SetBoolean(const std::wstring& path, bool in_value);
  bool SetInteger(const std::wstring& path, int in_value);
  bool SetReal(const std::wstring& path, double in_value);
  bool SetString(const std::wstring& path, const std::string& in_value);
  bool SetString(const std::wstring& path, const std::wstring& in_value);

  
  
  
  
  
  
  
  bool Get(const std::wstring& path, Value** out_value) const;

  
  
  
  bool GetBoolean(const std::wstring& path, bool* out_value) const;
  bool GetInteger(const std::wstring& path, int* out_value) const;
  bool GetReal(const std::wstring& path, double* out_value) const;
  bool GetString(const std::wstring& path, std::string* out_value) const;
  bool GetString(const std::wstring& path, std::wstring* out_value) const;
  bool GetBinary(const std::wstring& path, BinaryValue** out_value) const;
  bool GetDictionary(const std::wstring& path,
                     DictionaryValue** out_value) const;
  bool GetList(const std::wstring& path, ListValue** out_value) const;

  
  
  
  
  
  
  bool Remove(const std::wstring& path, Value** out_value);

  
  
  class key_iterator
    : private std::iterator<std::input_iterator_tag, const std::wstring> {
   public:
    key_iterator(ValueMap::const_iterator itr) { itr_ = itr; }
    key_iterator operator++() { ++itr_; return *this; }
    const std::wstring& operator*() { return itr_->first; }
    bool operator!=(const key_iterator& other) { return itr_ != other.itr_; }
    bool operator==(const key_iterator& other) { return itr_ == other.itr_; }

   private:
    ValueMap::const_iterator itr_;
  };

  key_iterator begin_keys() const { return key_iterator(dictionary_.begin()); }
  key_iterator end_keys() const { return key_iterator(dictionary_.end()); }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DictionaryValue);

  
  
  
  void SetInCurrentNode(const std::wstring& key, Value* in_value);

  ValueMap dictionary_;
};


class ListValue : public Value {
 public:
  ListValue() : Value(TYPE_LIST) {}
  ~ListValue();

  
  Value* DeepCopy() const;
  virtual bool Equals(const Value* other) const;

  
  void Clear();

  
  size_t GetSize() const { return list_.size(); }

  
  
  
  
  
  bool Set(size_t index, Value* in_value);

  
  
  
  bool Get(size_t index, Value** out_value) const;

  
  
  
  bool GetBoolean(size_t index, bool* out_value) const;
  bool GetInteger(size_t index, int* out_value) const;
  bool GetReal(size_t index, double* out_value) const;
  bool GetString(size_t index, std::string* out_value) const;
  bool GetBinary(size_t index, BinaryValue** out_value) const;
  bool GetDictionary(size_t index, DictionaryValue** out_value) const;
  bool GetList(size_t index, ListValue** out_value) const;

  
  
  
  
  
  bool Remove(size_t index, Value** out_value);

  
  void Append(Value* in_value);

  
  typedef ValueVector::iterator iterator;
  typedef ValueVector::const_iterator const_iterator;

  ListValue::iterator begin() { return list_.begin(); }
  ListValue::iterator end() { return list_.end(); }

  ListValue::const_iterator begin() const { return list_.begin(); }
  ListValue::const_iterator end() const { return list_.end(); }

  ListValue::iterator Erase(iterator item) {
    return list_.erase(item);
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ListValue);

  ValueVector list_;
};



class ValueSerializer {
 public:
  virtual ~ValueSerializer() {}

  virtual bool Serialize(const Value& root) = 0;

  
  
  
  
  virtual Value* Deserialize(std::string* error_message) = 0;
};

#endif  
