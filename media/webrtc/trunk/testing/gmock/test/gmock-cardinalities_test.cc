


































#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

namespace {

using std::stringstream;
using testing::AnyNumber;
using testing::AtLeast;
using testing::AtMost;
using testing::Between;
using testing::Cardinality;
using testing::CardinalityInterface;
using testing::Exactly;
using testing::IsSubstring;
using testing::MakeCardinality;

class MockFoo {
 public:
  MockFoo() {}
  MOCK_METHOD0(Bar, int());  

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockFoo);
};


TEST(CardinalityTest, IsDefaultConstructable) {
  Cardinality c;
}


TEST(CardinalityTest, IsCopyable) {
  
  Cardinality c = Exactly(1);
  EXPECT_FALSE(c.IsSatisfiedByCallCount(0));
  EXPECT_TRUE(c.IsSatisfiedByCallCount(1));
  EXPECT_TRUE(c.IsSaturatedByCallCount(1));

  
  c = Exactly(2);
  EXPECT_FALSE(c.IsSatisfiedByCallCount(1));
  EXPECT_TRUE(c.IsSatisfiedByCallCount(2));
  EXPECT_TRUE(c.IsSaturatedByCallCount(2));
}

TEST(CardinalityTest, IsOverSaturatedByCallCountWorks) {
  const Cardinality c = AtMost(5);
  EXPECT_FALSE(c.IsOverSaturatedByCallCount(4));
  EXPECT_FALSE(c.IsOverSaturatedByCallCount(5));
  EXPECT_TRUE(c.IsOverSaturatedByCallCount(6));
}



TEST(CardinalityTest, CanDescribeActualCallCount) {
  stringstream ss0;
  Cardinality::DescribeActualCallCountTo(0, &ss0);
  EXPECT_EQ("never called", ss0.str());

  stringstream ss1;
  Cardinality::DescribeActualCallCountTo(1, &ss1);
  EXPECT_EQ("called once", ss1.str());

  stringstream ss2;
  Cardinality::DescribeActualCallCountTo(2, &ss2);
  EXPECT_EQ("called twice", ss2.str());

  stringstream ss3;
  Cardinality::DescribeActualCallCountTo(3, &ss3);
  EXPECT_EQ("called 3 times", ss3.str());
}


TEST(AnyNumber, Works) {
  const Cardinality c = AnyNumber();
  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_FALSE(c.IsSaturatedByCallCount(0));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(1));
  EXPECT_FALSE(c.IsSaturatedByCallCount(1));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(9));
  EXPECT_FALSE(c.IsSaturatedByCallCount(9));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "called any number of times",
                      ss.str());
}

TEST(AnyNumberTest, HasCorrectBounds) {
  const Cardinality c = AnyNumber();
  EXPECT_EQ(0, c.ConservativeLowerBound());
  EXPECT_EQ(INT_MAX, c.ConservativeUpperBound());
}



TEST(AtLeastTest, OnNegativeNumber) {
  EXPECT_NONFATAL_FAILURE({  
    AtLeast(-1);
  }, "The invocation lower bound must be >= 0");
}

TEST(AtLeastTest, OnZero) {
  const Cardinality c = AtLeast(0);
  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_FALSE(c.IsSaturatedByCallCount(0));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(1));
  EXPECT_FALSE(c.IsSaturatedByCallCount(1));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "any number of times",
                      ss.str());
}

TEST(AtLeastTest, OnPositiveNumber) {
  const Cardinality c = AtLeast(2);
  EXPECT_FALSE(c.IsSatisfiedByCallCount(0));
  EXPECT_FALSE(c.IsSaturatedByCallCount(0));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(1));
  EXPECT_FALSE(c.IsSaturatedByCallCount(1));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(2));
  EXPECT_FALSE(c.IsSaturatedByCallCount(2));

  stringstream ss1;
  AtLeast(1).DescribeTo(&ss1);
  EXPECT_PRED_FORMAT2(IsSubstring, "at least once",
                      ss1.str());

  stringstream ss2;
  c.DescribeTo(&ss2);
  EXPECT_PRED_FORMAT2(IsSubstring, "at least twice",
                      ss2.str());

  stringstream ss3;
  AtLeast(3).DescribeTo(&ss3);
  EXPECT_PRED_FORMAT2(IsSubstring, "at least 3 times",
                      ss3.str());
}

TEST(AtLeastTest, HasCorrectBounds) {
  const Cardinality c = AtLeast(2);
  EXPECT_EQ(2, c.ConservativeLowerBound());
  EXPECT_EQ(INT_MAX, c.ConservativeUpperBound());
}



TEST(AtMostTest, OnNegativeNumber) {
  EXPECT_NONFATAL_FAILURE({  
    AtMost(-1);
  }, "The invocation upper bound must be >= 0");
}

TEST(AtMostTest, OnZero) {
  const Cardinality c = AtMost(0);
  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_TRUE(c.IsSaturatedByCallCount(0));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(1));
  EXPECT_TRUE(c.IsSaturatedByCallCount(1));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "never called",
                      ss.str());
}

TEST(AtMostTest, OnPositiveNumber) {
  const Cardinality c = AtMost(2);
  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_FALSE(c.IsSaturatedByCallCount(0));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(1));
  EXPECT_FALSE(c.IsSaturatedByCallCount(1));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(2));
  EXPECT_TRUE(c.IsSaturatedByCallCount(2));

  stringstream ss1;
  AtMost(1).DescribeTo(&ss1);
  EXPECT_PRED_FORMAT2(IsSubstring, "called at most once",
                      ss1.str());

  stringstream ss2;
  c.DescribeTo(&ss2);
  EXPECT_PRED_FORMAT2(IsSubstring, "called at most twice",
                      ss2.str());

  stringstream ss3;
  AtMost(3).DescribeTo(&ss3);
  EXPECT_PRED_FORMAT2(IsSubstring, "called at most 3 times",
                      ss3.str());
}

TEST(AtMostTest, HasCorrectBounds) {
  const Cardinality c = AtMost(2);
  EXPECT_EQ(0, c.ConservativeLowerBound());
  EXPECT_EQ(2, c.ConservativeUpperBound());
}



TEST(BetweenTest, OnNegativeStart) {
  EXPECT_NONFATAL_FAILURE({  
    Between(-1, 2);
  }, "The invocation lower bound must be >= 0, but is actually -1");
}

TEST(BetweenTest, OnNegativeEnd) {
  EXPECT_NONFATAL_FAILURE({  
    Between(1, -2);
  }, "The invocation upper bound must be >= 0, but is actually -2");
}

TEST(BetweenTest, OnStartBiggerThanEnd) {
  EXPECT_NONFATAL_FAILURE({  
    Between(2, 1);
  }, "The invocation upper bound (1) must be >= "
     "the invocation lower bound (2)");
}

TEST(BetweenTest, OnZeroStartAndZeroEnd) {
  const Cardinality c = Between(0, 0);

  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_TRUE(c.IsSaturatedByCallCount(0));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(1));
  EXPECT_TRUE(c.IsSaturatedByCallCount(1));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "never called",
                      ss.str());
}

TEST(BetweenTest, OnZeroStartAndNonZeroEnd) {
  const Cardinality c = Between(0, 2);

  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_FALSE(c.IsSaturatedByCallCount(0));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(2));
  EXPECT_TRUE(c.IsSaturatedByCallCount(2));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(4));
  EXPECT_TRUE(c.IsSaturatedByCallCount(4));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "called at most twice",
                      ss.str());
}

TEST(BetweenTest, OnSameStartAndEnd) {
  const Cardinality c = Between(3, 3);

  EXPECT_FALSE(c.IsSatisfiedByCallCount(2));
  EXPECT_FALSE(c.IsSaturatedByCallCount(2));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(3));
  EXPECT_TRUE(c.IsSaturatedByCallCount(3));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(4));
  EXPECT_TRUE(c.IsSaturatedByCallCount(4));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "called 3 times",
                      ss.str());
}

TEST(BetweenTest, OnDifferentStartAndEnd) {
  const Cardinality c = Between(3, 5);

  EXPECT_FALSE(c.IsSatisfiedByCallCount(2));
  EXPECT_FALSE(c.IsSaturatedByCallCount(2));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(3));
  EXPECT_FALSE(c.IsSaturatedByCallCount(3));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(5));
  EXPECT_TRUE(c.IsSaturatedByCallCount(5));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(6));
  EXPECT_TRUE(c.IsSaturatedByCallCount(6));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "called between 3 and 5 times",
                      ss.str());
}

TEST(BetweenTest, HasCorrectBounds) {
  const Cardinality c = Between(3, 5);
  EXPECT_EQ(3, c.ConservativeLowerBound());
  EXPECT_EQ(5, c.ConservativeUpperBound());
}



TEST(ExactlyTest, OnNegativeNumber) {
  EXPECT_NONFATAL_FAILURE({  
    Exactly(-1);
  }, "The invocation lower bound must be >= 0");
}

TEST(ExactlyTest, OnZero) {
  const Cardinality c = Exactly(0);
  EXPECT_TRUE(c.IsSatisfiedByCallCount(0));
  EXPECT_TRUE(c.IsSaturatedByCallCount(0));

  EXPECT_FALSE(c.IsSatisfiedByCallCount(1));
  EXPECT_TRUE(c.IsSaturatedByCallCount(1));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_PRED_FORMAT2(IsSubstring, "never called",
                      ss.str());
}

TEST(ExactlyTest, OnPositiveNumber) {
  const Cardinality c = Exactly(2);
  EXPECT_FALSE(c.IsSatisfiedByCallCount(0));
  EXPECT_FALSE(c.IsSaturatedByCallCount(0));

  EXPECT_TRUE(c.IsSatisfiedByCallCount(2));
  EXPECT_TRUE(c.IsSaturatedByCallCount(2));

  stringstream ss1;
  Exactly(1).DescribeTo(&ss1);
  EXPECT_PRED_FORMAT2(IsSubstring, "called once",
                      ss1.str());

  stringstream ss2;
  c.DescribeTo(&ss2);
  EXPECT_PRED_FORMAT2(IsSubstring, "called twice",
                      ss2.str());

  stringstream ss3;
  Exactly(3).DescribeTo(&ss3);
  EXPECT_PRED_FORMAT2(IsSubstring, "called 3 times",
                      ss3.str());
}

TEST(ExactlyTest, HasCorrectBounds) {
  const Cardinality c = Exactly(3);
  EXPECT_EQ(3, c.ConservativeLowerBound());
  EXPECT_EQ(3, c.ConservativeUpperBound());
}




class EvenCardinality : public CardinalityInterface {
 public:
  
  virtual bool IsSatisfiedByCallCount(int call_count) const {
    return (call_count % 2 == 0);
  }

  
  virtual bool IsSaturatedByCallCount(int ) const {
    return false;
  }

  
  virtual void DescribeTo(::std::ostream* ss) const {
    *ss << "called even number of times";
  }
};

TEST(MakeCardinalityTest, ConstructsCardinalityFromInterface) {
  const Cardinality c = MakeCardinality(new EvenCardinality);

  EXPECT_TRUE(c.IsSatisfiedByCallCount(2));
  EXPECT_FALSE(c.IsSatisfiedByCallCount(3));

  EXPECT_FALSE(c.IsSaturatedByCallCount(10000));

  stringstream ss;
  c.DescribeTo(&ss);
  EXPECT_EQ("called even number of times", ss.str());
}

}  
