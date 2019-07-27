







































#include "gtest/gtest.h"

namespace {



class FooTest : public testing::Test {
};

TEST_F(FooTest, Abc) {
}

TEST_F(FooTest, Xyz) {
  FAIL() << "Expected failure.";
}



TEST(BarTest, TestOne) {
}

TEST(BarTest, TestTwo) {
}

TEST(BarTest, TestThree) {
}

TEST(BarTest, DISABLED_TestFour) {
  FAIL() << "Expected failure.";
}

TEST(BarTest, DISABLED_TestFive) {
  FAIL() << "Expected failure.";
}



TEST(BazTest, TestOne) {
  FAIL() << "Expected failure.";
}

TEST(BazTest, TestA) {
}

TEST(BazTest, TestB) {
}

TEST(BazTest, DISABLED_TestC) {
  FAIL() << "Expected failure.";
}



TEST(HasDeathTest, Test1) {
  EXPECT_DEATH_IF_SUPPORTED(exit(1), ".*");
}



TEST(HasDeathTest, Test2) {
  EXPECT_DEATH_IF_SUPPORTED(exit(1), ".*");
}



TEST(DISABLED_FoobarTest, Test1) {
  FAIL() << "Expected failure.";
}

TEST(DISABLED_FoobarTest, DISABLED_Test2) {
  FAIL() << "Expected failure.";
}



TEST(DISABLED_FoobarbazTest, TestA) {
  FAIL() << "Expected failure.";
}

#if GTEST_HAS_PARAM_TEST
class ParamTest : public testing::TestWithParam<int> {
};

TEST_P(ParamTest, TestX) {
}

TEST_P(ParamTest, TestY) {
}

INSTANTIATE_TEST_CASE_P(SeqP, ParamTest, testing::Values(1, 2));
INSTANTIATE_TEST_CASE_P(SeqQ, ParamTest, testing::Values(5, 6));
#endif  

}  

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
