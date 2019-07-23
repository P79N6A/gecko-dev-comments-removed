




































#ifndef nsDrawingSurfaceBeOS_h___
#define nsDrawingSurfaceBeOS_h___

#include "nsIDrawingSurface.h"
#include "nsIDrawingSurfaceBeOS.h"

#include <Bitmap.h>
#include <View.h>

class nsDrawingSurfaceBeOS : public nsIDrawingSurface, 
                             public nsIDrawingSurfaceBeOS
{
public:
  nsDrawingSurfaceBeOS();
  virtual ~nsDrawingSurfaceBeOS();

  NS_DECL_ISUPPORTS

  

  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

  
  NS_IMETHOD Init(BView *aView);
  NS_IMETHOD Init(BView *aView, PRUint32 aWidth, PRUint32 aHeight,
                  PRUint32 aFlags);
                  




                 
  NS_IMETHOD AcquireView(BView **aView); 
  NS_IMETHOD ReleaseView(void);
  NS_IMETHOD AcquireBitmap(BBitmap **aBitmap);
  NS_IMETHOD ReleaseBitmap(void);

  void GetSize(PRUint32 *aWidth, PRUint32 *aHeight) { *aWidth = mWidth; *aHeight = mHeight; } 
  bool LockDrawable();
  void UnlockDrawable();
 
private: 
  BView         *mView;
  BBitmap       *mBitmap;
  nsPixelFormat mPixFormat;
  PRUint32      mWidth;
  PRUint32      mHeight;
  PRUint32      mFlags; 
  PRBool        mIsOffscreen; 
 
   
  PRUint32     mLockFlags;
  PRBool       mLocked;
};

#endif
