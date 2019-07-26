




























#include "breakpad_googletest_includes.h"
#include "common/linux/linux_libc_support.h"

namespace {
typedef testing::Test LinuxLibcSupportTest;
}

TEST(LinuxLibcSupportTest, strlen) {
  static const char* test_data[] = { "", "a", "aa", "aaa", "aabc", NULL };
  for (unsigned i = 0; ; ++i) {
    if (!test_data[i])
      break;
    ASSERT_EQ(strlen(test_data[i]), my_strlen(test_data[i]));
  }
}

TEST(LinuxLibcSupportTest, strcmp) {
  static const char* test_data[] = {
    "", "",
    "a", "",
    "", "a",
    "a", "b",
    "a", "a",
    "ab", "aa",
    "abc", "ab",
    "abc", "abc",
    NULL,
  };

  for (unsigned i = 0; ; ++i) {
    if (!test_data[i*2])
      break;
    int libc_result = strcmp(test_data[i*2], test_data[i*2 + 1]);
    if (libc_result > 1)
      libc_result = 1;
    else if (libc_result < -1)
      libc_result = -1;
    ASSERT_EQ(my_strcmp(test_data[i*2], test_data[i*2 + 1]), libc_result);
  }
}

TEST(LinuxLibcSupportTest, strtoui) {
  int result;

  ASSERT_FALSE(my_strtoui(&result, ""));
  ASSERT_FALSE(my_strtoui(&result, "-1"));
  ASSERT_FALSE(my_strtoui(&result, "-"));
  ASSERT_FALSE(my_strtoui(&result, "a"));
  ASSERT_FALSE(my_strtoui(&result, "23472893472938472987987398472398"));

  ASSERT_TRUE(my_strtoui(&result, "0"));
  ASSERT_EQ(result, 0);
  ASSERT_TRUE(my_strtoui(&result, "1"));
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(my_strtoui(&result, "12"));
  ASSERT_EQ(result, 12);
  ASSERT_TRUE(my_strtoui(&result, "123"));
  ASSERT_EQ(result, 123);
  ASSERT_TRUE(my_strtoui(&result, "0123"));
  ASSERT_EQ(result, 123);
}

TEST(LinuxLibcSupportTest, uint_len) {
  ASSERT_EQ(my_uint_len(0), 1U);
  ASSERT_EQ(my_uint_len(2), 1U);
  ASSERT_EQ(my_uint_len(5), 1U);
  ASSERT_EQ(my_uint_len(9), 1U);
  ASSERT_EQ(my_uint_len(10), 2U);
  ASSERT_EQ(my_uint_len(99), 2U);
  ASSERT_EQ(my_uint_len(100), 3U);
  ASSERT_EQ(my_uint_len(101), 3U);
  ASSERT_EQ(my_uint_len(1000), 4U);
  
  ASSERT_EQ(my_uint_len(18446744073709551615LLU), 20U);
}

TEST(LinuxLibcSupportTest, uitos) {
  char buf[32];

  my_uitos(buf, 0, 1);
  ASSERT_EQ(0, memcmp(buf, "0", 1));

  my_uitos(buf, 1, 1);
  ASSERT_EQ(0, memcmp(buf, "1", 1));

  my_uitos(buf, 10, 2);
  ASSERT_EQ(0, memcmp(buf, "10", 2));

  my_uitos(buf, 63, 2);
  ASSERT_EQ(0, memcmp(buf, "63", 2));

  my_uitos(buf, 101, 3);
  ASSERT_EQ(0, memcmp(buf, "101", 2));

  
  my_uitos(buf, 18446744073709551615LLU, 20);
  ASSERT_EQ(0, memcmp(buf, "18446744073709551615", 20));
}

TEST(LinuxLibcSupportTest, strchr) {
  ASSERT_EQ(NULL, my_strchr("abc", 'd'));
  ASSERT_EQ(NULL, my_strchr("", 'd'));
  ASSERT_EQ(NULL, my_strchr("efghi", 'd'));

  ASSERT_TRUE(my_strchr("a", 'a'));
  ASSERT_TRUE(my_strchr("abc", 'a'));
  ASSERT_TRUE(my_strchr("bcda", 'a'));
  ASSERT_TRUE(my_strchr("sdfasdf", 'a'));

  static const char abc3[] = "abcabcabc";
  ASSERT_EQ(abc3, my_strchr(abc3, 'a'));
}

TEST(LinuxLibcSupportTest, strrchr) {
  ASSERT_EQ(NULL, my_strrchr("abc", 'd'));
  ASSERT_EQ(NULL, my_strrchr("", 'd'));
  ASSERT_EQ(NULL, my_strrchr("efghi", 'd'));

  ASSERT_TRUE(my_strrchr("a", 'a'));
  ASSERT_TRUE(my_strrchr("abc", 'a'));
  ASSERT_TRUE(my_strrchr("bcda", 'a'));
  ASSERT_TRUE(my_strrchr("sdfasdf", 'a'));

  static const char abc3[] = "abcabcabc";
  ASSERT_EQ(abc3 + 6, my_strrchr(abc3, 'a'));
}

TEST(LinuxLibcSupportTest, read_hex_ptr) {
  uintptr_t result;
  const char* last;

  last = my_read_hex_ptr(&result, "");
  ASSERT_EQ(result, 0U);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0");
  ASSERT_EQ(result, 0U);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0123");
  ASSERT_EQ(result, 0x123U);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0123a");
  ASSERT_EQ(result, 0x123aU);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0123a-");
  ASSERT_EQ(result, 0x123aU);
  ASSERT_EQ(*last, '-');
}

TEST(LinuxLibcSupportTest, read_decimal_ptr) {
  uintptr_t result;
  const char* last;

  last = my_read_decimal_ptr(&result, "0");
  ASSERT_EQ(result, 0U);
  ASSERT_EQ(*last, 0);

  last = my_read_decimal_ptr(&result, "0123");
  ASSERT_EQ(result, 123U);
  ASSERT_EQ(*last, 0);

  last = my_read_decimal_ptr(&result, "1234");
  ASSERT_EQ(result, 1234U);
  ASSERT_EQ(*last, 0);

  last = my_read_decimal_ptr(&result, "01234-");
  ASSERT_EQ(result, 1234U);
  ASSERT_EQ(*last, '-');
}
