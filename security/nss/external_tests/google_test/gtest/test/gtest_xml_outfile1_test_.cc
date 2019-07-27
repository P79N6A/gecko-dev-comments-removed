

































#include "gtest/gtest.h"

class PropertyOne : public testing::Test {
 protected:
  virtual void SetUp() {
    RecordProperty("SetUpProp", 1);
  }
  virtual void TearDown() {
    RecordProperty("TearDownProp", 1);
  }
};

TEST_F(PropertyOne, TestSomeProperties) {
  RecordProperty("TestSomeProperty", 1);
}
