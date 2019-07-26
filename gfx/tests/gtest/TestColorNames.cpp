




#include "gtest/gtest.h"

#include <string.h>
#include "nsColor.h"
#include "nsColorNames.h"
#include "prprf.h"
#include "nsString.h"
#include "mozilla/Util.h"


#define GFX_COLOR(_name, _value) #_name,
static const char* const kColorNames[] = {
#include "nsColorNameList.h"
};
#undef GFX_COLOR


#define GFX_COLOR(_name, _value) _value,
static const nscolor kColors[] = {
#include "nsColorNameList.h"
};
#undef GFX_COLOR

using namespace mozilla;

static const char* kJunkNames[] = {
  nullptr,
  "",
  "123",
  "backgroundz",
  "zzzzzz",
  "#@$&@#*@*$@$#"
};

static
void RunColorTests() {
  nscolor rgb;
  
  
  

  for (uint32_t index = 0 ; index < ArrayLength(kColorNames); index++) {
    
    nsCString tagName(kColorNames[index]);

    
    ASSERT_TRUE(NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) <<
      "can't find '" << tagName.get() << "'";
    ASSERT_TRUE((rgb == kColors[index])) <<
      "failed at index " << index << " out of " << ArrayLength(kColorNames);

    
    tagName.SetCharAt(tagName.CharAt(0) - 32, 0);
    ASSERT_TRUE(NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) <<
      "can't find '" << tagName.get() << "'";
    ASSERT_TRUE((rgb == kColors[index])) <<
      "failed at index " << index << " out of " << ArrayLength(kColorNames);

    
    uint8_t r = NS_GET_R(rgb);
    uint8_t g = NS_GET_G(rgb);
    uint8_t b = NS_GET_B(rgb);
    uint8_t a = NS_GET_A(rgb);
    if (a != UINT8_MAX) {
      
      rgb = NS_RGB(r, g, b);
    }
    char cbuf[50];
    PR_snprintf(cbuf, sizeof(cbuf), "%02x%02x%02x", r, g, b);
    nscolor hexrgb;
    ASSERT_TRUE(NS_HexToRGB(NS_ConvertASCIItoUTF16(cbuf), &hexrgb)) <<
      "hex conversion to color of '" << cbuf << "'";
    ASSERT_TRUE(hexrgb == rgb);
  }
}

static
void RunJunkColorTests() {
  nscolor rgb;
  
  for (uint32_t i = 0; i < ArrayLength(kJunkNames); i++) {
    nsCString tag(kJunkNames[i]);
    ASSERT_FALSE(NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tag), &rgb)) <<
      "Failed at junk color " << kJunkNames[i];
  }
}

TEST(Gfx, ColorNames) {
  RunColorTests();
}

TEST(Gfx, JunkColorNames) {
  RunJunkColorTests();
}
