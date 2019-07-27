




#ifndef nsColor_h___
#define nsColor_h___

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxCore.h"                    
#include "nscore.h"                     

class nsAString;
class nsString;



typedef uint32_t nscolor;



#define NS_RGB(_r,_g,_b) \
  ((nscolor) ((255 << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))



#define NS_RGBA(_r,_g,_b,_a) \
  ((nscolor) (((_a) << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))


#define NS_RGBA_FROM_GFXRGBA(gfxColor) \
  ((nscolor) (gfxColor.Packed()))


#define NS_GET_R(_rgba) ((uint8_t) ((_rgba) & 0xff))
#define NS_GET_G(_rgba) ((uint8_t) (((_rgba) >> 8) & 0xff))
#define NS_GET_B(_rgba) ((uint8_t) (((_rgba) >> 16) & 0xff))
#define NS_GET_A(_rgba) ((uint8_t) (((_rgba) >> 24) & 0xff))







#define FAST_DIVIDE_BY_255(target,v)               \
  PR_BEGIN_MACRO                                   \
    unsigned tmp_ = v;                             \
    target = ((tmp_ << 8) + tmp_ + 255) >> 16;     \
  PR_END_MACRO




NS_GFX_(bool) NS_HexToRGB(const nsAString& aBuf, nscolor* aResult);



NS_GFX_(nscolor) NS_ComposeColors(nscolor aBG, nscolor aFG);




NS_GFX_(bool) NS_LooseHexToRGB(const nsString& aBuf, nscolor* aResult);






NS_GFX_(bool) NS_ColorNameToRGB(const nsAString& aBuf, nscolor* aResult);



NS_GFX_(const char * const *) NS_AllColorNames(size_t *aSizeArray);



NS_GFX_(nscolor) NS_HSL2RGB(float h, float s, float l);






NS_GFX_(const char*) NS_RGBToColorName(nscolor aColor);

#endif 
