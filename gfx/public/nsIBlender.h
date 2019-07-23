




































#ifndef nsIBlender_h___
#define nsIBlender_h___

#include "nscore.h"
#include "nsISupports.h"
#include "nsIRenderingContext.h"


#define NS_IBLENDER_IID    \
{ 0xbdb4b5b0, 0xf0db, 0x11d1, \
{ 0xa8, 0x2a, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }




class nsIBlender : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBLENDER_IID)

  





  NS_IMETHOD Init(nsIDeviceContext *aContext) = 0;

  





















  NS_IMETHOD Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight,nsIDrawingSurface* aSrc,
                   nsIDrawingSurface* aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                   nsIDrawingSurface* aSecondSrc = nsnull, nscolor aSrcBackColor = NS_RGB(0, 0, 0),
                   nscolor aSecondSrcBackColor = NS_RGB(0, 0, 0)) = 0;

  NS_IMETHOD Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight, nsIRenderingContext *aSrc,
                   nsIRenderingContext *aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                   nsIRenderingContext *aSecondSrc = nsnull, nscolor aSrcBackColor = NS_RGB(0, 0, 0),
                   nscolor aSecondSrcBackColor = NS_RGB(0, 0, 0)) = 0;

  





  NS_IMETHOD GetAlphas(const nsRect& aRect, nsIDrawingSurface* aBlack,
                       nsIDrawingSurface* aWhite, PRUint8** aAlphas) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIBlender, NS_IBLENDER_IID)

#endif
