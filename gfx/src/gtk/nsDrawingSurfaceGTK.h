




































#ifndef nsDrawingSurfaceGTK_h___
#define nsDrawingSurfaceGTK_h___

#include "nsIDrawingSurface.h"

#include "nsTimer.h"
#include "nsIRegion.h"
#include "nsCOMPtr.h"

#include <gtk/gtk.h>

#ifdef MOZ_ENABLE_XFT
typedef struct _XftDraw XftDraw;
#endif

class nsDrawingSurfaceGTK : public nsIDrawingSurface
{
public:
  nsDrawingSurfaceGTK();
  virtual ~nsDrawingSurfaceGTK();

  







  nsresult Init(GdkDrawable *aDrawable, GdkGC *aGC);

  













  nsresult Init(GdkGC *aGC, PRUint32 aWidth, PRUint32 aHeight,
                PRUint32 aFlags);
  
  NS_DECL_ISUPPORTS

  

  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);






  GdkDrawable *GetDrawable(void) { return mPixmap; }

  void GetSize(PRUint32 *aWidth, PRUint32 *aHeight) { *aWidth = mWidth; *aHeight = mHeight; }

  PRInt32 GetDepth() { return mDepth; }

#ifdef MOZ_ENABLE_XFT
  XftDraw *GetXftDraw     (void);
  void     GetLastXftClip (nsIRegion **aLastRegion);
  void     SetLastXftClip (nsIRegion  *aLastRegion);
#endif 

protected:
  inline PRUint8 ConvertMaskToCount(unsigned long val);

private:
  
  GdkPixmap	*mPixmap;
  GdkGC		*mGC;
  gint		mDepth;
  nsPixelFormat	mPixFormat;
  PRUint32      mWidth;
  PRUint32      mHeight;
  PRUint32	mFlags;
  PRBool	mIsOffscreen;

  
  GdkImage	*mImage;
  PRInt32	mLockX;
  PRInt32	mLockY;
  PRUint32	mLockWidth;
  PRUint32	mLockHeight;
  PRUint32	mLockFlags;
  PRBool	mLocked;

#ifdef MOZ_ENABLE_XFT
  XftDraw             *mXftDraw;
  nsCOMPtr<nsIRegion>  mLastXftClip;
#endif

  
  
};

#endif
