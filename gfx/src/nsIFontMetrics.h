




































#ifndef nsIFontMetrics_h___
#define nsIFontMetrics_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsFont.h"

class nsString;
class nsIDeviceContext;
class nsIAtom;
class gfxUserFontSet;


#define NS_IFONT_METRICS_IID   \
{ 0x360C5575, 0xF7AC, 0x4079, \
{ 0xB8, 0xA6, 0x56, 0x91, 0x4B, 0xEA, 0x2A, 0xEA } }






typedef void* nsFontHandle;


















class nsIFontMetrics : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFONT_METRICS_IID)

  





  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLanguage,
                   nsIDeviceContext *aContext, gfxUserFontSet *aUserFontSet = nsnull) = 0;

  



  NS_IMETHOD  Destroy() = 0;

  


  NS_IMETHOD  GetXHeight(nscoord& aResult) = 0;

  




  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult) = 0;

  




  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult) = 0;

  




  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize) = 0;

  




  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize) = 0;

  






  NS_IMETHOD  GetHeight(nscoord &aHeight) = 0;

  



  NS_IMETHOD  GetInternalLeading(nscoord &aLeading) = 0;

  




  NS_IMETHOD  GetExternalLeading(nscoord &aLeading) = 0;

  



  NS_IMETHOD  GetEmHeight(nscoord &aHeight) = 0;

  


  NS_IMETHOD  GetEmAscent(nscoord &aAscent) = 0;

  


  NS_IMETHOD  GetEmDescent(nscoord &aDescent) = 0;

  



  NS_IMETHOD  GetMaxHeight(nscoord &aHeight) = 0;

  



  NS_IMETHOD  GetMaxAscent(nscoord &aAscent) = 0;

  



  NS_IMETHOD  GetMaxDescent(nscoord &aDescent) = 0;

  


  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance) = 0;

  



  const nsFont &Font() { return mFont; }

  


  NS_IMETHOD  GetLanguage(nsIAtom** aLanguage) = 0;

  


  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle) = 0;

  


  NS_IMETHOD  GetAveCharWidth(nscoord& aAveCharWidth) = 0;

  


  NS_IMETHOD  GetSpaceWidth(nscoord& aSpaceCharWidth) = 0;

protected:

  nsFont mFont;		
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFontMetrics, NS_IFONT_METRICS_IID)

#endif 
