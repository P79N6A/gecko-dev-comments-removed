




#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::AtLeast;



TEST(MozillaGTestSanity, Runs) {
  EXPECT_EQ(1, 1);
}
namespace {
class TestMock {
public:
  TestMock() {}
  MOCK_METHOD0(MockedCall, void());
};
}
TEST(MozillaGMockSanity, Runs) {
  TestMock mockedClass;
  EXPECT_CALL(mockedClass, MockedCall())
    .Times(AtLeast(3));

  mockedClass.MockedCall();
  mockedClass.MockedCall();
  mockedClass.MockedCall();
}
