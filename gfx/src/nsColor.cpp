




































#include "plstr.h"
#include "nsColor.h"
#include "nsColorNames.h"
#include "nsString.h"
#include "nscore.h"
#include "nsCoord.h"
#include "nsUnitConversion.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include <math.h>
#include "prprf.h"

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

NS_GFX_(PRBool) NS_ASCIIHexToRGB(const nsCString& aColorSpec,
                                            nscolor* aResult)
{
  return NS_HexToRGB(NS_ConvertASCIItoUTF16(aColorSpec), aResult);
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

NS_GFX_(void) NS_RGBToHex(nscolor aColor, nsAString& aResult)
{
  char buf[10];
  PR_snprintf(buf, sizeof(buf), "#%02x%02x%02x",
              NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor));
  CopyASCIItoUTF16(buf, aResult);
}

NS_GFX_(void) NS_RGBToASCIIHex(nscolor aColor,
                                          nsAFlatCString& aResult)
{
  aResult.SetLength(7);
  NS_ASSERTION(aResult.Length() == 7, "small SetLength failed, use an autostring instead!");
  char *buf = aResult.BeginWriting();
  PR_snprintf(buf, 8, "#%02x%02x%02x",
              NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor));
}

NS_GFX_(PRBool) NS_ColorNameToRGB(const nsAString& aColorName, nscolor* aResult)
{
  nsColorName id = nsColorNames::LookupName(aColorName);
  if (eColorName_UNKNOWN < id) {
    NS_ASSERTION(id < eColorName_COUNT, "LookupName mess up");
    if (nsnull != aResult) {
      *aResult = nsColorNames::kColors[id];
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_GFX_(nscolor) NS_BrightenColor(nscolor inColor)
{
  PRIntn r, g, b, max, over;

  r = NS_GET_R(inColor);
  g = NS_GET_G(inColor);
  b = NS_GET_B(inColor);

  
  r += 25;
  g += 25;
  b += 25;

  
  if (r > g)
  {
    if (b > r)
      max = b;
    else
      max = r;
  }
  else
  {
    if (b > g)
      max = b;
    else
      max = g;
  }

  
  
  if (max > 255)
  {
    over = max - 255;

    if (max == r)
    {
      g += over;
      b += over;
    }
    else if (max == g)
    {
      r += over;
      b += over;
    }
    else
    {
      r += over;
      g += over;
    }
  }

  
  if (r > 255)
    r = 255;
  if (g > 255)
    g = 255;
  if (b > 255)
    b = 255;

  return NS_RGBA(r, g, b, NS_GET_A(inColor));
}

NS_GFX_(nscolor) NS_DarkenColor(nscolor inColor)
{
  PRIntn r, g, b, max;

  r = NS_GET_R(inColor);
  g = NS_GET_G(inColor);
  b = NS_GET_B(inColor);

  
  r -= 25;
  g -= 25;
  b -= 25;

  
  if (r > g)
  {
    if (b > r)
      max = b;
    else
      max = r;
  }
  else
  {
    if (b > g)
      max = b;
    else
      max = g;
  }

  
  
  if (max < 0)
  {
    if (max == r)
    {
      g += max;
      b += max;
    }
    else if (max == g)
    {
      r += max;
      b += max;
    }
    else
    {
      r += max;
      g += max;
    }
  }

  
  if (r < 0)
    r = 0;
  if (g < 0)
    g = 0;
  if (b < 0)
    b = 0;

  return NS_RGBA(r, g, b, NS_GET_A(inColor));
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
