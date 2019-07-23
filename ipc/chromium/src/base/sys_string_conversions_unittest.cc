



#include "base/string_piece.h"
#include "base/sys_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

#ifdef WCHAR_T_IS_UTF32
static const std::wstring kSysWideOldItalicLetterA = L"\x10300";
#else
static const std::wstring kSysWideOldItalicLetterA = L"\xd800\xdf00";
#endif

TEST(SysStrings, SysWideToUTF8) {
  using base::SysWideToUTF8;
  EXPECT_EQ("Hello, world", SysWideToUTF8(L"Hello, world"));
  EXPECT_EQ("\xe4\xbd\xa0\xe5\xa5\xbd", SysWideToUTF8(L"\x4f60\x597d"));

  
  EXPECT_EQ("\xF0\x90\x8C\x80", SysWideToUTF8(kSysWideOldItalicLetterA));

  
  
  
  
  
  
  
  

  
  std::wstring wide_null(L"a");
  wide_null.push_back(0);
  wide_null.push_back('b');

  std::string expected_null("a");
  expected_null.push_back(0);
  expected_null.push_back('b');

  EXPECT_EQ(expected_null, SysWideToUTF8(wide_null));
}

TEST(SysStrings, SysUTF8ToWide) {
  using base::SysUTF8ToWide;
  EXPECT_EQ(L"Hello, world", SysUTF8ToWide("Hello, world"));
  EXPECT_EQ(L"\x4f60\x597d", SysUTF8ToWide("\xe4\xbd\xa0\xe5\xa5\xbd"));
  
  EXPECT_EQ(kSysWideOldItalicLetterA, SysUTF8ToWide("\xF0\x90\x8C\x80"));

  
  
  
  
  
  

  
  std::string utf8_null("a");
  utf8_null.push_back(0);
  utf8_null.push_back('b');

  std::wstring expected_null(L"a");
  expected_null.push_back(0);
  expected_null.push_back('b');

  EXPECT_EQ(expected_null, SysUTF8ToWide(utf8_null));
}
