




































#ifndef nsDrawingSurfacePh_h___
#define nsDrawingSurfacePh_h___

#include "nsIDrawingSurface.h"
#include "nsIDrawingSurfacePh.h"


class nsDrawingSurfacePh : public nsIDrawingSurface,
                           public nsIDrawingSurfacePh
{
public:
  nsDrawingSurfacePh();
  virtual ~nsDrawingSurfacePh();
  
  NS_DECL_ISUPPORTS

  

  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);

	inline
  NS_IMETHODIMP GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight)
		{
		*aWidth = mWidth;
		*aHeight = mHeight;
		return NS_OK;
		}

  inline NS_IMETHODIMP IsOffscreen( PRBool *aOffScreen ) { *aOffScreen = mIsOffscreen; return NS_OK; }
  inline NS_IMETHODIMP IsPixelAddressable( PRBool *aAddressable ) { *aAddressable = PR_FALSE; return NS_OK; }

	inline
  NS_IMETHODIMP GetPixelFormat(nsPixelFormat *aFormat)
		{ 
		*aFormat = mPixFormat;
		return NS_OK;
		}

  

  
	inline
  NS_IMETHODIMP Init( PhDrawContext_t *aDC, PhGC_t *aGC )
		{
		mGC = aGC;
		mIsOffscreen = PR_FALSE;
		mDrawContext =  aDC;
		return NS_OK;
		}

  
  NS_IMETHOD Init( PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFlags );

  
  inline PhDrawContext_t* Select( )
		{
		PgSetGC( NULL );
		PhDCSetCurrent( mDrawContext );
		PgSetGCCx( mDrawContext, mGC );
		return mDrawContext;
		}

  
  inline PhGC_t								*GetGC( void ) { return mGC; }
  inline PhDrawContext_t			*GetDC( void ) { return mDrawContext; }
  inline void									GetSize( PRUint32 *aWidth, PRUint32 *aHeight ) { *aWidth = mWidth; *aHeight = mHeight;}

private:
  PRUint32					mWidth;
  PRUint32					mHeight;
  PRBool						mIsOffscreen;
  PhGC_t        		*mGC;
  PhDrawContext_t		*mDrawContext;
  
  PRUint32					mFlags;

  
  PhDrawContext_t		*mLockDrawContext;
  PRInt32						mLockX;
  PRInt32						mLockY;
  PRUint32					mLockWidth;
  PRUint32					mLockHeight;
  PRUint32					mLockFlags;
  PRBool						mLocked;
  static nsPixelFormat	mPixFormat;
  static int    				prefChanged(const char* aPref, void* aClosure);
};

#endif
