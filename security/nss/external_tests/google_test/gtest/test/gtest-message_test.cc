
































#include "gtest/gtest-message.h"

#include "gtest/gtest.h"

namespace {

using ::testing::Message;




TEST(MessageTest, DefaultConstructor) {
  const Message msg;
  EXPECT_EQ("", msg.GetString());
}


TEST(MessageTest, CopyConstructor) {
  const Message msg1("Hello");
  const Message msg2(msg1);
  EXPECT_EQ("Hello", msg2.GetString());
}


TEST(MessageTest, ConstructsFromCString) {
  Message msg("Hello");
  EXPECT_EQ("Hello", msg.GetString());
}


TEST(MessageTest, StreamsFloat) {
  const std::string s = (Message() << 1.23456F << " " << 2.34567F).GetString();
  
  EXPECT_PRED_FORMAT2(testing::IsSubstring, "1.234560", s.c_str());
  EXPECT_PRED_FORMAT2(testing::IsSubstring, " 2.345669", s.c_str());
}


TEST(MessageTest, StreamsDouble) {
  const std::string s = (Message() << 1260570880.4555497 << " "
                                  << 1260572265.1954534).GetString();
  
  EXPECT_PRED_FORMAT2(testing::IsSubstring, "1260570880.45", s.c_str());
  EXPECT_PRED_FORMAT2(testing::IsSubstring, " 1260572265.19", s.c_str());
}


TEST(MessageTest, StreamsPointer) {
  int n = 0;
  int* p = &n;
  EXPECT_NE("(null)", (Message() << p).GetString());
}


TEST(MessageTest, StreamsNullPointer) {
  int* p = NULL;
  EXPECT_EQ("(null)", (Message() << p).GetString());
}


TEST(MessageTest, StreamsCString) {
  EXPECT_EQ("Foo", (Message() << "Foo").GetString());
}


TEST(MessageTest, StreamsNullCString) {
  char* p = NULL;
  EXPECT_EQ("(null)", (Message() << p).GetString());
}


TEST(MessageTest, StreamsString) {
  const ::std::string str("Hello");
  EXPECT_EQ("Hello", (Message() << str).GetString());
}


TEST(MessageTest, StreamsStringWithEmbeddedNUL) {
  const char char_array_with_nul[] =
      "Here's a NUL\0 and some more string";
  const ::std::string string_with_nul(char_array_with_nul,
                                      sizeof(char_array_with_nul) - 1);
  EXPECT_EQ("Here's a NUL\\0 and some more string",
            (Message() << string_with_nul).GetString());
}


TEST(MessageTest, StreamsNULChar) {
  EXPECT_EQ("\\0", (Message() << '\0').GetString());
}


TEST(MessageTest, StreamsInt) {
  EXPECT_EQ("123", (Message() << 123).GetString());
}



TEST(MessageTest, StreamsBasicIoManip) {
  EXPECT_EQ("Line 1.\nA NUL char \\0 in line 2.",
               (Message() << "Line 1." << std::endl
                         << "A NUL char " << std::ends << std::flush
                         << " in line 2.").GetString());
}


TEST(MessageTest, GetString) {
  Message msg;
  msg << 1 << " lamb";
  EXPECT_EQ("1 lamb", msg.GetString());
}


TEST(MessageTest, StreamsToOStream) {
  Message msg("Hello");
  ::std::stringstream ss;
  ss << msg;
  EXPECT_EQ("Hello", testing::internal::StringStreamToString(&ss));
}


TEST(MessageTest, DoesNotTakeUpMuchStackSpace) {
  EXPECT_LE(sizeof(Message), 16U);
}

}  
