






































#include <stdio.h>
#include <string.h>
#include "nsColorNames.h"
#include "prprf.h"
#include "nsString.h"

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
  nsColorName id;
  nsColorName index;
  int rv = 0;

  
  nsColorNames::AddRefTable();

  
  
  
    
  index = eColorName_UNKNOWN;
  while (PRInt32(index) < (PRInt32 (eColorName_COUNT) - 1)) {
    
    index = nsColorName(PRInt32(index) + 1);
    nsCString tagName(nsColorNames::GetStringValue(index));
    if (tagName.IsEmpty()) {
      printf("bug: tagName for nsColorNames::GetStringValue(%d) is ''\n", index);
      rv = -1;
      continue;
    }

    id = nsColorNames::LookupName(NS_ConvertASCIItoUTF16(tagName));
    if (id == eColorName_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName.get());
      rv = -1;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName.get(), id, index);
      rv = -1;
    }

    
    tagName.SetCharAt(tagName.CharAt(0) - 32, 0);
    id = nsColorNames::LookupName(NS_ConvertASCIItoUTF16(tagName));
    if (id == eColorName_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName.get());
      rv = -1;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName.get(), id, index);
      rv = -1;
    }

    
    nscolor rgb;
    if (!NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(tagName), &rgb)) {
      printf("bug: name='%s' didn't NS_ColorNameToRGB\n", tagName.get());
      rv = -1;
    }
    if (nsColorNames::kColors[index] != rgb) {
      printf("bug: name='%s' ColorNameToRGB=%x kColors[%d]=%x\n",
             tagName.get(), rgb, nsColorNames::kColors[index],
             nsColorNames::kColors[index]);
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

  
  for (int i = 0; i < (int) (sizeof(kJunkNames) / sizeof(const char*)); i++) {
    const char* tag = kJunkNames[i];
    id = nsColorNames::LookupName(NS_ConvertASCIItoUTF16(tag));
    if (id > eColorName_UNKNOWN) {
      printf("bug: found '%s'\n", tag ? tag : "(null)");
      rv = -1;
    }
  }

  return rv;
}
