



#ifndef CHROME_COMMON_JSON_VALUE_SERIALIZER_H_
#define CHROME_COMMON_JSON_VALUE_SERIALIZER_H_

#include <string>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/values.h"

class JSONStringValueSerializer : public ValueSerializer {
 public:
  
  
  
  JSONStringValueSerializer(std::string* json_string)
      : json_string_(json_string),
        initialized_with_const_string_(false),
        pretty_print_(false),
        allow_trailing_comma_(false) {
  }

  
  
  JSONStringValueSerializer(const std::string& json_string)
      : json_string_(&const_cast<std::string&>(json_string)),
        initialized_with_const_string_(true),
        allow_trailing_comma_(false) {
  }

  ~JSONStringValueSerializer();

  
  
  
  bool Serialize(const Value& root);

  
  
  
  
  Value* Deserialize(std::string* error_message);

  void set_pretty_print(bool new_value) { pretty_print_ = new_value; }
  bool pretty_print() { return pretty_print_; }

  void set_allow_trailing_comma(bool new_value) {
    allow_trailing_comma_ = new_value;
  }

 private:
  std::string* json_string_;
  bool initialized_with_const_string_;
  bool pretty_print_;  
  
  bool allow_trailing_comma_;

  DISALLOW_EVIL_CONSTRUCTORS(JSONStringValueSerializer);
};

class JSONFileValueSerializer : public ValueSerializer {
 public:
  
  
  
  
  explicit JSONFileValueSerializer(const FilePath& json_file_path)
    : json_file_path_(json_file_path) {}

  ~JSONFileValueSerializer() {}

  
  
  
  
  
  
  
  
  bool Serialize(const Value& root);

  
  
  
  
  
  Value* Deserialize(std::string* error_message);

 private:
  FilePath json_file_path_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(JSONFileValueSerializer);
};

#endif  
