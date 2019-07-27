


































#include "gtest/gtest-printers.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"


#if _MSC_VER
# define GTEST_HAS_HASH_MAP_ 1  // Indicates that hash_map is available.
# include <hash_map>            
# define GTEST_HAS_HASH_SET_ 1  // Indicates that hash_set is available.
# include <hash_set>            
#endif  




enum AnonymousEnum {
  kAE1 = -1,
  kAE2 = 1
};


enum EnumWithoutPrinter {
  kEWP1 = -2,
  kEWP2 = 42
};


enum EnumWithStreaming {
  kEWS1 = 10
};

std::ostream& operator<<(std::ostream& os, EnumWithStreaming e) {
  return os << (e == kEWS1 ? "kEWS1" : "invalid");
}


enum EnumWithPrintTo {
  kEWPT1 = 1
};

void PrintTo(EnumWithPrintTo e, std::ostream* os) {
  *os << (e == kEWPT1 ? "kEWPT1" : "invalid");
}


class BiggestIntConvertible {
 public:
  operator ::testing::internal::BiggestInt() const { return 42; }
};


template <typename T>
class UnprintableTemplateInGlobal {
 public:
  UnprintableTemplateInGlobal() : value_() {}
 private:
  T value_;
};


class StreamableInGlobal {
 public:
  virtual ~StreamableInGlobal() {}
};

inline void operator<<(::std::ostream& os, const StreamableInGlobal& ) {
  os << "StreamableInGlobal";
}

void operator<<(::std::ostream& os, const StreamableInGlobal* ) {
  os << "StreamableInGlobal*";
}

namespace foo {


class UnprintableInFoo {
 public:
  UnprintableInFoo() : z_(0) { memcpy(xy_, "\xEF\x12\x0\x0\x34\xAB\x0\x0", 8); }
  double z() const { return z_; }
 private:
  char xy_[8];
  double z_;
};


struct PrintableViaPrintTo {
  PrintableViaPrintTo() : value() {}
  int value;
};

void PrintTo(const PrintableViaPrintTo& x, ::std::ostream* os) {
  *os << "PrintableViaPrintTo: " << x.value;
}


struct PointerPrintable {
};

::std::ostream& operator<<(::std::ostream& os,
                           const PointerPrintable* ) {
  return os << "PointerPrintable*";
}


template <typename T>
class PrintableViaPrintToTemplate {
 public:
  explicit PrintableViaPrintToTemplate(const T& a_value) : value_(a_value) {}

  const T& value() const { return value_; }
 private:
  T value_;
};

template <typename T>
void PrintTo(const PrintableViaPrintToTemplate<T>& x, ::std::ostream* os) {
  *os << "PrintableViaPrintToTemplate: " << x.value();
}


template <typename T>
class StreamableTemplateInFoo {
 public:
  StreamableTemplateInFoo() : value_() {}

  const T& value() const { return value_; }
 private:
  T value_;
};

template <typename T>
inline ::std::ostream& operator<<(::std::ostream& os,
                                  const StreamableTemplateInFoo<T>& x) {
  return os << "StreamableTemplateInFoo: " << x.value();
}

}  

namespace testing {
namespace gtest_printers_test {

using ::std::deque;
using ::std::list;
using ::std::make_pair;
using ::std::map;
using ::std::multimap;
using ::std::multiset;
using ::std::pair;
using ::std::set;
using ::std::vector;
using ::testing::PrintToString;
using ::testing::internal::FormatForComparisonFailureMessage;
using ::testing::internal::ImplicitCast_;
using ::testing::internal::NativeArray;
using ::testing::internal::RE;
using ::testing::internal::RelationToSourceReference;
using ::testing::internal::Strings;
using ::testing::internal::UniversalPrint;
using ::testing::internal::UniversalPrinter;
using ::testing::internal::UniversalTersePrint;
using ::testing::internal::UniversalTersePrintTupleFieldsToStrings;
using ::testing::internal::string;




#ifdef _STLP_HASH_MAP  
using ::std::hash_map;
using ::std::hash_set;
using ::std::hash_multimap;
using ::std::hash_multiset;
#elif _MSC_VER
using ::stdext::hash_map;
using ::stdext::hash_set;
using ::stdext::hash_multimap;
using ::stdext::hash_multiset;
#endif



template <typename T>
string Print(const T& value) {
  ::std::stringstream ss;
  UniversalPrinter<T>::Print(value, &ss);
  return ss.str();
}




template <typename T>
string PrintByRef(const T& value) {
  ::std::stringstream ss;
  UniversalPrinter<T&>::Print(value, &ss);
  return ss.str();
}



TEST(PrintEnumTest, AnonymousEnum) {
  EXPECT_EQ("-1", Print(kAE1));
  EXPECT_EQ("1", Print(kAE2));
}

TEST(PrintEnumTest, EnumWithoutPrinter) {
  EXPECT_EQ("-2", Print(kEWP1));
  EXPECT_EQ("42", Print(kEWP2));
}

TEST(PrintEnumTest, EnumWithStreaming) {
  EXPECT_EQ("kEWS1", Print(kEWS1));
  EXPECT_EQ("invalid", Print(static_cast<EnumWithStreaming>(0)));
}

TEST(PrintEnumTest, EnumWithPrintTo) {
  EXPECT_EQ("kEWPT1", Print(kEWPT1));
  EXPECT_EQ("invalid", Print(static_cast<EnumWithPrintTo>(0)));
}



TEST(PrintClassTest, BiggestIntConvertible) {
  EXPECT_EQ("42", Print(BiggestIntConvertible()));
}




TEST(PrintCharTest, PlainChar) {
  EXPECT_EQ("'\\0'", Print('\0'));
  EXPECT_EQ("'\\'' (39, 0x27)", Print('\''));
  EXPECT_EQ("'\"' (34, 0x22)", Print('"'));
  EXPECT_EQ("'?' (63, 0x3F)", Print('?'));
  EXPECT_EQ("'\\\\' (92, 0x5C)", Print('\\'));
  EXPECT_EQ("'\\a' (7)", Print('\a'));
  EXPECT_EQ("'\\b' (8)", Print('\b'));
  EXPECT_EQ("'\\f' (12, 0xC)", Print('\f'));
  EXPECT_EQ("'\\n' (10, 0xA)", Print('\n'));
  EXPECT_EQ("'\\r' (13, 0xD)", Print('\r'));
  EXPECT_EQ("'\\t' (9)", Print('\t'));
  EXPECT_EQ("'\\v' (11, 0xB)", Print('\v'));
  EXPECT_EQ("'\\x7F' (127)", Print('\x7F'));
  EXPECT_EQ("'\\xFF' (255)", Print('\xFF'));
  EXPECT_EQ("' ' (32, 0x20)", Print(' '));
  EXPECT_EQ("'a' (97, 0x61)", Print('a'));
}


TEST(PrintCharTest, SignedChar) {
  EXPECT_EQ("'\\0'", Print(static_cast<signed char>('\0')));
  EXPECT_EQ("'\\xCE' (-50)",
            Print(static_cast<signed char>(-50)));
}


TEST(PrintCharTest, UnsignedChar) {
  EXPECT_EQ("'\\0'", Print(static_cast<unsigned char>('\0')));
  EXPECT_EQ("'b' (98, 0x62)",
            Print(static_cast<unsigned char>('b')));
}




TEST(PrintBuiltInTypeTest, Bool) {
  EXPECT_EQ("false", Print(false));
  EXPECT_EQ("true", Print(true));
}


TEST(PrintBuiltInTypeTest, Wchar_t) {
  EXPECT_EQ("L'\\0'", Print(L'\0'));
  EXPECT_EQ("L'\\'' (39, 0x27)", Print(L'\''));
  EXPECT_EQ("L'\"' (34, 0x22)", Print(L'"'));
  EXPECT_EQ("L'?' (63, 0x3F)", Print(L'?'));
  EXPECT_EQ("L'\\\\' (92, 0x5C)", Print(L'\\'));
  EXPECT_EQ("L'\\a' (7)", Print(L'\a'));
  EXPECT_EQ("L'\\b' (8)", Print(L'\b'));
  EXPECT_EQ("L'\\f' (12, 0xC)", Print(L'\f'));
  EXPECT_EQ("L'\\n' (10, 0xA)", Print(L'\n'));
  EXPECT_EQ("L'\\r' (13, 0xD)", Print(L'\r'));
  EXPECT_EQ("L'\\t' (9)", Print(L'\t'));
  EXPECT_EQ("L'\\v' (11, 0xB)", Print(L'\v'));
  EXPECT_EQ("L'\\x7F' (127)", Print(L'\x7F'));
  EXPECT_EQ("L'\\xFF' (255)", Print(L'\xFF'));
  EXPECT_EQ("L' ' (32, 0x20)", Print(L' '));
  EXPECT_EQ("L'a' (97, 0x61)", Print(L'a'));
  EXPECT_EQ("L'\\x576' (1398)", Print(static_cast<wchar_t>(0x576)));
  EXPECT_EQ("L'\\xC74D' (51021)", Print(static_cast<wchar_t>(0xC74D)));
}


TEST(PrintTypeSizeTest, Wchar_t) {
  EXPECT_LT(sizeof(wchar_t), sizeof(testing::internal::Int64));
}


TEST(PrintBuiltInTypeTest, Integer) {
  EXPECT_EQ("'\\xFF' (255)", Print(static_cast<unsigned char>(255)));  
  EXPECT_EQ("'\\x80' (-128)", Print(static_cast<signed char>(-128)));  
  EXPECT_EQ("65535", Print(USHRT_MAX));  
  EXPECT_EQ("-32768", Print(SHRT_MIN));  
  EXPECT_EQ("4294967295", Print(UINT_MAX));  
  EXPECT_EQ("-2147483648", Print(INT_MIN));  
  EXPECT_EQ("18446744073709551615",
            Print(static_cast<testing::internal::UInt64>(-1)));  
  EXPECT_EQ("-9223372036854775808",
            Print(static_cast<testing::internal::Int64>(1) << 63));  
}


TEST(PrintBuiltInTypeTest, Size_t) {
  EXPECT_EQ("1", Print(sizeof('a')));  
#if !GTEST_OS_WINDOWS
  
  EXPECT_EQ("-2", Print(static_cast<ssize_t>(-2)));  
#endif  
}


TEST(PrintBuiltInTypeTest, FloatingPoints) {
  EXPECT_EQ("1.5", Print(1.5f));   
  EXPECT_EQ("-2.5", Print(-2.5));  
}




static string PrintPointer(const void *p) {
  ::std::stringstream expected_result_stream;
  expected_result_stream << p;
  return expected_result_stream.str();
}




TEST(PrintCStringTest, Const) {
  const char* p = "World";
  EXPECT_EQ(PrintPointer(p) + " pointing to \"World\"", Print(p));
}


TEST(PrintCStringTest, NonConst) {
  char p[] = "Hi";
  EXPECT_EQ(PrintPointer(p) + " pointing to \"Hi\"",
            Print(static_cast<char*>(p)));
}


TEST(PrintCStringTest, Null) {
  const char* p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintCStringTest, EscapesProperly) {
  const char* p = "'\"?\\\a\b\f\n\r\t\v\x7F\xFF a";
  EXPECT_EQ(PrintPointer(p) + " pointing to \"'\\\"?\\\\\\a\\b\\f"
            "\\n\\r\\t\\v\\x7F\\xFF a\"",
            Print(p));
}







#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)


TEST(PrintWideCStringTest, Const) {
  const wchar_t* p = L"World";
  EXPECT_EQ(PrintPointer(p) + " pointing to L\"World\"", Print(p));
}


TEST(PrintWideCStringTest, NonConst) {
  wchar_t p[] = L"Hi";
  EXPECT_EQ(PrintPointer(p) + " pointing to L\"Hi\"",
            Print(static_cast<wchar_t*>(p)));
}


TEST(PrintWideCStringTest, Null) {
  const wchar_t* p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintWideCStringTest, EscapesProperly) {
  const wchar_t s[] = {'\'', '"', '?', '\\', '\a', '\b', '\f', '\n', '\r',
                       '\t', '\v', 0xD3, 0x576, 0x8D3, 0xC74D, ' ', 'a', '\0'};
  EXPECT_EQ(PrintPointer(s) + " pointing to L\"'\\\"?\\\\\\a\\b\\f"
            "\\n\\r\\t\\v\\xD3\\x576\\x8D3\\xC74D a\"",
            Print(static_cast<const wchar_t*>(s)));
}
#endif  




TEST(PrintCharPointerTest, SignedChar) {
  signed char* p = reinterpret_cast<signed char*>(0x1234);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintCharPointerTest, ConstSignedChar) {
  signed char* p = reinterpret_cast<signed char*>(0x1234);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintCharPointerTest, UnsignedChar) {
  unsigned char* p = reinterpret_cast<unsigned char*>(0x1234);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintCharPointerTest, ConstUnsignedChar) {
  const unsigned char* p = reinterpret_cast<const unsigned char*>(0x1234);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}




TEST(PrintPointerToBuiltInTypeTest, Bool) {
  bool* p = reinterpret_cast<bool*>(0xABCD);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintPointerToBuiltInTypeTest, Void) {
  void* p = reinterpret_cast<void*>(0xABCD);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintPointerToBuiltInTypeTest, ConstVoid) {
  const void* p = reinterpret_cast<const void*>(0xABCD);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}


TEST(PrintPointerToPointerTest, IntPointerPointer) {
  int** p = reinterpret_cast<int**>(0xABCD);
  EXPECT_EQ(PrintPointer(p), Print(p));
  p = NULL;
  EXPECT_EQ("NULL", Print(p));
}



void MyFunction(int ) {}

TEST(PrintPointerTest, NonMemberFunctionPointer) {
  
  
  
  
  EXPECT_EQ(
      PrintPointer(reinterpret_cast<const void*>(
          reinterpret_cast<internal::BiggestInt>(&MyFunction))),
      Print(&MyFunction));
  int (*p)(bool) = NULL;  
  EXPECT_EQ("NULL", Print(p));
}



template <typename StringType>
AssertionResult HasPrefix(const StringType& str, const StringType& prefix) {
  if (str.find(prefix, 0) == 0)
    return AssertionSuccess();

  const bool is_wide_string = sizeof(prefix[0]) > 1;
  const char* const begin_string_quote = is_wide_string ? "L\"" : "\"";
  return AssertionFailure()
      << begin_string_quote << prefix << "\" is not a prefix of "
      << begin_string_quote << str << "\"\n";
}






struct Foo {
 public:
  virtual ~Foo() {}
  int MyMethod(char x) { return x + 1; }
  virtual char MyVirtualMethod(int ) { return 'a'; }

  int value;
};

TEST(PrintPointerTest, MemberVariablePointer) {
  EXPECT_TRUE(HasPrefix(Print(&Foo::value),
                        Print(sizeof(&Foo::value)) + "-byte object "));
  int (Foo::*p) = NULL;  
  EXPECT_TRUE(HasPrefix(Print(p),
                        Print(sizeof(p)) + "-byte object "));
}





TEST(PrintPointerTest, MemberFunctionPointer) {
  EXPECT_TRUE(HasPrefix(Print(&Foo::MyMethod),
                        Print(sizeof(&Foo::MyMethod)) + "-byte object "));
  EXPECT_TRUE(
      HasPrefix(Print(&Foo::MyVirtualMethod),
                Print(sizeof((&Foo::MyVirtualMethod))) + "-byte object "));
  int (Foo::*p)(char) = NULL;  
  EXPECT_TRUE(HasPrefix(Print(p),
                        Print(sizeof(p)) + "-byte object "));
}





template <typename T, size_t N>
string PrintArrayHelper(T (&a)[N]) {
  return Print(a);
}


TEST(PrintArrayTest, OneDimensionalArray) {
  int a[5] = { 1, 2, 3, 4, 5 };
  EXPECT_EQ("{ 1, 2, 3, 4, 5 }", PrintArrayHelper(a));
}


TEST(PrintArrayTest, TwoDimensionalArray) {
  int a[2][5] = {
    { 1, 2, 3, 4, 5 },
    { 6, 7, 8, 9, 0 }
  };
  EXPECT_EQ("{ { 1, 2, 3, 4, 5 }, { 6, 7, 8, 9, 0 } }", PrintArrayHelper(a));
}


TEST(PrintArrayTest, ConstArray) {
  const bool a[1] = { false };
  EXPECT_EQ("{ false }", PrintArrayHelper(a));
}


TEST(PrintArrayTest, CharArrayWithNoTerminatingNul) {
  
  char a[] = { 'H', '\0', 'i' };
  EXPECT_EQ("\"H\\0i\" (no terminating NUL)", PrintArrayHelper(a));
}


TEST(PrintArrayTest, ConstCharArrayWithTerminatingNul) {
  const char a[] = "\0Hi";
  EXPECT_EQ("\"\\0Hi\"", PrintArrayHelper(a));
}


TEST(PrintArrayTest, WCharArrayWithNoTerminatingNul) {
  
  const wchar_t a[] = { L'H', L'\0', L'i' };
  EXPECT_EQ("L\"H\\0i\" (no terminating NUL)", PrintArrayHelper(a));
}


TEST(PrintArrayTest, WConstCharArrayWithTerminatingNul) {
  const wchar_t a[] = L"\0Hi";
  EXPECT_EQ("L\"\\0Hi\"", PrintArrayHelper(a));
}


TEST(PrintArrayTest, ObjectArray) {
  string a[3] = { "Hi", "Hello", "Ni hao" };
  EXPECT_EQ("{ \"Hi\", \"Hello\", \"Ni hao\" }", PrintArrayHelper(a));
}


TEST(PrintArrayTest, BigArray) {
  int a[100] = { 1, 2, 3 };
  EXPECT_EQ("{ 1, 2, 3, 0, 0, 0, 0, 0, ..., 0, 0, 0, 0, 0, 0, 0, 0 }",
            PrintArrayHelper(a));
}



#if GTEST_HAS_GLOBAL_STRING

TEST(PrintStringTest, StringInGlobalNamespace) {
  const char s[] = "'\"?\\\a\b\f\n\0\r\t\v\x7F\xFF a";
  const ::string str(s, sizeof(s));
  EXPECT_EQ("\"'\\\"?\\\\\\a\\b\\f\\n\\0\\r\\t\\v\\x7F\\xFF a\\0\"",
            Print(str));
}
#endif  


TEST(PrintStringTest, StringInStdNamespace) {
  const char s[] = "'\"?\\\a\b\f\n\0\r\t\v\x7F\xFF a";
  const ::std::string str(s, sizeof(s));
  EXPECT_EQ("\"'\\\"?\\\\\\a\\b\\f\\n\\0\\r\\t\\v\\x7F\\xFF a\\0\"",
            Print(str));
}

TEST(PrintStringTest, StringAmbiguousHex) {
  
  

  
  EXPECT_EQ("\"0\\x12\" \"3\"", Print(::std::string("0\x12" "3")));
  
  EXPECT_EQ("\"mm\\x6\" \"bananas\"", Print(::std::string("mm\x6" "bananas")));
  
  EXPECT_EQ("\"NOM\\x6\" \"BANANA\"", Print(::std::string("NOM\x6" "BANANA")));
  
  EXPECT_EQ("\"!\\x5-!\"", Print(::std::string("!\x5-!")));
}



#if GTEST_HAS_GLOBAL_WSTRING

TEST(PrintWideStringTest, StringInGlobalNamespace) {
  const wchar_t s[] = L"'\"?\\\a\b\f\n\0\r\t\v\xD3\x576\x8D3\xC74D a";
  const ::wstring str(s, sizeof(s)/sizeof(wchar_t));
  EXPECT_EQ("L\"'\\\"?\\\\\\a\\b\\f\\n\\0\\r\\t\\v"
            "\\xD3\\x576\\x8D3\\xC74D a\\0\"",
            Print(str));
}
#endif  

#if GTEST_HAS_STD_WSTRING

TEST(PrintWideStringTest, StringInStdNamespace) {
  const wchar_t s[] = L"'\"?\\\a\b\f\n\0\r\t\v\xD3\x576\x8D3\xC74D a";
  const ::std::wstring str(s, sizeof(s)/sizeof(wchar_t));
  EXPECT_EQ("L\"'\\\"?\\\\\\a\\b\\f\\n\\0\\r\\t\\v"
            "\\xD3\\x576\\x8D3\\xC74D a\\0\"",
            Print(str));
}

TEST(PrintWideStringTest, StringAmbiguousHex) {
  
  EXPECT_EQ("L\"0\\x12\" L\"3\"", Print(::std::wstring(L"0\x12" L"3")));
  EXPECT_EQ("L\"mm\\x6\" L\"bananas\"",
            Print(::std::wstring(L"mm\x6" L"bananas")));
  EXPECT_EQ("L\"NOM\\x6\" L\"BANANA\"",
            Print(::std::wstring(L"NOM\x6" L"BANANA")));
  EXPECT_EQ("L\"!\\x5-!\"", Print(::std::wstring(L"!\x5-!")));
}
#endif  // GTEST_HAS_STD_WSTRING







class AllowsGenericStreaming {};

template <typename Char, typename CharTraits>
std::basic_ostream<Char, CharTraits>& operator<<(
    std::basic_ostream<Char, CharTraits>& os,
    const AllowsGenericStreaming& ) {
  return os << "AllowsGenericStreaming";
}

TEST(PrintTypeWithGenericStreamingTest, NonTemplateType) {
  AllowsGenericStreaming a;
  EXPECT_EQ("AllowsGenericStreaming", Print(a));
}



template <typename T>
class AllowsGenericStreamingTemplate {};

template <typename Char, typename CharTraits, typename T>
std::basic_ostream<Char, CharTraits>& operator<<(
    std::basic_ostream<Char, CharTraits>& os,
    const AllowsGenericStreamingTemplate<T>& ) {
  return os << "AllowsGenericStreamingTemplate";
}

TEST(PrintTypeWithGenericStreamingTest, TemplateType) {
  AllowsGenericStreamingTemplate<int> a;
  EXPECT_EQ("AllowsGenericStreamingTemplate", Print(a));
}




template <typename T>
class AllowsGenericStreamingAndImplicitConversionTemplate {
 public:
  operator bool() const { return false; }
};

template <typename Char, typename CharTraits, typename T>
std::basic_ostream<Char, CharTraits>& operator<<(
    std::basic_ostream<Char, CharTraits>& os,
    const AllowsGenericStreamingAndImplicitConversionTemplate<T>& ) {
  return os << "AllowsGenericStreamingAndImplicitConversionTemplate";
}

TEST(PrintTypeWithGenericStreamingTest, TypeImplicitlyConvertible) {
  AllowsGenericStreamingAndImplicitConversionTemplate<int> a;
  EXPECT_EQ("AllowsGenericStreamingAndImplicitConversionTemplate", Print(a));
}

#if GTEST_HAS_STRING_PIECE_



TEST(PrintStringPieceTest, SimpleStringPiece) {
  const StringPiece sp = "Hello";
  EXPECT_EQ("\"Hello\"", Print(sp));
}

TEST(PrintStringPieceTest, UnprintableCharacters) {
  const char str[] = "NUL (\0) and \r\t";
  const StringPiece sp(str, sizeof(str) - 1);
  EXPECT_EQ("\"NUL (\\0) and \\r\\t\"", Print(sp));
}

#endif  



TEST(PrintStlContainerTest, EmptyDeque) {
  deque<char> empty;
  EXPECT_EQ("{}", Print(empty));
}

TEST(PrintStlContainerTest, NonEmptyDeque) {
  deque<int> non_empty;
  non_empty.push_back(1);
  non_empty.push_back(3);
  EXPECT_EQ("{ 1, 3 }", Print(non_empty));
}

#if GTEST_HAS_HASH_MAP_

TEST(PrintStlContainerTest, OneElementHashMap) {
  hash_map<int, char> map1;
  map1[1] = 'a';
  EXPECT_EQ("{ (1, 'a' (97, 0x61)) }", Print(map1));
}

TEST(PrintStlContainerTest, HashMultiMap) {
  hash_multimap<int, bool> map1;
  map1.insert(make_pair(5, true));
  map1.insert(make_pair(5, false));

  
  const string result = Print(map1);
  EXPECT_TRUE(result == "{ (5, true), (5, false) }" ||
              result == "{ (5, false), (5, true) }")
                  << " where Print(map1) returns \"" << result << "\".";
}

#endif  

#if GTEST_HAS_HASH_SET_

TEST(PrintStlContainerTest, HashSet) {
  hash_set<string> set1;
  set1.insert("hello");
  EXPECT_EQ("{ \"hello\" }", Print(set1));
}

TEST(PrintStlContainerTest, HashMultiSet) {
  const int kSize = 5;
  int a[kSize] = { 1, 1, 2, 5, 1 };
  hash_multiset<int> set1(a, a + kSize);

  
  const string result = Print(set1);
  const string expected_pattern = "{ d, d, d, d, d }";  

  
  
  ASSERT_EQ(expected_pattern.length(), result.length());
  std::vector<int> numbers;
  for (size_t i = 0; i != result.length(); i++) {
    if (expected_pattern[i] == 'd') {
      ASSERT_NE(isdigit(static_cast<unsigned char>(result[i])), 0);
      numbers.push_back(result[i] - '0');
    } else {
      EXPECT_EQ(expected_pattern[i], result[i]) << " where result is "
                                                << result;
    }
  }

  
  std::sort(numbers.begin(), numbers.end());
  std::sort(a, a + kSize);
  EXPECT_TRUE(std::equal(a, a + kSize, numbers.begin()));
}

#endif  

TEST(PrintStlContainerTest, List) {
  const string a[] = {
    "hello",
    "world"
  };
  const list<string> strings(a, a + 2);
  EXPECT_EQ("{ \"hello\", \"world\" }", Print(strings));
}

TEST(PrintStlContainerTest, Map) {
  map<int, bool> map1;
  map1[1] = true;
  map1[5] = false;
  map1[3] = true;
  EXPECT_EQ("{ (1, true), (3, true), (5, false) }", Print(map1));
}

TEST(PrintStlContainerTest, MultiMap) {
  multimap<bool, int> map1;
  
  
  
  
  
  
  map1.insert(pair<const bool, int>(true, 0));
  map1.insert(pair<const bool, int>(true, 1));
  map1.insert(pair<const bool, int>(false, 2));
  EXPECT_EQ("{ (false, 2), (true, 0), (true, 1) }", Print(map1));
}

TEST(PrintStlContainerTest, Set) {
  const unsigned int a[] = { 3, 0, 5 };
  set<unsigned int> set1(a, a + 3);
  EXPECT_EQ("{ 0, 3, 5 }", Print(set1));
}

TEST(PrintStlContainerTest, MultiSet) {
  const int a[] = { 1, 1, 2, 5, 1 };
  multiset<int> set1(a, a + 5);
  EXPECT_EQ("{ 1, 1, 1, 2, 5 }", Print(set1));
}

TEST(PrintStlContainerTest, Pair) {
  pair<const bool, int> p(true, 5);
  EXPECT_EQ("(true, 5)", Print(p));
}

TEST(PrintStlContainerTest, Vector) {
  vector<int> v;
  v.push_back(1);
  v.push_back(2);
  EXPECT_EQ("{ 1, 2 }", Print(v));
}

TEST(PrintStlContainerTest, LongSequence) {
  const int a[100] = { 1, 2, 3 };
  const vector<int> v(a, a + 100);
  EXPECT_EQ("{ 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "
            "0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ... }", Print(v));
}

TEST(PrintStlContainerTest, NestedContainer) {
  const int a1[] = { 1, 2 };
  const int a2[] = { 3, 4, 5 };
  const list<int> l1(a1, a1 + 2);
  const list<int> l2(a2, a2 + 3);

  vector<list<int> > v;
  v.push_back(l1);
  v.push_back(l2);
  EXPECT_EQ("{ { 1, 2 }, { 3, 4, 5 } }", Print(v));
}

TEST(PrintStlContainerTest, OneDimensionalNativeArray) {
  const int a[3] = { 1, 2, 3 };
  NativeArray<int> b(a, 3, RelationToSourceReference());
  EXPECT_EQ("{ 1, 2, 3 }", Print(b));
}

TEST(PrintStlContainerTest, TwoDimensionalNativeArray) {
  const int a[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
  NativeArray<int[3]> b(a, 2, RelationToSourceReference());
  EXPECT_EQ("{ { 1, 2, 3 }, { 4, 5, 6 } }", Print(b));
}



struct iterator {
  char x;
};

TEST(PrintStlContainerTest, Iterator) {
  iterator it = {};
  EXPECT_EQ("1-byte object <00>", Print(it));
}



struct const_iterator {
  char x;
};

TEST(PrintStlContainerTest, ConstIterator) {
  const_iterator it = {};
  EXPECT_EQ("1-byte object <00>", Print(it));
}

#if GTEST_HAS_TR1_TUPLE



TEST(PrintTr1TupleTest, VariousSizes) {
  ::std::tr1::tuple<> t0;
  EXPECT_EQ("()", Print(t0));

  ::std::tr1::tuple<int> t1(5);
  EXPECT_EQ("(5)", Print(t1));

  ::std::tr1::tuple<char, bool> t2('a', true);
  EXPECT_EQ("('a' (97, 0x61), true)", Print(t2));

  ::std::tr1::tuple<bool, int, int> t3(false, 2, 3);
  EXPECT_EQ("(false, 2, 3)", Print(t3));

  ::std::tr1::tuple<bool, int, int, int> t4(false, 2, 3, 4);
  EXPECT_EQ("(false, 2, 3, 4)", Print(t4));

  ::std::tr1::tuple<bool, int, int, int, bool> t5(false, 2, 3, 4, true);
  EXPECT_EQ("(false, 2, 3, 4, true)", Print(t5));

  ::std::tr1::tuple<bool, int, int, int, bool, int> t6(false, 2, 3, 4, true, 6);
  EXPECT_EQ("(false, 2, 3, 4, true, 6)", Print(t6));

  ::std::tr1::tuple<bool, int, int, int, bool, int, int> t7(
      false, 2, 3, 4, true, 6, 7);
  EXPECT_EQ("(false, 2, 3, 4, true, 6, 7)", Print(t7));

  ::std::tr1::tuple<bool, int, int, int, bool, int, int, bool> t8(
      false, 2, 3, 4, true, 6, 7, true);
  EXPECT_EQ("(false, 2, 3, 4, true, 6, 7, true)", Print(t8));

  ::std::tr1::tuple<bool, int, int, int, bool, int, int, bool, int> t9(
      false, 2, 3, 4, true, 6, 7, true, 9);
  EXPECT_EQ("(false, 2, 3, 4, true, 6, 7, true, 9)", Print(t9));

  const char* const str = "8";
  
  
  ::std::tr1::tuple<bool, char, short, testing::internal::Int32,  
      testing::internal::Int64, float, double, const char*, void*, string>
      t10(false, 'a', 3, 4, 5, 1.5F, -2.5, str,
          ImplicitCast_<void*>(NULL), "10");
  EXPECT_EQ("(false, 'a' (97, 0x61), 3, 4, 5, 1.5, -2.5, " + PrintPointer(str) +
            " pointing to \"8\", NULL, \"10\")",
            Print(t10));
}


TEST(PrintTr1TupleTest, NestedTuple) {
  ::std::tr1::tuple< ::std::tr1::tuple<int, bool>, char> nested(
      ::std::tr1::make_tuple(5, true), 'a');
  EXPECT_EQ("((5, true), 'a' (97, 0x61))", Print(nested));
}

#endif  

#if GTEST_LANG_CXX11



TEST(PrintStdTupleTest, VariousSizes) {
  ::std::tuple<> t0;
  EXPECT_EQ("()", Print(t0));

  ::std::tuple<int> t1(5);
  EXPECT_EQ("(5)", Print(t1));

  ::std::tuple<char, bool> t2('a', true);
  EXPECT_EQ("('a' (97, 0x61), true)", Print(t2));

  ::std::tuple<bool, int, int> t3(false, 2, 3);
  EXPECT_EQ("(false, 2, 3)", Print(t3));

  ::std::tuple<bool, int, int, int> t4(false, 2, 3, 4);
  EXPECT_EQ("(false, 2, 3, 4)", Print(t4));

  ::std::tuple<bool, int, int, int, bool> t5(false, 2, 3, 4, true);
  EXPECT_EQ("(false, 2, 3, 4, true)", Print(t5));

  ::std::tuple<bool, int, int, int, bool, int> t6(false, 2, 3, 4, true, 6);
  EXPECT_EQ("(false, 2, 3, 4, true, 6)", Print(t6));

  ::std::tuple<bool, int, int, int, bool, int, int> t7(
      false, 2, 3, 4, true, 6, 7);
  EXPECT_EQ("(false, 2, 3, 4, true, 6, 7)", Print(t7));

  ::std::tuple<bool, int, int, int, bool, int, int, bool> t8(
      false, 2, 3, 4, true, 6, 7, true);
  EXPECT_EQ("(false, 2, 3, 4, true, 6, 7, true)", Print(t8));

  ::std::tuple<bool, int, int, int, bool, int, int, bool, int> t9(
      false, 2, 3, 4, true, 6, 7, true, 9);
  EXPECT_EQ("(false, 2, 3, 4, true, 6, 7, true, 9)", Print(t9));

  const char* const str = "8";
  
  
  ::std::tuple<bool, char, short, testing::internal::Int32,  
      testing::internal::Int64, float, double, const char*, void*, string>
      t10(false, 'a', 3, 4, 5, 1.5F, -2.5, str,
          ImplicitCast_<void*>(NULL), "10");
  EXPECT_EQ("(false, 'a' (97, 0x61), 3, 4, 5, 1.5, -2.5, " + PrintPointer(str) +
            " pointing to \"8\", NULL, \"10\")",
            Print(t10));
}


TEST(PrintStdTupleTest, NestedTuple) {
  ::std::tuple< ::std::tuple<int, bool>, char> nested(
      ::std::make_tuple(5, true), 'a');
  EXPECT_EQ("((5, true), 'a' (97, 0x61))", Print(nested));
}

#endif




TEST(PrintUnprintableTypeTest, InGlobalNamespace) {
  EXPECT_EQ("1-byte object <00>",
            Print(UnprintableTemplateInGlobal<char>()));
}


TEST(PrintUnprintableTypeTest, InUserNamespace) {
  EXPECT_EQ("16-byte object <EF-12 00-00 34-AB 00-00 00-00 00-00 00-00 00-00>",
            Print(::foo::UnprintableInFoo()));
}



struct Big {
  Big() { memset(array, 0, sizeof(array)); }
  char array[257];
};

TEST(PrintUnpritableTypeTest, BigObject) {
  EXPECT_EQ("257-byte object <00-00 00-00 00-00 00-00 00-00 00-00 "
            "00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 "
            "00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 "
            "00-00 00-00 00-00 00-00 00-00 00-00 ... 00-00 00-00 00-00 "
            "00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 "
            "00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 "
            "00-00 00-00 00-00 00-00 00-00 00-00 00-00 00-00 00>",
            Print(Big()));
}




TEST(PrintStreamableTypeTest, InGlobalNamespace) {
  StreamableInGlobal x;
  EXPECT_EQ("StreamableInGlobal", Print(x));
  EXPECT_EQ("StreamableInGlobal*", Print(&x));
}


TEST(PrintStreamableTypeTest, TemplateTypeInUserNamespace) {
  EXPECT_EQ("StreamableTemplateInFoo: 0",
            Print(::foo::StreamableTemplateInFoo<int>()));
}


TEST(PrintPrintableTypeTest, InUserNamespace) {
  EXPECT_EQ("PrintableViaPrintTo: 0",
            Print(::foo::PrintableViaPrintTo()));
}



TEST(PrintPrintableTypeTest, PointerInUserNamespace) {
  ::foo::PointerPrintable x;
  EXPECT_EQ("PointerPrintable*", Print(&x));
}


TEST(PrintPrintableTypeTest, TemplateInUserNamespace) {
  EXPECT_EQ("PrintableViaPrintToTemplate: 5",
            Print(::foo::PrintableViaPrintToTemplate<int>(5)));
}

#if GTEST_HAS_PROTOBUF_


TEST(PrintProto2MessageTest, PrintsShortDebugStringWhenItIsShort) {
  testing::internal::FooMessage msg;
  msg.set_int_field(2);
  msg.set_string_field("hello");
  EXPECT_PRED2(RE::FullMatch, Print(msg),
               "<int_field:\\s*2\\s+string_field:\\s*\"hello\">");
}


TEST(PrintProto2MessageTest, PrintsDebugStringWhenItIsLong) {
  testing::internal::FooMessage msg;
  msg.set_int_field(2);
  msg.set_string_field("hello");
  msg.add_names("peter");
  msg.add_names("paul");
  msg.add_names("mary");
  EXPECT_PRED2(RE::FullMatch, Print(msg),
               "<\n"
               "int_field:\\s*2\n"
               "string_field:\\s*\"hello\"\n"
               "names:\\s*\"peter\"\n"
               "names:\\s*\"paul\"\n"
               "names:\\s*\"mary\"\n"
               ">");
}

#endif  



TEST(PrintReferenceTest, PrintsAddressAndValue) {
  int n = 5;
  EXPECT_EQ("@" + PrintPointer(&n) + " 5", PrintByRef(n));

  int a[2][3] = {
    { 0, 1, 2 },
    { 3, 4, 5 }
  };
  EXPECT_EQ("@" + PrintPointer(a) + " { { 0, 1, 2 }, { 3, 4, 5 } }",
            PrintByRef(a));

  const ::foo::UnprintableInFoo x;
  EXPECT_EQ("@" + PrintPointer(&x) + " 16-byte object "
            "<EF-12 00-00 34-AB 00-00 00-00 00-00 00-00 00-00>",
            PrintByRef(x));
}



TEST(PrintReferenceTest, HandlesFunctionPointer) {
  void (*fp)(int n) = &MyFunction;
  const string fp_pointer_string =
      PrintPointer(reinterpret_cast<const void*>(&fp));
  
  
  
  
  const string fp_string = PrintPointer(reinterpret_cast<const void*>(
      reinterpret_cast<internal::BiggestInt>(fp)));
  EXPECT_EQ("@" + fp_pointer_string + " " + fp_string,
            PrintByRef(fp));
}



TEST(PrintReferenceTest, HandlesMemberFunctionPointer) {
  int (Foo::*p)(char ch) = &Foo::MyMethod;
  EXPECT_TRUE(HasPrefix(
      PrintByRef(p),
      "@" + PrintPointer(reinterpret_cast<const void*>(&p)) + " " +
          Print(sizeof(p)) + "-byte object "));

  char (Foo::*p2)(int n) = &Foo::MyVirtualMethod;
  EXPECT_TRUE(HasPrefix(
      PrintByRef(p2),
      "@" + PrintPointer(reinterpret_cast<const void*>(&p2)) + " " +
          Print(sizeof(p2)) + "-byte object "));
}



TEST(PrintReferenceTest, HandlesMemberVariablePointer) {
  int (Foo::*p) = &Foo::value;  
  EXPECT_TRUE(HasPrefix(
      PrintByRef(p),
      "@" + PrintPointer(&p) + " " + Print(sizeof(p)) + "-byte object "));
}






TEST(FormatForComparisonFailureMessageTest, WorksForScalar) {
  EXPECT_STREQ("123",
               FormatForComparisonFailureMessage(123, 124).c_str());
}


TEST(FormatForComparisonFailureMessageTest, WorksForNonCharPointer) {
  int n = 0;
  EXPECT_EQ(PrintPointer(&n),
            FormatForComparisonFailureMessage(&n, &n).c_str());
}


TEST(FormatForComparisonFailureMessageTest, FormatsNonCharArrayAsPointer) {
  
  
  int n[] = { 1, 2, 3 };
  EXPECT_EQ(PrintPointer(n),
            FormatForComparisonFailureMessage(n, n).c_str());
}






TEST(FormatForComparisonFailureMessageTest, WorksForCharPointerVsPointer) {
  
  
  
  

  
  const char* s = "hello";
  EXPECT_EQ(PrintPointer(s),
            FormatForComparisonFailureMessage(s, s).c_str());

  
  char ch = 'a';
  EXPECT_EQ(PrintPointer(&ch),
            FormatForComparisonFailureMessage(&ch, &ch).c_str());
}


TEST(FormatForComparisonFailureMessageTest, WorksForWCharPointerVsPointer) {
  
  
  
  

  
  const wchar_t* s = L"hello";
  EXPECT_EQ(PrintPointer(s),
            FormatForComparisonFailureMessage(s, s).c_str());

  
  wchar_t ch = L'a';
  EXPECT_EQ(PrintPointer(&ch),
            FormatForComparisonFailureMessage(&ch, &ch).c_str());
}




#if GTEST_HAS_GLOBAL_STRING

TEST(FormatForComparisonFailureMessageTest, WorksForCharPointerVsString) {
  const char* s = "hello \"world";
  EXPECT_STREQ("\"hello \\\"world\"",  
               FormatForComparisonFailureMessage(s, ::string()).c_str());

  
  char str[] = "hi\1";
  char* p = str;
  EXPECT_STREQ("\"hi\\x1\"",  
               FormatForComparisonFailureMessage(p, ::string()).c_str());
}
#endif


TEST(FormatForComparisonFailureMessageTest, WorksForCharPointerVsStdString) {
  const char* s = "hello \"world";
  EXPECT_STREQ("\"hello \\\"world\"",  
               FormatForComparisonFailureMessage(s, ::std::string()).c_str());

  
  char str[] = "hi\1";
  char* p = str;
  EXPECT_STREQ("\"hi\\x1\"",  
               FormatForComparisonFailureMessage(p, ::std::string()).c_str());
}

#if GTEST_HAS_GLOBAL_WSTRING

TEST(FormatForComparisonFailureMessageTest, WorksForWCharPointerVsWString) {
  const wchar_t* s = L"hi \"world";
  EXPECT_STREQ("L\"hi \\\"world\"",  
               FormatForComparisonFailureMessage(s, ::wstring()).c_str());

  
  wchar_t str[] = L"hi\1";
  wchar_t* p = str;
  EXPECT_STREQ("L\"hi\\x1\"",  
               FormatForComparisonFailureMessage(p, ::wstring()).c_str());
}
#endif

#if GTEST_HAS_STD_WSTRING

TEST(FormatForComparisonFailureMessageTest, WorksForWCharPointerVsStdWString) {
  const wchar_t* s = L"hi \"world";
  EXPECT_STREQ("L\"hi \\\"world\"",  
               FormatForComparisonFailureMessage(s, ::std::wstring()).c_str());

  
  wchar_t str[] = L"hi\1";
  wchar_t* p = str;
  EXPECT_STREQ("L\"hi\\x1\"",  
               FormatForComparisonFailureMessage(p, ::std::wstring()).c_str());
}
#endif






TEST(FormatForComparisonFailureMessageTest, WorksForCharArrayVsPointer) {
  char str[] = "hi \"world\"";
  char* p = NULL;
  EXPECT_EQ(PrintPointer(str),
            FormatForComparisonFailureMessage(str, p).c_str());
}


TEST(FormatForComparisonFailureMessageTest, WorksForCharArrayVsCharArray) {
  const char str[] = "hi \"world\"";
  EXPECT_EQ(PrintPointer(str),
            FormatForComparisonFailureMessage(str, str).c_str());
}


TEST(FormatForComparisonFailureMessageTest, WorksForWCharArrayVsPointer) {
  wchar_t str[] = L"hi \"world\"";
  wchar_t* p = NULL;
  EXPECT_EQ(PrintPointer(str),
            FormatForComparisonFailureMessage(str, p).c_str());
}


TEST(FormatForComparisonFailureMessageTest, WorksForWCharArrayVsWCharArray) {
  const wchar_t str[] = L"hi \"world\"";
  EXPECT_EQ(PrintPointer(str),
            FormatForComparisonFailureMessage(str, str).c_str());
}




#if GTEST_HAS_GLOBAL_STRING

TEST(FormatForComparisonFailureMessageTest, WorksForCharArrayVsString) {
  const char str[] = "hi \"w\0rld\"";
  EXPECT_STREQ("\"hi \\\"w\"",  
                                
               FormatForComparisonFailureMessage(str, ::string()).c_str());
}
#endif


TEST(FormatForComparisonFailureMessageTest, WorksForCharArrayVsStdString) {
  const char str[] = "hi \"world\"";
  EXPECT_STREQ("\"hi \\\"world\\\"\"",  
               FormatForComparisonFailureMessage(str, ::std::string()).c_str());
}

#if GTEST_HAS_GLOBAL_WSTRING

TEST(FormatForComparisonFailureMessageTest, WorksForWCharArrayVsWString) {
  const wchar_t str[] = L"hi \"world\"";
  EXPECT_STREQ("L\"hi \\\"world\\\"\"",  
               FormatForComparisonFailureMessage(str, ::wstring()).c_str());
}
#endif

#if GTEST_HAS_STD_WSTRING

TEST(FormatForComparisonFailureMessageTest, WorksForWCharArrayVsStdWString) {
  const wchar_t str[] = L"hi \"w\0rld\"";
  EXPECT_STREQ(
      "L\"hi \\\"w\"",  
                        
      FormatForComparisonFailureMessage(str, ::std::wstring()).c_str());
}
#endif




#define EXPECT_PRINT_TO_STRING_(value, expected_string)         \
  EXPECT_TRUE(PrintToString(value) == (expected_string))        \
      << " where " #value " prints as " << (PrintToString(value))

TEST(PrintToStringTest, WorksForScalar) {
  EXPECT_PRINT_TO_STRING_(123, "123");
}

TEST(PrintToStringTest, WorksForPointerToConstChar) {
  const char* p = "hello";
  EXPECT_PRINT_TO_STRING_(p, "\"hello\"");
}

TEST(PrintToStringTest, WorksForPointerToNonConstChar) {
  char s[] = "hello";
  char* p = s;
  EXPECT_PRINT_TO_STRING_(p, "\"hello\"");
}

TEST(PrintToStringTest, EscapesForPointerToConstChar) {
  const char* p = "hello\n";
  EXPECT_PRINT_TO_STRING_(p, "\"hello\\n\"");
}

TEST(PrintToStringTest, EscapesForPointerToNonConstChar) {
  char s[] = "hello\1";
  char* p = s;
  EXPECT_PRINT_TO_STRING_(p, "\"hello\\x1\"");
}

TEST(PrintToStringTest, WorksForArray) {
  int n[3] = { 1, 2, 3 };
  EXPECT_PRINT_TO_STRING_(n, "{ 1, 2, 3 }");
}

TEST(PrintToStringTest, WorksForCharArray) {
  char s[] = "hello";
  EXPECT_PRINT_TO_STRING_(s, "\"hello\"");
}

TEST(PrintToStringTest, WorksForCharArrayWithEmbeddedNul) {
  const char str_with_nul[] = "hello\0 world";
  EXPECT_PRINT_TO_STRING_(str_with_nul, "\"hello\\0 world\"");

  char mutable_str_with_nul[] = "hello\0 world";
  EXPECT_PRINT_TO_STRING_(mutable_str_with_nul, "\"hello\\0 world\"");
}

#undef EXPECT_PRINT_TO_STRING_

TEST(UniversalTersePrintTest, WorksForNonReference) {
  ::std::stringstream ss;
  UniversalTersePrint(123, &ss);
  EXPECT_EQ("123", ss.str());
}

TEST(UniversalTersePrintTest, WorksForReference) {
  const int& n = 123;
  ::std::stringstream ss;
  UniversalTersePrint(n, &ss);
  EXPECT_EQ("123", ss.str());
}

TEST(UniversalTersePrintTest, WorksForCString) {
  const char* s1 = "abc";
  ::std::stringstream ss1;
  UniversalTersePrint(s1, &ss1);
  EXPECT_EQ("\"abc\"", ss1.str());

  char* s2 = const_cast<char*>(s1);
  ::std::stringstream ss2;
  UniversalTersePrint(s2, &ss2);
  EXPECT_EQ("\"abc\"", ss2.str());

  const char* s3 = NULL;
  ::std::stringstream ss3;
  UniversalTersePrint(s3, &ss3);
  EXPECT_EQ("NULL", ss3.str());
}

TEST(UniversalPrintTest, WorksForNonReference) {
  ::std::stringstream ss;
  UniversalPrint(123, &ss);
  EXPECT_EQ("123", ss.str());
}

TEST(UniversalPrintTest, WorksForReference) {
  const int& n = 123;
  ::std::stringstream ss;
  UniversalPrint(n, &ss);
  EXPECT_EQ("123", ss.str());
}

TEST(UniversalPrintTest, WorksForCString) {
  const char* s1 = "abc";
  ::std::stringstream ss1;
  UniversalPrint(s1, &ss1);
  EXPECT_EQ(PrintPointer(s1) + " pointing to \"abc\"", string(ss1.str()));

  char* s2 = const_cast<char*>(s1);
  ::std::stringstream ss2;
  UniversalPrint(s2, &ss2);
  EXPECT_EQ(PrintPointer(s2) + " pointing to \"abc\"", string(ss2.str()));

  const char* s3 = NULL;
  ::std::stringstream ss3;
  UniversalPrint(s3, &ss3);
  EXPECT_EQ("NULL", ss3.str());
}

TEST(UniversalPrintTest, WorksForCharArray) {
  const char str[] = "\"Line\0 1\"\nLine 2";
  ::std::stringstream ss1;
  UniversalPrint(str, &ss1);
  EXPECT_EQ("\"\\\"Line\\0 1\\\"\\nLine 2\"", ss1.str());

  const char mutable_str[] = "\"Line\0 1\"\nLine 2";
  ::std::stringstream ss2;
  UniversalPrint(mutable_str, &ss2);
  EXPECT_EQ("\"\\\"Line\\0 1\\\"\\nLine 2\"", ss2.str());
}

#if GTEST_HAS_TR1_TUPLE

TEST(UniversalTersePrintTupleFieldsToStringsTestWithTr1, PrintsEmptyTuple) {
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::tr1::make_tuple());
  EXPECT_EQ(0u, result.size());
}

TEST(UniversalTersePrintTupleFieldsToStringsTestWithTr1, PrintsOneTuple) {
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::tr1::make_tuple(1));
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("1", result[0]);
}

TEST(UniversalTersePrintTupleFieldsToStringsTestWithTr1, PrintsTwoTuple) {
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::tr1::make_tuple(1, 'a'));
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("1", result[0]);
  EXPECT_EQ("'a' (97, 0x61)", result[1]);
}

TEST(UniversalTersePrintTupleFieldsToStringsTestWithTr1, PrintsTersely) {
  const int n = 1;
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::tr1::tuple<const int&, const char*>(n, "a"));
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("1", result[0]);
  EXPECT_EQ("\"a\"", result[1]);
}

#endif  

#if GTEST_HAS_STD_TUPLE_

TEST(UniversalTersePrintTupleFieldsToStringsTestWithStd, PrintsEmptyTuple) {
  Strings result = UniversalTersePrintTupleFieldsToStrings(::std::make_tuple());
  EXPECT_EQ(0u, result.size());
}

TEST(UniversalTersePrintTupleFieldsToStringsTestWithStd, PrintsOneTuple) {
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::make_tuple(1));
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("1", result[0]);
}

TEST(UniversalTersePrintTupleFieldsToStringsTestWithStd, PrintsTwoTuple) {
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::make_tuple(1, 'a'));
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("1", result[0]);
  EXPECT_EQ("'a' (97, 0x61)", result[1]);
}

TEST(UniversalTersePrintTupleFieldsToStringsTestWithStd, PrintsTersely) {
  const int n = 1;
  Strings result = UniversalTersePrintTupleFieldsToStrings(
      ::std::tuple<const int&, const char*>(n, "a"));
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ("1", result[0]);
  EXPECT_EQ("\"a\"", result[1]);
}

#endif  

}  
}  

