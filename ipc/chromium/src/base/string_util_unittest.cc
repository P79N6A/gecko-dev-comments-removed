



#include <math.h>
#include <stdarg.h>

#include <limits>
#include <sstream>

#include "base/basictypes.h"
#include "base/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
}

static const struct trim_case {
  const wchar_t* input;
  const TrimPositions positions;
  const wchar_t* output;
  const TrimPositions return_value;
} trim_cases[] = {
  {L" Google Video ", TRIM_LEADING, L"Google Video ", TRIM_LEADING},
  {L" Google Video ", TRIM_TRAILING, L" Google Video", TRIM_TRAILING},
  {L" Google Video ", TRIM_ALL, L"Google Video", TRIM_ALL},
  {L"Google Video", TRIM_ALL, L"Google Video", TRIM_NONE},
  {L"", TRIM_ALL, L"", TRIM_NONE},
  {L"  ", TRIM_LEADING, L"", TRIM_LEADING},
  {L"  ", TRIM_TRAILING, L"", TRIM_TRAILING},
  {L"  ", TRIM_ALL, L"", TRIM_ALL},
  {L"\t\rTest String\n", TRIM_ALL, L"Test String", TRIM_ALL},
  {L"\x2002Test String\x00A0\x3000", TRIM_ALL, L"Test String", TRIM_ALL},
};

static const struct trim_case_ascii {
  const char* input;
  const TrimPositions positions;
  const char* output;
  const TrimPositions return_value;
} trim_cases_ascii[] = {
  {" Google Video ", TRIM_LEADING, "Google Video ", TRIM_LEADING},
  {" Google Video ", TRIM_TRAILING, " Google Video", TRIM_TRAILING},
  {" Google Video ", TRIM_ALL, "Google Video", TRIM_ALL},
  {"Google Video", TRIM_ALL, "Google Video", TRIM_NONE},
  {"", TRIM_ALL, "", TRIM_NONE},
  {"  ", TRIM_LEADING, "", TRIM_LEADING},
  {"  ", TRIM_TRAILING, "", TRIM_TRAILING},
  {"  ", TRIM_ALL, "", TRIM_ALL},
  {"\t\rTest String\n", TRIM_ALL, "Test String", TRIM_ALL},
};

TEST(StringUtilTest, TrimWhitespace) {
  std::wstring output;  
  for (size_t i = 0; i < arraysize(trim_cases); ++i) {
    const trim_case& value = trim_cases[i];
    EXPECT_EQ(value.return_value,
              TrimWhitespace(value.input, value.positions, &output));
    EXPECT_EQ(value.output, output);
  }

  
  output = L"  This is a test \r\n";
  EXPECT_EQ(TRIM_ALL, TrimWhitespace(output, TRIM_ALL, &output));
  EXPECT_EQ(L"This is a test", output);

  
  output = L"  \r\n";
  EXPECT_EQ(TRIM_ALL, TrimWhitespace(output, TRIM_ALL, &output));
  EXPECT_EQ(L"", output);

  std::string output_ascii;
  for (size_t i = 0; i < arraysize(trim_cases_ascii); ++i) {
    const trim_case_ascii& value = trim_cases_ascii[i];
    EXPECT_EQ(value.return_value,
              TrimWhitespace(value.input, value.positions, &output_ascii));
    EXPECT_EQ(value.output, output_ascii);
  }
}

static const struct trim_case_utf8 {
  const char* input;
  const TrimPositions positions;
  const char* output;
  const TrimPositions return_value;
} trim_cases_utf8[] = {
  
  
  {"\xE2\x80\x80Test String\xE2\x80\x81", TRIM_ALL, "Test String", TRIM_ALL},
  {"\xE2\x80\x82Test String\xE2\x80\x83", TRIM_ALL, "Test String", TRIM_ALL},
  {"\xE2\x80\x84Test String\xE2\x80\x85", TRIM_ALL, "Test String", TRIM_ALL},
  {"\xE2\x80\x86Test String\xE2\x80\x87", TRIM_ALL, "Test String", TRIM_ALL},
  {"\xE2\x80\x88Test String\xE2\x80\x8A", TRIM_ALL, "Test String", TRIM_ALL},
  {"\xE3\x80\x80Test String\xE3\x80\x80", TRIM_ALL, "Test String", TRIM_ALL},
  
  {"\xD0\x85", TRIM_TRAILING, "\xD0\x85", TRIM_NONE},
  {"\xD9\x85", TRIM_TRAILING, "\xD9\x85", TRIM_NONE},
  {"\xEC\x97\x85", TRIM_TRAILING, "\xEC\x97\x85", TRIM_NONE},
  {"\xF0\x90\x80\x85", TRIM_TRAILING, "\xF0\x90\x80\x85", TRIM_NONE},
  
  {"\xD0\xA0", TRIM_TRAILING, "\xD0\xA0", TRIM_NONE},
  {"\xD9\xA0", TRIM_TRAILING, "\xD9\xA0", TRIM_NONE},
  {"\xEC\x97\xA0", TRIM_TRAILING, "\xEC\x97\xA0", TRIM_NONE},
  {"\xF0\x90\x80\xA0", TRIM_TRAILING, "\xF0\x90\x80\xA0", TRIM_NONE},
};

TEST(StringUtilTest, TrimWhitespaceUTF8) {
  std::string output_ascii;
  for (size_t i = 0; i < arraysize(trim_cases_ascii); ++i) {
    const trim_case_ascii& value = trim_cases_ascii[i];
    EXPECT_EQ(value.return_value,
              TrimWhitespaceASCII(value.input, value.positions, &output_ascii));
    EXPECT_EQ(value.output, output_ascii);
  }

  
  
  std::string output_utf8;
  for (size_t i = 0; i < arraysize(trim_cases_utf8); ++i) {
    const trim_case_utf8& value = trim_cases_utf8[i];
    EXPECT_EQ(value.return_value,
              TrimWhitespaceUTF8(value.input, value.positions, &output_utf8));
    EXPECT_EQ(value.output, output_utf8);
  }
}

static const struct collapse_case {
  const wchar_t* input;
  const bool trim;
  const wchar_t* output;
} collapse_cases[] = {
  {L" Google Video ", false, L"Google Video"},
  {L"Google Video", false, L"Google Video"},
  {L"", false, L""},
  {L"  ", false, L""},
  {L"\t\rTest String\n", false, L"Test String"},
  {L"\x2002Test String\x00A0\x3000", false, L"Test String"},
  {L"    Test     \n  \t String    ", false, L"Test String"},
  {L"\x2002Test\x1680 \x2028 \tString\x00A0\x3000", false, L"Test String"},
  {L"   Test String", false, L"Test String"},
  {L"Test String    ", false, L"Test String"},
  {L"Test String", false, L"Test String"},
  {L"", true, L""},
  {L"\n", true, L""},
  {L"  \r  ", true, L""},
  {L"\nFoo", true, L"Foo"},
  {L"\r  Foo  ", true, L"Foo"},
  {L" Foo bar ", true, L"Foo bar"},
  {L"  \tFoo  bar  \n", true, L"Foo bar"},
  {L" a \r b\n c \r\n d \t\re \t f \n ", true, L"abcde f"},
};

TEST(StringUtilTest, CollapseWhitespace) {
  for (size_t i = 0; i < arraysize(collapse_cases); ++i) {
    const collapse_case& value = collapse_cases[i];
    EXPECT_EQ(value.output, CollapseWhitespace(value.input, value.trim));
  }
}


TEST(StringUtilTest, IsStringUTF8) {
  EXPECT_TRUE(IsStringUTF8("abc"));
  EXPECT_TRUE(IsStringUTF8("\xc2\x81"));
  EXPECT_TRUE(IsStringUTF8("\xe1\x80\xbf"));
  EXPECT_TRUE(IsStringUTF8("\xf1\x80\xa0\xbf"));
  EXPECT_TRUE(IsStringUTF8("a\xc2\x81\xe1\x80\xbf\xf1\x80\xa0\xbf"));
  EXPECT_TRUE(IsStringUTF8("\xef\xbb\xbf" "abc")); 

  
  EXPECT_FALSE(IsStringUTF8("\xed\xa0\x80\xed\xbf\xbf"));
  EXPECT_FALSE(IsStringUTF8("\xed\xa0\x8f"));
  EXPECT_FALSE(IsStringUTF8("\xed\xbf\xbf"));

  
  EXPECT_FALSE(IsStringUTF8("\xc0\x80")); 
  EXPECT_FALSE(IsStringUTF8("\xc1\x80\xc1\x81")); 
  EXPECT_FALSE(IsStringUTF8("\xe0\x80\x80")); 
  EXPECT_FALSE(IsStringUTF8("\xe0\x82\x80")); 
  EXPECT_FALSE(IsStringUTF8("\xe0\x9f\xbf")); 
  EXPECT_FALSE(IsStringUTF8("\xf0\x80\x80\x8D")); 
  EXPECT_FALSE(IsStringUTF8("\xf0\x80\x82\x91")); 
  EXPECT_FALSE(IsStringUTF8("\xf0\x80\xa0\x80")); 
  EXPECT_FALSE(IsStringUTF8("\xf0\x8f\xbb\xbf")); 
  EXPECT_FALSE(IsStringUTF8("\xf8\x80\x80\x80\xbf")); 
  EXPECT_FALSE(IsStringUTF8("\xfc\x80\x80\x80\xa0\xa5")); 

  
  EXPECT_FALSE(IsStringUTF8("\xf4\x90\x80\x80")); 
  EXPECT_FALSE(IsStringUTF8("\xf8\xa0\xbf\x80\xbf")); 
  EXPECT_FALSE(IsStringUTF8("\xfc\x9c\xbf\x80\xbf\x80")); 

  
  EXPECT_FALSE(IsStringUTF8("\xfe\xff"));
  EXPECT_FALSE(IsStringUTF8("\xff\xfe"));
  EXPECT_FALSE(IsStringUTF8(std::string("\x00\x00\xfe\xff", 4)));
  EXPECT_FALSE(IsStringUTF8("\xff\xfe\x00\x00"));

  
  EXPECT_FALSE(IsStringUTF8("\xef\xbf\xbe")); 
  EXPECT_FALSE(IsStringUTF8("\xf0\x8f\xbf\xbe")); 
  EXPECT_FALSE(IsStringUTF8("\xf3\xbf\xbf\xbf")); 

  
  
#if 0
  EXPECT_FALSE(IsStringUTF8("\xef\xb7\x90")); 
  EXPECT_FALSE(IsStringUTF8("\xef\xb7\xaf")); 
#endif

  
  
  
  EXPECT_FALSE(IsStringUTF8("caf\xe9")); 
  EXPECT_FALSE(IsStringUTF8("\xb0\xa1\xb0\xa2")); 
  EXPECT_FALSE(IsStringUTF8("\xa7\x41\xa6\x6e")); 
  
  EXPECT_FALSE(IsStringUTF8("\x93" "abc\x94"));
  
  EXPECT_FALSE(IsStringUTF8("\xd9\xee\xe4\xee"));
  
  EXPECT_FALSE(IsStringUTF8("\xe3\xe5\xe9\xdC"));
}

static const wchar_t* const kConvertRoundtripCases[] = {
  L"Google Video",
  
  L"\x7f51\x9875\x0020\x56fe\x7247\x0020\x8d44\x8baf\x66f4\x591a\x0020\x00bb",
  
  L"\x03a0\x03b1\x03b3\x03ba\x03cc\x03c3\x03bc\x03b9"
  L"\x03bf\x03c2\x0020\x0399\x03c3\x03c4\x03cc\x03c2",
  
  L"\x041f\x043e\x0438\x0441\x043a\x0020\x0441\x0442"
  L"\x0440\x0430\x043d\x0438\x0446\x0020\x043d\x0430"
  L"\x0020\x0440\x0443\x0441\x0441\x043a\x043e\x043c",
  
  L"\xc804\xccb4\xc11c\xbe44\xc2a4",

  
  
#if defined(WCHAR_T_IS_UTF16)
  L"\xd800\xdf00",
  
  L"\xd807\xdd40\xd807\xdd41\xd807\xdd42\xd807\xdd43\xd807\xdd44",
#elif defined(WCHAR_T_IS_UTF32)
  L"\x10300",
  
  L"\x11d40\x11d41\x11d42\x11d43\x11d44",
#endif
};

TEST(StringUtilTest, ConvertUTF8AndWide) {
  
  
  
  for (size_t i = 0; i < arraysize(kConvertRoundtripCases); ++i) {
    std::ostringstream utf8;
    utf8 << WideToUTF8(kConvertRoundtripCases[i]);
    std::wostringstream wide;
    wide << UTF8ToWide(utf8.str());

    EXPECT_EQ(kConvertRoundtripCases[i], wide.str());
  }
}

TEST(StringUtilTest, ConvertUTF8AndWideEmptyString) {
  
  
  std::wstring wempty;
  std::string empty;
  EXPECT_EQ(empty, WideToUTF8(wempty));
  EXPECT_EQ(wempty, UTF8ToWide(empty));
}

TEST(StringUtilTest, ConvertUTF8ToWide) {
  struct UTF8ToWideCase {
    const char* utf8;
    const wchar_t* wide;
    bool success;
  } convert_cases[] = {
    
    {"\xe4\xbd\xa0\xe5\xa5\xbd", L"\x4f60\x597d", true},
    
    {"\xef\xbf\xbfHello", L"Hello", false},
    
    {"\xe4\xa0\xe5\xa5\xbd", L"\x597d", false},
    
    {"\xe5\xa5\xbd\xe4\xa0", L"\x597d", false},
    
    {"\xf0\x84\xbd\xa0\xe5\xa5\xbd", L"\x597d", false},
    
    {"\xed\xb0\x80", L"", false},
    
#if defined(WCHAR_T_IS_UTF16)
    {"A\xF0\x90\x8C\x80z", L"A\xd800\xdf00z", true},
#elif defined(WCHAR_T_IS_UTF32)
    {"A\xF0\x90\x8C\x80z", L"A\x10300z", true},
#endif
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(convert_cases); i++) {
    std::wstring converted;
    EXPECT_EQ(convert_cases[i].success,
              UTF8ToWide(convert_cases[i].utf8,
                         strlen(convert_cases[i].utf8),
                         &converted));
    std::wstring expected(convert_cases[i].wide);
    EXPECT_EQ(expected, converted);
  }

  
  std::wstring converted;
  EXPECT_TRUE(UTF8ToWide("\00Z\t", 3, &converted));
  ASSERT_EQ(3U, converted.length());
#if defined(WCHAR_T_IS_UNSIGNED)
  EXPECT_EQ(0U, converted[0]);
#else
  EXPECT_EQ(0, converted[0]);
#endif
  EXPECT_EQ('Z', converted[1]);
  EXPECT_EQ('\t', converted[2]);

  
  EXPECT_TRUE(UTF8ToWide("B", 1, &converted));
  ASSERT_EQ(1U, converted.length());
  EXPECT_EQ('B', converted[0]);
}

#if defined(WCHAR_T_IS_UTF16)

TEST(StringUtilTest, ConvertUTF16ToUTF8) {
  struct UTF16ToUTF8Case {
    const wchar_t* utf16;
    const char* utf8;
    bool success;
  } convert_cases[] = {
    
    {L"\x4f60\x597d", "\xe4\xbd\xa0\xe5\xa5\xbd", true},
    
    {L"\xd800\xdf00", "\xF0\x90\x8C\x80", true},
    
    {L"\xffffHello", "Hello", false},
    
    {L"\xd800\x597d", "\xe5\xa5\xbd", false},
    
    {L"\x597d\xd800", "\xe5\xa5\xbd", false},
  };

  for (int i = 0; i < arraysize(convert_cases); i++) {
    std::string converted;
    EXPECT_EQ(convert_cases[i].success,
              WideToUTF8(convert_cases[i].utf16,
                         wcslen(convert_cases[i].utf16),
                         &converted));
    std::string expected(convert_cases[i].utf8);
    EXPECT_EQ(expected, converted);
  }
}

#elif defined(WCHAR_T_IS_UTF32)

TEST(StringUtilTest, ConvertUTF32ToUTF8) {
  struct UTF8ToWideCase {
    const wchar_t* utf32;
    const char* utf8;
    bool success;
  } convert_cases[] = {
    
    {L"\x4f60\x597d", "\xe4\xbd\xa0\xe5\xa5\xbd", true},
    
    {L"A\x10300z", "A\xF0\x90\x8C\x80z", true},
    
    {L"\xffffHello", "Hello", false},
    {L"\xfffffffHello", "Hello", false},
    
    {L"\xd800\x597d", "\xe5\xa5\xbd", false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(convert_cases); i++) {
    std::string converted;
    EXPECT_EQ(convert_cases[i].success,
              WideToUTF8(convert_cases[i].utf32,
                         wcslen(convert_cases[i].utf32),
                         &converted));
    std::string expected(convert_cases[i].utf8);
    EXPECT_EQ(expected, converted);
  }
}
#endif  

TEST(StringUtilTest, ConvertMultiString) {
  static wchar_t wmulti[] = {
    L'f', L'o', L'o', L'\0',
    L'b', L'a', L'r', L'\0',
    L'b', L'a', L'z', L'\0',
    L'\0'
  };
  static char multi[] = {
    'f', 'o', 'o', '\0',
    'b', 'a', 'r', '\0',
    'b', 'a', 'z', '\0',
    '\0'
  };
  std::wstring wmultistring;
  memcpy(WriteInto(&wmultistring, arraysize(wmulti)), wmulti, sizeof(wmulti));
  EXPECT_EQ(arraysize(wmulti) - 1, wmultistring.length());
  std::string expected;
  memcpy(WriteInto(&expected, arraysize(multi)), multi, sizeof(multi));
  EXPECT_EQ(arraysize(multi) - 1, expected.length());
  const std::string& converted = WideToUTF8(wmultistring);
  EXPECT_EQ(arraysize(multi) - 1, converted.length());
  EXPECT_EQ(expected, converted);
}

TEST(StringUtilTest, ConvertCodepageUTF8) {
  
  for (size_t i = 0; i < arraysize(kConvertRoundtripCases); ++i) {
    std::string expected(WideToUTF8(kConvertRoundtripCases[i]));
    std::string utf8;
    EXPECT_TRUE(WideToCodepage(kConvertRoundtripCases[i], kCodepageUTF8,
                               OnStringUtilConversionError::SKIP, &utf8));
    EXPECT_EQ(expected, utf8);
  }
}

TEST(StringUtilTest, ConvertBetweenCodepageAndWide) {
  static const struct {
    const char* codepage_name;
    const char* encoded;
    OnStringUtilConversionError::Type on_error;
    bool success;
    const wchar_t* wide;
  } kConvertCodepageCases[] = {
    
    
    {"big5",
     "\xA7\x41\xA6",
     OnStringUtilConversionError::FAIL,
     false,
     L""},
    {"big5",
     "\xA7\x41\xA6",
     OnStringUtilConversionError::SKIP,
     true,
     L"\x4F60"},
    
    {"iso-8859-6",
     "\xC7\xEE\xE4\xD3\xF1\xEE\xE4\xC7\xE5\xEF" " "
     "\xD9\xEE\xE4\xEE\xEA\xF2\xE3\xEF\xE5\xF2",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0627\x064E\x0644\x0633\x0651\x064E\x0644\x0627\x0645\x064F" L" "
     L"\x0639\x064E\x0644\x064E\x064A\x0652\x0643\x064F\x0645\x0652"},
    
    {"gb2312",
     "\xC4\xE3\xBA\xC3",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x4F60\x597D"},
    
    {"big5",
     "\xA7\x41\xA6\x6E",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x4F60\x597D"},
    
    {"iso-8859-7",
     "\xE3\xE5\xE9\xDC" " " "\xF3\xEF\xF5",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5"},
    
    {"windows-1255", 
     "\xF9\xD1\xC8\xEC\xE5\xC9\xED",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x05E9\x05C1\x05B8\x05DC\x05D5\x05B9\x05DD"},
    
    {"iscii-dev",
     "\xEF\x42" "\xC6\xCC\xD7\xE8\xB3\xDA\xCF",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0928\x092E\x0938\x094D\x0915\x093E\x0930"},
    
    {"euc-kr",
     "\xBE\xC8\xB3\xE7\xC7\xCF\xBC\xBC\xBF\xE4",
     OnStringUtilConversionError::FAIL,
     true,
     L"\xC548\xB155\xD558\xC138\xC694"},
    
    {"euc-jp",
     "\xA4\xB3\xA4\xF3\xA4\xCB\xA4\xC1\xA4\xCF",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x3053\x3093\x306B\x3061\x306F"},
    
    {"iso-2022-jp",
     "\x1B\x24\x42" "\x24\x33\x24\x73\x24\x4B\x24\x41\x24\x4F" "\x1B\x28\x42",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x3053\x3093\x306B\x3061\x306F"},
    
    {"sjis",
     "\x82\xB1\x82\xF1\x82\xC9\x82\xBF\x82\xCD",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x3053\x3093\x306B\x3061\x306F"},
    
    {"koi8-r",
     "\xDA\xC4\xD2\xC1\xD7\xD3\xD4\xD7\xD5\xCA\xD4\xC5",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0437\x0434\x0440\x0430\x0432\x0441\x0442\x0432"
     L"\x0443\x0439\x0442\x0435"},
    
    {"windows-874", 
     "\xCA\xC7\xD1\xCA\xB4\xD5" "\xA4\xC3\xD1\xBA",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0E2A\x0E27\x0E31\x0E2A\x0E14\x0E35"
     L"\x0E04\x0E23\x0e31\x0E1A"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kConvertCodepageCases); ++i) {
    std::wstring wide;
    bool success = CodepageToWide(kConvertCodepageCases[i].encoded,
                                  kConvertCodepageCases[i].codepage_name,
                                  kConvertCodepageCases[i].on_error,
                                  &wide);
    EXPECT_EQ(kConvertCodepageCases[i].success, success);
    EXPECT_EQ(kConvertCodepageCases[i].wide, wide);

    
    
    if (success &&
        kConvertCodepageCases[i].on_error ==
            OnStringUtilConversionError::FAIL) {
      std::string encoded;
      success = WideToCodepage(wide, kConvertCodepageCases[i].codepage_name,
                               kConvertCodepageCases[i].on_error, &encoded);
      EXPECT_EQ(kConvertCodepageCases[i].success, success);
      EXPECT_EQ(kConvertCodepageCases[i].encoded, encoded);
    }
  }

  
  
  std::string encoded("Temp data");  

  
  EXPECT_FALSE(WideToCodepage(L"Chinese\xff27", "iso-8859-1",
                              OnStringUtilConversionError::FAIL, &encoded));
  EXPECT_TRUE(encoded.empty());
  EXPECT_TRUE(WideToCodepage(L"Chinese\xff27", "iso-8859-1",
                             OnStringUtilConversionError::SKIP, &encoded));
  EXPECT_STREQ("Chinese", encoded.c_str());

#if defined(WCHAR_T_IS_UTF16)
  
  EXPECT_FALSE(WideToCodepage(L"a\xd800z", "iso-8859-1",
                              OnStringUtilConversionError::FAIL, &encoded));
  EXPECT_TRUE(encoded.empty());
  EXPECT_TRUE(WideToCodepage(L"a\xd800z", "iso-8859-1",
                             OnStringUtilConversionError::SKIP, &encoded));
  EXPECT_STREQ("az", encoded.c_str());
#endif  

  
  EXPECT_TRUE(WideToCodepage(L"a\xffffz", "iso-8859-1",
                             OnStringUtilConversionError::SKIP, &encoded));
  EXPECT_STREQ("az", encoded.c_str());

  
  EXPECT_FALSE(WideToCodepage(L"Hello, world", "awesome-8571-2",
                              OnStringUtilConversionError::SKIP, &encoded));
}

TEST(StringUtilTest, ConvertASCII) {
  static const char* char_cases[] = {
    "Google Video",
    "Hello, world\n",
    "0123ABCDwxyz \a\b\t\r\n!+,.~"
  };

  static const wchar_t* const wchar_cases[] = {
    L"Google Video",
    L"Hello, world\n",
    L"0123ABCDwxyz \a\b\t\r\n!+,.~"
  };

  for (size_t i = 0; i < arraysize(char_cases); ++i) {
    EXPECT_TRUE(IsStringASCII(char_cases[i]));
    std::wstring wide = ASCIIToWide(char_cases[i]);
    EXPECT_EQ(wchar_cases[i], wide);

    EXPECT_TRUE(IsStringASCII(wchar_cases[i]));
    std::string ascii = WideToASCII(wchar_cases[i]);
    EXPECT_EQ(char_cases[i], ascii);
  }

  EXPECT_FALSE(IsStringASCII("Google \x80Video"));
  EXPECT_FALSE(IsStringASCII(L"Google \x80Video"));

  
  std::wstring wempty;
  std::string empty;
  EXPECT_EQ(empty, WideToASCII(wempty));
  EXPECT_EQ(wempty, ASCIIToWide(empty));

  
  const char chars_with_nul[] = "test\0string";
  const int length_with_nul = arraysize(chars_with_nul) - 1;
  std::string string_with_nul(chars_with_nul, length_with_nul);
  std::wstring wide_with_nul = ASCIIToWide(string_with_nul);
  EXPECT_EQ(static_cast<std::wstring::size_type>(length_with_nul),
            wide_with_nul.length());
  std::string narrow_with_nul = WideToASCII(wide_with_nul);
  EXPECT_EQ(static_cast<std::string::size_type>(length_with_nul),
            narrow_with_nul.length());
  EXPECT_EQ(0, string_with_nul.compare(narrow_with_nul));
}

TEST(StringUtilTest, ToUpperASCII) {
  EXPECT_EQ('C', ToUpperASCII('C'));
  EXPECT_EQ('C', ToUpperASCII('c'));
  EXPECT_EQ('2', ToUpperASCII('2'));

  EXPECT_EQ(L'C', ToUpperASCII(L'C'));
  EXPECT_EQ(L'C', ToUpperASCII(L'c'));
  EXPECT_EQ(L'2', ToUpperASCII(L'2'));

  std::string in_place_a("Cc2");
  StringToUpperASCII(&in_place_a);
  EXPECT_EQ("CC2", in_place_a);

  std::wstring in_place_w(L"Cc2");
  StringToUpperASCII(&in_place_w);
  EXPECT_EQ(L"CC2", in_place_w);

  std::string original_a("Cc2");
  std::string upper_a = StringToUpperASCII(original_a);
  EXPECT_EQ("CC2", upper_a);

  std::wstring original_w(L"Cc2");
  std::wstring upper_w = StringToUpperASCII(original_w);
  EXPECT_EQ(L"CC2", upper_w);
}

static const struct {
  const wchar_t* src_w;
  const char*    src_a;
  const char*    dst;
} lowercase_cases[] = {
  {L"FoO", "FoO", "foo"},
  {L"foo", "foo", "foo"},
  {L"FOO", "FOO", "foo"},
};

TEST(StringUtilTest, LowerCaseEqualsASCII) {
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(lowercase_cases); ++i) {
    EXPECT_TRUE(LowerCaseEqualsASCII(lowercase_cases[i].src_w,
                                     lowercase_cases[i].dst));
    EXPECT_TRUE(LowerCaseEqualsASCII(lowercase_cases[i].src_a,
                                     lowercase_cases[i].dst));
  }
}

TEST(StringUtilTest, GetByteDisplayUnits) {
  static const struct {
    int64 bytes;
    DataUnits expected;
  } cases[] = {
    {0, DATA_UNITS_BYTE},
    {512, DATA_UNITS_BYTE},
    {10*1024, DATA_UNITS_KILOBYTE},
    {10*1024*1024, DATA_UNITS_MEGABYTE},
    {10LL*1024*1024*1024, DATA_UNITS_GIGABYTE},
    {~(1LL<<63), DATA_UNITS_GIGABYTE},
#ifdef NDEBUG
    {-1, DATA_UNITS_BYTE},
#endif
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i)
    EXPECT_EQ(cases[i].expected, GetByteDisplayUnits(cases[i].bytes));
}

TEST(StringUtilTest, FormatBytes) {
  static const struct {
    int64 bytes;
    DataUnits units;
    const wchar_t* expected;
    const wchar_t* expected_with_units;
  } cases[] = {
    {0, DATA_UNITS_BYTE, L"0", L"0 B"},
    {512, DATA_UNITS_BYTE, L"512", L"512 B"},
    {512, DATA_UNITS_KILOBYTE, L"0.5", L"0.5 kB"},
    {1024*1024, DATA_UNITS_KILOBYTE, L"1024", L"1024 kB"},
    {1024*1024, DATA_UNITS_MEGABYTE, L"1", L"1 MB"},
    {1024*1024*1024, DATA_UNITS_GIGABYTE, L"1", L"1 GB"},
    {10LL*1024*1024*1024, DATA_UNITS_GIGABYTE, L"10", L"10 GB"},
    {~(1LL<<63), DATA_UNITS_GIGABYTE, L"8589934592", L"8589934592 GB"},
    
    {1024*1024 + 103, DATA_UNITS_KILOBYTE, L"1024.1", L"1024.1 kB"},
    {1024*1024 + 205 * 1024, DATA_UNITS_MEGABYTE, L"1.2", L"1.2 MB"},
    {1024*1024*1024 + (927 * 1024*1024), DATA_UNITS_GIGABYTE,
     L"1.9", L"1.9 GB"},
    {10LL*1024*1024*1024, DATA_UNITS_GIGABYTE, L"10", L"10 GB"},
#ifdef NDEBUG
    {-1, DATA_UNITS_BYTE, L"", L""},
#endif
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].expected,
              FormatBytes(cases[i].bytes, cases[i].units, false));
    EXPECT_EQ(cases[i].expected_with_units,
              FormatBytes(cases[i].bytes, cases[i].units, true));
  }
}

TEST(StringUtilTest, ReplaceSubstringsAfterOffset) {
  static const struct {
    const char* str;
    string16::size_type start_offset;
    const char* find_this;
    const char* replace_with;
    const char* expected;
  } cases[] = {
    {"aaa", 0, "a", "b", "bbb"},
    {"abb", 0, "ab", "a", "ab"},
    {"Removing some substrings inging", 0, "ing", "", "Remov some substrs "},
    {"Not found", 0, "x", "0", "Not found"},
    {"Not found again", 5, "x", "0", "Not found again"},
    {" Making it much longer ", 0, " ", "Four score and seven years ago",
     "Four score and seven years agoMakingFour score and seven years agoit"
     "Four score and seven years agomuchFour score and seven years agolonger"
     "Four score and seven years ago"},
    {"Invalid offset", 9999, "t", "foobar", "Invalid offset"},
    {"Replace me only me once", 9, "me ", "", "Replace me only once"},
    {"abababab", 2, "ab", "c", "abccc"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    string16 str = ASCIIToUTF16(cases[i].str);
    ReplaceSubstringsAfterOffset(&str, cases[i].start_offset,
                                 ASCIIToUTF16(cases[i].find_this),
                                 ASCIIToUTF16(cases[i].replace_with));
    EXPECT_EQ(ASCIIToUTF16(cases[i].expected), str);
  }
}

TEST(StringUtilTest, ReplaceFirstSubstringAfterOffset) {
  static const struct {
    const char* str;
    string16::size_type start_offset;
    const char* find_this;
    const char* replace_with;
    const char* expected;
  } cases[] = {
    {"aaa", 0, "a", "b", "baa"},
    {"abb", 0, "ab", "a", "ab"},
    {"Removing some substrings inging", 0, "ing", "",
      "Remov some substrings inging"},
    {"Not found", 0, "x", "0", "Not found"},
    {"Not found again", 5, "x", "0", "Not found again"},
    {" Making it much longer ", 0, " ", "Four score and seven years ago",
     "Four score and seven years agoMaking it much longer "},
    {"Invalid offset", 9999, "t", "foobar", "Invalid offset"},
    {"Replace me only me once", 4, "me ", "", "Replace only me once"},
    {"abababab", 2, "ab", "c", "abcabab"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    string16 str = ASCIIToUTF16(cases[i].str);
    ReplaceFirstSubstringAfterOffset(&str, cases[i].start_offset,
                                     ASCIIToUTF16(cases[i].find_this),
                                     ASCIIToUTF16(cases[i].replace_with));
    EXPECT_EQ(ASCIIToUTF16(cases[i].expected), str);
  }
}

namespace {

template <typename INT>
struct IntToStringTest {
  INT num;
  const char* sexpected;
  const char* uexpected;
};

}

TEST(StringUtilTest, IntToString) {

  static const IntToStringTest<int> int_tests[] = {
      { 0, "0", "0" },
      { -1, "-1", "4294967295" },
      { std::numeric_limits<int>::max(), "2147483647", "2147483647" },
      { std::numeric_limits<int>::min(), "-2147483648", "2147483648" },
  };
  static const IntToStringTest<int64> int64_tests[] = {
      { 0, "0", "0" },
      { -1, "-1", "18446744073709551615" },
      { std::numeric_limits<int64>::max(),
        "9223372036854775807",
        "9223372036854775807", },
      { std::numeric_limits<int64>::min(),
        "-9223372036854775808",
        "9223372036854775808" },
  };

  for (size_t i = 0; i < arraysize(int_tests); ++i) {
    const IntToStringTest<int>* test = &int_tests[i];
    EXPECT_EQ(IntToString(test->num), test->sexpected);
    EXPECT_EQ(IntToWString(test->num), UTF8ToWide(test->sexpected));
    EXPECT_EQ(UintToString(test->num), test->uexpected);
    EXPECT_EQ(UintToWString(test->num), UTF8ToWide(test->uexpected));
  }
  for (size_t i = 0; i < arraysize(int64_tests); ++i) {
    const IntToStringTest<int64>* test = &int64_tests[i];
    EXPECT_EQ(Int64ToString(test->num), test->sexpected);
    EXPECT_EQ(Int64ToWString(test->num), UTF8ToWide(test->sexpected));
    EXPECT_EQ(Uint64ToString(test->num), test->uexpected);
    EXPECT_EQ(Uint64ToWString(test->num), UTF8ToWide(test->uexpected));
  }
}

TEST(StringUtilTest, Uint64ToString) {
  static const struct {
    uint64 input;
    std::string output;
  } cases[] = {
    {0, "0"},
    {42, "42"},
    {INT_MAX, "2147483647"},
    {kuint64max, "18446744073709551615"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i)
    EXPECT_EQ(cases[i].output, Uint64ToString(cases[i].input));
}

TEST(StringUtilTest, StringToInt) {
  static const struct {
    std::string input;
    int output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 42, true},
    {"-2147483648", INT_MIN, true},
    {"2147483647", INT_MAX, true},
    {"", 0, false},
    {" 42", 42, false},
    {"42 ", 42, false},
    {"\t\n\v\f\r 42", 42, false},
    {"blah42", 0, false},
    {"42blah", 42, false},
    {"blah42blah", 0, false},
    {"-273.15", -273, false},
    {"+98.6", 98, false},
    {"--123", 0, false},
    {"++123", 0, false},
    {"-+123", 0, false},
    {"+-123", 0, false},
    {"-", 0, false},
    {"-2147483649", INT_MIN, false},
    {"-99999999999", INT_MIN, false},
    {"2147483648", INT_MAX, false},
    {"99999999999", INT_MAX, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].output, StringToInt(cases[i].input));
    int output;
    EXPECT_EQ(cases[i].success, StringToInt(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].output, StringToInt(WideToUTF16Hack(wide_input)));
    EXPECT_EQ(cases[i].success, StringToInt(WideToUTF16Hack(wide_input),
                                            &output));
    EXPECT_EQ(cases[i].output, output);
  }

  
  
  
  const char input[] = "6\06";
  std::string input_string(input, arraysize(input) - 1);
  int output;
  EXPECT_FALSE(StringToInt(input_string, &output));
  EXPECT_EQ(6, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(StringToInt(WideToUTF16Hack(wide_input), &output));
  EXPECT_EQ(6, output);
}

TEST(StringUtilTest, StringToInt64) {
  static const struct {
    std::string input;
    int64 output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 42, true},
    {"-2147483648", INT_MIN, true},
    {"2147483647", INT_MAX, true},
    {"-2147483649", GG_INT64_C(-2147483649), true},
    {"-99999999999", GG_INT64_C(-99999999999), true},
    {"2147483648", GG_INT64_C(2147483648), true},
    {"99999999999", GG_INT64_C(99999999999), true},
    {"9223372036854775807", kint64max, true},
    {"-9223372036854775808", kint64min, true},
    {"09", 9, true},
    {"-09", -9, true},
    {"", 0, false},
    {" 42", 42, false},
    {"42 ", 42, false},
    {"\t\n\v\f\r 42", 42, false},
    {"blah42", 0, false},
    {"42blah", 42, false},
    {"blah42blah", 0, false},
    {"-273.15", -273, false},
    {"+98.6", 98, false},
    {"--123", 0, false},
    {"++123", 0, false},
    {"-+123", 0, false},
    {"+-123", 0, false},
    {"-", 0, false},
    {"-9223372036854775809", kint64min, false},
    {"-99999999999999999999", kint64min, false},
    {"9223372036854775808", kint64max, false},
    {"99999999999999999999", kint64max, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].output, StringToInt64(cases[i].input));
    int64 output;
    EXPECT_EQ(cases[i].success, StringToInt64(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].output, StringToInt64(WideToUTF16Hack(wide_input)));
    EXPECT_EQ(cases[i].success, StringToInt64(WideToUTF16Hack(wide_input),
                                              &output));
    EXPECT_EQ(cases[i].output, output);
  }

  
  
  
  const char input[] = "6\06";
  std::string input_string(input, arraysize(input) - 1);
  int64 output;
  EXPECT_FALSE(StringToInt64(input_string, &output));
  EXPECT_EQ(6, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(StringToInt64(WideToUTF16Hack(wide_input), &output));
  EXPECT_EQ(6, output);
}

TEST(StringUtilTest, HexStringToInt) {
  static const struct {
    std::string input;
    int output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 66, true},
    {"-42", -66, true},
    {"+42", 66, true},
    {"7fffffff", INT_MAX, true},
    {"80000000", INT_MIN, true},
    {"ffffffff", -1, true},
    {"DeadBeef", 0xdeadbeef, true},
    {"0x42", 66, true},
    {"-0x42", -66, true},
    {"+0x42", 66, true},
    {"0x7fffffff", INT_MAX, true},
    {"0x80000000", INT_MIN, true},
    {"0xffffffff", -1, true},
    {"0XDeadBeef", 0xdeadbeef, true},
    {"0x0f", 15, true},
    {"0f", 15, true},
    {" 45", 0x45, false},
    {"\t\n\v\f\r 0x45", 0x45, false},
    {" 45", 0x45, false},
    {"45 ", 0x45, false},
    {"efgh", 0xef, false},
    {"0xefgh", 0xef, false},
    {"hgfe", 0, false},
    {"100000000", -1, false},  
    {"-", 0, false},
    {"", 0, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].output, HexStringToInt(cases[i].input));
    int output;
    EXPECT_EQ(cases[i].success, HexStringToInt(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].output, HexStringToInt(WideToUTF16Hack(wide_input)));
    EXPECT_EQ(cases[i].success, HexStringToInt(WideToUTF16Hack(wide_input),
                                               &output));
    EXPECT_EQ(cases[i].output, output);
  }
  
  
  
  const char input[] = "0xc0ffee\09";
  std::string input_string(input, arraysize(input) - 1);
  int output;
  EXPECT_FALSE(HexStringToInt(input_string, &output));
  EXPECT_EQ(0xc0ffee, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(HexStringToInt(WideToUTF16Hack(wide_input), &output));
  EXPECT_EQ(0xc0ffee, output);
}

TEST(StringUtilTest, HexStringToBytes) {
  static const struct {
    const std::string input;
    const char* output;
    size_t output_len;
    bool success;
  } cases[] = {
    {"0", "", 0, false},  
    {"00", "\0", 1, true},
    {"42", "\x42", 1, true},
    {"-42", "", 0, false},  
    {"+42", "", 0, false},
    {"7fffffff", "\x7f\xff\xff\xff", 4, true},
    {"80000000", "\x80\0\0\0", 4, true},
    {"deadbeef", "\xde\xad\xbe\xef", 4, true},
    {"DeadBeef", "\xde\xad\xbe\xef", 4, true},
    {"0x42", "", 0, false},  
    {"0f", "\xf", 1, true},
    {"45  ", "\x45", 1, false},
    {"efgh", "\xef", 1, false},
    {"", "", 0, false},
    {"0123456789ABCDEF", "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 8, true},
    {"0123456789ABCDEF012345",
     "\x01\x23\x45\x67\x89\xAB\xCD\xEF\x01\x23\x45", 11, true},
  };


  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    std::vector<uint8> output;
    std::vector<uint8> compare;
    EXPECT_EQ(cases[i].success, HexStringToBytes(cases[i].input, &output)) <<
        i << ": " << cases[i].input;
    for (size_t j = 0; j < cases[i].output_len; ++j)
      compare.push_back(static_cast<uint8>(cases[i].output[j]));
    ASSERT_EQ(output.size(), compare.size()) << i << ": " << cases[i].input;
    EXPECT_TRUE(std::equal(output.begin(), output.end(), compare.begin())) <<
        i << ": " << cases[i].input;

    output.clear();
    compare.clear();

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].success,
              HexStringToBytes(WideToUTF16Hack(wide_input), &output)) <<
        i << ": " << cases[i].input;
    for (size_t j = 0; j < cases[i].output_len; ++j)
      compare.push_back(static_cast<uint8>(cases[i].output[j]));
    ASSERT_EQ(output.size(), compare.size()) << i << ": " << cases[i].input;
    EXPECT_TRUE(std::equal(output.begin(), output.end(), compare.begin())) <<
        i << ": " << cases[i].input;
  }
}

TEST(StringUtilTest, StringToDouble) {
  static const struct {
    std::string input;
    double output;
    bool success;
  } cases[] = {
    {"0", 0.0, true},
    {"42", 42.0, true},
    {"-42", -42.0, true},
    {"123.45", 123.45, true},
    {"-123.45", -123.45, true},
    {"+123.45", 123.45, true},
    {"2.99792458e8", 299792458.0, true},
    {"149597870.691E+3", 149597870691.0, true},
    {"6.", 6.0, true},
    {"9e99999999999999999999", HUGE_VAL, false},
    {"-9e99999999999999999999", -HUGE_VAL, false},
    {"1e-2", 0.01, true},
    {" 1e-2", 0.01, false},
    {"1e-2 ", 0.01, false},
    {"-1E-7", -0.0000001, true},
    {"01e02", 100, true},
    {"2.3e15", 2.3e15, true},
    {"\t\n\v\f\r -123.45e2", -12345.0, false},
    {"+123 e4", 123.0, false},
    {"123e ", 123.0, false},
    {"123e", 123.0, false},
    {" 2.99", 2.99, false},
    {"1e3.4", 1000.0, false},
    {"nothing", 0.0, false},
    {"-", 0.0, false},
    {"+", 0.0, false},
    {"", 0.0, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_DOUBLE_EQ(cases[i].output, StringToDouble(cases[i].input));
    double output;
    EXPECT_EQ(cases[i].success, StringToDouble(cases[i].input, &output));
    EXPECT_DOUBLE_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_DOUBLE_EQ(cases[i].output,
                     StringToDouble(WideToUTF16Hack(wide_input)));
    EXPECT_EQ(cases[i].success, StringToDouble(WideToUTF16Hack(wide_input),
                                               &output));
    EXPECT_DOUBLE_EQ(cases[i].output, output);
  }

  
  
  
  const char input[] = "3.14\0159";
  std::string input_string(input, arraysize(input) - 1);
  double output;
  EXPECT_FALSE(StringToDouble(input_string, &output));
  EXPECT_DOUBLE_EQ(3.14, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(StringToDouble(WideToUTF16Hack(wide_input), &output));
  EXPECT_DOUBLE_EQ(3.14, output);
}





static void VariableArgsFunc(const char* format, ...) {
  va_list org;
  va_start(org, format);

  va_list dup;
  base::va_copy(dup, org);
  int i1 = va_arg(org, int);
  int j1 = va_arg(org, int);
  char* s1 = va_arg(org, char*);
  double d1 = va_arg(org, double);
  va_end(org);

  int i2 = va_arg(dup, int);
  int j2 = va_arg(dup, int);
  char* s2 = va_arg(dup, char*);
  double d2 = va_arg(dup, double);

  EXPECT_EQ(i1, i2);
  EXPECT_EQ(j1, j2);
  EXPECT_STREQ(s1, s2);
  EXPECT_EQ(d1, d2);

  va_end(dup);
}

TEST(StringUtilTest, VAList) {
  VariableArgsFunc("%d %d %s %lf", 45, 92, "This is interesting", 9.21);
}

TEST(StringUtilTest, StringPrintfEmptyFormat) {
  const char* empty = "";
  EXPECT_EQ("", StringPrintf(empty));
  EXPECT_EQ("", StringPrintf("%s", ""));
}

TEST(StringUtilTest, StringPrintfMisc) {
  EXPECT_EQ("123hello w", StringPrintf("%3d%2s %1c", 123, "hello", 'w'));
  EXPECT_EQ(L"123hello w", StringPrintf(L"%3d%2ls %1lc", 123, L"hello", 'w'));
}

TEST(StringUtilTest, StringAppendfStringEmptyParam) {
  std::string value("Hello");
  StringAppendF(&value, "");
  EXPECT_EQ("Hello", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L"");
  EXPECT_EQ(L"Hello", valuew);
}

TEST(StringUtilTest, StringAppendfEmptyString) {
  std::string value("Hello");
  StringAppendF(&value, "%s", "");
  EXPECT_EQ("Hello", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L"%ls", L"");
  EXPECT_EQ(L"Hello", valuew);
}

TEST(StringUtilTest, StringAppendfString) {
  std::string value("Hello");
  StringAppendF(&value, " %s", "World");
  EXPECT_EQ("Hello World", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L" %ls", L"World");
  EXPECT_EQ(L"Hello World", valuew);
}

TEST(StringUtilTest, StringAppendfInt) {
  std::string value("Hello");
  StringAppendF(&value, " %d", 123);
  EXPECT_EQ("Hello 123", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L" %d", 123);
  EXPECT_EQ(L"Hello 123", valuew);
}



TEST(StringUtilTest, StringPrintfBounds) {
  const int src_len = 1026;
  char src[src_len];
  for (size_t i = 0; i < arraysize(src); i++)
    src[i] = 'A';

  wchar_t srcw[src_len];
  for (size_t i = 0; i < arraysize(srcw); i++)
    srcw[i] = 'A';

  for (int i = 1; i < 3; i++) {
    src[src_len - i] = 0;
    std::string out;
    SStringPrintf(&out, "%s", src);
    EXPECT_STREQ(src, out.c_str());

    srcw[src_len - i] = 0;
    std::wstring outw;
    SStringPrintf(&outw, L"%ls", srcw);
    EXPECT_STREQ(srcw, outw.c_str());
  }
}


TEST(StringUtilTest, Grow) {
  char src[1026];
  for (size_t i = 0; i < arraysize(src); i++)
    src[i] = 'A';
  src[1025] = 0;

  const char* fmt = "%sB%sB%sB%sB%sB%sB%s";

  std::string out;
  SStringPrintf(&out, fmt, src, src, src, src, src, src, src);

  char* ref = new char[320000];
#if defined(OS_WIN)
  sprintf_s(ref, 320000, fmt, src, src, src, src, src, src, src);
#elif defined(OS_POSIX)
  snprintf(ref, 320000, fmt, src, src, src, src, src, src, src);
#endif

  EXPECT_STREQ(ref, out.c_str());
  delete[] ref;
}



TEST(StringUtilTest, GrowBoundary) {
  const int string_util_buf_len = 1024;
  
  
  const int buf_len = string_util_buf_len + 1;
  char src[buf_len + 1];  
  for (int i = 0; i < buf_len; ++i)
    src[i] = 'a';
  src[buf_len] = 0;

  std::string out;
  SStringPrintf(&out, "%s", src);

  EXPECT_STREQ(src, out.c_str());
}


#if defined(OS_WIN)


TEST(StringUtilTest, Invalid) {
  wchar_t invalid[2];
  invalid[0] = 0xffff;
  invalid[1] = 0;

  std::wstring out;
  SStringPrintf(&out, L"%ls", invalid);
  EXPECT_STREQ(L"", out.c_str());
}
#endif


TEST(StringUtilTest, SplitString) {
  std::vector<std::wstring> r;

  SplitString(L"a,b,c", L',', &r);
  EXPECT_EQ(3U, r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"b");
  EXPECT_EQ(r[2], L"c");
  r.clear();

  SplitString(L"a, b, c", L',', &r);
  EXPECT_EQ(3U, r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"b");
  EXPECT_EQ(r[2], L"c");
  r.clear();

  SplitString(L"a,,c", L',', &r);
  EXPECT_EQ(3U, r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"");
  EXPECT_EQ(r[2], L"c");
  r.clear();

  SplitString(L"", L'*', &r);
  EXPECT_EQ(1U, r.size());
  EXPECT_EQ(r[0], L"");
  r.clear();

  SplitString(L"foo", L'*', &r);
  EXPECT_EQ(1U, r.size());
  EXPECT_EQ(r[0], L"foo");
  r.clear();

  SplitString(L"foo ,", L',', &r);
  EXPECT_EQ(2U, r.size());
  EXPECT_EQ(r[0], L"foo");
  EXPECT_EQ(r[1], L"");
  r.clear();

  SplitString(L",", L',', &r);
  EXPECT_EQ(2U, r.size());
  EXPECT_EQ(r[0], L"");
  EXPECT_EQ(r[1], L"");
  r.clear();

  SplitString(L"\t\ta\t", L'\t', &r);
  EXPECT_EQ(4U, r.size());
  EXPECT_EQ(r[0], L"");
  EXPECT_EQ(r[1], L"");
  EXPECT_EQ(r[2], L"a");
  EXPECT_EQ(r[3], L"");
  r.clear();

  SplitStringDontTrim(L"\t\ta\t", L'\t', &r);
  EXPECT_EQ(4U, r.size());
  EXPECT_EQ(r[0], L"");
  EXPECT_EQ(r[1], L"");
  EXPECT_EQ(r[2], L"a");
  EXPECT_EQ(r[3], L"");
  r.clear();

  SplitString(L"\ta\t\nb\tcc", L'\n', &r);
  EXPECT_EQ(2U, r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"b\tcc");
  r.clear();

  SplitStringDontTrim(L"\ta\t\nb\tcc", L'\n', &r);
  EXPECT_EQ(2U, r.size());
  EXPECT_EQ(r[0], L"\ta\t");
  EXPECT_EQ(r[1], L"b\tcc");
  r.clear();
}


TEST(StringUtilTest, JoinString) {
  std::vector<std::string> in;
  EXPECT_EQ("", JoinString(in, ','));

  in.push_back("a");
  EXPECT_EQ("a", JoinString(in, ','));

  in.push_back("b");
  in.push_back("c");
  EXPECT_EQ("a,b,c", JoinString(in, ','));

  in.push_back("");
  EXPECT_EQ("a,b,c,", JoinString(in, ','));
  in.push_back(" ");
  EXPECT_EQ("a|b|c|| ", JoinString(in, '|'));
}

TEST(StringUtilTest, StartsWith) {
  EXPECT_TRUE(StartsWithASCII("javascript:url", "javascript", true));
  EXPECT_FALSE(StartsWithASCII("JavaScript:url", "javascript", true));
  EXPECT_TRUE(StartsWithASCII("javascript:url", "javascript", false));
  EXPECT_TRUE(StartsWithASCII("JavaScript:url", "javascript", false));
  EXPECT_FALSE(StartsWithASCII("java", "javascript", true));
  EXPECT_FALSE(StartsWithASCII("java", "javascript", false));
  EXPECT_FALSE(StartsWithASCII("", "javascript", false));
  EXPECT_FALSE(StartsWithASCII("", "javascript", true));
  EXPECT_TRUE(StartsWithASCII("java", "", false));
  EXPECT_TRUE(StartsWithASCII("java", "", true));

  EXPECT_TRUE(StartsWith(L"javascript:url", L"javascript", true));
  EXPECT_FALSE(StartsWith(L"JavaScript:url", L"javascript", true));
  EXPECT_TRUE(StartsWith(L"javascript:url", L"javascript", false));
  EXPECT_TRUE(StartsWith(L"JavaScript:url", L"javascript", false));
  EXPECT_FALSE(StartsWith(L"java", L"javascript", true));
  EXPECT_FALSE(StartsWith(L"java", L"javascript", false));
  EXPECT_FALSE(StartsWith(L"", L"javascript", false));
  EXPECT_FALSE(StartsWith(L"", L"javascript", true));
  EXPECT_TRUE(StartsWith(L"java", L"", false));
  EXPECT_TRUE(StartsWith(L"java", L"", true));
}

TEST(StringUtilTest, GetStringFWithOffsets) {
  std::vector<size_t> offsets;

  ReplaceStringPlaceholders(ASCIIToUTF16("Hello, $1. Your number is $2."),
                            ASCIIToUTF16("1"),
                            ASCIIToUTF16("2"),
                            &offsets);
  EXPECT_EQ(2U, offsets.size());
  EXPECT_EQ(7U, offsets[0]);
  EXPECT_EQ(25U, offsets[1]);
  offsets.clear();

  ReplaceStringPlaceholders(ASCIIToUTF16("Hello, $2. Your number is $1."),
                            ASCIIToUTF16("1"),
                            ASCIIToUTF16("2"),
                            &offsets);
  EXPECT_EQ(2U, offsets.size());
  EXPECT_EQ(25U, offsets[0]);
  EXPECT_EQ(7U, offsets[1]);
  offsets.clear();
}

TEST(StringUtilTest, SplitStringAlongWhitespace) {
  struct TestData {
    const std::wstring input;
    const size_t expected_result_count;
    const std::wstring output1;
    const std::wstring output2;
  } data[] = {
    { L"a",       1, L"a",  L""   },
    { L" ",       0, L"",   L""   },
    { L" a",      1, L"a",  L""   },
    { L" ab ",    1, L"ab", L""   },
    { L" ab c",   2, L"ab", L"c"  },
    { L" ab c ",  2, L"ab", L"c"  },
    { L" ab cd",  2, L"ab", L"cd" },
    { L" ab cd ", 2, L"ab", L"cd" },
    { L" \ta\t",  1, L"a",  L""   },
    { L" b\ta\t", 2, L"b",  L"a"  },
    { L" b\tat",  2, L"b",  L"at" },
    { L"b\tat",   2, L"b",  L"at" },
    { L"b\t at",  2, L"b",  L"at" },
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(data); ++i) {
    std::vector<std::wstring> results;
    SplitStringAlongWhitespace(data[i].input, &results);
    ASSERT_EQ(data[i].expected_result_count, results.size());
    if (data[i].expected_result_count > 0)
      ASSERT_EQ(data[i].output1, results[0]);
    if (data[i].expected_result_count > 1)
      ASSERT_EQ(data[i].output2, results[1]);
  }
}

TEST(StringUtilTest, MatchPatternTest) {
  EXPECT_EQ(MatchPattern(L"www.google.com", L"*.com"), true);
  EXPECT_EQ(MatchPattern(L"www.google.com", L"*"), true);
  EXPECT_EQ(MatchPattern(L"www.google.com", L"www*.g*.org"), false);
  EXPECT_EQ(MatchPattern(L"Hello", L"H?l?o"), true);
  EXPECT_EQ(MatchPattern(L"www.google.com", L"http://*)"), false);
  EXPECT_EQ(MatchPattern(L"www.msn.com", L"*.COM"), false);
  EXPECT_EQ(MatchPattern(L"Hello*1234", L"He??o\\*1*"), true);
  EXPECT_EQ(MatchPattern(L"", L"*.*"), false);
  EXPECT_EQ(MatchPattern(L"", L"*"), true);
  EXPECT_EQ(MatchPattern(L"", L"?"), true);
  EXPECT_EQ(MatchPattern(L"", L""), true);
  EXPECT_EQ(MatchPattern(L"Hello", L""), false);
  EXPECT_EQ(MatchPattern(L"Hello*", L"Hello*"), true);
  EXPECT_EQ(MatchPattern("Hello*", "Hello*"), true);  
}

TEST(StringUtilTest, LcpyTest) {
  
  {
    char dst[10];
    wchar_t wdst[10];
    EXPECT_EQ(7U, base::strlcpy(dst, "abcdefg", arraysize(dst)));
    EXPECT_EQ(0, memcmp(dst, "abcdefg", 8));
    EXPECT_EQ(7U, base::wcslcpy(wdst, L"abcdefg", arraysize(wdst)));
    EXPECT_EQ(0, memcmp(wdst, L"abcdefg", sizeof(wchar_t) * 8));
  }

  
  
  {
    char dst[2] = {1, 2};
    wchar_t wdst[2] = {1, 2};
    EXPECT_EQ(7U, base::strlcpy(dst, "abcdefg", 0));
    EXPECT_EQ(1, dst[0]);
    EXPECT_EQ(2, dst[1]);
    EXPECT_EQ(7U, base::wcslcpy(wdst, L"abcdefg", 0));
#if defined(WCHAR_T_IS_UNSIGNED)
    EXPECT_EQ(1U, wdst[0]);
    EXPECT_EQ(2U, wdst[1]);
#else
    EXPECT_EQ(1, wdst[0]);
    EXPECT_EQ(2, wdst[1]);
#endif
  }

  
  {
    char dst[8];
    wchar_t wdst[8];
    EXPECT_EQ(7U, base::strlcpy(dst, "abcdefg", arraysize(dst)));
    EXPECT_EQ(0, memcmp(dst, "abcdefg", 8));
    EXPECT_EQ(7U, base::wcslcpy(wdst, L"abcdefg", arraysize(wdst)));
    EXPECT_EQ(0, memcmp(wdst, L"abcdefg", sizeof(wchar_t) * 8));
  }

  
  {
    char dst[7];
    wchar_t wdst[7];
    EXPECT_EQ(7U, base::strlcpy(dst, "abcdefg", arraysize(dst)));
    EXPECT_EQ(0, memcmp(dst, "abcdef", 7));
    EXPECT_EQ(7U, base::wcslcpy(wdst, L"abcdefg", arraysize(wdst)));
    EXPECT_EQ(0, memcmp(wdst, L"abcdef", sizeof(wchar_t) * 7));
  }

  
  {
    char dst[3];
    wchar_t wdst[3];
    EXPECT_EQ(7U, base::strlcpy(dst, "abcdefg", arraysize(dst)));
    EXPECT_EQ(0, memcmp(dst, "ab", 3));
    EXPECT_EQ(7U, base::wcslcpy(wdst, L"abcdefg", arraysize(wdst)));
    EXPECT_EQ(0, memcmp(wdst, L"ab", sizeof(wchar_t) * 3));
  }
}

TEST(StringUtilTest, WprintfFormatPortabilityTest) {
  struct TestData {
    const wchar_t* input;
    bool portable;
  } cases[] = {
    { L"%ls", true },
    { L"%s", false },
    { L"%S", false },
    { L"%lS", false },
    { L"Hello, %s", false },
    { L"%lc", true },
    { L"%c", false },
    { L"%C", false },
    { L"%lC", false },
    { L"%ls %s", false },
    { L"%s %ls", false },
    { L"%s %ls %s", false },
    { L"%f", true },
    { L"%f %F", false },
    { L"%d %D", false },
    { L"%o %O", false },
    { L"%u %U", false },
    { L"%f %d %o %u", true },
    { L"%-8d (%02.1f%)", true },
    { L"% 10s", false },
    { L"% 10ls", true }
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].portable, base::IsWprintfFormatPortable(cases[i].input));
  }
}

TEST(StringUtilTest, ElideString) {
  struct TestData {
    const wchar_t* input;
    int max_len;
    bool result;
    const wchar_t* output;
  } cases[] = {
    { L"Hello", 0, true, L"" },
    { L"", 0, false, L"" },
    { L"Hello, my name is Tom", 1, true, L"H" },
    { L"Hello, my name is Tom", 2, true, L"He" },
    { L"Hello, my name is Tom", 3, true, L"H.m" },
    { L"Hello, my name is Tom", 4, true, L"H..m" },
    { L"Hello, my name is Tom", 5, true, L"H...m" },
    { L"Hello, my name is Tom", 6, true, L"He...m" },
    { L"Hello, my name is Tom", 7, true, L"He...om" },
    { L"Hello, my name is Tom", 10, true, L"Hell...Tom" },
    { L"Hello, my name is Tom", 100, false, L"Hello, my name is Tom" }
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    std::wstring output;
    EXPECT_EQ(cases[i].result,
              ElideString(cases[i].input, cases[i].max_len, &output));
    EXPECT_TRUE(output == cases[i].output);
  }
}

TEST(StringUtilTest, HexEncode) {
  std::string hex(HexEncode(NULL, 0));
  EXPECT_EQ(hex.length(), 0U);
  unsigned char bytes[] = {0x01, 0xff, 0x02, 0xfe, 0x03, 0x80, 0x81};
  hex = HexEncode(bytes, sizeof(bytes));
  EXPECT_EQ(hex.compare("01FF02FE038081"), 0);
}
