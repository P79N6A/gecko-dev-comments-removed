 






































#ifndef nsBlender_h___
#define nsBlender_h___

#include "nsCOMPtr.h"
#include "nsIBlender.h"
#include "nsIDeviceContext.h"

typedef enum
{
  nsLowQual = 0,
  nsLowMedQual,
  nsMedQual,
  nsHighMedQual,
  nsHighQual
} nsBlendQuality;




class NS_GFX nsBlender : public nsIBlender
{
public:
 


  nsBlender();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD Init(nsIDeviceContext *aContext);
  NS_IMETHOD Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight,nsIDrawingSurface* aSrc,
                   nsIDrawingSurface* aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                   nsIDrawingSurface* aSecondSrc = nsnull, nscolor aSrcBackColor = NS_RGB(0, 0, 0),
                   nscolor aSecondSrcBackColor = NS_RGB(0, 0, 0));
  NS_IMETHOD Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight, nsIRenderingContext *aSrc,
                   nsIRenderingContext *aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                   nsIRenderingContext *aSecondSrc = nsnull, nscolor aSrcBackColor = NS_RGB(0, 0, 0),
                   nscolor aSecondSrcBackColor = NS_RGB(0, 0, 0));

  NS_IMETHOD GetAlphas(const nsRect& aRect, nsIDrawingSurface* aBlack,
                       nsIDrawingSurface* aWhite, PRUint8** aAlphas);

protected:

 


  virtual ~nsBlender();

  
  nsresult Blend(PRUint8 *aSrcBits, PRInt32 aSrcStride,
                 PRUint8 *aDestBits, PRInt32 aDestStride,
                 PRUint8 *aSecondSrcBits,
                 PRInt32 aSrcBytes, PRInt32 aLines, float aOpacity,
                 PRUint8 aDepth);

  











  void Do32Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                 PRUint8 *aSImage, PRUint8 *aDImage, PRUint8 *aSecondSImage,
                 PRInt32 aSLSpan, PRInt32 aDLSpan, nsBlendQuality aTheQual);

 











  void Do24Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                 PRUint8 *aSImage, PRUint8 *aDImage, PRUint8 *aSecondSImage,
                 PRInt32 aSLSpan, PRInt32 aDLSpan, nsBlendQuality aBlendQuality);

 











  void Do16Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                 PRUint8 *aSImage, PRUint8 *aDImage, PRUint8 *aSecondSImage,
                 PRInt32 aSLSpan, PRInt32 aDLSpan, nsBlendQuality aBlendQuality);
};

#endif 

