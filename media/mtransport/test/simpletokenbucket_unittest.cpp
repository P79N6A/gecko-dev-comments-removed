







#include "simpletokenbucket.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

using mozilla::SimpleTokenBucket;

class TestSimpleTokenBucket : public SimpleTokenBucket {
  public:
    TestSimpleTokenBucket(size_t bucketSize, size_t tokensPerSecond) :
      SimpleTokenBucket(bucketSize, tokensPerSecond) {
    }

    void fastForward(int32_t timeMilliSeconds) {
      if (timeMilliSeconds >= 0) {
        last_time_tokens_added_ -= PR_MillisecondsToInterval(timeMilliSeconds);
      } else {
        last_time_tokens_added_ += PR_MillisecondsToInterval(-timeMilliSeconds);
      }
    }
};

TEST(SimpleTokenBucketTest, TestConstruct) {
  TestSimpleTokenBucket b(10, 1);
}

TEST(SimpleTokenBucketTest, TestGet) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(5U, b.getTokens(5));
}

TEST(SimpleTokenBucketTest, TestGetAll) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(10U, b.getTokens(10));
}

TEST(SimpleTokenBucketTest, TestGetInsufficient) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(5U, b.getTokens(5));
  ASSERT_EQ(5U, b.getTokens(6));
}

TEST(SimpleTokenBucketTest, TestGetBucketCount) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(10U, b.getTokens(UINT32_MAX));
  ASSERT_EQ(5U, b.getTokens(5));
  ASSERT_EQ(5U, b.getTokens(UINT32_MAX));
}

TEST(SimpleTokenBucketTest, TestTokenRefill) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(5U, b.getTokens(5));
  b.fastForward(1000);
  ASSERT_EQ(6U, b.getTokens(6));
}

TEST(SimpleTokenBucketTest, TestNoTimeWasted) {
  
  
  
  
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(5U, b.getTokens(5));
  b.fastForward(500);
  ASSERT_EQ(5U, b.getTokens(6));
  b.fastForward(500);
  ASSERT_EQ(6U, b.getTokens(6));
}

TEST(SimpleTokenBucketTest, TestNegativeTime) {
  TestSimpleTokenBucket b(10, 1);
  b.fastForward(-1000);
  
  
  ASSERT_GT(11U, b.getTokens(100));
}

TEST(SimpleTokenBucketTest, TestEmptyBucket) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(10U, b.getTokens(10));
  ASSERT_EQ(0U, b.getTokens(10));
}

TEST(SimpleTokenBucketTest, TestEmptyThenFillBucket) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(10U, b.getTokens(10));
  ASSERT_EQ(0U, b.getTokens(1));
  b.fastForward(50000);
  ASSERT_EQ(10U, b.getTokens(10));
}

TEST(SimpleTokenBucketTest, TestNoOverflow) {
  TestSimpleTokenBucket b(10, 1);
  ASSERT_EQ(10U, b.getTokens(10));
  ASSERT_EQ(0U, b.getTokens(1));
  b.fastForward(50000);
  ASSERT_EQ(10U, b.getTokens(11));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int rv = RUN_ALL_TESTS();
  return rv;
}

