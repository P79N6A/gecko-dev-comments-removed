















#ifndef BASE_VALUES_H_
#define BASE_VALUES_H_

#include <stddef.h>

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"

namespace base {

class DictionaryValue;
class FundamentalValue;
class ListValue;
class StringValue;
class Value;

typedef std::vector<Value*> ValueVector;
typedef std::map<std::string, Value*> ValueMap;






class BASE_EXPORT Value {
 public:
  enum Type {
    TYPE_NULL = 0,
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BINARY,
    TYPE_DICTIONARY,
    TYPE_LIST
    
  };

  virtual ~Value();

  static Value* CreateNullValue();

  
  
  
  
  
  Type GetType() const { return type_; }

  
  bool IsType(Type type) const { return type == type_; }

  
  
  
  
  virtual bool GetAsBoolean(bool* out_value) const;
  virtual bool GetAsInteger(int* out_value) const;
  virtual bool GetAsDouble(double* out_value) const;
  virtual bool GetAsString(std::string* out_value) const;
  virtual bool GetAsString(string16* out_value) const;
  virtual bool GetAsString(const StringValue** out_value) const;
  virtual bool GetAsList(ListValue** out_value);
  virtual bool GetAsList(const ListValue** out_value) const;
  virtual bool GetAsDictionary(DictionaryValue** out_value);
  virtual bool GetAsDictionary(const DictionaryValue** out_value) const;
  

  
  
  
  
  
  virtual Value* DeepCopy() const;

  
  virtual bool Equals(const Value* other) const;

  
  
  static bool Equals(const Value* a, const Value* b);

 protected:
  
  explicit Value(Type type);
  Value(const Value& that);
  Value& operator=(const Value& that);

 private:
  Type type_;
};


class BASE_EXPORT FundamentalValue : public Value {
 public:
  explicit FundamentalValue(bool in_value);
  explicit FundamentalValue(int in_value);
  explicit FundamentalValue(double in_value);
  ~FundamentalValue() override;

  
  bool GetAsBoolean(bool* out_value) const override;
  bool GetAsInteger(int* out_value) const override;
  
  
  bool GetAsDouble(double* out_value) const override;
  FundamentalValue* DeepCopy() const override;
  bool Equals(const Value* other) const override;

 private:
  union {
    bool boolean_value_;
    int integer_value_;
    double double_value_;
  };
};

class BASE_EXPORT StringValue : public Value {
 public:
  
  explicit StringValue(const std::string& in_value);

  
  explicit StringValue(const string16& in_value);

  ~StringValue() override;

  
  std::string* GetString();
  const std::string& GetString() const;

  
  bool GetAsString(std::string* out_value) const override;
  bool GetAsString(string16* out_value) const override;
  bool GetAsString(const StringValue** out_value) const override;
  StringValue* DeepCopy() const override;
  bool Equals(const Value* other) const override;

 private:
  std::string value_;
};

class BASE_EXPORT BinaryValue: public Value {
 public:
  
  BinaryValue();

  
  
  BinaryValue(scoped_ptr<char[]> buffer, size_t size);

  ~BinaryValue() override;

  
  
  
  static BinaryValue* CreateWithCopiedBuffer(const char* buffer, size_t size);

  size_t GetSize() const { return size_; }

  
  char* GetBuffer() { return buffer_.get(); }
  const char* GetBuffer() const { return buffer_.get(); }

  
  BinaryValue* DeepCopy() const override;
  bool Equals(const Value* other) const override;

 private:
  scoped_ptr<char[]> buffer_;
  size_t size_;

  DISALLOW_COPY_AND_ASSIGN(BinaryValue);
};




class BASE_EXPORT DictionaryValue : public Value {
 public:
  DictionaryValue();
  ~DictionaryValue() override;

  
  bool GetAsDictionary(DictionaryValue** out_value) override;
  bool GetAsDictionary(const DictionaryValue** out_value) const override;

  
  bool HasKey(const std::string& key) const;

  
  size_t size() const { return dictionary_.size(); }

  
  bool empty() const { return dictionary_.empty(); }

  
  void Clear();

  
  
  
  
  
  
  
  
  
  void Set(const std::string& path, Value* in_value);

  
  
  void SetBoolean(const std::string& path, bool in_value);
  void SetInteger(const std::string& path, int in_value);
  void SetDouble(const std::string& path, double in_value);
  void SetString(const std::string& path, const std::string& in_value);
  void SetString(const std::string& path, const string16& in_value);

  
  
  void SetWithoutPathExpansion(const std::string& key, Value* in_value);

  
  void SetBooleanWithoutPathExpansion(const std::string& path, bool in_value);
  void SetIntegerWithoutPathExpansion(const std::string& path, int in_value);
  void SetDoubleWithoutPathExpansion(const std::string& path, double in_value);
  void SetStringWithoutPathExpansion(const std::string& path,
                                     const std::string& in_value);
  void SetStringWithoutPathExpansion(const std::string& path,
                                     const string16& in_value);

  
  
  
  
  
  
  
  
  bool Get(const std::string& path, const Value** out_value) const;
  bool Get(const std::string& path, Value** out_value);

  
  
  
  
  bool GetBoolean(const std::string& path, bool* out_value) const;
  bool GetInteger(const std::string& path, int* out_value) const;
  
  
  bool GetDouble(const std::string& path, double* out_value) const;
  bool GetString(const std::string& path, std::string* out_value) const;
  bool GetString(const std::string& path, string16* out_value) const;
  bool GetStringASCII(const std::string& path, std::string* out_value) const;
  bool GetBinary(const std::string& path, const BinaryValue** out_value) const;
  bool GetBinary(const std::string& path, BinaryValue** out_value);
  bool GetDictionary(const std::string& path,
                     const DictionaryValue** out_value) const;
  bool GetDictionary(const std::string& path, DictionaryValue** out_value);
  bool GetList(const std::string& path, const ListValue** out_value) const;
  bool GetList(const std::string& path, ListValue** out_value);

  
  
  bool GetWithoutPathExpansion(const std::string& key,
                               const Value** out_value) const;
  bool GetWithoutPathExpansion(const std::string& key, Value** out_value);
  bool GetBooleanWithoutPathExpansion(const std::string& key,
                                      bool* out_value) const;
  bool GetIntegerWithoutPathExpansion(const std::string& key,
                                      int* out_value) const;
  bool GetDoubleWithoutPathExpansion(const std::string& key,
                                     double* out_value) const;
  bool GetStringWithoutPathExpansion(const std::string& key,
                                     std::string* out_value) const;
  bool GetStringWithoutPathExpansion(const std::string& key,
                                     string16* out_value) const;
  bool GetDictionaryWithoutPathExpansion(
      const std::string& key,
      const DictionaryValue** out_value) const;
  bool GetDictionaryWithoutPathExpansion(const std::string& key,
                                         DictionaryValue** out_value);
  bool GetListWithoutPathExpansion(const std::string& key,
                                   const ListValue** out_value) const;
  bool GetListWithoutPathExpansion(const std::string& key,
                                   ListValue** out_value);

  
  
  
  
  
  
  virtual bool Remove(const std::string& path, scoped_ptr<Value>* out_value);

  
  
  virtual bool RemoveWithoutPathExpansion(const std::string& key,
                                          scoped_ptr<Value>* out_value);

  
  
  virtual bool RemovePath(const std::string& path,
                          scoped_ptr<Value>* out_value);

  
  
  DictionaryValue* DeepCopyWithoutEmptyChildren() const;

  
  
  
  
  
  void MergeDictionary(const DictionaryValue* dictionary);

  
  virtual void Swap(DictionaryValue* other);

  
  
  class BASE_EXPORT Iterator {
   public:
    explicit Iterator(const DictionaryValue& target);
    ~Iterator();

    bool IsAtEnd() const { return it_ == target_.dictionary_.end(); }
    void Advance() { ++it_; }

    const std::string& key() const { return it_->first; }
    const Value& value() const { return *it_->second; }

   private:
    const DictionaryValue& target_;
    ValueMap::const_iterator it_;
  };

  
  DictionaryValue* DeepCopy() const override;
  bool Equals(const Value* other) const override;

 private:
  ValueMap dictionary_;

  DISALLOW_COPY_AND_ASSIGN(DictionaryValue);
};


class BASE_EXPORT ListValue : public Value {
 public:
  typedef ValueVector::iterator iterator;
  typedef ValueVector::const_iterator const_iterator;

  ListValue();
  ~ListValue() override;

  
  void Clear();

  
  size_t GetSize() const { return list_.size(); }

  
  bool empty() const { return list_.empty(); }

  
  
  
  
  
  bool Set(size_t index, Value* in_value);

  
  
  
  
  bool Get(size_t index, const Value** out_value) const;
  bool Get(size_t index, Value** out_value);

  
  
  
  
  bool GetBoolean(size_t index, bool* out_value) const;
  bool GetInteger(size_t index, int* out_value) const;
  
  
  bool GetDouble(size_t index, double* out_value) const;
  bool GetString(size_t index, std::string* out_value) const;
  bool GetString(size_t index, string16* out_value) const;
  bool GetBinary(size_t index, const BinaryValue** out_value) const;
  bool GetBinary(size_t index, BinaryValue** out_value);
  bool GetDictionary(size_t index, const DictionaryValue** out_value) const;
  bool GetDictionary(size_t index, DictionaryValue** out_value);
  bool GetList(size_t index, const ListValue** out_value) const;
  bool GetList(size_t index, ListValue** out_value);

  
  
  
  
  
  virtual bool Remove(size_t index, scoped_ptr<Value>* out_value);

  
  
  
  bool Remove(const Value& value, size_t* index);

  
  
  
  
  iterator Erase(iterator iter, scoped_ptr<Value>* out_value);

  
  void Append(Value* in_value);

  
  void AppendBoolean(bool in_value);
  void AppendInteger(int in_value);
  void AppendDouble(double in_value);
  void AppendString(const std::string& in_value);
  void AppendString(const string16& in_value);
  void AppendStrings(const std::vector<std::string>& in_values);
  void AppendStrings(const std::vector<string16>& in_values);

  
  
  
  bool AppendIfNotPresent(Value* in_value);

  
  
  bool Insert(size_t index, Value* in_value);

  
  
  
  const_iterator Find(const Value& value) const;

  
  virtual void Swap(ListValue* other);

  
  iterator begin() { return list_.begin(); }
  iterator end() { return list_.end(); }

  const_iterator begin() const { return list_.begin(); }
  const_iterator end() const { return list_.end(); }

  
  bool GetAsList(ListValue** out_value) override;
  bool GetAsList(const ListValue** out_value) const override;
  ListValue* DeepCopy() const override;
  bool Equals(const Value* other) const override;

 private:
  ValueVector list_;

  DISALLOW_COPY_AND_ASSIGN(ListValue);
};



class BASE_EXPORT ValueSerializer {
 public:
  virtual ~ValueSerializer();

  virtual bool Serialize(const Value& root) = 0;

  
  
  
  
  
  
  virtual Value* Deserialize(int* error_code, std::string* error_str) = 0;
};





BASE_EXPORT std::ostream& operator<<(std::ostream& out, const Value& value);

BASE_EXPORT inline std::ostream& operator<<(std::ostream& out,
                                            const FundamentalValue& value) {
  return out << static_cast<const Value&>(value);
}

BASE_EXPORT inline std::ostream& operator<<(std::ostream& out,
                                            const StringValue& value) {
  return out << static_cast<const Value&>(value);
}

BASE_EXPORT inline std::ostream& operator<<(std::ostream& out,
                                            const DictionaryValue& value) {
  return out << static_cast<const Value&>(value);
}

BASE_EXPORT inline std::ostream& operator<<(std::ostream& out,
                                            const ListValue& value) {
  return out << static_cast<const Value&>(value);
}

}  

#endif  
