
































#include "gtest/gtest-message.h"

#include "gtest/gtest.h"

namespace {

using ::testing::Message;


const char* ToCString(const Message& msg) {
  static testing::internal::String result;
  result = msg.GetString();
  return result.c_str();
}




TEST(MessageTest, DefaultConstructor) {
  const Message msg;
  EXPECT_STREQ("", ToCString(msg));
}


TEST(MessageTest, CopyConstructor) {
  const Message msg1("Hello");
  const Message msg2(msg1);
  EXPECT_STREQ("Hello", ToCString(msg2));
}


TEST(MessageTest, ConstructsFromCString) {
  Message msg("Hello");
  EXPECT_STREQ("Hello", ToCString(msg));
}


TEST(MessageTest, StreamsFloat) {
  const char* const s = ToCString(Message() << 1.23456F << " " << 2.34567F);
  
  EXPECT_PRED_FORMAT2(testing::IsSubstring, "1.234560", s);
  EXPECT_PRED_FORMAT2(testing::IsSubstring, " 2.345669", s);
}


TEST(MessageTest, StreamsDouble) {
  const char* const s = ToCString(Message() << 1260570880.4555497 << " "
                                  << 1260572265.1954534);
  
  EXPECT_PRED_FORMAT2(testing::IsSubstring, "1260570880.45", s);
  EXPECT_PRED_FORMAT2(testing::IsSubstring, " 1260572265.19", s);
}


TEST(MessageTest, StreamsPointer) {
  int n = 0;
  int* p = &n;
  EXPECT_STRNE("(null)", ToCString(Message() << p));
}


TEST(MessageTest, StreamsNullPointer) {
  int* p = NULL;
  EXPECT_STREQ("(null)", ToCString(Message() << p));
}


TEST(MessageTest, StreamsCString) {
  EXPECT_STREQ("Foo", ToCString(Message() << "Foo"));
}


TEST(MessageTest, StreamsNullCString) {
  char* p = NULL;
  EXPECT_STREQ("(null)", ToCString(Message() << p));
}


TEST(MessageTest, StreamsString) {
  const ::std::string str("Hello");
  EXPECT_STREQ("Hello", ToCString(Message() << str));
}


TEST(MessageTest, StreamsStringWithEmbeddedNUL) {
  const char char_array_with_nul[] =
      "Here's a NUL\0 and some more string";
  const ::std::string string_with_nul(char_array_with_nul,
                                      sizeof(char_array_with_nul) - 1);
  EXPECT_STREQ("Here's a NUL\\0 and some more string",
               ToCString(Message() << string_with_nul));
}


TEST(MessageTest, StreamsNULChar) {
  EXPECT_STREQ("\\0", ToCString(Message() << '\0'));
}


TEST(MessageTest, StreamsInt) {
  EXPECT_STREQ("123", ToCString(Message() << 123));
}



TEST(MessageTest, StreamsBasicIoManip) {
  EXPECT_STREQ("Line 1.\nA NUL char \\0 in line 2.",
               ToCString(Message() << "Line 1." << std::endl
                         << "A NUL char " << std::ends << std::flush
                         << " in line 2."));
}


TEST(MessageTest, GetString) {
  Message msg;
  msg << 1 << " lamb";
  EXPECT_STREQ("1 lamb", msg.GetString().c_str());
}


TEST(MessageTest, StreamsToOStream) {
  Message msg("Hello");
  ::std::stringstream ss;
  ss << msg;
  EXPECT_STREQ("Hello", testing::internal::StringStreamToString(&ss).c_str());
}


TEST(MessageTest, DoesNotTakeUpMuchStackSpace) {
  EXPECT_LE(sizeof(Message), 16U);
}

}  
