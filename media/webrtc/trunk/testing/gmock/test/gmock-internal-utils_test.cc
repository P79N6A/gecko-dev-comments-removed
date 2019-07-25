


































#include "gmock/internal/gmock-internal-utils.h"
#include <stdlib.h>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include "gmock/gmock.h"
#include "gmock/internal/gmock-port.h"
#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

#if GTEST_OS_CYGWIN
# include <sys/types.h>  
#endif

class ProtocolMessage;

namespace proto2 {
class Message;
}  

namespace testing {
namespace internal {

namespace {

using ::std::tr1::make_tuple;
using ::std::tr1::tuple;

TEST(ConvertIdentifierNameToWordsTest, WorksWhenNameContainsNoWord) {
  EXPECT_EQ("", ConvertIdentifierNameToWords(""));
  EXPECT_EQ("", ConvertIdentifierNameToWords("_"));
  EXPECT_EQ("", ConvertIdentifierNameToWords("__"));
}

TEST(ConvertIdentifierNameToWordsTest, WorksWhenNameContainsDigits) {
  EXPECT_EQ("1", ConvertIdentifierNameToWords("_1"));
  EXPECT_EQ("2", ConvertIdentifierNameToWords("2_"));
  EXPECT_EQ("34", ConvertIdentifierNameToWords("_34_"));
  EXPECT_EQ("34 56", ConvertIdentifierNameToWords("_34_56"));
}

TEST(ConvertIdentifierNameToWordsTest, WorksWhenNameContainsCamelCaseWords) {
  EXPECT_EQ("a big word", ConvertIdentifierNameToWords("ABigWord"));
  EXPECT_EQ("foo bar", ConvertIdentifierNameToWords("FooBar"));
  EXPECT_EQ("foo", ConvertIdentifierNameToWords("Foo_"));
  EXPECT_EQ("foo bar", ConvertIdentifierNameToWords("_Foo_Bar_"));
  EXPECT_EQ("foo and bar", ConvertIdentifierNameToWords("_Foo__And_Bar"));
}

TEST(ConvertIdentifierNameToWordsTest, WorksWhenNameContains_SeparatedWords) {
  EXPECT_EQ("foo bar", ConvertIdentifierNameToWords("foo_bar"));
  EXPECT_EQ("foo", ConvertIdentifierNameToWords("_foo_"));
  EXPECT_EQ("foo bar", ConvertIdentifierNameToWords("_foo_bar_"));
  EXPECT_EQ("foo and bar", ConvertIdentifierNameToWords("_foo__and_bar"));
}

TEST(ConvertIdentifierNameToWordsTest, WorksWhenNameIsMixture) {
  EXPECT_EQ("foo bar 123", ConvertIdentifierNameToWords("Foo_bar123"));
  EXPECT_EQ("chapter 11 section 1",
            ConvertIdentifierNameToWords("_Chapter11Section_1_"));
}

TEST(PointeeOfTest, WorksForSmartPointers) {
  CompileAssertTypesEqual<const char,
      PointeeOf<internal::linked_ptr<const char> >::type>();
}

TEST(PointeeOfTest, WorksForRawPointers) {
  CompileAssertTypesEqual<int, PointeeOf<int*>::type>();
  CompileAssertTypesEqual<const char, PointeeOf<const char*>::type>();
  CompileAssertTypesEqual<void, PointeeOf<void*>::type>();
}

TEST(GetRawPointerTest, WorksForSmartPointers) {
  const char* const raw_p4 = new const char('a');  
  const internal::linked_ptr<const char> p4(raw_p4);
  EXPECT_EQ(raw_p4, GetRawPointer(p4));
}

TEST(GetRawPointerTest, WorksForRawPointers) {
  int* p = NULL;
  
  EXPECT_TRUE(NULL == GetRawPointer(p));
  int n = 1;
  EXPECT_EQ(&n, GetRawPointer(&n));
}



class Base {};
class Derived : public Base {};

TEST(KindOfTest, Bool) {
  EXPECT_EQ(kBool, GMOCK_KIND_OF_(bool));  
}

TEST(KindOfTest, Integer) {
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(char));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(signed char));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(unsigned char));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(short));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(unsigned short));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(int));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(unsigned int));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(long));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(unsigned long));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(wchar_t));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(Int64));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(UInt64));  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(size_t));  
#if GTEST_OS_LINUX || GTEST_OS_MAC || GTEST_OS_CYGWIN
  
  EXPECT_EQ(kInteger, GMOCK_KIND_OF_(ssize_t));  
#endif
}

TEST(KindOfTest, FloatingPoint) {
  EXPECT_EQ(kFloatingPoint, GMOCK_KIND_OF_(float));  
  EXPECT_EQ(kFloatingPoint, GMOCK_KIND_OF_(double));  
  EXPECT_EQ(kFloatingPoint, GMOCK_KIND_OF_(long double));  
}

TEST(KindOfTest, Other) {
  EXPECT_EQ(kOther, GMOCK_KIND_OF_(void*));  
  EXPECT_EQ(kOther, GMOCK_KIND_OF_(char**));  
  EXPECT_EQ(kOther, GMOCK_KIND_OF_(Base));  
}



TEST(LosslessArithmeticConvertibleTest, BoolToBool) {
  EXPECT_TRUE((LosslessArithmeticConvertible<bool, bool>::value));
}

TEST(LosslessArithmeticConvertibleTest, BoolToInteger) {
  EXPECT_TRUE((LosslessArithmeticConvertible<bool, char>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<bool, int>::value));
  EXPECT_TRUE(
      (LosslessArithmeticConvertible<bool, unsigned long>::value));  
}

TEST(LosslessArithmeticConvertibleTest, BoolToFloatingPoint) {
  EXPECT_TRUE((LosslessArithmeticConvertible<bool, float>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<bool, double>::value));
}

TEST(LosslessArithmeticConvertibleTest, IntegerToBool) {
  EXPECT_FALSE((LosslessArithmeticConvertible<unsigned char, bool>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<int, bool>::value));
}

TEST(LosslessArithmeticConvertibleTest, IntegerToInteger) {
  
  EXPECT_TRUE((LosslessArithmeticConvertible<unsigned char, int>::value));

  
  EXPECT_TRUE(
      (LosslessArithmeticConvertible<unsigned short, UInt64>::value)); 

  
  EXPECT_FALSE((LosslessArithmeticConvertible<short, UInt64>::value)); 
  EXPECT_FALSE((LosslessArithmeticConvertible<
      signed char, unsigned int>::value));  

  
  EXPECT_TRUE((LosslessArithmeticConvertible<
               unsigned char, unsigned char>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<int, int>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<wchar_t, wchar_t>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<
               unsigned long, unsigned long>::value));  

  
  EXPECT_FALSE((LosslessArithmeticConvertible<
                unsigned char, signed char>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<int, unsigned int>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<UInt64, Int64>::value));

  
  EXPECT_FALSE((LosslessArithmeticConvertible<long, char>::value));  
  EXPECT_FALSE((LosslessArithmeticConvertible<int, signed char>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<Int64, unsigned int>::value));
}

TEST(LosslessArithmeticConvertibleTest, IntegerToFloatingPoint) {
  
  
  EXPECT_FALSE((LosslessArithmeticConvertible<char, float>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<int, double>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<
                short, long double>::value));  
}

TEST(LosslessArithmeticConvertibleTest, FloatingPointToBool) {
  EXPECT_FALSE((LosslessArithmeticConvertible<float, bool>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<double, bool>::value));
}

TEST(LosslessArithmeticConvertibleTest, FloatingPointToInteger) {
  EXPECT_FALSE((LosslessArithmeticConvertible<float, long>::value));  
  EXPECT_FALSE((LosslessArithmeticConvertible<double, Int64>::value));
  EXPECT_FALSE((LosslessArithmeticConvertible<long double, int>::value));
}

TEST(LosslessArithmeticConvertibleTest, FloatingPointToFloatingPoint) {
  
  EXPECT_TRUE((LosslessArithmeticConvertible<float, double>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<float, long double>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<double, long double>::value));

  
  EXPECT_TRUE((LosslessArithmeticConvertible<float, float>::value));
  EXPECT_TRUE((LosslessArithmeticConvertible<double, double>::value));

  
  EXPECT_FALSE((LosslessArithmeticConvertible<double, float>::value));
  if (sizeof(double) == sizeof(long double)) {  
    
    
    EXPECT_TRUE((LosslessArithmeticConvertible<long double, double>::value));
  } else {
    EXPECT_FALSE((LosslessArithmeticConvertible<long double, double>::value));
  }
}



TEST(TupleMatchesTest, WorksForSize0) {
  tuple<> matchers;
  tuple<> values;

  EXPECT_TRUE(TupleMatches(matchers, values));
}

TEST(TupleMatchesTest, WorksForSize1) {
  tuple<Matcher<int> > matchers(Eq(1));
  tuple<int> values1(1),
      values2(2);

  EXPECT_TRUE(TupleMatches(matchers, values1));
  EXPECT_FALSE(TupleMatches(matchers, values2));
}

TEST(TupleMatchesTest, WorksForSize2) {
  tuple<Matcher<int>, Matcher<char> > matchers(Eq(1), Eq('a'));
  tuple<int, char> values1(1, 'a'),
      values2(1, 'b'),
      values3(2, 'a'),
      values4(2, 'b');

  EXPECT_TRUE(TupleMatches(matchers, values1));
  EXPECT_FALSE(TupleMatches(matchers, values2));
  EXPECT_FALSE(TupleMatches(matchers, values3));
  EXPECT_FALSE(TupleMatches(matchers, values4));
}

TEST(TupleMatchesTest, WorksForSize5) {
  tuple<Matcher<int>, Matcher<char>, Matcher<bool>, Matcher<long>,  
      Matcher<string> >
      matchers(Eq(1), Eq('a'), Eq(true), Eq(2L), Eq("hi"));
  tuple<int, char, bool, long, string>  
      values1(1, 'a', true, 2L, "hi"),
      values2(1, 'a', true, 2L, "hello"),
      values3(2, 'a', true, 2L, "hi");

  EXPECT_TRUE(TupleMatches(matchers, values1));
  EXPECT_FALSE(TupleMatches(matchers, values2));
  EXPECT_FALSE(TupleMatches(matchers, values3));
}


TEST(AssertTest, SucceedsOnTrue) {
  Assert(true, __FILE__, __LINE__, "This should succeed.");
  Assert(true, __FILE__, __LINE__);  
}


TEST(AssertTest, FailsFatallyOnFalse) {
  EXPECT_DEATH_IF_SUPPORTED({
    Assert(false, __FILE__, __LINE__, "This should fail.");
  }, "");

  EXPECT_DEATH_IF_SUPPORTED({
    Assert(false, __FILE__, __LINE__);
  }, "");
}


TEST(ExpectTest, SucceedsOnTrue) {
  Expect(true, __FILE__, __LINE__, "This should succeed.");
  Expect(true, __FILE__, __LINE__);  
}


TEST(ExpectTest, FailsNonfatallyOnFalse) {
  EXPECT_NONFATAL_FAILURE({  
    Expect(false, __FILE__, __LINE__, "This should fail.");
  }, "This should fail");

  EXPECT_NONFATAL_FAILURE({  
    Expect(false, __FILE__, __LINE__);
  }, "Expectation failed");
}



class LogIsVisibleTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    
    
    
    
    
    
    original_verbose_ = GMOCK_FLAG(verbose).c_str();
  }

  virtual void TearDown() { GMOCK_FLAG(verbose) = original_verbose_; }

  string original_verbose_;
};

TEST_F(LogIsVisibleTest, AlwaysReturnsTrueIfVerbosityIsInfo) {
  GMOCK_FLAG(verbose) = kInfoVerbosity;
  EXPECT_TRUE(LogIsVisible(INFO));
  EXPECT_TRUE(LogIsVisible(WARNING));
}

TEST_F(LogIsVisibleTest, AlwaysReturnsFalseIfVerbosityIsError) {
  GMOCK_FLAG(verbose) = kErrorVerbosity;
  EXPECT_FALSE(LogIsVisible(INFO));
  EXPECT_FALSE(LogIsVisible(WARNING));
}

TEST_F(LogIsVisibleTest, WorksWhenVerbosityIsWarning) {
  GMOCK_FLAG(verbose) = kWarningVerbosity;
  EXPECT_FALSE(LogIsVisible(INFO));
  EXPECT_TRUE(LogIsVisible(WARNING));
}

#if GTEST_HAS_STREAM_REDIRECTION





void TestLogWithSeverity(const string& verbosity, LogSeverity severity,
                         bool should_print) {
  const string old_flag = GMOCK_FLAG(verbose);
  GMOCK_FLAG(verbose) = verbosity;
  CaptureStdout();
  Log(severity, "Test log.\n", 0);
  if (should_print) {
    EXPECT_THAT(GetCapturedStdout().c_str(),
                ContainsRegex(
                    severity == WARNING ?
                    "^\nGMOCK WARNING:\nTest log\\.\nStack trace:\n" :
                    "^\nTest log\\.\nStack trace:\n"));
  } else {
    EXPECT_STREQ("", GetCapturedStdout().c_str());
  }
  GMOCK_FLAG(verbose) = old_flag;
}



TEST(LogTest, NoStackTraceWhenStackFramesToSkipIsNegative) {
  const string saved_flag = GMOCK_FLAG(verbose);
  GMOCK_FLAG(verbose) = kInfoVerbosity;
  CaptureStdout();
  Log(INFO, "Test log.\n", -1);
  EXPECT_STREQ("\nTest log.\n", GetCapturedStdout().c_str());
  GMOCK_FLAG(verbose) = saved_flag;
}



TEST(LogTest, NoSkippingStackFrameInOptMode) {
  CaptureStdout();
  Log(WARNING, "Test log.\n", 100);
  const String log = GetCapturedStdout();

# if defined(NDEBUG) && GTEST_GOOGLE3_MODE_

  
  EXPECT_THAT(log, ContainsRegex("\nGMOCK WARNING:\n"
                                 "Test log\\.\n"
                                 "Stack trace:\n"
                                 ".+"));
# else

  
  EXPECT_STREQ("\nGMOCK WARNING:\n"
               "Test log.\n"
               "Stack trace:\n", log.c_str());
# endif
}



TEST(LogTest, AllLogsArePrintedWhenVerbosityIsInfo) {
  TestLogWithSeverity(kInfoVerbosity, INFO, true);
  TestLogWithSeverity(kInfoVerbosity, WARNING, true);
}



TEST(LogTest, OnlyWarningsArePrintedWhenVerbosityIsWarning) {
  TestLogWithSeverity(kWarningVerbosity, INFO, false);
  TestLogWithSeverity(kWarningVerbosity, WARNING, true);
}



TEST(LogTest, NoLogsArePrintedWhenVerbosityIsError) {
  TestLogWithSeverity(kErrorVerbosity, INFO, false);
  TestLogWithSeverity(kErrorVerbosity, WARNING, false);
}



TEST(LogTest, OnlyWarningsArePrintedWhenVerbosityIsInvalid) {
  TestLogWithSeverity("invalid", INFO, false);
  TestLogWithSeverity("invalid", WARNING, true);
}

#endif  

TEST(TypeTraitsTest, true_type) {
  EXPECT_TRUE(true_type::value);
}

TEST(TypeTraitsTest, false_type) {
  EXPECT_FALSE(false_type::value);
}

TEST(TypeTraitsTest, is_reference) {
  EXPECT_FALSE(is_reference<int>::value);
  EXPECT_FALSE(is_reference<char*>::value);
  EXPECT_TRUE(is_reference<const int&>::value);
}

TEST(TypeTraitsTest, is_pointer) {
  EXPECT_FALSE(is_pointer<int>::value);
  EXPECT_FALSE(is_pointer<char&>::value);
  EXPECT_TRUE(is_pointer<const int*>::value);
}

TEST(TypeTraitsTest, type_equals) {
  EXPECT_FALSE((type_equals<int, const int>::value));
  EXPECT_FALSE((type_equals<int, int&>::value));
  EXPECT_FALSE((type_equals<int, double>::value));
  EXPECT_TRUE((type_equals<char, char>::value));
}

TEST(TypeTraitsTest, remove_reference) {
  EXPECT_TRUE((type_equals<char, remove_reference<char&>::type>::value));
  EXPECT_TRUE((type_equals<const int,
               remove_reference<const int&>::type>::value));
  EXPECT_TRUE((type_equals<int, remove_reference<int>::type>::value));
  EXPECT_TRUE((type_equals<double*, remove_reference<double*>::type>::value));
}

#if GTEST_HAS_STREAM_REDIRECTION



String GrabOutput(void(*logger)(), const char* verbosity) {
  const string saved_flag = GMOCK_FLAG(verbose);
  GMOCK_FLAG(verbose) = verbosity;
  CaptureStdout();
  logger();
  GMOCK_FLAG(verbose) = saved_flag;
  return GetCapturedStdout();
}

class DummyMock {
 public:
  MOCK_METHOD0(TestMethod, void());
  MOCK_METHOD1(TestMethodArg, void(int dummy));
};

void ExpectCallLogger() {
  DummyMock mock;
  EXPECT_CALL(mock, TestMethod());
  mock.TestMethod();
};


TEST(ExpectCallTest, LogsWhenVerbosityIsInfo) {
  EXPECT_THAT(GrabOutput(ExpectCallLogger, kInfoVerbosity),
              HasSubstr("EXPECT_CALL(mock, TestMethod())"));
}



TEST(ExpectCallTest, DoesNotLogWhenVerbosityIsWarning) {
  EXPECT_STREQ("", GrabOutput(ExpectCallLogger, kWarningVerbosity).c_str());
}



TEST(ExpectCallTest,  DoesNotLogWhenVerbosityIsError) {
  EXPECT_STREQ("", GrabOutput(ExpectCallLogger, kErrorVerbosity).c_str());
}

void OnCallLogger() {
  DummyMock mock;
  ON_CALL(mock, TestMethod());
};


TEST(OnCallTest, LogsWhenVerbosityIsInfo) {
  EXPECT_THAT(GrabOutput(OnCallLogger, kInfoVerbosity),
              HasSubstr("ON_CALL(mock, TestMethod())"));
}



TEST(OnCallTest, DoesNotLogWhenVerbosityIsWarning) {
  EXPECT_STREQ("", GrabOutput(OnCallLogger, kWarningVerbosity).c_str());
}



TEST(OnCallTest, DoesNotLogWhenVerbosityIsError) {
  EXPECT_STREQ("", GrabOutput(OnCallLogger, kErrorVerbosity).c_str());
}

void OnCallAnyArgumentLogger() {
  DummyMock mock;
  ON_CALL(mock, TestMethodArg(_));
}


TEST(OnCallTest, LogsAnythingArgument) {
  EXPECT_THAT(GrabOutput(OnCallAnyArgumentLogger, kInfoVerbosity),
              HasSubstr("ON_CALL(mock, TestMethodArg(_)"));
}

#endif  



TEST(StlContainerViewTest, WorksForStlContainer) {
  StaticAssertTypeEq<std::vector<int>,
      StlContainerView<std::vector<int> >::type>();
  StaticAssertTypeEq<const std::vector<double>&,
      StlContainerView<std::vector<double> >::const_reference>();

  typedef std::vector<char> Chars;
  Chars v1;
  const Chars& v2(StlContainerView<Chars>::ConstReference(v1));
  EXPECT_EQ(&v1, &v2);

  v1.push_back('a');
  Chars v3 = StlContainerView<Chars>::Copy(v1);
  EXPECT_THAT(v3, Eq(v3));
}

TEST(StlContainerViewTest, WorksForStaticNativeArray) {
  StaticAssertTypeEq<NativeArray<int>,
      StlContainerView<int[3]>::type>();
  StaticAssertTypeEq<NativeArray<double>,
      StlContainerView<const double[4]>::type>();
  StaticAssertTypeEq<NativeArray<char[3]>,
      StlContainerView<const char[2][3]>::type>();

  StaticAssertTypeEq<const NativeArray<int>,
      StlContainerView<int[2]>::const_reference>();

  int a1[3] = { 0, 1, 2 };
  NativeArray<int> a2 = StlContainerView<int[3]>::ConstReference(a1);
  EXPECT_EQ(3U, a2.size());
  EXPECT_EQ(a1, a2.begin());

  const NativeArray<int> a3 = StlContainerView<int[3]>::Copy(a1);
  ASSERT_EQ(3U, a3.size());
  EXPECT_EQ(0, a3.begin()[0]);
  EXPECT_EQ(1, a3.begin()[1]);
  EXPECT_EQ(2, a3.begin()[2]);

  
  a1[0] = 3;
  EXPECT_EQ(0, a3.begin()[0]);
}

TEST(StlContainerViewTest, WorksForDynamicNativeArray) {
  StaticAssertTypeEq<NativeArray<int>,
      StlContainerView<tuple<const int*, size_t> >::type>();
  StaticAssertTypeEq<NativeArray<double>,
      StlContainerView<tuple<linked_ptr<double>, int> >::type>();

  StaticAssertTypeEq<const NativeArray<int>,
      StlContainerView<tuple<const int*, int> >::const_reference>();

  int a1[3] = { 0, 1, 2 };
  const int* const p1 = a1;
  NativeArray<int> a2 = StlContainerView<tuple<const int*, int> >::
      ConstReference(make_tuple(p1, 3));
  EXPECT_EQ(3U, a2.size());
  EXPECT_EQ(a1, a2.begin());

  const NativeArray<int> a3 = StlContainerView<tuple<int*, size_t> >::
      Copy(make_tuple(static_cast<int*>(a1), 3));
  ASSERT_EQ(3U, a3.size());
  EXPECT_EQ(0, a3.begin()[0]);
  EXPECT_EQ(1, a3.begin()[1]);
  EXPECT_EQ(2, a3.begin()[2]);

  
  a1[0] = 3;
  EXPECT_EQ(0, a3.begin()[0]);
}

}  
}  
}  
