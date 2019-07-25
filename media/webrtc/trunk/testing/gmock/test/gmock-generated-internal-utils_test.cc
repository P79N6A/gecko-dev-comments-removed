


































#include "gmock/internal/gmock-generated-internal-utils.h"
#include "gmock/internal/gmock-internal-utils.h"
#include "gtest/gtest.h"

namespace {

using ::std::tr1::tuple;
using ::testing::Matcher;
using ::testing::internal::CompileAssertTypesEqual;
using ::testing::internal::MatcherTuple;
using ::testing::internal::Function;
using ::testing::internal::IgnoredValue;



TEST(MatcherTupleTest, ForSize0) {
  CompileAssertTypesEqual<tuple<>, MatcherTuple<tuple<> >::type>();
}

TEST(MatcherTupleTest, ForSize1) {
  CompileAssertTypesEqual<tuple<Matcher<int> >,
                          MatcherTuple<tuple<int> >::type>();
}

TEST(MatcherTupleTest, ForSize2) {
  CompileAssertTypesEqual<tuple<Matcher<int>, Matcher<char> >,
                          MatcherTuple<tuple<int, char> >::type>();
}

TEST(MatcherTupleTest, ForSize5) {
  CompileAssertTypesEqual<tuple<Matcher<int>, Matcher<char>, Matcher<bool>,
                                Matcher<double>, Matcher<char*> >,
                          MatcherTuple<tuple<int, char, bool, double, char*>
                                      >::type>();
}



TEST(FunctionTest, Nullary) {
  typedef Function<int()> F;  
  CompileAssertTypesEqual<int, F::Result>();
  CompileAssertTypesEqual<tuple<>, F::ArgumentTuple>();
  CompileAssertTypesEqual<tuple<>, F::ArgumentMatcherTuple>();
  CompileAssertTypesEqual<void(), F::MakeResultVoid>();
  CompileAssertTypesEqual<IgnoredValue(), F::MakeResultIgnoredValue>();
}

TEST(FunctionTest, Unary) {
  typedef Function<int(bool)> F;  
  CompileAssertTypesEqual<int, F::Result>();
  CompileAssertTypesEqual<bool, F::Argument1>();
  CompileAssertTypesEqual<tuple<bool>, F::ArgumentTuple>();
  CompileAssertTypesEqual<tuple<Matcher<bool> >, F::ArgumentMatcherTuple>();
  CompileAssertTypesEqual<void(bool), F::MakeResultVoid>();  
  CompileAssertTypesEqual<IgnoredValue(bool),  
      F::MakeResultIgnoredValue>();
}

TEST(FunctionTest, Binary) {
  typedef Function<int(bool, const long&)> F;  
  CompileAssertTypesEqual<int, F::Result>();
  CompileAssertTypesEqual<bool, F::Argument1>();
  CompileAssertTypesEqual<const long&, F::Argument2>();  
  CompileAssertTypesEqual<tuple<bool, const long&>, F::ArgumentTuple>();  
  CompileAssertTypesEqual<tuple<Matcher<bool>, Matcher<const long&> >,  
                          F::ArgumentMatcherTuple>();
  CompileAssertTypesEqual<void(bool, const long&), F::MakeResultVoid>();  
  CompileAssertTypesEqual<IgnoredValue(bool, const long&),  
      F::MakeResultIgnoredValue>();
}

TEST(FunctionTest, LongArgumentList) {
  typedef Function<char(bool, int, char*, int&, const long&)> F;  
  CompileAssertTypesEqual<char, F::Result>();
  CompileAssertTypesEqual<bool, F::Argument1>();
  CompileAssertTypesEqual<int, F::Argument2>();
  CompileAssertTypesEqual<char*, F::Argument3>();
  CompileAssertTypesEqual<int&, F::Argument4>();
  CompileAssertTypesEqual<const long&, F::Argument5>();  
  CompileAssertTypesEqual<tuple<bool, int, char*, int&, const long&>,  
                          F::ArgumentTuple>();
  CompileAssertTypesEqual<tuple<Matcher<bool>, Matcher<int>, Matcher<char*>,
                                Matcher<int&>, Matcher<const long&> >,  
                          F::ArgumentMatcherTuple>();
  CompileAssertTypesEqual<void(bool, int, char*, int&, const long&),  
                          F::MakeResultVoid>();
  CompileAssertTypesEqual<
      IgnoredValue(bool, int, char*, int&, const long&),  
      F::MakeResultIgnoredValue>();
}

}  
