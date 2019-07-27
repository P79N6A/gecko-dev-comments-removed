









#include "webrtc/base/gunit.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/common.h"

namespace rtc {



TEST(string_matchTest, Matches) {
  EXPECT_TRUE( string_match("A.B.C.D", "a.b.c.d"));
  EXPECT_TRUE( string_match("www.TEST.GOOGLE.COM", "www.*.com"));
  EXPECT_TRUE( string_match("127.0.0.1",  "12*.0.*1"));
  EXPECT_TRUE( string_match("127.1.0.21", "12*.0.*1"));
  EXPECT_FALSE(string_match("127.0.0.0",  "12*.0.*1"));
  EXPECT_FALSE(string_match("127.0.0.0",  "12*.0.*1"));
  EXPECT_FALSE(string_match("127.1.1.21", "12*.0.*1"));
}





#if defined(WEBRTC_WIN)




TEST(ascii_string_compareTest, NullInput) {
  
  
  

  
}


TEST(ascii_string_compareTest, DifferentLengths) {
  EXPECT_EQ(-1, ascii_string_compare(L"Test", "Test1", 5, identity));
}



TEST(ascii_string_compareTest, SmallBuffer) {
  EXPECT_EQ(0, ascii_string_compare(L"Test", "Test1", 3, identity));
}


TEST(ascii_string_compareTest, LargeBuffer) {
  EXPECT_EQ(0, ascii_string_compare(L"Test", "Test", 10, identity));
}


TEST(ascii_string_compareTest, Equal) {
  EXPECT_EQ(0, ascii_string_compare(L"Test", "Test", 5, identity));
  EXPECT_EQ(0, ascii_string_compare(L"TeSt", "tEsT", 5, tolowercase));
}


TEST(ascii_string_compareTest, LessThan) {
  EXPECT_EQ(-1, ascii_string_compare(L"abc", "abd", 4, identity));
  EXPECT_EQ(-1, ascii_string_compare(L"ABC", "abD", 5, tolowercase));
}


TEST(ascii_string_compareTest, GreaterThan) {
  EXPECT_EQ(1, ascii_string_compare(L"xyz", "xy", 5, identity));
  EXPECT_EQ(1, ascii_string_compare(L"abc", "ABB", 5, tolowercase));
}
#endif  

TEST(string_trim_Test, Trimming) {
  EXPECT_EQ("temp", string_trim("\n\r\t temp \n\r\t"));
  EXPECT_EQ("temp\n\r\t temp", string_trim(" temp\n\r\t temp "));
  EXPECT_EQ("temp temp", string_trim("temp temp"));
  EXPECT_EQ("", string_trim(" \r\n\t"));
  EXPECT_EQ("", string_trim(""));
}

TEST(string_startsTest, StartsWith) {
  EXPECT_TRUE(starts_with("foobar", "foo"));
  EXPECT_TRUE(starts_with("foobar", "foobar"));
  EXPECT_TRUE(starts_with("foobar", ""));
  EXPECT_TRUE(starts_with("", ""));
  EXPECT_FALSE(starts_with("foobar", "bar"));
  EXPECT_FALSE(starts_with("foobar", "foobarbaz"));
  EXPECT_FALSE(starts_with("", "f"));
}

TEST(string_endsTest, EndsWith) {
  EXPECT_TRUE(ends_with("foobar", "bar"));
  EXPECT_TRUE(ends_with("foobar", "foobar"));
  EXPECT_TRUE(ends_with("foobar", ""));
  EXPECT_TRUE(ends_with("", ""));
  EXPECT_FALSE(ends_with("foobar", "foo"));
  EXPECT_FALSE(ends_with("foobar", "foobarbaz"));
  EXPECT_FALSE(ends_with("", "f"));
}

} 
