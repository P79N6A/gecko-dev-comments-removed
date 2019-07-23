




































#include "plstr.h"
#include "nsColor.h"
#include "nsColorNames.h"
#include "nsString.h"
#include "nscore.h"
#include "nsCoord.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include <math.h>
#include "prprf.h"
#include "nsStaticNameTable.h"


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

#define eColorName_COUNT (NS_ARRAY_LENGTH(kColorNames))
#define eColorName_UNKNOWN (-1)

static nsStaticCaseInsensitiveNameTable* gColorTable = nsnull;

void nsColorNames::AddRefTable(void) 
{
  NS_ASSERTION(!gColorTable, "pre existing array!");
  if (!gColorTable) {
    gColorTable = new nsStaticCaseInsensitiveNameTable();
    if (gColorTable) {
#ifdef DEBUG
    {
      
      for (PRInt32 index = 0; index < eColorName_COUNT; ++index) {
        nsCAutoString temp1(kColorNames[index]);
        nsCAutoString temp2(kColorNames[index]);
        ToLowerCase(temp1);
        NS_ASSERTION(temp1.Equals(temp2), "upper case char in table");
      }
    }
#endif      
      gColorTable->Init(kColorNames, eColorName_COUNT); 
    }
  }
}

void nsColorNames::ReleaseTable(void)
{
  if (gColorTable) {
    delete gColorTable;
    gColorTable = nsnull;
  }
}

static int ComponentValue(const PRUnichar* aColorSpec, int aLen, int color, int dpc)
{
  int component = 0;
  int index = (color * dpc);
  if (2 < dpc) {
    dpc = 2;
  }
  while (--dpc >= 0) {
    PRUnichar ch = ((index < aLen) ? aColorSpec[index++] : '0');
    if (('0' <= ch) && (ch <= '9')) {
      component = (component * 16) + (ch - '0');
    } else if ((('a' <= ch) && (ch <= 'f')) || 
               (('A' <= ch) && (ch <= 'F'))) {
      
      component = (component * 16) + (ch & 7) + 9;
    }
    else {  
      component = (component * 16);
    }
  }
  return component;
}

NS_GFX_(PRBool) NS_HexToRGB(const nsString& aColorSpec,
                                       nscolor* aResult)
{
  const PRUnichar* buffer = aColorSpec.get();

  int nameLen = aColorSpec.Length();
  if ((nameLen == 3) || (nameLen == 6)) {
    
    for (int i = 0; i < nameLen; i++) {
      PRUnichar ch = buffer[i];
      if (((ch >= '0') && (ch <= '9')) ||
          ((ch >= 'a') && (ch <= 'f')) ||
          ((ch >= 'A') && (ch <= 'F'))) {
        
        continue;
      }
      
      return PR_FALSE;
    }

    
    int dpc = ((3 == nameLen) ? 1 : 2);
    
    int r = ComponentValue(buffer, nameLen, 0, dpc);
    int g = ComponentValue(buffer, nameLen, 1, dpc);
    int b = ComponentValue(buffer, nameLen, 2, dpc);
    if (dpc == 1) {
      
      
      r = (r << 4) | r;
      g = (g << 4) | g;
      b = (b << 4) | b;
    }
    NS_ASSERTION((r >= 0) && (r <= 255), "bad r");
    NS_ASSERTION((g >= 0) && (g <= 255), "bad g");
    NS_ASSERTION((b >= 0) && (b <= 255), "bad b");
    if (nsnull != aResult) {
      *aResult = NS_RGB(r, g, b);
    }
    return PR_TRUE;
  }

  
  return PR_FALSE;
}


NS_GFX_(PRBool) NS_LooseHexToRGB(const nsString& aColorSpec, nscolor* aResult)
{
  int nameLen = aColorSpec.Length();
  const PRUnichar* colorSpec = aColorSpec.get();
  if ('#' == colorSpec[0]) {
    ++colorSpec;
    --nameLen;
  }

  if (3 < nameLen) {
    
    int dpc = (nameLen / 3) + (((nameLen % 3) != 0) ? 1 : 0);
    if (4 < dpc) {
      dpc = 4;
    }

    
    int r = ComponentValue(colorSpec, nameLen, 0, dpc);
    int g = ComponentValue(colorSpec, nameLen, 1, dpc);
    int b = ComponentValue(colorSpec, nameLen, 2, dpc);
    NS_ASSERTION((r >= 0) && (r <= 255), "bad r");
    NS_ASSERTION((g >= 0) && (g <= 255), "bad g");
    NS_ASSERTION((b >= 0) && (b <= 255), "bad b");
    if (nsnull != aResult) {
      *aResult = NS_RGB(r, g, b);
    }
  }
  else {
    if (nsnull != aResult) {
      *aResult = NS_RGB(0, 0, 0);
    }
  }
  return PR_TRUE;
}

NS_GFX_(PRBool) NS_ColorNameToRGB(const nsAString& aColorName, nscolor* aResult)
{
  if (!gColorTable) return PR_FALSE;

  PRInt32 id = gColorTable->Lookup(aColorName);
  if (eColorName_UNKNOWN < id) {
    NS_ASSERTION(id < eColorName_COUNT, "LookupName mess up");
    if (aResult) {
      *aResult = kColors[id];
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_GFX_(nscolor)
NS_ComposeColors(nscolor aBG, nscolor aFG)
{
  PRIntn bgAlpha = NS_GET_A(aBG);
  PRIntn r, g, b, a;

  
  MOZ_BLEND(r, 0, NS_GET_R(aBG), bgAlpha);
  MOZ_BLEND(g, 0, NS_GET_G(aBG), bgAlpha);
  MOZ_BLEND(b, 0, NS_GET_B(aBG), bgAlpha);
  a = bgAlpha;

  
  PRIntn fgAlpha = NS_GET_A(aFG);
  MOZ_BLEND(r, r, NS_GET_R(aFG), fgAlpha);
  MOZ_BLEND(g, g, NS_GET_G(aFG), fgAlpha);
  MOZ_BLEND(b, b, NS_GET_B(aFG), fgAlpha);
  MOZ_BLEND(a, a, 255, fgAlpha);
  
  return NS_RGBA(r, g, b, a);
}





static float
HSL_HueToRGB(float m1, float m2, float h)
{
  if (h < 0.0f)
    h += 1.0f;
  if (h > 1.0f)
    h -= 1.0f;
  if (h < (float)(1.0/6.0))
    return m1 + (m2 - m1)*h*6.0f;
  if (h < (float)(1.0/2.0))
    return m2;
  if (h < (float)(2.0/3.0))
    return m1 + (m2 - m1)*((float)(2.0/3.0) - h)*6.0f;
  return m1;      
}


NS_GFX_(nscolor)
NS_HSL2RGB(float h, float s, float l)
{
  PRUint8 r, g, b;
  float m1, m2;
  if (l <= 0.5f) {
    m2 = l*(s+1);
  } else {
    m2 = l + s - l*s;
  }
  m1 = l*2 - m2;
  r = PRUint8(255 * HSL_HueToRGB(m1, m2, h + 1.0f/3.0f));
  g = PRUint8(255 * HSL_HueToRGB(m1, m2, h));
  b = PRUint8(255 * HSL_HueToRGB(m1, m2, h - 1.0f/3.0f));
  return NS_RGB(r, g, b);  
}
