





































#ifndef nsDrawingSurfaceXlib_h__
#define nsDrawingSurfaceXlib_h__

#include "nsIDrawingSurface.h"
#include "nsGCCache.h"
#include "xlibrgb.h"




class nsIDrawingSurfaceXlib : public nsIDrawingSurface
{
public:
  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags) = 0;
  NS_IMETHOD Unlock(void) = 0;
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight) = 0;
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen) = 0;
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable) = 0;
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat) = 0;

  NS_IMETHOD GetDrawable(Drawable &aDrawable) = 0;
  NS_IMETHOD GetXlibRgbHandle(XlibRgbHandle *&aHandle) = 0;
  NS_IMETHOD GetGC(xGC *&aXGC) = 0;

  virtual Drawable GetDrawable() = 0;
};


class nsDrawingSurfaceXlibImpl : public nsIDrawingSurfaceXlib
{
public:
  nsDrawingSurfaceXlibImpl();
  virtual ~nsDrawingSurfaceXlibImpl();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

  NS_IMETHOD Init (XlibRgbHandle *aHandle,
                   Drawable  aDrawable, 
                   xGC       * aGC);

  NS_IMETHOD Init (XlibRgbHandle *aHandle,
                   xGC      *  aGC, 
                   PRUint32  aWidth, 
                   PRUint32  aHeight, 
                   PRUint32  aFlags);

  NS_IMETHOD GetDrawable(Drawable &aDrawable) { aDrawable = mDrawable; return NS_OK; }
  NS_IMETHOD GetXlibRgbHandle(XlibRgbHandle *&aHandle) { aHandle = mXlibRgbHandle; return NS_OK; }
  NS_IMETHOD GetGC(xGC *&aXGC) { mGC->AddRef(); aXGC = mGC; return NS_OK; }

  virtual Drawable GetDrawable() { return mDrawable; }

private:
  void       CommonInit();

  XlibRgbHandle *mXlibRgbHandle;
  Display *      mDisplay;
  Screen *       mScreen;
  Visual *       mVisual;
  int            mDepth;
  xGC           *mGC;
  Drawable       mDrawable;
  XImage *       mImage;
  nsPixelFormat  mPixFormat;

  
  PRInt32        mLockX;
  PRInt32        mLockY;
  PRUint32       mLockWidth;
  PRUint32       mLockHeight;
  PRUint32       mLockFlags;
  PRBool         mLocked;

  
  PRUint32       mWidth;
  PRUint32       mHeight;

  
  PRBool         mIsOffscreen;

private:
  static PRUint8 ConvertMaskToCount(unsigned long val);
  static PRUint8 GetShiftForMask(unsigned long val);
};

#endif 

