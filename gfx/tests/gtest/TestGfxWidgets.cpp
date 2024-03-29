




#include "gtest/gtest.h"
#include "GfxDriverInfo.h"
#include "nsVersionComparator.h"

using namespace mozilla::widget;

TEST(GfxWidgets, Split) {
  char aStr[8], bStr[8], cStr[8], dStr[8];

  ASSERT_TRUE(SplitDriverVersion("33.4.3.22", aStr, bStr, cStr, dStr));
  ASSERT_TRUE(atoi(aStr) == 33 && atoi(bStr) == 4 && atoi(cStr) == 3 && atoi(dStr) == 22);

  ASSERT_TRUE(SplitDriverVersion("28.74.0.0", aStr, bStr, cStr, dStr));
  ASSERT_TRUE(atoi(aStr) == 28 && atoi(bStr) == 74 && atoi(cStr) == 0 && atoi(dStr) == 0);

  ASSERT_TRUE(SplitDriverVersion("132.0.0.0", aStr, bStr, cStr, dStr));
  ASSERT_TRUE(atoi(aStr) == 132 && atoi(bStr) == 0 && atoi(cStr) == 0 && atoi(dStr) == 0);

  ASSERT_TRUE(SplitDriverVersion("2.3.0.0", aStr, bStr, cStr, dStr));
  ASSERT_TRUE(atoi(aStr) == 2 && atoi(bStr) == 3 && atoi(cStr) == 0 && atoi(dStr) == 0);

  ASSERT_TRUE(SplitDriverVersion("25.4.0.8", aStr, bStr, cStr, dStr));
  ASSERT_TRUE(atoi(aStr) == 25 && atoi(bStr) == 4 && atoi(cStr) == 0 && atoi(dStr) == 8);

}

TEST(GfxWidgets, Versioning) {
  ASSERT_TRUE(mozilla::Version("0") < mozilla::Version("41.0a1"));
  ASSERT_TRUE(mozilla::Version("39.0.5b7") < mozilla::Version("41.0a1"));
  ASSERT_TRUE(mozilla::Version("18.0.5b7") < mozilla::Version("18.2"));
  ASSERT_TRUE(mozilla::Version("30.0.5b7") < mozilla::Version("41.0b9"));
  ASSERT_TRUE(mozilla::Version("100") > mozilla::Version("43.0a1"));
  ASSERT_FALSE(mozilla::Version("42.0") < mozilla::Version("42.0"));
  ASSERT_TRUE(mozilla::Version("42.0b2") < mozilla::Version("42.0"));
  ASSERT_TRUE(mozilla::Version("42.0b2") < mozilla::Version("42"));
  ASSERT_TRUE(mozilla::Version("42.0b2") < mozilla::Version("43.0a1"));
  ASSERT_TRUE(mozilla::Version("42") < mozilla::Version("43.0a1"));
  ASSERT_TRUE(mozilla::Version("42.0") < mozilla::Version("43.0a1"));
  ASSERT_TRUE(mozilla::Version("42.0.5") < mozilla::Version("43.0a1"));
  ASSERT_TRUE(mozilla::Version("42.1") < mozilla::Version("43.0a1"));
  ASSERT_TRUE(mozilla::Version("42.0a1") < mozilla::Version("42"));
  ASSERT_TRUE(mozilla::Version("42.0a1") < mozilla::Version("42.0.5"));
  ASSERT_TRUE(mozilla::Version("42.0b7") < mozilla::Version("42.0.5"));
  ASSERT_TRUE(mozilla::Version("") == mozilla::Version("0"));

  
  
  
  ASSERT_TRUE(mozilla::Version("42.0a1") < mozilla::Version("42.0b2"));
  ASSERT_FALSE(mozilla::Version("42.0a1") < mozilla::Version("42b2"));
}

