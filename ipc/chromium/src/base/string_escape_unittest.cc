



#include "testing/gtest/include/gtest/gtest.h"
#include "base/string_escape.h"
#include "base/string_util.h"

TEST(StringEscapeTest, JavascriptDoubleQuote) {
  static const char* kToEscape          = "\b\001aZ\"\\wee";
  static const char* kEscaped           = "\\b\\x01aZ\\\"\\\\wee";
  static const char* kEscapedQuoted     = "\"\\b\\x01aZ\\\"\\\\wee\"";
  static const wchar_t* kUToEscape      = L"\b\x0001" L"a\x123fZ\"\\wee";
  static const char* kUEscaped          = "\\b\\x01a\\u123FZ\\\"\\\\wee";
  static const char* kUEscapedQuoted    = "\"\\b\\x01a\\u123FZ\\\"\\\\wee\"";

  std::string out;

  
  out = "testy: ";
  string_escape::JavascriptDoubleQuote(WideToUTF16(kUToEscape), false, &out);
  ASSERT_EQ(std::string("testy: ") + kUEscaped, out);

  out = "testy: ";
  string_escape::JavascriptDoubleQuote(WideToUTF16(kUToEscape), true, &out);
  ASSERT_EQ(std::string("testy: ") + kUEscapedQuoted, out);

  
  string16 str16 = UTF8ToUTF16("TeSt");
  str16.push_back(0);
  str16.push_back(0xffb1);
  str16.push_back(0x00ff);

  out = "testy: ";
  string_escape::JavascriptDoubleQuote(str16, false, &out);
  ASSERT_EQ("testy: TeSt\\x00\\uFFB1\\xFF", out);

  
  out = "testy: ";
  string_escape::JavascriptDoubleQuote(std::string(kToEscape), false, &out);
  ASSERT_EQ(std::string("testy: ") + kEscaped, out);

  out = "testy: ";
  string_escape::JavascriptDoubleQuote(std::string(kToEscape), true, &out);
  ASSERT_EQ(std::string("testy: ") + kEscapedQuoted, out);

  
  std::string str("TeSt");
  str.push_back(0);
  str.push_back(15);
  str.push_back(127);
  str.push_back(-16);
  str.push_back(-128);
  str.push_back('!');

  out = "testy: ";
  string_escape::JavascriptDoubleQuote(str, false, &out);
  ASSERT_EQ("testy: TeSt\\x00\\x0F\\x7F\xf0\x80!", out);

  
  out = "testy: ";
  string_escape::JavascriptDoubleQuote("a\b\f\n\r\t\v\1\\.\"z", false, &out);
  ASSERT_EQ("testy: a\\b\\f\\n\\r\\t\\v\\x01\\\\.\\\"z", out);
}
