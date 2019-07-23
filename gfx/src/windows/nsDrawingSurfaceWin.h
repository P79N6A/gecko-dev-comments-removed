




































#ifndef nsDrawingSurfaceWin_h___
#define nsDrawingSurfaceWin_h___

#include "nsIDrawingSurface.h"
#include "nsIDrawingSurfaceWin.h"

#ifdef NGLAYOUT_DDRAW
#include "ddraw.h"
#endif

class nsDrawingSurfaceWin : public nsIDrawingSurface,
                            nsIDrawingSurfaceWin
{
public:
  nsDrawingSurfaceWin();

  NS_DECL_ISUPPORTS

  

  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

  

  NS_IMETHOD Init(HDC aDC);
  NS_IMETHOD Init(HDC aDC, PRUint32 aWidth, PRUint32 aHeight,
                  PRUint32 aFlags);
  NS_IMETHOD GetDC(HDC *aDC);
  NS_IMETHOD GetTECHNOLOGY(PRInt32  *aTechnology); 
  NS_IMETHOD ReleaseDC(void);
  NS_IMETHOD IsReleaseDCDestructive(PRBool *aDestructive);

  
#ifdef NGLAYOUT_DDRAW
  nsresult GetDDraw(IDirectDraw2 **aDDraw);
#endif

private:
  ~nsDrawingSurfaceWin();

  BITMAPINFO *CreateBitmapInfo(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
                               void **aBits = nsnull);

#ifdef NGLAYOUT_DDRAW
  nsresult CreateDDraw(void);
  PRBool LockSurface(IDirectDrawSurface *aSurface, DDSURFACEDESC *aDesc,
                     BITMAP *aBitmap, RECT *aRect, DWORD aLockFlags, nsPixelFormat *aPixFormat);
#endif

  HDC           mDC;
  HBITMAP       mOrigBitmap;
  HBITMAP       mSelectedBitmap;
  PRBool        mKillDC;
  BITMAPINFO    *mBitmapInfo;
  PRUint8       *mDIBits;
  BITMAP        mBitmap;
  nsPixelFormat mPixFormat;
  HBITMAP       mLockedBitmap;
  PRUint32      mWidth;
  PRUint32      mHeight;
  PRInt32       mLockOffset;
  PRInt32       mLockHeight;
  PRUint32      mLockFlags;
  PRInt32       mTechnology;


#ifdef NGLAYOUT_DDRAW
  IDirectDrawSurface  *mSurface;
  PRInt32             mSurfLockCnt;
  DDSURFACEDESC       mSurfDesc;

  static IDirectDraw  *mDDraw;
  static IDirectDraw2 *mDDraw2;
  static nsresult     mDDrawResult;
#endif
};

#endif
