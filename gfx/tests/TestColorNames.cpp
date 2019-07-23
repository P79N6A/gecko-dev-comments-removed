




































#include "TestHarness.h"

#include <string.h>
#include "nsColor.h"
#include "nsColorNames.h"
#include "prprf.h"
#include "nsString.h"


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

static const char* kJunkNames[] = {
  nsnull,
  "",
  "123",
  "backgroundz",
  "zzzzzz",
  "#@$&@#*@*$@$#"
};

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("TestColorNames");
  if (xpcom.failed())
    return -1;

  nscolor rgb;
  int rv = 0;

  
  nsColorNames::AddRefTable();

  
  
  
    
  for (PRUint32 index = 0 ; index < NS_ARRAY_LENGTH(kColorNames); index++) {
    
    nsCString tagName(kColorNames[index]);

    
    if (!NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) {
      fail("can't find '%s'", tagName.get());
      rv = -1;
    }
    if (rgb != kColors[index]) {
      fail("name='%s' ColorNameToRGB=%x kColors[%d]=%08x",
           tagName.get(), rgb, index, kColors[index]);
      rv = -1;
    }

    
    tagName.SetCharAt(tagName.CharAt(0) - 32, 0);
    if (!NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) {
      fail("can't find '%s'", tagName.get());
      rv = -1;
    }
    if (rgb != kColors[index]) {
      fail("name='%s' ColorNameToRGB=%x kColors[%d]=%08x",
           tagName.get(), rgb, index, kColors[index]);
      rv = -1;
    }

    
    PRUint8 r = NS_GET_R(rgb);
    PRUint8 g = NS_GET_G(rgb);
    PRUint8 b = NS_GET_B(rgb);
    PRUint8 a = NS_GET_A(rgb);
    if (a != PR_UINT8_MAX) {
      
      rgb = NS_RGB(r, g, b);
    }
    char cbuf[50];
    PR_snprintf(cbuf, sizeof(cbuf), "%02x%02x%02x", r, g, b);
    nscolor hexrgb;
    if (!NS_HexToRGB(NS_ConvertASCIItoUTF16(cbuf), &hexrgb)) {
      fail("hex conversion to color of '%s'", cbuf);
      rv = -1;
    }
    if (hexrgb != rgb) {
      fail("rgb=%x hexrgb=%x", rgb, hexrgb);
      rv = -1;
    }
  }

  
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(kJunkNames); i++) {
    nsCString tag(kJunkNames[i]);
    if (NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tag), &rgb)) {
      fail("found '%s'", kJunkNames[i] ? kJunkNames[i] : "(null)");
      rv = -1;
    }
  }

  nsColorNames::ReleaseTable();

  if (rv == 0)
    passed("TestColorNames");
  return rv;
}
