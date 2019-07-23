





























#ifndef BASE_JSON_READER_H_
#define BASE_JSON_READER_H_

#include <string>

#include "base/basictypes.h"
#include "testing/gtest/include/gtest/gtest_prod.h"

class Value;

class JSONReader {
 public:
  
  class Token {
   public:
    enum Type {
     OBJECT_BEGIN,           
     OBJECT_END,             
     ARRAY_BEGIN,            
     ARRAY_END,              
     STRING,
     NUMBER,
     BOOL_TRUE,              
     BOOL_FALSE,             
     NULL_TOKEN,             
     LIST_SEPARATOR,         
     OBJECT_PAIR_SEPARATOR,  
     END_OF_INPUT,
     INVALID_TOKEN,
    };
    Token(Type t, const wchar_t* b, int len)
      : type(t), begin(b), length(len) {}

    Type type;

    
    const wchar_t* begin;

    
    int length;

    
    wchar_t NextChar() {
      return *(begin + length);
    }
  };

  
  static const char* kBadRootElementType;
  static const char* kInvalidEscape;
  static const char* kSyntaxError;
  static const char* kTrailingComma;
  static const char* kTooMuchNesting;
  static const char* kUnexpectedDataAfterRoot;
  static const char* kUnsupportedEncoding;
  static const char* kUnquotedDictionaryKey;

  JSONReader();

  
  
  
  
  static Value* Read(const std::string& json, bool allow_trailing_comma);

  
  
  
  
  static Value* ReadAndReturnError(const std::string& json,
                                   bool allow_trailing_comma,
                                   std::string* error_message_out);

  
  
  std::string error_message() { return error_message_; }

  
  
  
  
  
  
  
  Value* JsonToValue(const std::string& json, bool check_root,
                     bool allow_trailing_comma);

 private:
  static std::string FormatErrorMessage(int line, int column,
                                        const char* description);

  DISALLOW_EVIL_CONSTRUCTORS(JSONReader);

  FRIEND_TEST(JSONReaderTest, Reading);
  FRIEND_TEST(JSONReaderTest, ErrorMessages);

  
  
  
  Value* BuildValue(bool is_root);

  
  
  
  
  Token ParseNumberToken();

  
  
  Value* DecodeNumber(const Token& token);

  
  
  
  
  Token ParseStringToken();

  
  
  Value* DecodeString(const Token& token);

  
  
  Token ParseToken();

  
  void EatWhitespaceAndComments();

  
  
  bool EatComment();

  
  bool NextStringMatch(const std::wstring& str);

  
  
  void SetErrorMessage(const char* description, const wchar_t* error_pos);

  
  const wchar_t* start_pos_;

  
  const wchar_t* json_pos_;

  
  int stack_depth_;

  
  bool allow_trailing_comma_;

  
  std::string error_message_;
};

#endif
