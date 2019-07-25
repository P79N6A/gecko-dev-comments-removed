


































#include "gmock/gmock-spec-builders.h"

#include <ostream>  
#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gmock/internal/gmock-port.h"
#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"
#include "gtest/internal/gtest-port.h"

namespace testing {
namespace internal {


class ExpectationTester {
 public:
  
  void SetCallCount(int n, ExpectationBase* exp) {
    exp->call_count_ = n;
  }
};

}  
}  

namespace {

using testing::_;
using testing::AnyNumber;
using testing::AtLeast;
using testing::AtMost;
using testing::Between;
using testing::Cardinality;
using testing::CardinalityInterface;
using testing::ContainsRegex;
using testing::Const;
using testing::DoAll;
using testing::DoDefault;
using testing::Eq;
using testing::Expectation;
using testing::ExpectationSet;
using testing::GMOCK_FLAG(verbose);
using testing::Gt;
using testing::InSequence;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::IsSubstring;
using testing::Lt;
using testing::Message;
using testing::Mock;
using testing::Ne;
using testing::Return;
using testing::Sequence;
using testing::internal::ExpectationTester;
using testing::internal::FormatFileLocation;
using testing::internal::g_gmock_mutex;
using testing::internal::kErrorVerbosity;
using testing::internal::kInfoVerbosity;
using testing::internal::kWarningVerbosity;
using testing::internal::String;
using testing::internal::string;

#if GTEST_HAS_STREAM_REDIRECTION
using testing::HasSubstr;
using testing::internal::CaptureStdout;
using testing::internal::GetCapturedStdout;
#endif

class Incomplete;

class MockIncomplete {
 public:
  
  
  MOCK_METHOD1(ByRefFunc, void(const Incomplete& x));
};


void PrintTo(const Incomplete& x, ::std::ostream* os);

TEST(MockMethodTest, CanInstantiateWithIncompleteArgType) {
  
  
  
  
  MockIncomplete incomplete;
  EXPECT_CALL(incomplete, ByRefFunc(_))
      .Times(AnyNumber());
}



void PrintTo(const Incomplete& , ::std::ostream* os) {
  *os << "incomplete";
}

class Result {};

class MockA {
 public:
  MockA() {}

  MOCK_METHOD1(DoA, void(int n));  
  MOCK_METHOD1(ReturnResult, Result(int n));  
  MOCK_METHOD2(Binary, bool(int x, int y));  
  MOCK_METHOD2(ReturnInt, int(int x, int y));  

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockA);
};

class MockB {
 public:
  MockB() {}

  MOCK_CONST_METHOD0(DoB, int());  
  MOCK_METHOD1(DoB, int(int n));  

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockB);
};






#define Method MethodW

class CC {
 public:
  virtual ~CC() {}
  virtual int Method() = 0;
};
class MockCC : public CC {
 public:
  MockCC() {}

  MOCK_METHOD0(Method, int());

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockCC);
};


TEST(OnCallSyntaxTest, CompilesWithMethodNameExpandedFromMacro) {
  MockCC cc;
  ON_CALL(cc, Method());
}



TEST(OnCallSyntaxTest, WorksWithMethodNameExpandedFromMacro) {
  MockCC cc;
  ON_CALL(cc, Method()).WillByDefault(Return(42));
  EXPECT_EQ(42, cc.Method());
}


TEST(ExpectCallSyntaxTest, CompilesWithMethodNameExpandedFromMacro) {
  MockCC cc;
  EXPECT_CALL(cc, Method());
  cc.Method();
}


TEST(ExpectCallSyntaxTest, WorksWithMethodNameExpandedFromMacro) {
  MockCC cc;
  EXPECT_CALL(cc, Method()).WillOnce(Return(42));
  EXPECT_EQ(42, cc.Method());
}

#undef Method  // Done with macro redefinition tests.



TEST(OnCallSyntaxTest, EvaluatesFirstArgumentOnce) {
  MockA a;
  MockA* pa = &a;

  ON_CALL(*pa++, DoA(_));
  EXPECT_EQ(&a + 1, pa);
}

TEST(OnCallSyntaxTest, EvaluatesSecondArgumentOnce) {
  MockA a;
  int n = 0;

  ON_CALL(a, DoA(n++));
  EXPECT_EQ(1, n);
}



TEST(OnCallSyntaxTest, WithIsOptional) {
  MockA a;

  ON_CALL(a, DoA(5))
      .WillByDefault(Return());
  ON_CALL(a, DoA(_))
      .With(_)
      .WillByDefault(Return());
}

TEST(OnCallSyntaxTest, WithCanAppearAtMostOnce) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    ON_CALL(a, ReturnResult(_))
        .With(_)
        .With(_)
        .WillByDefault(Return(Result()));
  }, ".With() cannot appear more than once in an ON_CALL()");
}

TEST(OnCallSyntaxTest, WillByDefaultIsMandatory) {
  MockA a;

  EXPECT_DEATH_IF_SUPPORTED({
    ON_CALL(a, DoA(5));
    a.DoA(5);
  }, "");
}

TEST(OnCallSyntaxTest, WillByDefaultCanAppearAtMostOnce) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    ON_CALL(a, DoA(5))
        .WillByDefault(Return())
        .WillByDefault(Return());
  }, ".WillByDefault() must appear exactly once in an ON_CALL()");
}



TEST(ExpectCallSyntaxTest, EvaluatesFirstArgumentOnce) {
  MockA a;
  MockA* pa = &a;

  EXPECT_CALL(*pa++, DoA(_));
  a.DoA(0);
  EXPECT_EQ(&a + 1, pa);
}

TEST(ExpectCallSyntaxTest, EvaluatesSecondArgumentOnce) {
  MockA a;
  int n = 0;

  EXPECT_CALL(a, DoA(n++));
  a.DoA(0);
  EXPECT_EQ(1, n);
}



TEST(ExpectCallSyntaxTest, WithIsOptional) {
  MockA a;

  EXPECT_CALL(a, DoA(5))
      .Times(0);
  EXPECT_CALL(a, DoA(6))
      .With(_)
      .Times(0);
}

TEST(ExpectCallSyntaxTest, WithCanAppearAtMostOnce) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(6))
        .With(_)
        .With(_);
  }, ".With() cannot appear more than once in an EXPECT_CALL()");

  a.DoA(6);
}

TEST(ExpectCallSyntaxTest, WithMustBeFirstClause) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .Times(1)
        .With(_);
  }, ".With() must be the first clause in an EXPECT_CALL()");

  a.DoA(1);

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(2))
        .WillOnce(Return())
        .With(_);
  }, ".With() must be the first clause in an EXPECT_CALL()");

  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, TimesCanBeInferred) {
  MockA a;

  EXPECT_CALL(a, DoA(1))
      .WillOnce(Return());

  EXPECT_CALL(a, DoA(2))
      .WillOnce(Return())
      .WillRepeatedly(Return());

  a.DoA(1);
  a.DoA(2);
  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, TimesCanAppearAtMostOnce) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .Times(1)
        .Times(2);
  }, ".Times() cannot appear more than once in an EXPECT_CALL()");

  a.DoA(1);
  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, TimesMustBeBeforeInSequence) {
  MockA a;
  Sequence s;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .InSequence(s)
        .Times(1);
  }, ".Times() cannot appear after ");

  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, InSequenceIsOptional) {
  MockA a;
  Sequence s;

  EXPECT_CALL(a, DoA(1));
  EXPECT_CALL(a, DoA(2))
      .InSequence(s);

  a.DoA(1);
  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, InSequenceCanAppearMultipleTimes) {
  MockA a;
  Sequence s1, s2;

  EXPECT_CALL(a, DoA(1))
      .InSequence(s1, s2)
      .InSequence(s1);

  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, InSequenceMustBeBeforeAfter) {
  MockA a;
  Sequence s;

  Expectation e = EXPECT_CALL(a, DoA(1))
      .Times(AnyNumber());
  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(2))
        .After(e)
        .InSequence(s);
  }, ".InSequence() cannot appear after ");

  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, InSequenceMustBeBeforeWillOnce) {
  MockA a;
  Sequence s;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .WillOnce(Return())
        .InSequence(s);
  }, ".InSequence() cannot appear after ");

  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, AfterMustBeBeforeWillOnce) {
  MockA a;

  Expectation e = EXPECT_CALL(a, DoA(1));
  EXPECT_NONFATAL_FAILURE({
    EXPECT_CALL(a, DoA(2))
        .WillOnce(Return())
        .After(e);
  }, ".After() cannot appear after ");

  a.DoA(1);
  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, WillIsOptional) {
  MockA a;

  EXPECT_CALL(a, DoA(1));
  EXPECT_CALL(a, DoA(2))
      .WillOnce(Return());

  a.DoA(1);
  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, WillCanAppearMultipleTimes) {
  MockA a;

  EXPECT_CALL(a, DoA(1))
      .Times(AnyNumber())
      .WillOnce(Return())
      .WillOnce(Return())
      .WillOnce(Return());
}

TEST(ExpectCallSyntaxTest, WillMustBeBeforeWillRepeatedly) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .WillRepeatedly(Return())
        .WillOnce(Return());
  }, ".WillOnce() cannot appear after ");

  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, WillRepeatedlyIsOptional) {
  MockA a;

  EXPECT_CALL(a, DoA(1))
      .WillOnce(Return());
  EXPECT_CALL(a, DoA(2))
      .WillOnce(Return())
      .WillRepeatedly(Return());

  a.DoA(1);
  a.DoA(2);
  a.DoA(2);
}

TEST(ExpectCallSyntaxTest, WillRepeatedlyCannotAppearMultipleTimes) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .WillRepeatedly(Return())
        .WillRepeatedly(Return());
  }, ".WillRepeatedly() cannot appear more than once in an "
     "EXPECT_CALL()");
}

TEST(ExpectCallSyntaxTest, WillRepeatedlyMustBeBeforeRetiresOnSaturation) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .RetiresOnSaturation()
        .WillRepeatedly(Return());
  }, ".WillRepeatedly() cannot appear after ");
}

TEST(ExpectCallSyntaxTest, RetiresOnSaturationIsOptional) {
  MockA a;

  EXPECT_CALL(a, DoA(1));
  EXPECT_CALL(a, DoA(1))
      .RetiresOnSaturation();

  a.DoA(1);
  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, RetiresOnSaturationCannotAppearMultipleTimes) {
  MockA a;

  EXPECT_NONFATAL_FAILURE({  
    EXPECT_CALL(a, DoA(1))
        .RetiresOnSaturation()
        .RetiresOnSaturation();
  }, ".RetiresOnSaturation() cannot appear more than once");

  a.DoA(1);
}

TEST(ExpectCallSyntaxTest, DefaultCardinalityIsOnce) {
  {
    MockA a;
    EXPECT_CALL(a, DoA(1));
    a.DoA(1);
  }
  EXPECT_NONFATAL_FAILURE({  
    MockA a;
    EXPECT_CALL(a, DoA(1));
  }, "to be called once");
  EXPECT_NONFATAL_FAILURE({  
    MockA a;
    EXPECT_CALL(a, DoA(1));
    a.DoA(1);
    a.DoA(1);
  }, "to be called once");
}

#if GTEST_HAS_STREAM_REDIRECTION



TEST(ExpectCallSyntaxTest, DoesNotWarnOnAdequateActionCount) {
  CaptureStdout();
  {
    MockB b;

    
    EXPECT_CALL(b, DoB())
        .Times(0);
    EXPECT_CALL(b, DoB(1))
        .Times(AtMost(1));
    EXPECT_CALL(b, DoB(2))
        .Times(1)
        .WillRepeatedly(Return(1));

    
    EXPECT_CALL(b, DoB(3))
        .Times(Between(1, 2))
        .WillOnce(Return(1))
        .WillOnce(Return(2));

    
    
    EXPECT_CALL(b, DoB(4))
        .Times(AtMost(3))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(2));

    
    b.DoB(2);
    b.DoB(3);
  }
  EXPECT_STREQ("", GetCapturedStdout().c_str());
}



TEST(ExpectCallSyntaxTest, WarnsOnTooManyActions) {
  CaptureStdout();
  {
    MockB b;

    
    EXPECT_CALL(b, DoB())
        .Times(0)
        .WillOnce(Return(1));  
    EXPECT_CALL(b, DoB())
        .Times(AtMost(1))
        .WillOnce(Return(1))
        .WillOnce(Return(2));  
    EXPECT_CALL(b, DoB(1))
        .Times(1)
        .WillOnce(Return(1))
        .WillOnce(Return(2))
        .RetiresOnSaturation();  

    
    
    EXPECT_CALL(b, DoB())
        .Times(0)
        .WillRepeatedly(Return(1));  
    EXPECT_CALL(b, DoB(2))
        .Times(1)
        .WillOnce(Return(1))
        .WillRepeatedly(Return(2));  

    
    b.DoB(1);
    b.DoB(2);
  }
  const String output = GetCapturedStdout();
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Too many actions specified in EXPECT_CALL(b, DoB())...\n"
      "Expected to be never called, but has 1 WillOnce().",
      output);  
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Too many actions specified in EXPECT_CALL(b, DoB())...\n"
      "Expected to be called at most once, "
      "but has 2 WillOnce()s.",
      output);  
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Too many actions specified in EXPECT_CALL(b, DoB(1))...\n"
      "Expected to be called once, but has 2 WillOnce()s.",
      output);  
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Too many actions specified in EXPECT_CALL(b, DoB())...\n"
      "Expected to be never called, but has 0 WillOnce()s "
      "and a WillRepeatedly().",
      output);  
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Too many actions specified in EXPECT_CALL(b, DoB(2))...\n"
      "Expected to be called once, but has 1 WillOnce() "
      "and a WillRepeatedly().",
      output);  
}



TEST(ExpectCallSyntaxTest, WarnsOnTooFewActions) {
  MockB b;

  EXPECT_CALL(b, DoB())
      .Times(Between(2, 3))
      .WillOnce(Return(1));

  CaptureStdout();
  b.DoB();
  const String output = GetCapturedStdout();
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Too few actions specified in EXPECT_CALL(b, DoB())...\n"
      "Expected to be called between 2 and 3 times, "
      "but has only 1 WillOnce().",
      output);
  b.DoB();
}

#endif  





TEST(OnCallTest, TakesBuiltInDefaultActionWhenNoOnCall) {
  MockB b;
  EXPECT_CALL(b, DoB());

  EXPECT_EQ(0, b.DoB());
}



TEST(OnCallTest, TakesBuiltInDefaultActionWhenNoOnCallMatches) {
  MockB b;
  ON_CALL(b, DoB(1))
      .WillByDefault(Return(1));
  EXPECT_CALL(b, DoB(_));

  EXPECT_EQ(0, b.DoB(2));
}


TEST(OnCallTest, PicksLastMatchingOnCall) {
  MockB b;
  ON_CALL(b, DoB(_))
      .WillByDefault(Return(3));
  ON_CALL(b, DoB(2))
      .WillByDefault(Return(2));
  ON_CALL(b, DoB(1))
      .WillByDefault(Return(1));
  EXPECT_CALL(b, DoB(_));

  EXPECT_EQ(2, b.DoB(2));
}




TEST(ExpectCallTest, AllowsAnyCallWhenNoSpec) {
  MockB b;
  EXPECT_CALL(b, DoB());
  

  b.DoB();

  
  b.DoB(1);
  b.DoB(2);
}


TEST(ExpectCallTest, PicksLastMatchingExpectCall) {
  MockB b;
  EXPECT_CALL(b, DoB(_))
      .WillRepeatedly(Return(2));
  EXPECT_CALL(b, DoB(1))
      .WillRepeatedly(Return(1));

  EXPECT_EQ(1, b.DoB(1));
}


TEST(ExpectCallTest, CatchesTooFewCalls) {
  EXPECT_NONFATAL_FAILURE({  
    MockB b;
    EXPECT_CALL(b, DoB(5))
        .Times(AtLeast(2));

    b.DoB(5);
  }, "Actual function call count doesn't match EXPECT_CALL(b, DoB(5))...\n"
     "         Expected: to be called at least twice\n"
     "           Actual: called once - unsatisfied and active");
}



TEST(ExpectCallTest, InfersCardinalityWhenThereIsNoWillRepeatedly) {
  {
    MockB b;
    EXPECT_CALL(b, DoB())
        .WillOnce(Return(1))
        .WillOnce(Return(2));

    EXPECT_EQ(1, b.DoB());
    EXPECT_EQ(2, b.DoB());
  }

  EXPECT_NONFATAL_FAILURE({  
    MockB b;
    EXPECT_CALL(b, DoB())
        .WillOnce(Return(1))
        .WillOnce(Return(2));

    EXPECT_EQ(1, b.DoB());
  }, "to be called twice");

  {  
    MockB b;
    EXPECT_CALL(b, DoB())
        .WillOnce(Return(1))
        .WillOnce(Return(2));

    EXPECT_EQ(1, b.DoB());
    EXPECT_EQ(2, b.DoB());
    EXPECT_NONFATAL_FAILURE(b.DoB(), "to be called twice");
  }
}

TEST(ExpectCallTest, InfersCardinality1WhenThereIsWillRepeatedly) {
  {
    MockB b;
    EXPECT_CALL(b, DoB())
        .WillOnce(Return(1))
        .WillRepeatedly(Return(2));

    EXPECT_EQ(1, b.DoB());
  }

  {  
    MockB b;
    EXPECT_CALL(b, DoB())
        .WillOnce(Return(1))
        .WillRepeatedly(Return(2));

    EXPECT_EQ(1, b.DoB());
    EXPECT_EQ(2, b.DoB());
    EXPECT_EQ(2, b.DoB());
  }

  EXPECT_NONFATAL_FAILURE({  
    MockB b;
    EXPECT_CALL(b, DoB())
        .WillOnce(Return(1))
        .WillRepeatedly(Return(2));
  }, "to be called at least once");
}



TEST(ExpectCallTest, NthMatchTakesNthAction) {
  MockB b;
  EXPECT_CALL(b, DoB())
      .WillOnce(Return(1))
      .WillOnce(Return(2))
      .WillOnce(Return(3));

  EXPECT_EQ(1, b.DoB());
  EXPECT_EQ(2, b.DoB());
  EXPECT_EQ(3, b.DoB());
}



TEST(ExpectCallTest, TakesRepeatedActionWhenWillListIsExhausted) {
  MockB b;
  EXPECT_CALL(b, DoB())
      .WillOnce(Return(1))
      .WillRepeatedly(Return(2));

  EXPECT_EQ(1, b.DoB());
  EXPECT_EQ(2, b.DoB());
  EXPECT_EQ(2, b.DoB());
}

#if GTEST_HAS_STREAM_REDIRECTION



TEST(ExpectCallTest, TakesDefaultActionWhenWillListIsExhausted) {
  MockB b;
  EXPECT_CALL(b, DoB(_))
      .Times(1);
  EXPECT_CALL(b, DoB())
      .Times(AnyNumber())
      .WillOnce(Return(1))
      .WillOnce(Return(2));

  CaptureStdout();
  EXPECT_EQ(0, b.DoB(1));  
                           
  EXPECT_EQ(1, b.DoB());
  EXPECT_EQ(2, b.DoB());
  const String output1 = GetCapturedStdout();
  EXPECT_STREQ("", output1.c_str());

  CaptureStdout();
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB());
  const String output2 = GetCapturedStdout();
  EXPECT_THAT(output2.c_str(),
              HasSubstr("Actions ran out in EXPECT_CALL(b, DoB())...\n"
                        "Called 3 times, but only 2 WillOnce()s are specified"
                        " - returning default value."));
  EXPECT_THAT(output2.c_str(),
              HasSubstr("Actions ran out in EXPECT_CALL(b, DoB())...\n"
                        "Called 4 times, but only 2 WillOnce()s are specified"
                        " - returning default value."));
}

TEST(FunctionMockerTest, ReportsExpectCallLocationForExhausedActions) {
  MockB b;
  std::string expect_call_location = FormatFileLocation(__FILE__, __LINE__ + 1);
  EXPECT_CALL(b, DoB()).Times(AnyNumber()).WillOnce(Return(1));

  EXPECT_EQ(1, b.DoB());

  CaptureStdout();
  EXPECT_EQ(0, b.DoB());
  const String output = GetCapturedStdout();
  
  EXPECT_PRED_FORMAT2(IsSubstring, expect_call_location, output);
}

TEST(FunctionMockerTest, ReportsDefaultActionLocationOfUninterestingCalls) {
  std::string on_call_location;
  CaptureStdout();
  {
    MockB b;
    on_call_location = FormatFileLocation(__FILE__, __LINE__ + 1);
    ON_CALL(b, DoB(_)).WillByDefault(Return(0));
    b.DoB(0);
  }
  EXPECT_PRED_FORMAT2(IsSubstring, on_call_location, GetCapturedStdout());
}

#endif  


TEST(UninterestingCallTest, DoesDefaultAction) {
  
  
  MockA a;
  ON_CALL(a, Binary(_, _))
      .WillByDefault(Return(true));
  EXPECT_TRUE(a.Binary(1, 2));

  
  
  MockB b;
  EXPECT_EQ(0, b.DoB());
}


TEST(UnexpectedCallTest, DoesDefaultAction) {
  
  
  MockA a;
  ON_CALL(a, Binary(_, _))
      .WillByDefault(Return(true));
  EXPECT_CALL(a, Binary(0, 0));
  a.Binary(0, 0);
  bool result = false;
  EXPECT_NONFATAL_FAILURE(result = a.Binary(1, 2),
                          "Unexpected mock function call");
  EXPECT_TRUE(result);

  
  
  MockB b;
  EXPECT_CALL(b, DoB(0))
      .Times(0);
  int n = -1;
  EXPECT_NONFATAL_FAILURE(n = b.DoB(1),
                          "Unexpected mock function call");
  EXPECT_EQ(0, n);
}



TEST(UnexpectedCallTest, GeneratesFailureForVoidFunction) {
  
  MockA a1;
  EXPECT_CALL(a1, DoA(1));
  a1.DoA(1);
  
  
  
  EXPECT_NONFATAL_FAILURE(
      a1.DoA(9),
      "Unexpected mock function call - returning directly.\n"
      "    Function call: DoA(9)\n"
      "Google Mock tried the following 1 expectation, but it didn't match:");
  EXPECT_NONFATAL_FAILURE(
      a1.DoA(9),
      "  Expected arg #0: is equal to 1\n"
      "           Actual: 9\n"
      "         Expected: to be called once\n"
      "           Actual: called once - saturated and active");

  
  MockA a2;
  EXPECT_CALL(a2, DoA(1));
  EXPECT_CALL(a2, DoA(3));
  a2.DoA(1);
  EXPECT_NONFATAL_FAILURE(
      a2.DoA(2),
      "Unexpected mock function call - returning directly.\n"
      "    Function call: DoA(2)\n"
      "Google Mock tried the following 2 expectations, but none matched:");
  EXPECT_NONFATAL_FAILURE(
      a2.DoA(2),
      "tried expectation #0: EXPECT_CALL(a2, DoA(1))...\n"
      "  Expected arg #0: is equal to 1\n"
      "           Actual: 2\n"
      "         Expected: to be called once\n"
      "           Actual: called once - saturated and active");
  EXPECT_NONFATAL_FAILURE(
      a2.DoA(2),
      "tried expectation #1: EXPECT_CALL(a2, DoA(3))...\n"
      "  Expected arg #0: is equal to 3\n"
      "           Actual: 2\n"
      "         Expected: to be called once\n"
      "           Actual: never called - unsatisfied and active");
  a2.DoA(3);
}



TEST(UnexpectedCallTest, GeneartesFailureForNonVoidFunction) {
  MockB b1;
  EXPECT_CALL(b1, DoB(1));
  b1.DoB(1);
  EXPECT_NONFATAL_FAILURE(
      b1.DoB(2),
      "Unexpected mock function call - returning default value.\n"
      "    Function call: DoB(2)\n"
      "          Returns: 0\n"
      "Google Mock tried the following 1 expectation, but it didn't match:");
  EXPECT_NONFATAL_FAILURE(
      b1.DoB(2),
      "  Expected arg #0: is equal to 1\n"
      "           Actual: 2\n"
      "         Expected: to be called once\n"
      "           Actual: called once - saturated and active");
}



TEST(UnexpectedCallTest, RetiredExpectation) {
  MockB b;
  EXPECT_CALL(b, DoB(1))
      .RetiresOnSaturation();

  b.DoB(1);
  EXPECT_NONFATAL_FAILURE(
      b.DoB(1),
      "         Expected: the expectation is active\n"
      "           Actual: it is retired");
}



TEST(UnexpectedCallTest, UnmatchedArguments) {
  MockB b;
  EXPECT_CALL(b, DoB(1));

  EXPECT_NONFATAL_FAILURE(
      b.DoB(2),
      "  Expected arg #0: is equal to 1\n"
      "           Actual: 2\n");
  b.DoB(1);
}



TEST(UnexpectedCallTest, UnsatisifiedPrerequisites) {
  Sequence s1, s2;
  MockB b;
  EXPECT_CALL(b, DoB(1))
      .InSequence(s1);
  EXPECT_CALL(b, DoB(2))
      .Times(AnyNumber())
      .InSequence(s1);
  EXPECT_CALL(b, DoB(3))
      .InSequence(s2);
  EXPECT_CALL(b, DoB(4))
      .InSequence(s1, s2);

  ::testing::TestPartResultArray failures;
  {
    ::testing::ScopedFakeTestPartResultReporter reporter(&failures);
    b.DoB(4);
    
    
  }

  
  ASSERT_EQ(1, failures.size());
  const ::testing::TestPartResult& r = failures.GetTestPartResult(0);
  EXPECT_EQ(::testing::TestPartResult::kNonFatalFailure, r.type());

  
  
#if GTEST_USES_PCRE
  EXPECT_THAT(r.message(), ContainsRegex(
      
      
      "(?s)the following immediate pre-requisites are not satisfied:\n"
      ".*: pre-requisite #0\n"
      ".*: pre-requisite #1"));
#elif GTEST_USES_POSIX_RE
  EXPECT_THAT(r.message(), ContainsRegex(
      
      
      "the following immediate pre-requisites are not satisfied:\n"
      "(.|\n)*: pre-requisite #0\n"
      "(.|\n)*: pre-requisite #1"));
#else
  
  EXPECT_THAT(r.message(), ContainsRegex(
      "the following immediate pre-requisites are not satisfied:"));
  EXPECT_THAT(r.message(), ContainsRegex(": pre-requisite #0"));
  EXPECT_THAT(r.message(), ContainsRegex(": pre-requisite #1"));
#endif  

  b.DoB(1);
  b.DoB(3);
  b.DoB(4);
}

TEST(UndefinedReturnValueTest, ReturnValueIsMandatory) {
  MockA a;
  
  
  
  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(1), "");
}



TEST(ExcessiveCallTest, DoesDefaultAction) {
  
  
  MockA a;
  ON_CALL(a, Binary(_, _))
      .WillByDefault(Return(true));
  EXPECT_CALL(a, Binary(0, 0));
  a.Binary(0, 0);
  bool result = false;
  EXPECT_NONFATAL_FAILURE(result = a.Binary(0, 0),
                          "Mock function called more times than expected");
  EXPECT_TRUE(result);

  
  
  MockB b;
  EXPECT_CALL(b, DoB(0))
      .Times(0);
  int n = -1;
  EXPECT_NONFATAL_FAILURE(n = b.DoB(0),
                          "Mock function called more times than expected");
  EXPECT_EQ(0, n);
}



TEST(ExcessiveCallTest, GeneratesFailureForVoidFunction) {
  MockA a;
  EXPECT_CALL(a, DoA(_))
      .Times(0);
  EXPECT_NONFATAL_FAILURE(
      a.DoA(9),
      "Mock function called more times than expected - returning directly.\n"
      "    Function call: DoA(9)\n"
      "         Expected: to be never called\n"
      "           Actual: called once - over-saturated and active");
}



TEST(ExcessiveCallTest, GeneratesFailureForNonVoidFunction) {
  MockB b;
  EXPECT_CALL(b, DoB(_));
  b.DoB(1);
  EXPECT_NONFATAL_FAILURE(
      b.DoB(2),
      "Mock function called more times than expected - "
      "returning default value.\n"
      "    Function call: DoB(2)\n"
      "          Returns: 0\n"
      "         Expected: to be called once\n"
      "           Actual: called twice - over-saturated and active");
}



TEST(InSequenceTest, AllExpectationInScopeAreInSequence) {
  MockA a;
  {
    InSequence dummy;

    EXPECT_CALL(a, DoA(1));
    EXPECT_CALL(a, DoA(2));
  }

  EXPECT_NONFATAL_FAILURE({  
    a.DoA(2);
  }, "Unexpected mock function call");

  a.DoA(1);
  a.DoA(2);
}

TEST(InSequenceTest, NestedInSequence) {
  MockA a;
  {
    InSequence dummy;

    EXPECT_CALL(a, DoA(1));
    {
      InSequence dummy2;

      EXPECT_CALL(a, DoA(2));
      EXPECT_CALL(a, DoA(3));
    }
  }

  EXPECT_NONFATAL_FAILURE({  
    a.DoA(1);
    a.DoA(3);
  }, "Unexpected mock function call");

  a.DoA(2);
  a.DoA(3);
}

TEST(InSequenceTest, ExpectationsOutOfScopeAreNotAffected) {
  MockA a;
  {
    InSequence dummy;

    EXPECT_CALL(a, DoA(1));
    EXPECT_CALL(a, DoA(2));
  }
  EXPECT_CALL(a, DoA(3));

  EXPECT_NONFATAL_FAILURE({  
    a.DoA(2);
  }, "Unexpected mock function call");

  a.DoA(3);
  a.DoA(1);
  a.DoA(2);
}


TEST(SequenceTest, AnyOrderIsOkByDefault) {
  {
    MockA a;
    MockB b;

    EXPECT_CALL(a, DoA(1));
    EXPECT_CALL(b, DoB())
        .Times(AnyNumber());

    a.DoA(1);
    b.DoB();
  }

  {  
    MockA a;
    MockB b;

    EXPECT_CALL(a, DoA(1));
    EXPECT_CALL(b, DoB())
        .Times(AnyNumber());

    b.DoB();
    a.DoA(1);
  }
}



TEST(SequenceTest, CallsMustBeInStrictOrderWhenSaidSo) {
  MockA a;
  Sequence s;

  EXPECT_CALL(a, ReturnResult(1))
      .InSequence(s)
      .WillOnce(Return(Result()));

  EXPECT_CALL(a, ReturnResult(2))
      .InSequence(s)
      .WillOnce(Return(Result()));

  EXPECT_CALL(a, ReturnResult(3))
      .InSequence(s)
      .WillOnce(Return(Result()));

  EXPECT_DEATH_IF_SUPPORTED({
    a.ReturnResult(1);
    a.ReturnResult(3);
    a.ReturnResult(2);
  }, "");

  EXPECT_DEATH_IF_SUPPORTED({
    a.ReturnResult(2);
    a.ReturnResult(1);
    a.ReturnResult(3);
  }, "");

  a.ReturnResult(1);
  a.ReturnResult(2);
  a.ReturnResult(3);
}


TEST(SequenceTest, CallsMustConformToSpecifiedDag) {
  MockA a;
  MockB b;
  Sequence x, y;

  EXPECT_CALL(a, ReturnResult(1))
      .InSequence(x)
      .WillOnce(Return(Result()));

  EXPECT_CALL(b, DoB())
      .Times(2)
      .InSequence(y);

  EXPECT_CALL(a, ReturnResult(2))
      .InSequence(x, y)
      .WillRepeatedly(Return(Result()));

  EXPECT_CALL(a, ReturnResult(3))
      .InSequence(x)
      .WillOnce(Return(Result()));

  EXPECT_DEATH_IF_SUPPORTED({
    a.ReturnResult(1);
    b.DoB();
    a.ReturnResult(2);
  }, "");

  EXPECT_DEATH_IF_SUPPORTED({
    a.ReturnResult(2);
  }, "");

  EXPECT_DEATH_IF_SUPPORTED({
    a.ReturnResult(3);
  }, "");

  EXPECT_DEATH_IF_SUPPORTED({
    a.ReturnResult(1);
    b.DoB();
    b.DoB();
    a.ReturnResult(3);
    a.ReturnResult(2);
  }, "");

  b.DoB();
  a.ReturnResult(1);
  b.DoB();
  a.ReturnResult(3);
}

TEST(SequenceTest, Retirement) {
  MockA a;
  Sequence s;

  EXPECT_CALL(a, DoA(1))
      .InSequence(s);
  EXPECT_CALL(a, DoA(_))
      .InSequence(s)
      .RetiresOnSaturation();
  EXPECT_CALL(a, DoA(1))
      .InSequence(s);

  a.DoA(1);
  a.DoA(2);
  a.DoA(1);
}



TEST(ExpectationTest, ConstrutorsWork) {
  MockA a;
  Expectation e1;  

  
  Expectation e2 = EXPECT_CALL(a, DoA(2));
  Expectation e3 = EXPECT_CALL(a, DoA(3)).With(_);
  {
    Sequence s;
    Expectation e4 = EXPECT_CALL(a, DoA(4)).Times(1);
    Expectation e5 = EXPECT_CALL(a, DoA(5)).InSequence(s);
  }
  Expectation e6 = EXPECT_CALL(a, DoA(6)).After(e2);
  Expectation e7 = EXPECT_CALL(a, DoA(7)).WillOnce(Return());
  Expectation e8 = EXPECT_CALL(a, DoA(8)).WillRepeatedly(Return());
  Expectation e9 = EXPECT_CALL(a, DoA(9)).RetiresOnSaturation();

  Expectation e10 = e2;  

  EXPECT_THAT(e1, Ne(e2));
  EXPECT_THAT(e2, Eq(e10));

  a.DoA(2);
  a.DoA(3);
  a.DoA(4);
  a.DoA(5);
  a.DoA(6);
  a.DoA(7);
  a.DoA(8);
  a.DoA(9);
}

TEST(ExpectationTest, AssignmentWorks) {
  MockA a;
  Expectation e1;
  Expectation e2 = EXPECT_CALL(a, DoA(1));

  EXPECT_THAT(e1, Ne(e2));

  e1 = e2;
  EXPECT_THAT(e1, Eq(e2));

  a.DoA(1);
}



TEST(ExpectationSetTest, MemberTypesAreCorrect) {
  ::testing::StaticAssertTypeEq<Expectation, ExpectationSet::value_type>();
}

TEST(ExpectationSetTest, ConstructorsWork) {
  MockA a;

  Expectation e1;
  const Expectation e2;
  ExpectationSet es1;  
  ExpectationSet es2 = EXPECT_CALL(a, DoA(1));  
  ExpectationSet es3 = e1;  
  ExpectationSet es4(e1);   
  ExpectationSet es5 = e2;  
  ExpectationSet es6(e2);   
  ExpectationSet es7 = es2;  

  EXPECT_EQ(0, es1.size());
  EXPECT_EQ(1, es2.size());
  EXPECT_EQ(1, es3.size());
  EXPECT_EQ(1, es4.size());
  EXPECT_EQ(1, es5.size());
  EXPECT_EQ(1, es6.size());
  EXPECT_EQ(1, es7.size());

  EXPECT_THAT(es3, Ne(es2));
  EXPECT_THAT(es4, Eq(es3));
  EXPECT_THAT(es5, Eq(es4));
  EXPECT_THAT(es6, Eq(es5));
  EXPECT_THAT(es7, Eq(es2));
  a.DoA(1);
}

TEST(ExpectationSetTest, AssignmentWorks) {
  ExpectationSet es1;
  ExpectationSet es2 = Expectation();

  es1 = es2;
  EXPECT_EQ(1, es1.size());
  EXPECT_THAT(*(es1.begin()), Eq(Expectation()));
  EXPECT_THAT(es1, Eq(es2));
}

TEST(ExpectationSetTest, InsertionWorks) {
  ExpectationSet es1;
  Expectation e1;
  es1 += e1;
  EXPECT_EQ(1, es1.size());
  EXPECT_THAT(*(es1.begin()), Eq(e1));

  MockA a;
  Expectation e2 = EXPECT_CALL(a, DoA(1));
  es1 += e2;
  EXPECT_EQ(2, es1.size());

  ExpectationSet::const_iterator it1 = es1.begin();
  ExpectationSet::const_iterator it2 = it1;
  ++it2;
  EXPECT_TRUE(*it1 == e1 || *it2 == e1);  
  EXPECT_TRUE(*it1 == e2 || *it2 == e2);  
  a.DoA(1);
}

TEST(ExpectationSetTest, SizeWorks) {
  ExpectationSet es;
  EXPECT_EQ(0, es.size());

  es += Expectation();
  EXPECT_EQ(1, es.size());

  MockA a;
  es += EXPECT_CALL(a, DoA(1));
  EXPECT_EQ(2, es.size());

  a.DoA(1);
}

TEST(ExpectationSetTest, IsEnumerable) {
  ExpectationSet es;
  EXPECT_THAT(es.begin(), Eq(es.end()));

  es += Expectation();
  ExpectationSet::const_iterator it = es.begin();
  EXPECT_THAT(it, Ne(es.end()));
  EXPECT_THAT(*it, Eq(Expectation()));
  ++it;
  EXPECT_THAT(it, Eq(es.end()));
}



TEST(AfterTest, SucceedsWhenPartialOrderIsSatisfied) {
  MockA a;
  ExpectationSet es;
  es += EXPECT_CALL(a, DoA(1));
  es += EXPECT_CALL(a, DoA(2));
  EXPECT_CALL(a, DoA(3))
      .After(es);

  a.DoA(1);
  a.DoA(2);
  a.DoA(3);
}

TEST(AfterTest, SucceedsWhenTotalOrderIsSatisfied) {
  MockA a;
  MockB b;
  
  
  const Expectation e1 = EXPECT_CALL(a, DoA(1));
  const Expectation e2 = EXPECT_CALL(b, DoB())
      .Times(2)
      .After(e1);
  EXPECT_CALL(a, DoA(2)).After(e2);

  a.DoA(1);
  b.DoB();
  b.DoB();
  a.DoA(2);
}


TEST(AfterDeathTest, CallsMustBeInStrictOrderWhenSpecifiedSo) {
  MockA a;
  MockB b;
  Expectation e1 = EXPECT_CALL(a, DoA(1));
  Expectation e2 = EXPECT_CALL(b, DoB())
      .Times(2)
      .After(e1);
  EXPECT_CALL(a, ReturnResult(2))
      .After(e2)
      .WillOnce(Return(Result()));

  a.DoA(1);
  
  
  
  
  
  
  
  
  
  
  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(2), "");

  b.DoB();
  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(2), "");

  b.DoB();
  a.ReturnResult(2);
}


TEST(AfterDeathTest, CallsMustSatisfyPartialOrderWhenSpecifiedSo) {
  MockA a;
  Expectation e = EXPECT_CALL(a, DoA(1));
  const ExpectationSet es = EXPECT_CALL(a, DoA(2));
  EXPECT_CALL(a, ReturnResult(3))
      .After(e, es)
      .WillOnce(Return(Result()));

  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(3), "");

  a.DoA(2);
  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(3), "");

  a.DoA(1);
  a.ReturnResult(3);
}


TEST(AfterDeathTest, CanBeUsedWithInSequence) {
  MockA a;
  Sequence s;
  Expectation e = EXPECT_CALL(a, DoA(1));
  EXPECT_CALL(a, DoA(2)).InSequence(s);
  EXPECT_CALL(a, ReturnResult(3))
      .InSequence(s).After(e)
      .WillOnce(Return(Result()));

  a.DoA(1);
  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(3), "");

  a.DoA(2);
  a.ReturnResult(3);
}


TEST(AfterTest, CanBeCalledManyTimes) {
  MockA a;
  Expectation e1 = EXPECT_CALL(a, DoA(1));
  Expectation e2 = EXPECT_CALL(a, DoA(2));
  Expectation e3 = EXPECT_CALL(a, DoA(3));
  EXPECT_CALL(a, DoA(4))
      .After(e1)
      .After(e2)
      .After(e3);

  a.DoA(3);
  a.DoA(1);
  a.DoA(2);
  a.DoA(4);
}


TEST(AfterTest, AcceptsUpToFiveArguments) {
  MockA a;
  Expectation e1 = EXPECT_CALL(a, DoA(1));
  Expectation e2 = EXPECT_CALL(a, DoA(2));
  Expectation e3 = EXPECT_CALL(a, DoA(3));
  ExpectationSet es1 = EXPECT_CALL(a, DoA(4));
  ExpectationSet es2 = EXPECT_CALL(a, DoA(5));
  EXPECT_CALL(a, DoA(6))
      .After(e1, e2, e3, es1, es2);

  a.DoA(5);
  a.DoA(2);
  a.DoA(4);
  a.DoA(1);
  a.DoA(3);
  a.DoA(6);
}


TEST(AfterTest, AcceptsDuplicatedInput) {
  MockA a;
  Expectation e1 = EXPECT_CALL(a, DoA(1));
  Expectation e2 = EXPECT_CALL(a, DoA(2));
  ExpectationSet es;
  es += e1;
  es += e2;
  EXPECT_CALL(a, ReturnResult(3))
      .After(e1, e2, es, e1)
      .WillOnce(Return(Result()));

  a.DoA(1);
  EXPECT_DEATH_IF_SUPPORTED(a.ReturnResult(3), "");

  a.DoA(2);
  a.ReturnResult(3);
}



TEST(AfterTest, ChangesToExpectationSetHaveNoEffectAfterwards) {
  MockA a;
  ExpectationSet es1 = EXPECT_CALL(a, DoA(1));
  Expectation e2 = EXPECT_CALL(a, DoA(2));
  EXPECT_CALL(a, DoA(3))
      .After(es1);
  es1 += e2;

  a.DoA(1);
  a.DoA(3);
  a.DoA(2);
}





TEST(DeletingMockEarlyTest, Success1) {
  MockB* const b1 = new MockB;
  MockA* const a = new MockA;
  MockB* const b2 = new MockB;

  {
    InSequence dummy;
    EXPECT_CALL(*b1, DoB(_))
        .WillOnce(Return(1));
    EXPECT_CALL(*a, Binary(_, _))
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*b2, DoB(_))
        .Times(AnyNumber())
        .WillRepeatedly(Return(2));
  }

  EXPECT_EQ(1, b1->DoB(1));
  delete b1;
  
  EXPECT_TRUE(a->Binary(0, 1));
  delete b2;
  
  EXPECT_TRUE(a->Binary(1, 2));
  delete a;
}


TEST(DeletingMockEarlyTest, Success2) {
  MockB* const b1 = new MockB;
  MockA* const a = new MockA;
  MockB* const b2 = new MockB;

  {
    InSequence dummy;
    EXPECT_CALL(*b1, DoB(_))
        .WillOnce(Return(1));
    EXPECT_CALL(*a, Binary(_, _))
        .Times(AnyNumber());
    EXPECT_CALL(*b2, DoB(_))
        .Times(AnyNumber())
        .WillRepeatedly(Return(2));
  }

  delete a;  
  EXPECT_EQ(1, b1->DoB(1));
  EXPECT_EQ(2, b2->DoB(2));
  delete b1;
  delete b2;
}





#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4100)
#endif

ACTION_P(Delete, ptr) { delete ptr; }

#ifdef _MSC_VER
# pragma warning(pop)
#endif

TEST(DeletingMockEarlyTest, CanDeleteSelfInActionReturningVoid) {
  MockA* const a = new MockA;
  EXPECT_CALL(*a, DoA(_)).WillOnce(Delete(a));
  a->DoA(42);  
}

TEST(DeletingMockEarlyTest, CanDeleteSelfInActionReturningValue) {
  MockA* const a = new MockA;
  EXPECT_CALL(*a, ReturnResult(_))
      .WillOnce(DoAll(Delete(a), Return(Result())));
  a->ReturnResult(42);  
}


TEST(DeletingMockEarlyTest, Failure1) {
  MockB* const b1 = new MockB;
  MockA* const a = new MockA;
  MockB* const b2 = new MockB;

  {
    InSequence dummy;
    EXPECT_CALL(*b1, DoB(_))
        .WillOnce(Return(1));
    EXPECT_CALL(*a, Binary(_, _))
        .Times(AnyNumber());
    EXPECT_CALL(*b2, DoB(_))
        .Times(AnyNumber())
        .WillRepeatedly(Return(2));
  }

  delete a;  
  EXPECT_NONFATAL_FAILURE({
    b2->DoB(2);
  }, "Unexpected mock function call");
  EXPECT_EQ(1, b1->DoB(1));
  delete b1;
  delete b2;
}


TEST(DeletingMockEarlyTest, Failure2) {
  MockB* const b1 = new MockB;
  MockA* const a = new MockA;
  MockB* const b2 = new MockB;

  {
    InSequence dummy;
    EXPECT_CALL(*b1, DoB(_));
    EXPECT_CALL(*a, Binary(_, _))
        .Times(AnyNumber());
    EXPECT_CALL(*b2, DoB(_))
        .Times(AnyNumber());
  }

  EXPECT_NONFATAL_FAILURE(delete b1,
                          "Actual: never called");
  EXPECT_NONFATAL_FAILURE(a->Binary(0, 1),
                          "Unexpected mock function call");
  EXPECT_NONFATAL_FAILURE(b2->DoB(1),
                          "Unexpected mock function call");
  delete a;
  delete b2;
}

class EvenNumberCardinality : public CardinalityInterface {
 public:
  
  virtual bool IsSatisfiedByCallCount(int call_count) const {
    return call_count % 2 == 0;
  }

  
  virtual bool IsSaturatedByCallCount(int ) const {
    return false;
  }

  
  virtual void DescribeTo(::std::ostream* os) const {
    *os << "called even number of times";
  }
};

Cardinality EvenNumber() {
  return Cardinality(new EvenNumberCardinality);
}

TEST(ExpectationBaseTest,
     AllPrerequisitesAreSatisfiedWorksForNonMonotonicCardinality) {
  MockA* a = new MockA;
  Sequence s;

  EXPECT_CALL(*a, DoA(1))
      .Times(EvenNumber())
      .InSequence(s);
  EXPECT_CALL(*a, DoA(2))
      .Times(AnyNumber())
      .InSequence(s);
  EXPECT_CALL(*a, DoA(3))
      .Times(AnyNumber());

  a->DoA(3);
  a->DoA(1);
  EXPECT_NONFATAL_FAILURE(a->DoA(2), "Unexpected mock function call");
  EXPECT_NONFATAL_FAILURE(delete a, "to be called even number of times");
}




struct Printable {
};

inline void operator<<(::std::ostream& os, const Printable&) {
  os << "Printable";
}

struct Unprintable {
  Unprintable() : value(0) {}
  int value;
};

class MockC {
 public:
  MockC() {}

  MOCK_METHOD6(VoidMethod, void(bool cond, int n, string s, void* p,
                                const Printable& x, Unprintable y));
  MOCK_METHOD0(NonVoidMethod, int());  

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockC);
};

class VerboseFlagPreservingFixture : public testing::Test {
 protected:
  
  
  
  
  
  
  VerboseFlagPreservingFixture()
      : saved_verbose_flag_(GMOCK_FLAG(verbose).c_str()) {}

  ~VerboseFlagPreservingFixture() { GMOCK_FLAG(verbose) = saved_verbose_flag_; }

 private:
  const string saved_verbose_flag_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(VerboseFlagPreservingFixture);
};

#if GTEST_HAS_STREAM_REDIRECTION



TEST(FunctionCallMessageTest, UninterestingCallGeneratesFyiWithStackTrace) {
  MockC c;
  CaptureStdout();
  c.VoidMethod(false, 5, "Hi", NULL, Printable(), Unprintable());
  const String output = GetCapturedStdout();
  EXPECT_PRED_FORMAT2(IsSubstring, "GMOCK WARNING", output);
  EXPECT_PRED_FORMAT2(IsSubstring, "Stack trace:", output);

# ifndef NDEBUG

  
  

  
  
  EXPECT_PRED_FORMAT2(IsSubstring, "VoidMethod(", output);

  
  
  CaptureStdout();
  c.NonVoidMethod();
  const String output2 = GetCapturedStdout();
  EXPECT_PRED_FORMAT2(IsSubstring, "NonVoidMethod(", output2);

# endif  
}



TEST(FunctionCallMessageTest, UninterestingCallPrintsArgumentsAndReturnValue) {
  
  MockB b;
  CaptureStdout();
  b.DoB();
  const String output1 = GetCapturedStdout();
  EXPECT_PRED_FORMAT2(
      IsSubstring,
      "Uninteresting mock function call - returning default value.\n"
      "    Function call: DoB()\n"
      "          Returns: 0\n", output1.c_str());
  

  
  MockC c;
  CaptureStdout();
  c.VoidMethod(false, 5, "Hi", NULL, Printable(), Unprintable());
  const String output2 = GetCapturedStdout();
  EXPECT_THAT(output2.c_str(),
              ContainsRegex(
                  "Uninteresting mock function call - returning directly\\.\n"
                  "    Function call: VoidMethod"
                  "\\(false, 5, \"Hi\", NULL, @.+ "
                  "Printable, 4-byte object <00-00 00-00>\\)"));
  
}



class GMockVerboseFlagTest : public VerboseFlagPreservingFixture {
 public:
  
  
  
  
  void VerifyOutput(const String& output, bool should_print,
                    const string& expected_substring,
                    const string& function_name) {
    if (should_print) {
      EXPECT_THAT(output.c_str(), HasSubstr(expected_substring));
# ifndef NDEBUG
      
      
      EXPECT_THAT(output.c_str(), HasSubstr(function_name));
# else
      
      static_cast<void>(function_name);
# endif  
    } else {
      EXPECT_STREQ("", output.c_str());
    }
  }

  
  void TestExpectedCall(bool should_print) {
    MockA a;
    EXPECT_CALL(a, DoA(5));
    EXPECT_CALL(a, Binary(_, 1))
        .WillOnce(Return(true));

    
    CaptureStdout();
    a.DoA(5);
    VerifyOutput(
        GetCapturedStdout(),
        should_print,
        "Mock function call matches EXPECT_CALL(a, DoA(5))...\n"
        "    Function call: DoA(5)\n"
        "Stack trace:\n",
        "DoA");

    
    CaptureStdout();
    a.Binary(2, 1);
    VerifyOutput(
        GetCapturedStdout(),
        should_print,
        "Mock function call matches EXPECT_CALL(a, Binary(_, 1))...\n"
        "    Function call: Binary(2, 1)\n"
        "          Returns: true\n"
        "Stack trace:\n",
        "Binary");
  }

  
  void TestUninterestingCall(bool should_print) {
    MockA a;

    
    CaptureStdout();
    a.DoA(5);
    VerifyOutput(
        GetCapturedStdout(),
        should_print,
        "\nGMOCK WARNING:\n"
        "Uninteresting mock function call - returning directly.\n"
        "    Function call: DoA(5)\n"
        "Stack trace:\n",
        "DoA");

    
    CaptureStdout();
    a.Binary(2, 1);
    VerifyOutput(
        GetCapturedStdout(),
        should_print,
        "\nGMOCK WARNING:\n"
        "Uninteresting mock function call - returning default value.\n"
        "    Function call: Binary(2, 1)\n"
        "          Returns: false\n"
        "Stack trace:\n",
        "Binary");
  }
};



TEST_F(GMockVerboseFlagTest, Info) {
  GMOCK_FLAG(verbose) = kInfoVerbosity;
  TestExpectedCall(true);
  TestUninterestingCall(true);
}



TEST_F(GMockVerboseFlagTest, Warning) {
  GMOCK_FLAG(verbose) = kWarningVerbosity;
  TestExpectedCall(false);
  TestUninterestingCall(true);
}



TEST_F(GMockVerboseFlagTest, Error) {
  GMOCK_FLAG(verbose) = kErrorVerbosity;
  TestExpectedCall(false);
  TestUninterestingCall(false);
}



TEST_F(GMockVerboseFlagTest, InvalidFlagIsTreatedAsWarning) {
  GMOCK_FLAG(verbose) = "invalid";  
  TestExpectedCall(false);
  TestUninterestingCall(true);
}

#endif  




class PrintMeNot {};

void PrintTo(PrintMeNot , ::std::ostream* ) {
  ADD_FAILURE() << "Google Mock is printing a value that shouldn't be "
                << "printed even to an internal buffer.";
}

class LogTestHelper {
 public:
  LogTestHelper() {}

  MOCK_METHOD1(Foo, PrintMeNot(PrintMeNot));

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(LogTestHelper);
};

class GMockLogTest : public VerboseFlagPreservingFixture {
 protected:
  LogTestHelper helper_;
};

TEST_F(GMockLogTest, DoesNotPrintGoodCallInternallyIfVerbosityIsWarning) {
  GMOCK_FLAG(verbose) = kWarningVerbosity;
  EXPECT_CALL(helper_, Foo(_))
      .WillOnce(Return(PrintMeNot()));
  helper_.Foo(PrintMeNot());  
}

TEST_F(GMockLogTest, DoesNotPrintGoodCallInternallyIfVerbosityIsError) {
  GMOCK_FLAG(verbose) = kErrorVerbosity;
  EXPECT_CALL(helper_, Foo(_))
      .WillOnce(Return(PrintMeNot()));
  helper_.Foo(PrintMeNot());  
}

TEST_F(GMockLogTest, DoesNotPrintWarningInternallyIfVerbosityIsError) {
  GMOCK_FLAG(verbose) = kErrorVerbosity;
  ON_CALL(helper_, Foo(_))
      .WillByDefault(Return(PrintMeNot()));
  helper_.Foo(PrintMeNot());  
}



TEST(AllowLeakTest, AllowsLeakingUnusedMockObject) {
  MockA* a = new MockA;
  Mock::AllowLeak(a);
}

TEST(AllowLeakTest, CanBeCalledBeforeOnCall) {
  MockA* a = new MockA;
  Mock::AllowLeak(a);
  ON_CALL(*a, DoA(_)).WillByDefault(Return());
  a->DoA(0);
}

TEST(AllowLeakTest, CanBeCalledAfterOnCall) {
  MockA* a = new MockA;
  ON_CALL(*a, DoA(_)).WillByDefault(Return());
  Mock::AllowLeak(a);
}

TEST(AllowLeakTest, CanBeCalledBeforeExpectCall) {
  MockA* a = new MockA;
  Mock::AllowLeak(a);
  EXPECT_CALL(*a, DoA(_));
  a->DoA(0);
}

TEST(AllowLeakTest, CanBeCalledAfterExpectCall) {
  MockA* a = new MockA;
  EXPECT_CALL(*a, DoA(_)).Times(AnyNumber());
  Mock::AllowLeak(a);
}

TEST(AllowLeakTest, WorksWhenBothOnCallAndExpectCallArePresent) {
  MockA* a = new MockA;
  ON_CALL(*a, DoA(_)).WillByDefault(Return());
  EXPECT_CALL(*a, DoA(_)).Times(AnyNumber());
  Mock::AllowLeak(a);
}



TEST(VerifyAndClearExpectationsTest, NoMethodHasExpectations) {
  MockB b;
  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&b));

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}




TEST(VerifyAndClearExpectationsTest, SomeMethodsHaveExpectationsAndSucceed) {
  MockB b;
  EXPECT_CALL(b, DoB())
      .WillOnce(Return(1));
  b.DoB();
  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&b));

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}




TEST(VerifyAndClearExpectationsTest, SomeMethodsHaveExpectationsAndFail) {
  MockB b;
  EXPECT_CALL(b, DoB())
      .WillOnce(Return(1));
  bool result = true;
  EXPECT_NONFATAL_FAILURE(result = Mock::VerifyAndClearExpectations(&b),
                          "Actual: never called");
  ASSERT_FALSE(result);

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearExpectationsTest, AllMethodsHaveExpectations) {
  MockB b;
  EXPECT_CALL(b, DoB())
      .WillOnce(Return(1));
  EXPECT_CALL(b, DoB(_))
      .WillOnce(Return(2));
  b.DoB();
  b.DoB(1);
  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&b));

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearExpectationsTest, AMethodHasManyExpectations) {
  MockB b;
  EXPECT_CALL(b, DoB(0))
      .WillOnce(Return(1));
  EXPECT_CALL(b, DoB(_))
      .WillOnce(Return(2));
  b.DoB(1);
  bool result = true;
  EXPECT_NONFATAL_FAILURE(result = Mock::VerifyAndClearExpectations(&b),
                          "Actual: never called");
  ASSERT_FALSE(result);

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearExpectationsTest, CanCallManyTimes) {
  MockB b;
  EXPECT_CALL(b, DoB());
  b.DoB();
  Mock::VerifyAndClearExpectations(&b);

  EXPECT_CALL(b, DoB(_))
      .WillOnce(Return(1));
  b.DoB(1);
  Mock::VerifyAndClearExpectations(&b);
  Mock::VerifyAndClearExpectations(&b);

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearTest, NoMethodHasDefaultActions) {
  MockB b;
  
  Mock::VerifyAndClear(&b);
  EXPECT_EQ(0, b.DoB());
}



TEST(VerifyAndClearTest, SomeMethodsHaveDefaultActions) {
  MockB b;
  ON_CALL(b, DoB())
      .WillByDefault(Return(1));

  Mock::VerifyAndClear(&b);

  
  EXPECT_EQ(0, b.DoB());
}



TEST(VerifyAndClearTest, AllMethodsHaveDefaultActions) {
  MockB b;
  ON_CALL(b, DoB())
      .WillByDefault(Return(1));
  ON_CALL(b, DoB(_))
      .WillByDefault(Return(2));

  Mock::VerifyAndClear(&b);

  
  EXPECT_EQ(0, b.DoB());

  
  EXPECT_EQ(0, b.DoB(0));
}



TEST(VerifyAndClearTest, AMethodHasManyDefaultActions) {
  MockB b;
  ON_CALL(b, DoB(0))
      .WillByDefault(Return(1));
  ON_CALL(b, DoB(_))
      .WillByDefault(Return(2));

  Mock::VerifyAndClear(&b);

  
  
  EXPECT_EQ(0, b.DoB(0));
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearTest, CanCallManyTimes) {
  MockB b;
  ON_CALL(b, DoB())
      .WillByDefault(Return(1));
  Mock::VerifyAndClear(&b);
  Mock::VerifyAndClear(&b);

  ON_CALL(b, DoB(_))
      .WillByDefault(Return(1));
  Mock::VerifyAndClear(&b);

  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}


TEST(VerifyAndClearTest, Success) {
  MockB b;
  ON_CALL(b, DoB())
      .WillByDefault(Return(1));
  EXPECT_CALL(b, DoB(1))
      .WillOnce(Return(2));

  b.DoB();
  b.DoB(1);
  ASSERT_TRUE(Mock::VerifyAndClear(&b));

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}


TEST(VerifyAndClearTest, Failure) {
  MockB b;
  ON_CALL(b, DoB(_))
      .WillByDefault(Return(1));
  EXPECT_CALL(b, DoB())
      .WillOnce(Return(2));

  b.DoB(1);
  bool result = true;
  EXPECT_NONFATAL_FAILURE(result = Mock::VerifyAndClear(&b),
                          "Actual: never called");
  ASSERT_FALSE(result);

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearTest, Const) {
  MockB b;
  ON_CALL(Const(b), DoB())
      .WillByDefault(Return(1));

  EXPECT_CALL(Const(b), DoB())
      .WillOnce(DoDefault())
      .WillOnce(Return(2));

  b.DoB();
  b.DoB();
  ASSERT_TRUE(Mock::VerifyAndClear(&b));

  
  
  EXPECT_EQ(0, b.DoB());
  EXPECT_EQ(0, b.DoB(1));
}



TEST(VerifyAndClearTest, CanSetDefaultActionsAndExpectationsAfterwards) {
  MockB b;
  ON_CALL(b, DoB())
      .WillByDefault(Return(1));
  EXPECT_CALL(b, DoB(_))
      .WillOnce(Return(2));
  b.DoB(1);

  Mock::VerifyAndClear(&b);

  EXPECT_CALL(b, DoB())
      .WillOnce(Return(3));
  ON_CALL(b, DoB(_))
      .WillByDefault(Return(4));

  EXPECT_EQ(3, b.DoB());
  EXPECT_EQ(4, b.DoB(1));
}



TEST(VerifyAndClearTest, DoesNotAffectOtherMockObjects) {
  MockA a;
  MockB b1;
  MockB b2;

  ON_CALL(a, Binary(_, _))
      .WillByDefault(Return(true));
  EXPECT_CALL(a, Binary(_, _))
      .WillOnce(DoDefault())
      .WillOnce(Return(false));

  ON_CALL(b1, DoB())
      .WillByDefault(Return(1));
  EXPECT_CALL(b1, DoB(_))
      .WillOnce(Return(2));

  ON_CALL(b2, DoB())
      .WillByDefault(Return(3));
  EXPECT_CALL(b2, DoB(_));

  b2.DoB(0);
  Mock::VerifyAndClear(&b2);

  
  
  EXPECT_TRUE(a.Binary(0, 0));
  EXPECT_FALSE(a.Binary(0, 0));

  EXPECT_EQ(1, b1.DoB());
  EXPECT_EQ(2, b1.DoB(0));
}






TEST(SynchronizationTest, CanCallMockMethodInAction) {
  MockA a;
  MockC c;
  ON_CALL(a, DoA(_))
      .WillByDefault(IgnoreResult(InvokeWithoutArgs(&c,
                                                    &MockC::NonVoidMethod)));
  EXPECT_CALL(a, DoA(1));
  EXPECT_CALL(a, DoA(1))
      .WillOnce(Invoke(&a, &MockA::DoA))
      .RetiresOnSaturation();
  EXPECT_CALL(c, NonVoidMethod());

  a.DoA(1);
  
  
  
  
}

}  




#if GMOCK_RENAME_MAIN
int gmock_main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
  testing::InitGoogleMock(&argc, argv);

  
  
  testing::GMOCK_FLAG(catch_leaked_mocks) = true;
  testing::GMOCK_FLAG(verbose) = testing::internal::kWarningVerbosity;

  return RUN_ALL_TESTS();
}
