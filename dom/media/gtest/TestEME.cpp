





#include "gtest/gtest.h"
#include "mozilla/EMEUtils.h"

using namespace std;
using namespace mozilla;

struct ParseKeySystemTestCase {
  const char16_t* mInputKeySystemString;
  int32_t mOutCDMVersion;
  bool mShouldPass;
};

const ParseKeySystemTestCase ParseKeySystemTests[] = {
  {
    MOZ_UTF16("org.w3.clearkey"),
    NO_CDM_VERSION,
    true,
  }, {
    MOZ_UTF16("org.w3.clearkey.123"),
    123,
    true,
  }, {
    MOZ_UTF16("org.w3.clearkey.-1"),
    NO_CDM_VERSION,
    false,
  }, {
    MOZ_UTF16("org.w3.clearkey.NaN"),
    NO_CDM_VERSION,
    false,
  }, {
    MOZ_UTF16("org.w3.clearkey.0"),
    0,
    true,
  }, {
    MOZ_UTF16("org.w3.clearkey.123567890123567890123567890123567890123567890"),
    NO_CDM_VERSION,
    false,
  }, {
    MOZ_UTF16("org.w3.clearkey.0.1"),
    NO_CDM_VERSION,
    false,
  }
};

TEST(EME, EMEParseKeySystem) {
  const nsAutoString clearkey(MOZ_UTF16("org.w3.clearkey"));
  for (const ParseKeySystemTestCase& test : ParseKeySystemTests) {
    nsAutoString keySystem;
    int32_t version;
    bool rv = ParseKeySystem(nsDependentString(test.mInputKeySystemString),
                             keySystem,
                             version);
    EXPECT_EQ(rv, test.mShouldPass) << "parse should succeed if expected to";
    if (!test.mShouldPass) {
      continue;
    }
    EXPECT_TRUE(keySystem.Equals(clearkey)) << NS_ConvertUTF16toUTF8(keySystem).get(); 
    EXPECT_EQ(test.mOutCDMVersion, version) << "should extract expected version";
  }
}
