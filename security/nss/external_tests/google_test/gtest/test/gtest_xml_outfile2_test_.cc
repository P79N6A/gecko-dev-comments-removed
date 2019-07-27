

































#include "gtest/gtest.h"

class PropertyTwo : public testing::Test {
 protected:
  virtual void SetUp() {
    RecordProperty("SetUpProp", 2);
  }
  virtual void TearDown() {
    RecordProperty("TearDownProp", 2);
  }
};

TEST_F(PropertyTwo, TestSomeProperties) {
  RecordProperty("TestSomeProperty", 2);
}
