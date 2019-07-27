









#include <string>

#include "webrtc/base/gunit.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/ssladapter.h"

namespace rtc {

class RandomTest : public testing::Test {};

TEST_F(RandomTest, TestCreateRandomId) {
  CreateRandomId();
}

TEST_F(RandomTest, TestCreateRandomDouble) {
  for (int i = 0; i < 100; ++i) {
    double r = CreateRandomDouble();
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 1.0);
  }
}

TEST_F(RandomTest, TestCreateNonZeroRandomId) {
  EXPECT_NE(0U, CreateRandomNonZeroId());
}

TEST_F(RandomTest, TestCreateRandomString) {
  std::string random = CreateRandomString(256);
  EXPECT_EQ(256U, random.size());
  std::string random2;
  EXPECT_TRUE(CreateRandomString(256, &random2));
  EXPECT_NE(random, random2);
  EXPECT_EQ(256U, random2.size());
}

TEST_F(RandomTest, TestCreateRandomForTest) {
  
  SetRandomTestMode(true);
  EXPECT_EQ(2154761789U, CreateRandomId());
  EXPECT_EQ("h0ISP4S5SJKH/9EY", CreateRandomString(16));

  
  SetRandomTestMode(true);
  EXPECT_EQ(2154761789U, CreateRandomId());
  EXPECT_EQ("h0ISP4S5SJKH/9EY", CreateRandomString(16));

  
  SetRandomTestMode(true);
  std::string str;
  EXPECT_TRUE(CreateRandomString(16, "a", &str));
  EXPECT_EQ("aaaaaaaaaaaaaaaa", str);
  EXPECT_TRUE(CreateRandomString(16, "abc", &str));
  EXPECT_EQ("acbccaaaabbaacbb", str);

  
  SetRandomTestMode(false);
}

}  
