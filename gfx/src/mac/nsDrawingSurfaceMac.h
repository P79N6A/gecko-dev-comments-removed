




































#ifndef nsDrawingSurfaceMac_h___
#define nsDrawingSurfaceMac_h___

#include "nsIDrawingSurface.h"
#include "nsIDrawingSurfaceMac.h"

#include "nsGfxUtils.h"
#include "nsCarbonHelpers.h"

class nsGraphicState;

class nsDrawingSurfaceMac : public nsIDrawingSurface,
                            nsIDrawingSurfaceMac
{
public:
  nsDrawingSurfaceMac();
  virtual ~nsDrawingSurfaceMac();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen) { *aOffScreen = mIsOffscreen; return NS_OK; }
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

  
  NS_IMETHOD Init(nsIDrawingSurface* aDS);
  NS_IMETHOD Init(CGrafPtr aThePort);
  NS_IMETHOD Init(nsIWidget *aTheWidget);
  NS_IMETHOD Init(PRUint32 aDepth,PRUint32 aWidth, PRUint32 aHeight,PRUint32 aFlags);
	NS_IMETHOD GetGrafPtr(CGrafPtr	*aTheGrafPtr) { *aTheGrafPtr = mPort; return NS_OK; }
  NS_IMETHOD_(CGContextRef) StartQuartzDrawing();
  NS_IMETHOD_(void) EndQuartzDrawing(CGContextRef aContext);

  
	nsGraphicState*	GetGS(void) {return mGS;}

private:
  CGrafPtr      mPort;      

  PRUint32      mWidth;
  PRUint32      mHeight;
  PRInt32       mLockOffset;
  PRInt32       mLockHeight;
  PRUint32      mLockFlags;
  PRBool        mIsOffscreen;
  PRBool        mIsLocked;

  nsGraphicState* mGS;      
#ifdef MOZ_WIDGET_COCOA
  void* mWidgetView;        
#endif
};

#endif
