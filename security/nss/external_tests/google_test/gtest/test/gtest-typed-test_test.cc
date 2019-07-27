






























#include "test/gtest-typed-test_test.h"

#include <set>
#include <vector>

#include "gtest/gtest.h"

using testing::Test;




template <typename T>
class CommonTest : public Test {
  
  
 public:
  static void SetUpTestCase() {
    shared_ = new T(5);
  }

  static void TearDownTestCase() {
    delete shared_;
    shared_ = NULL;
  }

  
  
 protected:
  
  
  typedef std::vector<T> Vector;
  typedef std::set<int> IntSet;

  CommonTest() : value_(1) {}

  virtual ~CommonTest() { EXPECT_EQ(3, value_); }

  virtual void SetUp() {
    EXPECT_EQ(1, value_);
    value_++;
  }

  virtual void TearDown() {
    EXPECT_EQ(2, value_);
    value_++;
  }

  T value_;
  static T* shared_;
};

template <typename T>
T* CommonTest<T>::shared_ = NULL;


#if GTEST_HAS_TYPED_TEST

using testing::Types;




typedef Types<char, int> TwoTypes;
TYPED_TEST_CASE(CommonTest, TwoTypes);

TYPED_TEST(CommonTest, ValuesAreCorrect) {
  
  
  EXPECT_EQ(5, *TestFixture::shared_);

  
  
  typename TestFixture::Vector empty;
  EXPECT_EQ(0U, empty.size());

  typename TestFixture::IntSet empty2;
  EXPECT_EQ(0U, empty2.size());

  
  
  EXPECT_EQ(2, this->value_);
}



TYPED_TEST(CommonTest, ValuesAreStillCorrect) {
  
  
  ASSERT_TRUE(this->shared_ != NULL);
  EXPECT_EQ(5, *this->shared_);

  
  EXPECT_EQ(static_cast<TypeParam>(2), this->value_);
}




template <typename T>
class TypedTest1 : public Test {
};



TYPED_TEST_CASE(TypedTest1, int);
TYPED_TEST(TypedTest1, A) {}

template <typename T>
class TypedTest2 : public Test {
};



TYPED_TEST_CASE(TypedTest2, Types<int>);



TYPED_TEST(TypedTest2, A) {}



namespace library1 {

template <typename T>
class NumericTest : public Test {
};

typedef Types<int, long> NumericTypes;
TYPED_TEST_CASE(NumericTest, NumericTypes);

TYPED_TEST(NumericTest, DefaultIsZero) {
  EXPECT_EQ(0, TypeParam());
}

}  

#endif  


#if GTEST_HAS_TYPED_TEST_P

using testing::Types;
using testing::internal::TypedTestCasePState;



class TypedTestCasePStateTest : public Test {
 protected:
  virtual void SetUp() {
    state_.AddTestName("foo.cc", 0, "FooTest", "A");
    state_.AddTestName("foo.cc", 0, "FooTest", "B");
    state_.AddTestName("foo.cc", 0, "FooTest", "C");
  }

  TypedTestCasePState state_;
};

TEST_F(TypedTestCasePStateTest, SucceedsForMatchingList) {
  const char* tests = "A, B, C";
  EXPECT_EQ(tests,
            state_.VerifyRegisteredTestNames("foo.cc", 1, tests));
}



TEST_F(TypedTestCasePStateTest, IgnoresOrderAndSpaces) {
  const char* tests = "A,C,   B";
  EXPECT_EQ(tests,
            state_.VerifyRegisteredTestNames("foo.cc", 1, tests));
}

typedef TypedTestCasePStateTest TypedTestCasePStateDeathTest;

TEST_F(TypedTestCasePStateDeathTest, DetectsDuplicates) {
  EXPECT_DEATH_IF_SUPPORTED(
      state_.VerifyRegisteredTestNames("foo.cc", 1, "A, B, A, C"),
      "foo\\.cc.1.?: Test A is listed more than once\\.");
}

TEST_F(TypedTestCasePStateDeathTest, DetectsExtraTest) {
  EXPECT_DEATH_IF_SUPPORTED(
      state_.VerifyRegisteredTestNames("foo.cc", 1, "A, B, C, D"),
      "foo\\.cc.1.?: No test named D can be found in this test case\\.");
}

TEST_F(TypedTestCasePStateDeathTest, DetectsMissedTest) {
  EXPECT_DEATH_IF_SUPPORTED(
      state_.VerifyRegisteredTestNames("foo.cc", 1, "A, C"),
      "foo\\.cc.1.?: You forgot to list test B\\.");
}



TEST_F(TypedTestCasePStateDeathTest, DetectsTestAfterRegistration) {
  state_.VerifyRegisteredTestNames("foo.cc", 1, "A, B, C");
  EXPECT_DEATH_IF_SUPPORTED(
      state_.AddTestName("foo.cc", 2, "FooTest", "D"),
      "foo\\.cc.2.?: Test D must be defined before REGISTER_TYPED_TEST_CASE_P"
      "\\(FooTest, \\.\\.\\.\\)\\.");
}




template <typename T>
class DerivedTest : public CommonTest<T> {
};

TYPED_TEST_CASE_P(DerivedTest);

TYPED_TEST_P(DerivedTest, ValuesAreCorrect) {
  
  
  EXPECT_EQ(5, *TestFixture::shared_);

  
  
  EXPECT_EQ(2, this->value_);
}



TYPED_TEST_P(DerivedTest, ValuesAreStillCorrect) {
  
  
  ASSERT_TRUE(this->shared_ != NULL);
  EXPECT_EQ(5, *this->shared_);
  EXPECT_EQ(2, this->value_);
}

REGISTER_TYPED_TEST_CASE_P(DerivedTest,
                           ValuesAreCorrect, ValuesAreStillCorrect);

typedef Types<short, long> MyTwoTypes;
INSTANTIATE_TYPED_TEST_CASE_P(My, DerivedTest, MyTwoTypes);




template <typename T>
class TypedTestP1 : public Test {
};

TYPED_TEST_CASE_P(TypedTestP1);



typedef int IntAfterTypedTestCaseP;

TYPED_TEST_P(TypedTestP1, A) {}
TYPED_TEST_P(TypedTestP1, B) {}



typedef int IntBeforeRegisterTypedTestCaseP;

REGISTER_TYPED_TEST_CASE_P(TypedTestP1, A, B);

template <typename T>
class TypedTestP2 : public Test {
};

TYPED_TEST_CASE_P(TypedTestP2);



TYPED_TEST_P(TypedTestP2, A) {}

REGISTER_TYPED_TEST_CASE_P(TypedTestP2, A);



IntAfterTypedTestCaseP after = 0;
IntBeforeRegisterTypedTestCaseP before = 0;



INSTANTIATE_TYPED_TEST_CASE_P(Int, TypedTestP1, int);
INSTANTIATE_TYPED_TEST_CASE_P(Int, TypedTestP2, Types<int>);



INSTANTIATE_TYPED_TEST_CASE_P(Double, TypedTestP2, Types<double>);




typedef Types<std::vector<double>, std::set<char> > MyContainers;
INSTANTIATE_TYPED_TEST_CASE_P(My, ContainerTest, MyContainers);




namespace library2 {

template <typename T>
class NumericTest : public Test {
};

TYPED_TEST_CASE_P(NumericTest);

TYPED_TEST_P(NumericTest, DefaultIsZero) {
  EXPECT_EQ(0, TypeParam());
}

TYPED_TEST_P(NumericTest, ZeroIsLessThanOne) {
  EXPECT_LT(TypeParam(0), TypeParam(1));
}

REGISTER_TYPED_TEST_CASE_P(NumericTest,
                           DefaultIsZero, ZeroIsLessThanOne);
typedef Types<int, double> NumericTypes;
INSTANTIATE_TYPED_TEST_CASE_P(My, NumericTest, NumericTypes);

}  

#endif  

#if !defined(GTEST_HAS_TYPED_TEST) && !defined(GTEST_HAS_TYPED_TEST_P)







TEST(DummyTest, TypedTestsAreNotSupportedOnThisPlatform) {}

#endif  
