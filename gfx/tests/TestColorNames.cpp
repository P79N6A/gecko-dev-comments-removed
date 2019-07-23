






































#include <stdio.h>
#include <string.h>
#include "nsColor.h"
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
  nscolor rgb;
  int rv = 0;

  
  nsColorNames::AddRefTable();

  
  
  
    
  for (int index = 0 ; index < NS_ARRAY_LENGTH(kColorNames); index++) {
    
    nsCString tagName(kColorNames[index]);

    
    if (!NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) {
      printf("bug: can't find '%s'\n", tagName.get());
      rv = -1;
    }
    if (rgb != kColors[index]) {
      printf("bug: name='%s' ColorNameToRGB=%x kColors[%d]=%08x\n",
             tagName.get(), rgb, index, kColors[index]);
      rv = -1;
    }

    
    tagName.SetCharAt(tagName.CharAt(0) - 32, 0);
    if (!NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) {
      printf("bug: can't find '%s'\n", tagName.get());
      rv = -1;
    }
    if (rgb != kColors[index]) {
      printf("bug: name='%s' ColorNameToRGB=%x kColors[%d]=%08x\n",
             tagName.get(), rgb, index, kColors[index]);
      rv = -1;
    }

    
    PRUint8 r = NS_GET_R(rgb);
    PRUint8 g = NS_GET_G(rgb);
    PRUint8 b = NS_GET_B(rgb);
    char cbuf[50];
    PR_snprintf(cbuf, sizeof(cbuf), "%02x%02x%02x", r, g, b);
    nscolor hexrgb;
    if (!NS_HexToRGB(NS_ConvertASCIItoUTF16(cbuf), &hexrgb)) {
      printf("bug: hex conversion to color of '%s' failed\n", cbuf);
      rv = -1;
    }
    if (hexrgb != rgb) {
      printf("bug: rgb=%x hexrgb=%x\n", rgb, hexrgb);
      rv = -1;
    }
  }

  
  for (int i = 0; i < NS_ARRAY_LENGTH(kJunkNames); i++) {
    nsCString tag(kJunkNames[i]);
    if (NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tag), &rgb)) {
      printf("bug: found '%s'\n", kJunkNames[i] ? kJunkNames[i] : "(null)");
      rv = -1;
    }
  }

  nsColorNames::ReleaseTable();

  return rv;
}
