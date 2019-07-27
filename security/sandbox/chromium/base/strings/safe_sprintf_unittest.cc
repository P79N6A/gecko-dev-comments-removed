



#include "base/strings/safe_sprintf.h"

#include <stdio.h>
#include <string.h>

#include <limits>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"




#if defined(GTEST_HAS_DEATH_TEST) && !defined(OS_ANDROID)
#define ALLOW_DEATH_TEST
#endif

namespace base {
namespace strings {

TEST(SafeSPrintfTest, Empty) {
  char buf[2] = { 'X', 'X' };

  
  EXPECT_EQ(-1, SafeSNPrintf(buf, static_cast<size_t>(-1), ""));
  EXPECT_EQ('X', buf[0]);
  EXPECT_EQ('X', buf[1]);

  
  EXPECT_EQ(-1, SafeSNPrintf(buf, 0, ""));
  EXPECT_EQ('X', buf[0]);
  EXPECT_EQ('X', buf[1]);

  
  EXPECT_EQ(0, SafeSNPrintf(buf, 1, ""));
  EXPECT_EQ(0, buf[0]);
  EXPECT_EQ('X', buf[1]);
  buf[0] = 'X';

  
  EXPECT_EQ(0, SafeSNPrintf(buf, 2, ""));
  EXPECT_EQ(0, buf[0]);
  EXPECT_EQ('X', buf[1]);
  buf[0] = 'X';

  
  EXPECT_EQ(0, SafeSPrintf(buf, ""));
  EXPECT_EQ(0, buf[0]);
  EXPECT_EQ('X', buf[1]);
  buf[0] = 'X';
}

TEST(SafeSPrintfTest, NoArguments) {
  
  
  
  static const char text[] = "hello world";
  char ref[20], buf[20];
  memset(ref, 'X', sizeof(char) * arraysize(buf));
  memcpy(buf, ref, sizeof(buf));

  
  EXPECT_EQ(-1, SafeSNPrintf(buf, static_cast<size_t>(-1), text));
  EXPECT_TRUE(!memcmp(buf, ref, sizeof(buf)));

  
  EXPECT_EQ(-1, SafeSNPrintf(buf, 0, text));
  EXPECT_TRUE(!memcmp(buf, ref, sizeof(buf)));

  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1, SafeSNPrintf(buf, 1, text));
  EXPECT_EQ(0, buf[0]);
  EXPECT_TRUE(!memcmp(buf+1, ref+1, sizeof(buf)-1));
  memcpy(buf, ref, sizeof(buf));

  
  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1, SafeSNPrintf(buf, 2, text));
  EXPECT_EQ(text[0], buf[0]);
  EXPECT_EQ(0, buf[1]);
  EXPECT_TRUE(!memcmp(buf+2, ref+2, sizeof(buf)-2));
  memcpy(buf, ref, sizeof(buf));

  
  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1,
            SafeSNPrintf(buf, sizeof(buf), text));
  EXPECT_EQ(std::string(text), std::string(buf));
  EXPECT_TRUE(!memcmp(buf + sizeof(text), ref + sizeof(text),
                      sizeof(buf) - sizeof(text)));
  memcpy(buf, ref, sizeof(buf));

  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1, SafeSPrintf(buf, text));
  EXPECT_EQ(std::string(text), std::string(buf));
  EXPECT_TRUE(!memcmp(buf + sizeof(text), ref + sizeof(text),
                      sizeof(buf) - sizeof(text)));
  memcpy(buf, ref, sizeof(buf));

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%%"));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%%%"));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%X"));
  EXPECT_EQ(3, SafeSPrintf(buf, "%%%%X"));
#if defined(NDEBUG)
  EXPECT_EQ(1, SafeSPrintf(buf, "%"));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%%"));
  EXPECT_EQ(2, SafeSPrintf(buf, "%X"));
  EXPECT_EQ(3, SafeSPrintf(buf, "%%%X"));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, "%"), "src.1. == '%'");
  EXPECT_DEATH(SafeSPrintf(buf, "%%%"), "src.1. == '%'");
  EXPECT_DEATH(SafeSPrintf(buf, "%X"), "src.1. == '%'");
  EXPECT_DEATH(SafeSPrintf(buf, "%%%X"), "src.1. == '%'");
#endif
}

TEST(SafeSPrintfTest, OneArgument) {
  
  const char text[] = "hello world";
  const char fmt[]  = "hello%cworld";
  char ref[20], buf[20];
  memset(ref, 'X', sizeof(buf));
  memcpy(buf, ref, sizeof(buf));

  
  EXPECT_EQ(-1, SafeSNPrintf(buf, static_cast<size_t>(-1), fmt, ' '));
  EXPECT_TRUE(!memcmp(buf, ref, sizeof(buf)));

  
  EXPECT_EQ(-1, SafeSNPrintf(buf, 0, fmt, ' '));
  EXPECT_TRUE(!memcmp(buf, ref, sizeof(buf)));

  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1,
            SafeSNPrintf(buf, 1, fmt, ' '));
  EXPECT_EQ(0, buf[0]);
  EXPECT_TRUE(!memcmp(buf+1, ref+1, sizeof(buf)-1));
  memcpy(buf, ref, sizeof(buf));

  
  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1,
            SafeSNPrintf(buf, 2, fmt, ' '));
  EXPECT_EQ(text[0], buf[0]);
  EXPECT_EQ(0, buf[1]);
  EXPECT_TRUE(!memcmp(buf+2, ref+2, sizeof(buf)-2));
  memcpy(buf, ref, sizeof(buf));

  
  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1,
            SafeSNPrintf(buf, sizeof(buf), fmt, ' '));
  EXPECT_EQ(std::string(text), std::string(buf));
  EXPECT_TRUE(!memcmp(buf + sizeof(text), ref + sizeof(text),
                      sizeof(buf) - sizeof(text)));
  memcpy(buf, ref, sizeof(buf));

  
  EXPECT_EQ(static_cast<ssize_t>(sizeof(text))-1, SafeSPrintf(buf, fmt, ' '));
  EXPECT_EQ(std::string(text), std::string(buf));
  EXPECT_TRUE(!memcmp(buf + sizeof(text), ref + sizeof(text),
                      sizeof(buf) - sizeof(text)));
  memcpy(buf, ref, sizeof(buf));

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%%", 0));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%%%", 0));
  EXPECT_EQ(2, SafeSPrintf(buf, "%Y", 0));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%Y", 0));
  EXPECT_EQ(3, SafeSPrintf(buf, "%%%Y", 0));
  EXPECT_EQ(3, SafeSPrintf(buf, "%%%%Y", 0));
#if defined(NDEBUG)
  EXPECT_EQ(1, SafeSPrintf(buf, "%", 0));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%%", 0));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, "%", 0), "ch");
  EXPECT_DEATH(SafeSPrintf(buf, "%%%", 0), "ch");
#endif
}

TEST(SafeSPrintfTest, MissingArg) {
#if defined(NDEBUG)
  char buf[20];
  EXPECT_EQ(3, SafeSPrintf(buf, "%c%c", 'A'));
  EXPECT_EQ("A%c", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  char buf[20];
  EXPECT_DEATH(SafeSPrintf(buf, "%c%c", 'A'), "cur_arg < max_args");
#endif
}

TEST(SafeSPrintfTest, ASANFriendlyBufferTest) {
  
  
  
  
  const char kTestString[] = "This is a test";
  scoped_ptr<char[]> buf(new char[sizeof(kTestString)]);
  EXPECT_EQ(static_cast<ssize_t>(sizeof(kTestString) - 1),
            SafeSNPrintf(buf.get(), sizeof(kTestString), kTestString));
  EXPECT_EQ(std::string(kTestString), std::string(buf.get()));
  EXPECT_EQ(static_cast<ssize_t>(sizeof(kTestString) - 1),
            SafeSNPrintf(buf.get(), sizeof(kTestString), "%s", kTestString));
  EXPECT_EQ(std::string(kTestString), std::string(buf.get()));
}

TEST(SafeSPrintfTest, NArgs) {
  
  
  
  
  
  char buf[12];
  EXPECT_EQ(1, SafeSPrintf(buf, "%c", 1));
  EXPECT_EQ("\1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%c%c", 1, 2));
  EXPECT_EQ("\1\2", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%c%c%c", 1, 2, 3));
  EXPECT_EQ("\1\2\3", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%c%c%c%c", 1, 2, 3, 4));
  EXPECT_EQ("\1\2\3\4", std::string(buf));
  EXPECT_EQ(5, SafeSPrintf(buf, "%c%c%c%c%c", 1, 2, 3, 4, 5));
  EXPECT_EQ("\1\2\3\4\5", std::string(buf));
  EXPECT_EQ(6, SafeSPrintf(buf, "%c%c%c%c%c%c", 1, 2, 3, 4, 5, 6));
  EXPECT_EQ("\1\2\3\4\5\6", std::string(buf));
  EXPECT_EQ(7, SafeSPrintf(buf, "%c%c%c%c%c%c%c", 1, 2, 3, 4, 5, 6, 7));
  EXPECT_EQ("\1\2\3\4\5\6\7", std::string(buf));
  EXPECT_EQ(8, SafeSPrintf(buf, "%c%c%c%c%c%c%c%c", 1, 2, 3, 4, 5, 6, 7, 8));
  EXPECT_EQ("\1\2\3\4\5\6\7\10", std::string(buf));
  EXPECT_EQ(9, SafeSPrintf(buf, "%c%c%c%c%c%c%c%c%c",
                           1, 2, 3, 4, 5, 6, 7, 8, 9));
  EXPECT_EQ("\1\2\3\4\5\6\7\10\11", std::string(buf));
  EXPECT_EQ(10, SafeSPrintf(buf, "%c%c%c%c%c%c%c%c%c%c",
                            1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

  
  EXPECT_EQ("\1\2\3\4\5\6\7\10\11\12", std::string(buf));
  EXPECT_EQ(1, SafeSNPrintf(buf, 11, "%c", 1));
  EXPECT_EQ("\1", std::string(buf));
  EXPECT_EQ(2, SafeSNPrintf(buf, 11, "%c%c", 1, 2));
  EXPECT_EQ("\1\2", std::string(buf));
  EXPECT_EQ(3, SafeSNPrintf(buf, 11, "%c%c%c", 1, 2, 3));
  EXPECT_EQ("\1\2\3", std::string(buf));
  EXPECT_EQ(4, SafeSNPrintf(buf, 11, "%c%c%c%c", 1, 2, 3, 4));
  EXPECT_EQ("\1\2\3\4", std::string(buf));
  EXPECT_EQ(5, SafeSNPrintf(buf, 11, "%c%c%c%c%c", 1, 2, 3, 4, 5));
  EXPECT_EQ("\1\2\3\4\5", std::string(buf));
  EXPECT_EQ(6, SafeSNPrintf(buf, 11, "%c%c%c%c%c%c", 1, 2, 3, 4, 5, 6));
  EXPECT_EQ("\1\2\3\4\5\6", std::string(buf));
  EXPECT_EQ(7, SafeSNPrintf(buf, 11, "%c%c%c%c%c%c%c", 1, 2, 3, 4, 5, 6, 7));
  EXPECT_EQ("\1\2\3\4\5\6\7", std::string(buf));
  EXPECT_EQ(8, SafeSNPrintf(buf, 11, "%c%c%c%c%c%c%c%c",
                            1, 2, 3, 4, 5, 6, 7, 8));
  EXPECT_EQ("\1\2\3\4\5\6\7\10", std::string(buf));
  EXPECT_EQ(9, SafeSNPrintf(buf, 11, "%c%c%c%c%c%c%c%c%c",
                            1, 2, 3, 4, 5, 6, 7, 8, 9));
  EXPECT_EQ("\1\2\3\4\5\6\7\10\11", std::string(buf));
  EXPECT_EQ(10, SafeSNPrintf(buf, 11, "%c%c%c%c%c%c%c%c%c%c",
                             1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
  EXPECT_EQ("\1\2\3\4\5\6\7\10\11\12", std::string(buf));

  EXPECT_EQ(11, SafeSPrintf(buf, "%c%c%c%c%c%c%c%c%c%c%c",
                            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11));
  EXPECT_EQ("\1\2\3\4\5\6\7\10\11\12\13", std::string(buf));
  EXPECT_EQ(11, SafeSNPrintf(buf, 12, "%c%c%c%c%c%c%c%c%c%c%c",
                             1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11));
  EXPECT_EQ("\1\2\3\4\5\6\7\10\11\12\13", std::string(buf));
}

TEST(SafeSPrintfTest, DataTypes) {
  char buf[40];

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (uint8_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%d", (uint8_t)-1));
  EXPECT_EQ("255", std::string(buf));
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (int8_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%d", (int8_t)-1));
  EXPECT_EQ("-1", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%d", (int8_t)-128));
  EXPECT_EQ("-128", std::string(buf));

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (uint16_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(5, SafeSPrintf(buf, "%d", (uint16_t)-1));
  EXPECT_EQ("65535", std::string(buf));
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (int16_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%d", (int16_t)-1));
  EXPECT_EQ("-1", std::string(buf));
  EXPECT_EQ(6, SafeSPrintf(buf, "%d", (int16_t)-32768));
  EXPECT_EQ("-32768", std::string(buf));

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (uint32_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(10, SafeSPrintf(buf, "%d", (uint32_t)-1));
  EXPECT_EQ("4294967295", std::string(buf));
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (int32_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%d", (int32_t)-1));
  EXPECT_EQ("-1", std::string(buf));
  
  EXPECT_EQ(11, SafeSPrintf(buf, "%d", (int32_t)-2147483647-1));
  EXPECT_EQ("-2147483648", std::string(buf));

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (uint64_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(20, SafeSPrintf(buf, "%d", (uint64_t)-1));
  EXPECT_EQ("18446744073709551615", std::string(buf));
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", (int64_t)1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%d", (int64_t)-1));
  EXPECT_EQ("-1", std::string(buf));
  
  EXPECT_EQ(20, SafeSPrintf(buf, "%d", (int64_t)-9223372036854775807LL-1));
  EXPECT_EQ("-9223372036854775808", std::string(buf));

  
  EXPECT_EQ(4, SafeSPrintf(buf, "test"));
  EXPECT_EQ("test", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, buf));
  EXPECT_EQ("test", std::string(buf));

  
  char addr[20];
  sprintf(addr, "0x%llX", (unsigned long long)(uintptr_t)buf);
  SafeSPrintf(buf, "%p", buf);
  EXPECT_EQ(std::string(addr), std::string(buf));
  SafeSPrintf(buf, "%p", (const char *)buf);
  EXPECT_EQ(std::string(addr), std::string(buf));
  sprintf(addr, "0x%llX", (unsigned long long)(uintptr_t)sprintf);
  SafeSPrintf(buf, "%p", sprintf);
  EXPECT_EQ(std::string(addr), std::string(buf));

  
  
  
  sprintf(addr, "0x%017llX", (unsigned long long)(uintptr_t)buf);
  SafeSPrintf(buf, "%019p", buf);
  EXPECT_EQ(std::string(addr), std::string(buf));
  sprintf(addr, "0x%llX", (unsigned long long)(uintptr_t)buf);
  memset(addr, ' ',
         (char*)memmove(addr + sizeof(addr) - strlen(addr) - 1,
                        addr, strlen(addr)+1) - addr);
  SafeSPrintf(buf, "%19p", buf);
  EXPECT_EQ(std::string(addr), std::string(buf));
}

namespace {
void PrintLongString(char* buf, size_t sz) {
  
  
  CHECK_GT(sz, static_cast<size_t>(0));

  
  
  scoped_ptr<char[]> tmp(new char[sz+2]);
  memset(tmp.get(), 'X', sz+2);

  
  
  
  
  
  
  
  
  char* out = tmp.get();
  size_t out_sz = sz;
  size_t len;
  for (scoped_ptr<char[]> perfect_buf;;) {
    size_t needed = SafeSNPrintf(out, out_sz,
#if defined(NDEBUG)
                            "A%2cong %s: %d %010X %d %p%7s", 'l', "string", "",
#else
                            "A%2cong %s: %%d %010X %d %p%7s", 'l', "string",
#endif
                            0xDEADBEEF, std::numeric_limits<intptr_t>::min(),
                            PrintLongString, static_cast<char*>(NULL)) + 1;

    
    
    
    len = strlen(tmp.get());
    CHECK_GE(needed, len+1);

    
    
    CHECK_LT(len, out_sz);

    
    
    EXPECT_FALSE(tmp[len]);

    
    
    
    
    
    if (!perfect_buf.get()) {
      out_sz = std::min(needed, sz);
      out = new char[out_sz];
      perfect_buf.reset(out);
    } else {
      break;
    }
  }

  
  for (size_t i = len+1; i < sz+2; ++i)
    EXPECT_EQ('X', tmp[i]);

  
  
  
  
  
  
  
  
  char ref[256];
  CHECK_LE(sz, sizeof(ref));
  sprintf(ref, "A long string: %%d 00DEADBEEF %lld 0x%llX <NULL>",
          static_cast<long long>(std::numeric_limits<intptr_t>::min()),
          static_cast<unsigned long long>(
            reinterpret_cast<uintptr_t>(PrintLongString)));
  ref[sz-1] = '\000';

#if defined(NDEBUG)
  const size_t kSSizeMax = std::numeric_limits<ssize_t>::max();
#else
  const size_t kSSizeMax = internal::GetSafeSPrintfSSizeMaxForTest();
#endif

  
  EXPECT_EQ(std::string(ref).substr(0, kSSizeMax-1), std::string(tmp.get()));

  
  
  
  memcpy(buf, tmp.get(), len+1);
}

#if !defined(NDEBUG)
class ScopedSafeSPrintfSSizeMaxSetter {
 public:
  ScopedSafeSPrintfSSizeMaxSetter(size_t sz) {
    old_ssize_max_ = internal::GetSafeSPrintfSSizeMaxForTest();
    internal::SetSafeSPrintfSSizeMaxForTest(sz);
  }

  ~ScopedSafeSPrintfSSizeMaxSetter() {
    internal::SetSafeSPrintfSSizeMaxForTest(old_ssize_max_);
  }

 private:
  size_t old_ssize_max_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSafeSPrintfSSizeMaxSetter);
};
#endif

}  

TEST(SafeSPrintfTest, Truncation) {
  
  
  
  
  char ref[256];
  PrintLongString(ref, sizeof(ref));
  for (size_t i = strlen(ref)+1; i; --i) {
    char buf[sizeof(ref)];
    PrintLongString(buf, i);
    EXPECT_EQ(std::string(ref, i - 1), std::string(buf));
  }

  
  
  
  
  
  
#if !defined(NDEBUG)
  for (size_t i = strlen(ref)+1; i > 1; --i) {
    ScopedSafeSPrintfSSizeMaxSetter ssize_max_setter(i);
    char buf[sizeof(ref)];
    PrintLongString(buf, sizeof(buf));
    EXPECT_EQ(std::string(ref, i - 1), std::string(buf));
  }

  
  
  ScopedSafeSPrintfSSizeMaxSetter ssize_max_setter(100);
  char buf[256];
  EXPECT_EQ(99, SafeSPrintf(buf, "%99c", ' '));
  EXPECT_EQ(std::string(99, ' '), std::string(buf));
  *buf = '\000';
#if defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, "%100c", ' '), "padding <= max_padding");
#endif
  EXPECT_EQ(0, *buf);
#endif
}

TEST(SafeSPrintfTest, Padding) {
  char buf[40], fmt[40];

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%c", 'A'));
  EXPECT_EQ("A", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%2c", 'A'));
  EXPECT_EQ(" A", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%02c", 'A'));
  EXPECT_EQ(" A", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%-2c", 'A'));
  EXPECT_EQ("%-2c", std::string(buf));
  SafeSPrintf(fmt, "%%%dc", std::numeric_limits<ssize_t>::max() - 1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1, SafeSPrintf(buf, fmt, 'A'));
  SafeSPrintf(fmt, "%%%dc",
              static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, fmt, 'A'));
  EXPECT_EQ("%c", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, fmt, 'A'), "padding <= max_padding");
#endif

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%o", 1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%2o", 1));
  EXPECT_EQ(" 1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%02o", 1));
  EXPECT_EQ("01", std::string(buf));
  EXPECT_EQ(12, SafeSPrintf(buf, "%12o", -1));
  EXPECT_EQ(" 37777777777", std::string(buf));
  EXPECT_EQ(12, SafeSPrintf(buf, "%012o", -1));
  EXPECT_EQ("037777777777", std::string(buf));
  EXPECT_EQ(23, SafeSPrintf(buf, "%23o", -1LL));
  EXPECT_EQ(" 1777777777777777777777", std::string(buf));
  EXPECT_EQ(23, SafeSPrintf(buf, "%023o", -1LL));
  EXPECT_EQ("01777777777777777777777", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%2o", 0111));
  EXPECT_EQ("111", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%-2o", 1));
  EXPECT_EQ("%-2o", std::string(buf));
  SafeSPrintf(fmt, "%%%do", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, 1));
  EXPECT_EQ("   ", std::string(buf));
  SafeSPrintf(fmt, "%%0%do", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, 1));
  EXPECT_EQ("000", std::string(buf));
  SafeSPrintf(fmt, "%%%do",
              static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, fmt, 1));
  EXPECT_EQ("%o", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, fmt, 1), "padding <= max_padding");
#endif

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", 1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%2d", 1));
  EXPECT_EQ(" 1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%02d", 1));
  EXPECT_EQ("01", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%3d", -1));
  EXPECT_EQ(" -1", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%03d", -1));
  EXPECT_EQ("-01", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%2d", 111));
  EXPECT_EQ("111", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%2d", -111));
  EXPECT_EQ("-111", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%-2d", 1));
  EXPECT_EQ("%-2d", std::string(buf));
  SafeSPrintf(fmt, "%%%dd", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, 1));
  EXPECT_EQ("   ", std::string(buf));
  SafeSPrintf(fmt, "%%0%dd", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, 1));
  EXPECT_EQ("000", std::string(buf));
  SafeSPrintf(fmt, "%%%dd",
              static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, fmt, 1));
  EXPECT_EQ("%d", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, fmt, 1), "padding <= max_padding");
#endif

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%X", 1));
  EXPECT_EQ("1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%2X", 1));
  EXPECT_EQ(" 1", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%02X", 1));
  EXPECT_EQ("01", std::string(buf));
  EXPECT_EQ(9, SafeSPrintf(buf, "%9X", -1));
  EXPECT_EQ(" FFFFFFFF", std::string(buf));
  EXPECT_EQ(9, SafeSPrintf(buf, "%09X", -1));
  EXPECT_EQ("0FFFFFFFF", std::string(buf));
  EXPECT_EQ(17, SafeSPrintf(buf, "%17X", -1LL));
  EXPECT_EQ(" FFFFFFFFFFFFFFFF", std::string(buf));
  EXPECT_EQ(17, SafeSPrintf(buf, "%017X", -1LL));
  EXPECT_EQ("0FFFFFFFFFFFFFFFF", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%2X", 0x111));
  EXPECT_EQ("111", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%-2X", 1));
  EXPECT_EQ("%-2X", std::string(buf));
  SafeSPrintf(fmt, "%%%dX", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, 1));
  EXPECT_EQ("   ", std::string(buf));
  SafeSPrintf(fmt, "%%0%dX", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, 1));
  EXPECT_EQ("000", std::string(buf));
  SafeSPrintf(fmt, "%%%dX",
              static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, fmt, 1));
  EXPECT_EQ("%X", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, fmt, 1), "padding <= max_padding");
#endif

  
  EXPECT_EQ(3, SafeSPrintf(buf, "%p", (void*)1));
  EXPECT_EQ("0x1", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%4p", (void*)1));
  EXPECT_EQ(" 0x1", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%04p", (void*)1));
  EXPECT_EQ("0x01", std::string(buf));
  EXPECT_EQ(5, SafeSPrintf(buf, "%4p", (void*)0x111));
  EXPECT_EQ("0x111", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%-2p", (void*)1));
  EXPECT_EQ("%-2p", std::string(buf));
  SafeSPrintf(fmt, "%%%dp", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, (void*)1));
  EXPECT_EQ("   ", std::string(buf));
  SafeSPrintf(fmt, "%%0%dp", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, (void*)1));
  EXPECT_EQ("0x0", std::string(buf));
  SafeSPrintf(fmt, "%%%dp",
              static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, fmt, 1));
  EXPECT_EQ("%p", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, fmt, 1), "padding <= max_padding");
#endif

  
  EXPECT_EQ(1, SafeSPrintf(buf, "%s", "A"));
  EXPECT_EQ("A", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%2s", "A"));
  EXPECT_EQ(" A", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%02s", "A"));
  EXPECT_EQ(" A", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%2s", "AAA"));
  EXPECT_EQ("AAA", std::string(buf));
  EXPECT_EQ(4, SafeSPrintf(buf, "%-2s", "A"));
  EXPECT_EQ("%-2s", std::string(buf));
  SafeSPrintf(fmt, "%%%ds", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, "A"));
  EXPECT_EQ("   ", std::string(buf));
  SafeSPrintf(fmt, "%%0%ds", std::numeric_limits<ssize_t>::max()-1);
  EXPECT_EQ(std::numeric_limits<ssize_t>::max()-1,
            SafeSNPrintf(buf, 4, fmt, "A"));
  EXPECT_EQ("   ", std::string(buf));
  SafeSPrintf(fmt, "%%%ds",
              static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, fmt, "A"));
  EXPECT_EQ("%s", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, fmt, "A"), "padding <= max_padding");
#endif
}

TEST(SafeSPrintfTest, EmbeddedNul) {
  char buf[] = { 'X', 'X', 'X', 'X' };
  EXPECT_EQ(2, SafeSPrintf(buf, "%3c", 0));
  EXPECT_EQ(' ', buf[0]);
  EXPECT_EQ(' ', buf[1]);
  EXPECT_EQ(0,   buf[2]);
  EXPECT_EQ('X', buf[3]);

  
  
  
  
#if defined(NDEBUG)
  EXPECT_EQ(2, SafeSPrintf(buf, "%%%"));
  EXPECT_EQ("%%", std::string(buf));
  EXPECT_EQ(2, SafeSPrintf(buf, "%%%", 0));
  EXPECT_EQ("%%", std::string(buf));
#elif defined(ALLOW_DEATH_TEST)
  EXPECT_DEATH(SafeSPrintf(buf, "%%%"), "src.1. == '%'");
  EXPECT_DEATH(SafeSPrintf(buf, "%%%", 0), "ch");
#endif
}

TEST(SafeSPrintfTest, EmitNULL) {
  char buf[40];
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion-null"
#endif
  EXPECT_EQ(1, SafeSPrintf(buf, "%d", NULL));
  EXPECT_EQ("0", std::string(buf));
  EXPECT_EQ(3, SafeSPrintf(buf, "%p", NULL));
  EXPECT_EQ("0x0", std::string(buf));
  EXPECT_EQ(6, SafeSPrintf(buf, "%s", NULL));
  EXPECT_EQ("<NULL>", std::string(buf));
#if defined(__GCC__)
#pragma GCC diagnostic pop
#endif
}

TEST(SafeSPrintfTest, PointerSize) {
  
  
  
  
  
  char *str = reinterpret_cast<char *>(0x80000000u);
  void *ptr = str;
  char buf[40];
  EXPECT_EQ(10, SafeSPrintf(buf, "%p", str));
  EXPECT_EQ("0x80000000", std::string(buf));
  EXPECT_EQ(10, SafeSPrintf(buf, "%p", ptr));
  EXPECT_EQ("0x80000000", std::string(buf));
}

}  
}  
