



































#include "gmock/gmock.h"

namespace {

using ::testing::Return;

class FooInterface {
 public:
  virtual ~FooInterface() {}
  virtual void DoThis() = 0;
};

class MockFoo : public FooInterface {
 public:
  MockFoo() {}

  MOCK_METHOD0(DoThis, void());

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(MockFoo);
};

TEST(LeakTest, LeakedMockWithExpectCallCausesFailureWhenLeakCheckingIsEnabled) {
  MockFoo* foo = new MockFoo;

  EXPECT_CALL(*foo, DoThis());
  foo->DoThis();

  

  
  
  exit(0);
}

TEST(LeakTest, LeakedMockWithOnCallCausesFailureWhenLeakCheckingIsEnabled) {
  MockFoo* foo = new MockFoo;

  ON_CALL(*foo, DoThis()).WillByDefault(Return());

  

  
  
  exit(0);
}

TEST(LeakTest, CatchesMultipleLeakedMockObjects) {
  MockFoo* foo1 = new MockFoo;
  MockFoo* foo2 = new MockFoo;

  ON_CALL(*foo1, DoThis()).WillByDefault(Return());
  EXPECT_CALL(*foo2, DoThis());
  foo2->DoThis();

  
  

  
  
  exit(0);
}

}  
