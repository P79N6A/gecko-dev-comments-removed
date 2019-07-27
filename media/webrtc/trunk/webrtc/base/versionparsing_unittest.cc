









#include "webrtc/base/versionparsing.h"

#include "webrtc/base/gunit.h"

namespace rtc {

static const int kExampleSegments = 4;

typedef int ExampleVersion[kExampleSegments];

TEST(VersionParsing, TestGoodParse) {
  ExampleVersion ver;
  std::string str1("1.1.2.0");
  static const ExampleVersion expect1 = {1, 1, 2, 0};
  EXPECT_TRUE(ParseVersionString(str1, kExampleSegments, ver));
  EXPECT_EQ(0, CompareVersions(ver, expect1, kExampleSegments));
  std::string str2("2.0.0.1");
  static const ExampleVersion expect2 = {2, 0, 0, 1};
  EXPECT_TRUE(ParseVersionString(str2, kExampleSegments, ver));
  EXPECT_EQ(0, CompareVersions(ver, expect2, kExampleSegments));
}

TEST(VersionParsing, TestBadParse) {
  ExampleVersion ver;
  std::string str1("1.1.2");
  EXPECT_FALSE(ParseVersionString(str1, kExampleSegments, ver));
  std::string str2("");
  EXPECT_FALSE(ParseVersionString(str2, kExampleSegments, ver));
  std::string str3("garbarge");
  EXPECT_FALSE(ParseVersionString(str3, kExampleSegments, ver));
}

TEST(VersionParsing, TestCompare) {
  static const ExampleVersion ver1 = {1, 0, 21, 0};
  static const ExampleVersion ver2 = {1, 1, 2, 0};
  static const ExampleVersion ver3 = {1, 1, 3, 0};
  static const ExampleVersion ver4 = {1, 1, 3, 9861};

  
  EXPECT_EQ(0, CompareVersions(ver1, ver1, kExampleSegments));
  EXPECT_EQ(0, CompareVersions(ver2, ver2, kExampleSegments));
  EXPECT_EQ(0, CompareVersions(ver3, ver3, kExampleSegments));
  EXPECT_EQ(0, CompareVersions(ver4, ver4, kExampleSegments));

  EXPECT_GT(0, CompareVersions(ver1, ver2, kExampleSegments));
  EXPECT_LT(0, CompareVersions(ver2, ver1, kExampleSegments));

  EXPECT_GT(0, CompareVersions(ver1, ver3, kExampleSegments));
  EXPECT_LT(0, CompareVersions(ver3, ver1, kExampleSegments));

  EXPECT_GT(0, CompareVersions(ver1, ver4, kExampleSegments));
  EXPECT_LT(0, CompareVersions(ver4, ver1, kExampleSegments));

  EXPECT_GT(0, CompareVersions(ver2, ver3, kExampleSegments));
  EXPECT_LT(0, CompareVersions(ver3, ver2, kExampleSegments));

  EXPECT_GT(0, CompareVersions(ver2, ver4, kExampleSegments));
  EXPECT_LT(0, CompareVersions(ver4, ver2, kExampleSegments));

  EXPECT_GT(0, CompareVersions(ver3, ver4, kExampleSegments));
  EXPECT_LT(0, CompareVersions(ver4, ver3, kExampleSegments));
}

}  
