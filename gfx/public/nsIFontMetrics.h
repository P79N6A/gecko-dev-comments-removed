




































#ifndef nsIFontMetrics_h___
#define nsIFontMetrics_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsFont.h"

class nsString;
class nsIDeviceContext;
class nsIAtom;


#define NS_IFONT_METRICS_IID   \
{ 0xc74cb770, 0xa33e, 0x11d1, \
{ 0xa8, 0x24, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }






typedef void* nsFontHandle;


















class nsIFontMetrics : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFONT_METRICS_IID)

  





  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                   nsIDeviceContext *aContext) = 0;

  



  NS_IMETHOD  Destroy() = 0;

  


  NS_IMETHOD  GetXHeight(nscoord& aResult) = 0;

  




  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult) = 0;

  




  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult) = 0;

  




  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize) = 0;

  




  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize) = 0;

  






  NS_IMETHOD  GetHeight(nscoord &aHeight) = 0;


#if defined(XP_WIN) || defined(XP_OS2) || defined(MOZ_CAIRO_GFX)
#define FONT_LEADING_APIS_V2 1
#endif 

#ifdef FONT_LEADING_APIS_V2
  



  NS_IMETHOD  GetInternalLeading(nscoord &aLeading) = 0;

  




  NS_IMETHOD  GetExternalLeading(nscoord &aLeading) = 0;
#else
  



  NS_IMETHOD  GetLeading(nscoord &aLeading) = 0;

  


  NS_IMETHOD  GetNormalLineHeight(nscoord &aHeight) = 0;
#endif 

  



  NS_IMETHOD  GetEmHeight(nscoord &aHeight) = 0;

  


  NS_IMETHOD  GetEmAscent(nscoord &aAscent) = 0;

  


  NS_IMETHOD  GetEmDescent(nscoord &aDescent) = 0;

  



  NS_IMETHOD  GetMaxHeight(nscoord &aHeight) = 0;

  



  NS_IMETHOD  GetMaxAscent(nscoord &aAscent) = 0;

  



  NS_IMETHOD  GetMaxDescent(nscoord &aDescent) = 0;

  


  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance) = 0;

  



  const nsFont &Font() { return mFont; }

  


  NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup) = 0;

  


  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle) = 0;

  


  NS_IMETHOD  GetAveCharWidth(nscoord& aAveCharWidth) = 0;

  


  NS_IMETHOD  GetSpaceWidth(nscoord& aSpaceCharWidth) = 0;

protected:

  nsFont mFont;		
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFontMetrics, NS_IFONT_METRICS_IID)

#endif 
