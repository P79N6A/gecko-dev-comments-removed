




































 
#ifndef nsRenderingContextXp_h___
#define nsRenderingContextXp_h___ 1

#include "nsXPrintContext.h"
#include "nsRenderingContextXlib.h"







class nsRenderingContextXp : public nsRenderingContextXlib
{
 public:
  nsRenderingContextXp();
  virtual ~nsRenderingContextXp();
   
  NS_IMETHOD Init(nsIDeviceContext* aContext);
  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWindow);
  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIDrawingSurface* aSurface);

  NS_IMETHOD LockDrawingSurface(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                                void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                                PRUint32 aFlags);
  NS_IMETHOD UnlockDrawingSurface(void);
 
  NS_IMETHOD DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect);

  NS_IMETHOD DrawTile(imgIContainer *aImage, nscoord aXImageStart, nscoord aYImageStart, const nsRect *aTargetRect);

  NS_IMETHOD RenderEPS(const nsRect& aRect, FILE *aDataFile);
                               
protected:
  nsXPrintContext *mPrintContext; 


};
#endif 
