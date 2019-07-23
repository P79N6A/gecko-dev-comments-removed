




































#ifndef nsColor_h___
#define nsColor_h___

#include "gfxCore.h"
#include "nscore.h"

class nsAString;
class nsString;
class nsCString;



typedef PRUint32 nscolor;



#define NS_RGB(_r,_g,_b) \
  ((nscolor) ((255 << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))



#define NS_RGBA(_r,_g,_b,_a) \
  ((nscolor) (((_a) << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))


#define NS_GET_R(_rgba) ((PRUint8) ((_rgba) & 0xff))
#define NS_GET_G(_rgba) ((PRUint8) (((_rgba) >> 8) & 0xff))
#define NS_GET_B(_rgba) ((PRUint8) (((_rgba) >> 16) & 0xff))
#define NS_GET_A(_rgba) ((PRUint8) (((_rgba) >> 24) & 0xff))







#define FAST_DIVIDE_BY_255(target,v)               \
  PR_BEGIN_MACRO                                   \
    unsigned tmp_ = v;                             \
    target = ((tmp_ << 8) + tmp_ + 255) >> 16;     \
  PR_END_MACRO




#define MOZ_BLEND(target, bg, fg, alpha) \
        FAST_DIVIDE_BY_255(target, (bg)*(255-(alpha)) + (fg)*(alpha))




NS_GFX_(PRBool) NS_HexToRGB(const nsString& aBuf, nscolor* aResult);
NS_GFX_(PRBool) NS_ASCIIHexToRGB(const nsCString& aBuf,
                                            nscolor* aResult);




NS_GFX_(nscolor) NS_ComposeColors(nscolor aBG, nscolor aFG);




NS_GFX_(PRBool) NS_LooseHexToRGB(const nsString& aBuf, nscolor* aResult);



NS_GFX_(void) NS_RGBToHex(nscolor aColor, nsAString& aResult);
NS_GFX_(void) NS_RGBToASCIIHex(nscolor aColor,
                                          nsCString& aResult);



NS_GFX_(PRBool) NS_ColorNameToRGB(const nsAString& aBuf, nscolor* aResult);



NS_GFX_(nscolor) NS_BrightenColor(nscolor inColor);



NS_GFX_(nscolor) NS_DarkenColor(nscolor inColor);



NS_GFX_(nscolor) NS_HSL2RGB(float h, float s, float l);

#endif 
