



































#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_

#include <gtest/internal/gtest-port.h>

#if GTEST_OS_LINUX
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif  

#include <ctype.h>
#include <string.h>
#include <iomanip>
#include <limits>
#include <set>

#include <gtest/internal/gtest-string.h>
#include <gtest/internal/gtest-filepath.h>
#include <gtest/internal/gtest-type-util.h>









#define GTEST_CONCAT_TOKEN_(foo, bar) GTEST_CONCAT_TOKEN_IMPL_(foo, bar)
#define GTEST_CONCAT_TOKEN_IMPL_(foo, bar) foo ## bar


























template <typename T>
inline void GTestStreamToHelper(std::ostream* os, const T& val) {
  *os << val;
}

namespace testing {



class Message;                         
class Test;                            
class TestCase;                        
class TestPartResult;                  
class TestInfo;                        
class UnitTest;                        
class UnitTestEventListenerInterface;  
class AssertionResult;                 

namespace internal {

struct TraceInfo;                      
class ScopedTrace;                     
class TestInfoImpl;                    
class TestResult;                      
class UnitTestImpl;                    

template <typename E> class List;      
template <typename E> class ListNode;  


extern int g_init_gtest_count;



extern const char kStackTraceMarker[];




class Secret;















char IsNullLiteralHelper(Secret* p);
char (&IsNullLiteralHelper(...))[2];  




#ifdef GTEST_ELLIPSIS_NEEDS_COPY_





#define GTEST_IS_NULL_LITERAL_(x) false
#else
#define GTEST_IS_NULL_LITERAL_(x) \
    (sizeof(::testing::internal::IsNullLiteralHelper(x)) == 1)
#endif  


String AppendUserMessage(const String& gtest_msg,
                         const Message& user_msg);


class ScopedTrace {
 public:
  
  
  ScopedTrace(const char* file, int line, const Message& message);

  
  
  
  
  ~ScopedTrace();

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(ScopedTrace);
} GTEST_ATTRIBUTE_UNUSED_;  
                            
                            








template <typename T>
String StreamableToString(const T& streamable);



#ifdef GTEST_NEEDS_IS_POINTER_









template <typename T>
inline String FormatValueForFailureMessage(internal::true_type dummy,
                                           T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

template <typename T>
inline String FormatValueForFailureMessage(internal::false_type dummy,
                                           const T& value) {
  return StreamableToString(value);
}

template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return FormatValueForFailureMessage(
      typename internal::is_pointer<T>::type(), value);
}

#else





template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return StreamableToString(value);
}



template <typename T>
inline String FormatForFailureMessage(T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

#endif  


String FormatForFailureMessage(char ch);
String FormatForFailureMessage(wchar_t wchar);






#define GTEST_FORMAT_IMPL_(operand2_type, operand1_printer)\
inline String FormatForComparisonFailureMessage(\
    operand2_type::value_type* str, const operand2_type& /*operand2*/) {\
  return operand1_printer(str);\
}\
inline String FormatForComparisonFailureMessage(\
    const operand2_type::value_type* str, const operand2_type& /*operand2*/) {\
  return operand1_printer(str);\
}

#if GTEST_HAS_STD_STRING
GTEST_FORMAT_IMPL_(::std::string, String::ShowCStringQuoted)
#endif  
#if GTEST_HAS_STD_WSTRING
GTEST_FORMAT_IMPL_(::std::wstring, String::ShowWideCStringQuoted)
#endif  

#if GTEST_HAS_GLOBAL_STRING
GTEST_FORMAT_IMPL_(::string, String::ShowCStringQuoted)
#endif  
#if GTEST_HAS_GLOBAL_WSTRING
GTEST_FORMAT_IMPL_(::wstring, String::ShowWideCStringQuoted)
#endif  

#undef GTEST_FORMAT_IMPL_
















AssertionResult EqFailure(const char* expected_expression,
                          const char* actual_expression,
                          const String& expected_value,
                          const String& actual_value,
                          bool ignoring_case);































template <typename RawType>
class FloatingPoint {
 public:
  
  
  typedef typename TypeWithSize<sizeof(RawType)>::UInt Bits;

  

  
  static const size_t kBitCount = 8*sizeof(RawType);

  
  static const size_t kFractionBitCount =
    std::numeric_limits<RawType>::digits - 1;

  
  static const size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

  
  static const Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

  
  static const Bits kFractionBitMask =
    ~static_cast<Bits>(0) >> (kExponentBitCount + 1);

  
  static const Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

  
  
  
  
  
  
  
  
  
  
  
  
  static const size_t kMaxUlps = 4;

  
  
  
  
  
  
  explicit FloatingPoint(const RawType& x) : value_(x) {}

  

  
  
  
  static RawType ReinterpretBits(const Bits bits) {
    FloatingPoint fp(0);
    fp.bits_ = bits;
    return fp.value_;
  }

  
  static RawType Infinity() {
    return ReinterpretBits(kExponentBitMask);
  }

  

  
  const Bits &bits() const { return bits_; }

  
  Bits exponent_bits() const { return kExponentBitMask & bits_; }

  
  Bits fraction_bits() const { return kFractionBitMask & bits_; }

  
  Bits sign_bit() const { return kSignBitMask & bits_; }

  
  bool is_nan() const {
    
    
    return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
  }

  
  
  
  
  
  
  bool AlmostEquals(const FloatingPoint& rhs) const {
    
    
    if (is_nan() || rhs.is_nan()) return false;

    return DistanceBetweenSignAndMagnitudeNumbers(bits_, rhs.bits_) <= kMaxUlps;
  }

 private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bits SignAndMagnitudeToBiased(const Bits &sam) {
    if (kSignBitMask & sam) {
      
      return ~sam + 1;
    } else {
      
      return kSignBitMask | sam;
    }
  }

  
  
  static Bits DistanceBetweenSignAndMagnitudeNumbers(const Bits &sam1,
                                                     const Bits &sam2) {
    const Bits biased1 = SignAndMagnitudeToBiased(sam1);
    const Bits biased2 = SignAndMagnitudeToBiased(sam2);
    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
  }

  union {
    RawType value_;  
    Bits bits_;      
  };
};



typedef FloatingPoint<float> Float;
typedef FloatingPoint<double> Double;







typedef const void* TypeId;

template <typename T>
class TypeIdHelper {
 public:
  
  
  
  static bool dummy_;
};

template <typename T>
bool TypeIdHelper<T>::dummy_ = false;




template <typename T>
TypeId GetTypeId() {
  
  
  
  
  return &(TypeIdHelper<T>::dummy_);
}






TypeId GetTestTypeId();



class TestFactoryBase {
 public:
  virtual ~TestFactoryBase() {}

  
  
  virtual Test* CreateTest() = 0;

 protected:
  TestFactoryBase() {}

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestFactoryBase);
};



template <class TestClass>
class TestFactoryImpl : public TestFactoryBase {
 public:
  virtual Test* CreateTest() { return new TestClass; }
};

#if GTEST_OS_WINDOWS





AssertionResult IsHRESULTSuccess(const char* expr, long hr);  
AssertionResult IsHRESULTFailure(const char* expr, long hr);  

#endif  



inline String FormatFileLocation(const char* file, int line) {
  const char* const file_name = file == NULL ? "unknown file" : file;
  if (line < 0) {
    return String::Format("%s:", file_name);
  }
#ifdef _MSC_VER
  return String::Format("%s(%d):", file_name, line);
#else
  return String::Format("%s:%d:", file_name, line);
#endif  
}


typedef void (*SetUpTestCaseFunc)();
typedef void (*TearDownTestCaseFunc)();


















TestInfo* MakeAndRegisterTestInfo(
    const char* test_case_name, const char* name,
    const char* test_case_comment, const char* comment,
    TypeId fixture_class_id,
    SetUpTestCaseFunc set_up_tc,
    TearDownTestCaseFunc tear_down_tc,
    TestFactoryBase* factory);

#if GTEST_HAS_TYPED_TEST || GTEST_HAS_TYPED_TEST_P


class TypedTestCasePState {
 public:
  TypedTestCasePState() : registered_(false) {}

  
  
  
  bool AddTestName(const char* file, int line, const char* case_name,
                   const char* test_name) {
    if (registered_) {
      fprintf(stderr, "%s Test %s must be defined before "
              "REGISTER_TYPED_TEST_CASE_P(%s, ...).\n",
              FormatFileLocation(file, line).c_str(), test_name, case_name);
      fflush(stderr);
      abort();
    }
    defined_test_names_.insert(test_name);
    return true;
  }

  
  
  
  const char* VerifyRegisteredTestNames(
      const char* file, int line, const char* registered_tests);

 private:
  bool registered_;
  ::std::set<const char*> defined_test_names_;
};



inline const char* SkipComma(const char* str) {
  const char* comma = strchr(str, ',');
  if (comma == NULL) {
    return NULL;
  }
  while (isspace(*(++comma))) {}
  return comma;
}



inline String GetPrefixUntilComma(const char* str) {
  const char* comma = strchr(str, ',');
  return comma == NULL ? String(str) : String(str, comma - str);
}








template <GTEST_TEMPLATE_ Fixture, class TestSel, typename Types>
class TypeParameterizedTest {
 public:
  
  
  
  
  static bool Register(const char* prefix, const char* case_name,
                       const char* test_names, int index) {
    typedef typename Types::Head Type;
    typedef Fixture<Type> FixtureClass;
    typedef typename GTEST_BIND_(TestSel, Type) TestClass;

    
    
    MakeAndRegisterTestInfo(
        String::Format("%s%s%s/%d", prefix, prefix[0] == '\0' ? "" : "/",
                       case_name, index).c_str(),
        GetPrefixUntilComma(test_names).c_str(),
        String::Format("TypeParam = %s", GetTypeName<Type>().c_str()).c_str(),
        "",
        GetTypeId<FixtureClass>(),
        TestClass::SetUpTestCase,
        TestClass::TearDownTestCase,
        new TestFactoryImpl<TestClass>);

    
    return TypeParameterizedTest<Fixture, TestSel, typename Types::Tail>
        ::Register(prefix, case_name, test_names, index + 1);
  }
};


template <GTEST_TEMPLATE_ Fixture, class TestSel>
class TypeParameterizedTest<Fixture, TestSel, Types0> {
 public:
  static bool Register(const char* , const char* ,
                       const char* , int ) {
    return true;
  }
};





template <GTEST_TEMPLATE_ Fixture, typename Tests, typename Types>
class TypeParameterizedTestCase {
 public:
  static bool Register(const char* prefix, const char* case_name,
                       const char* test_names) {
    typedef typename Tests::Head Head;

    
    TypeParameterizedTest<Fixture, Head, Types>::Register(
        prefix, case_name, test_names, 0);

    
    return TypeParameterizedTestCase<Fixture, typename Tests::Tail, Types>
        ::Register(prefix, case_name, SkipComma(test_names));
  }
};


template <GTEST_TEMPLATE_ Fixture, typename Types>
class TypeParameterizedTestCase<Fixture, Templates0, Types> {
 public:
  static bool Register(const char* prefix, const char* case_name,
                       const char* test_names) {
    return true;
  }
};

#endif  











String GetCurrentOsStackTraceExceptTop(UnitTest* unit_test, int skip_count);


int GetFailedPartCount(const TestResult* result);


bool AlwaysTrue();

}  
}  

#define GTEST_MESSAGE_(message, result_type) \
  ::testing::internal::AssertHelper(result_type, __FILE__, __LINE__, message) \
    = ::testing::Message()

#define GTEST_FATAL_FAILURE_(message) \
  return GTEST_MESSAGE_(message, ::testing::TPRT_FATAL_FAILURE)

#define GTEST_NONFATAL_FAILURE_(message) \
  GTEST_MESSAGE_(message, ::testing::TPRT_NONFATAL_FAILURE)

#define GTEST_SUCCESS_(message) \
  GTEST_MESSAGE_(message, ::testing::TPRT_SUCCESS)




#define GTEST_HIDE_UNREACHABLE_CODE_(statement) \
  if (::testing::internal::AlwaysTrue()) { statement; }

#define GTEST_TEST_THROW_(statement, expected_exception, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (const char* gtest_msg = "") { \
    bool gtest_caught_expected = false; \
    try { \
      GTEST_HIDE_UNREACHABLE_CODE_(statement); \
    } \
    catch (expected_exception const&) { \
      gtest_caught_expected = true; \
    } \
    catch (...) { \
      gtest_msg = "Expected: " #statement " throws an exception of type " \
                  #expected_exception ".\n  Actual: it throws a different " \
                  "type."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
    if (!gtest_caught_expected) { \
      gtest_msg = "Expected: " #statement " throws an exception of type " \
                  #expected_exception ".\n  Actual: it throws nothing."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__): \
      fail(gtest_msg)

#define GTEST_TEST_NO_THROW_(statement, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (const char* gtest_msg = "") { \
    try { \
      GTEST_HIDE_UNREACHABLE_CODE_(statement); \
    } \
    catch (...) { \
      gtest_msg = "Expected: " #statement " doesn't throw an exception.\n" \
                  "  Actual: it throws."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__): \
      fail(gtest_msg)

#define GTEST_TEST_ANY_THROW_(statement, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (const char* gtest_msg = "") { \
    bool gtest_caught_any = false; \
    try { \
      GTEST_HIDE_UNREACHABLE_CODE_(statement); \
    } \
    catch (...) { \
      gtest_caught_any = true; \
    } \
    if (!gtest_caught_any) { \
      gtest_msg = "Expected: " #statement " throws an exception.\n" \
                  "  Actual: it doesn't."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testanythrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testanythrow_, __LINE__): \
      fail(gtest_msg)


#define GTEST_TEST_BOOLEAN_(boolexpr, booltext, actual, expected, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (boolexpr) \
    ; \
  else \
    fail("Value of: " booltext "\n  Actual: " #actual "\nExpected: " #expected)

#define GTEST_TEST_NO_FATAL_FAILURE_(statement, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (const char* gtest_msg = "") { \
    ::testing::internal::HasNewFatalFailureHelper gtest_fatal_failure_checker; \
    GTEST_HIDE_UNREACHABLE_CODE_(statement); \
    if (gtest_fatal_failure_checker.has_new_fatal_failure()) { \
      gtest_msg = "Expected: " #statement " doesn't generate new fatal " \
                  "failures in the current thread.\n" \
                  "  Actual: it does."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testnofatal_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testnofatal_, __LINE__): \
      fail(gtest_msg)


#define GTEST_TEST_CLASS_NAME_(test_case_name, test_name) \
  test_case_name##_##test_name##_Test


#define GTEST_TEST_(test_case_name, test_name, parent_class, parent_id)\
class GTEST_TEST_CLASS_NAME_(test_case_name, test_name) : public parent_class {\
 public:\
  GTEST_TEST_CLASS_NAME_(test_case_name, test_name)() {}\
 private:\
  virtual void TestBody();\
  static ::testing::TestInfo* const test_info_;\
  GTEST_DISALLOW_COPY_AND_ASSIGN_(\
      GTEST_TEST_CLASS_NAME_(test_case_name, test_name));\
};\
\
::testing::TestInfo* const GTEST_TEST_CLASS_NAME_(test_case_name, test_name)\
  ::test_info_ =\
    ::testing::internal::MakeAndRegisterTestInfo(\
        #test_case_name, #test_name, "", "", \
        (parent_id), \
        parent_class::SetUpTestCase, \
        parent_class::TearDownTestCase, \
        new ::testing::internal::TestFactoryImpl<\
            GTEST_TEST_CLASS_NAME_(test_case_name, test_name)>);\
void GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::TestBody()

#endif  
