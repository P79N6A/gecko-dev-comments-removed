
































#include "gtest/gtest.h"

namespace {

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Message;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::UnitTest;
using ::testing::internal::scoped_ptr;




class A : public Test {};
TEST_F(A, A) {}
TEST_F(A, B) {}

TEST(ADeathTest, A) {}
TEST(ADeathTest, B) {}
TEST(ADeathTest, C) {}

TEST(B, A) {}
TEST(B, B) {}
TEST(B, C) {}
TEST(B, DISABLED_D) {}
TEST(B, DISABLED_E) {}

TEST(BDeathTest, A) {}
TEST(BDeathTest, B) {}

TEST(C, A) {}
TEST(C, B) {}
TEST(C, C) {}
TEST(C, DISABLED_D) {}

TEST(CDeathTest, A) {}

TEST(DISABLED_D, A) {}
TEST(DISABLED_D, DISABLED_B) {}



class TestNamePrinter : public EmptyTestEventListener {
 public:
  virtual void OnTestIterationStart(const UnitTest& ,
                                    int ) {
    printf("----\n");
  }

  virtual void OnTestStart(const TestInfo& test_info) {
    printf("%s.%s\n", test_info.test_case_name(), test_info.name());
  }
};

}  

int main(int argc, char **argv) {
  InitGoogleTest(&argc, argv);

  
  
  TestEventListeners& listeners = UnitTest::GetInstance()->listeners();
  delete listeners.Release(listeners.default_result_printer());
  listeners.Append(new TestNamePrinter);

  return RUN_ALL_TESTS();
}
