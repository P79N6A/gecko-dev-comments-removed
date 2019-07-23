




































#include "nsDrawingSurfaceBeOS.h"
#include "nsCoord.h"

#include <Region.h>

NS_IMPL_ISUPPORTS2(nsDrawingSurfaceBeOS, nsIDrawingSurface, nsIDrawingSurfaceBeOS)

#ifdef CHEAP_PERFORMANCE_MEASUREMENT 
static PRTime mLockTime, mUnlockTime; 
#endif 
 
nsDrawingSurfaceBeOS :: nsDrawingSurfaceBeOS()
{
  mView = nsnull;
  mBitmap = nsnull;
  mWidth = 0; 
  mHeight = 0; 
  mLockFlags = 0;
  mLocked = PR_FALSE;
}

nsDrawingSurfaceBeOS :: ~nsDrawingSurfaceBeOS()
{
  if(mBitmap)
  {
    
    mBitmap->Unlock();
    delete mBitmap;
    mView = nsnull;
    mBitmap = nsnull;
  }
}

















 
NS_IMETHODIMP nsDrawingSurfaceBeOS :: Lock(PRInt32 aX, PRInt32 aY,
                                          PRUint32 aWidth, PRUint32 aHeight,
                                          void **aBits, PRInt32 *aStride,
                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  mLockFlags = aFlags;

  if (mBitmap && !mLocked)
  {
    if (mView)
      mView->Sync();
    if (mLockFlags & NS_LOCK_SURFACE_READ_ONLY)
      mBitmap->LockBits();
    *aStride = mBitmap->BytesPerRow();
    *aBits = (uint8 *)mBitmap->Bits() + aX*4 + *aStride * aY;
    *aWidthBytes = aWidth*4;
    mLocked = PR_TRUE; 
  }
  else
  {
    NS_ASSERTION(0, "nested lock attempt");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: Unlock(void)
{
  if (mBitmap && mLocked)
  {
    if (mLockFlags & NS_LOCK_SURFACE_READ_ONLY)
      mBitmap->UnlockBits();
    mLocked = PR_FALSE; 
  }
  return NS_OK;
}


NS_IMETHODIMP nsDrawingSurfaceBeOS :: GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight)
{
  *aWidth = mWidth;
  *aHeight = mHeight;

  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: IsOffscreen(PRBool *aOffScreen)
{
  *aOffScreen = mIsOffscreen;
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: IsPixelAddressable(PRBool *aAddressable)
{
  *aAddressable = PR_FALSE; 
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: GetPixelFormat(nsPixelFormat *aFormat)
{
  *aFormat = mPixFormat;
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: Init(BView *aView)
{
  if (aView->LockLooper()) 
  { 
    
    BRect r = aView->Bounds();
    mWidth = nscoord(r.IntegerWidth() + 1);
    mHeight = nscoord(r.IntegerHeight() + 1);
    
    mView = aView;
    aView->UnlockLooper(); 
  } 
 
  
  mIsOffscreen = PR_FALSE; 
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: Init(BView *aView, PRUint32 aWidth,
                                          PRUint32 aHeight, PRUint32 aFlags)
{
  NS_ASSERTION(!(aView == nsnull), "null BView");

  
  mWidth=aWidth;
  mHeight=aHeight;
  mFlags = aFlags; 
  
  
  mIsOffscreen = PR_TRUE; 
  
  
  BRect r(0,0, mWidth-1, mHeight-1);
  
  mView = new BView(r, "", 0, 0);
  if (!mView)
    return NS_ERROR_OUT_OF_MEMORY;



  if (aWidth > 0 && aHeight > 0)
  {
    
    mBitmap = new BBitmap(r, B_RGBA32, true);
    if (!mBitmap)
      return NS_ERROR_OUT_OF_MEMORY;

    if (mBitmap->InitCheck()!=B_OK)
    {
      
      
      delete mBitmap;
      mBitmap=NULL;
      return NS_ERROR_FAILURE;
    }
    
    
    
    
    mBitmap->Lock();
    
    
    mView->SetViewColor(B_TRANSPARENT_32_BIT);
    mBitmap->AddChild(mView);
    
    if (aView && aView->LockLooper())
    {
      BRegion region;
      BFont font;
      mView->SetHighColor(aView->HighColor());
      mView->SetLowColor(aView->LowColor());
      aView->GetFont(&font);
      mView->SetFont(&font);
      aView->GetClippingRegion(&region);
      mView->ConstrainClippingRegion(&region);
      mView->SetOrigin(aView->Origin());
      mView->SetFlags(aView->Flags());
      aView->UnlockLooper();
    } 
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: AcquireView(BView **aView) 
{
  *aView = mView;
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: AcquireBitmap(BBitmap **aBitmap)
{
  if (mBitmap && mView)
  {
    mView->Sync();
  }
  *aBitmap = mBitmap;

  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: ReleaseView(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceBeOS :: ReleaseBitmap(void)
{
  return NS_OK;
}

bool nsDrawingSurfaceBeOS :: LockDrawable()
{
  
  
  bool rv = false;
  if (!mBitmap)
  {
    
    rv = mView && mView->LockLooper();
  }
  else
  {
    
  	rv = mBitmap->IsLocked();
  }
  return rv;
}

void nsDrawingSurfaceBeOS :: UnlockDrawable()
{
  
  if (mBitmap)
    return;
  
  
  if (mView && mView->Looper())
    mView->UnlockLooper();
}
