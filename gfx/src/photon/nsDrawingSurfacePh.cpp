




































#include "nsDrawingSurfacePh.h"
#include "nsCOMPtr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "prmem.h"

#include "nsPhGfxLog.h"
#include <photon/PhRender.h>
#include <Pt.h>
#include <errno.h>

nsPixelFormat nsDrawingSurfacePh::mPixFormat = {
	0, 
	0, 
	0, 
	0, 
	0xff0000, 
	0x00ff00, 
	0x0000ff, 
	0, 
	0, 
	0, 
	0, 
	0, 
	16, 
	8, 
	0, 
	0 
	};

NS_IMPL_ISUPPORTS2( nsDrawingSurfacePh, nsIDrawingSurface, nsIDrawingSurfacePh )

nsDrawingSurfacePh :: nsDrawingSurfacePh( ) 
{
	mDrawContext = nsnull;
	mGC = nsnull;
	mWidth = 0;
	mHeight = 0;
	mFlags = 0;

  mIsOffscreen = PR_FALSE;
	mLockDrawContext = nsnull;
	mLockWidth = 0;
	mLockHeight = 0;
	mLockFlags = 0;
	mLockX = 0;
	mLockY = 0;
	mLocked = PR_FALSE;
}

nsDrawingSurfacePh :: ~nsDrawingSurfacePh( ) 
{
	if( mIsOffscreen ) {
		mDrawContext->gc = NULL;
		PhDCRelease( mDrawContext ); 
	}
	
	if( mLockDrawContext ) {
		PhDCRelease(mLockDrawContext);
	}

	if( mIsOffscreen ) {
		nsresult rv;
		nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
		if (NS_SUCCEEDED(rv)) {
			prefs->UnregisterCallback("browser.display.internaluse.graphics_changed", prefChanged, (void *)this);
		}

		if( mGC ) PgDestroyGC( mGC );
	}
}

  
















NS_IMETHODIMP nsDrawingSurfacePh :: Lock( PRInt32 aX, PRInt32 aY,
                                          PRUint32 aWidth, PRUint32 aHeight,
                                          void **aBits, PRInt32 *aStride,
                                          PRInt32 *aWidthBytes, PRUint32 aFlags ) {

	if( mLocked ) return NS_ERROR_FAILURE;

	PdOffscreenContext_t *odc = (PdOffscreenContext_t *) mDrawContext, *dc;
	int bpp, format = odc->format, offset;

	switch( format ) {
		case Pg_IMAGE_PALETTE_BYTE:
			bpp = 1;
			break;
		case Pg_IMAGE_DIRECT_8888:
			bpp = 4;
		break;
		case Pg_IMAGE_DIRECT_888:
			bpp = 3;
			break;
		case Pg_IMAGE_DIRECT_565:
		case Pg_IMAGE_DIRECT_555:
			bpp = 2;
			break;
		default:
		    return NS_ERROR_FAILURE;
		}


	if( !( aFlags & NS_LOCK_SURFACE_READ_ONLY ) ) {
		PhArea_t dst_area, src_area;

		
		mLockDrawContext = ( PhDrawContext_t * )PdCreateOffscreenContext( format, aWidth, aHeight, Pg_OSC_MEM_PAGE_ALIGN );
		if( !mLockDrawContext ) return NS_ERROR_FAILURE;

		dst_area.pos.x = dst_area.pos.y = 0;
		dst_area.size.w = aWidth;
		dst_area.size.h = aHeight;
		src_area.pos.x = aX;
		src_area.pos.y = aY;
		src_area.size.w = aWidth;
		src_area.size.h = aHeight;

		PhDCSetCurrent( mLockDrawContext );
		PgContextBlitAreaCx( mLockDrawContext, (PdOffscreenContext_t *)mDrawContext, &src_area, (PdOffscreenContext_t *) mLockDrawContext, &dst_area );
		PgFlushCx( mLockDrawContext );
		dc = (PdOffscreenContext_t *) mLockDrawContext; 
		offset = 0;
		}
	else {
		mLockDrawContext = nsnull;
		dc = (PdOffscreenContext_t *) mDrawContext; 
		offset = aX * bpp + aY * dc->pitch;
		}

	*aBits = PdGetOffscreenContextPtr( dc );
	if( *aBits == nsnull ) return NS_ERROR_FAILURE; 

	if( offset ) *aBits = ( (char*) *aBits ) + offset;

	*aWidthBytes = bpp * aWidth;
	*aStride = dc->pitch;

	mLocked = PR_TRUE;
	mLockX = aX;
	mLockY = aY;
	mLockWidth = aWidth;
	mLockHeight = aHeight;
	mLockFlags = aFlags;

	return NS_OK;
	}

NS_IMETHODIMP nsDrawingSurfacePh :: Unlock( void ) {
	PhArea_t dst_area, src_area;

	if( !mLocked ) return NS_ERROR_FAILURE;

	
	if( !( mLockFlags & NS_LOCK_SURFACE_READ_ONLY ) ) {
		dst_area.pos.x = mLockX;
		dst_area.pos.y = mLockY;
		dst_area.size.w = mLockWidth;
		dst_area.size.h = mLockHeight;
		src_area.pos.x = src_area.pos.y = 0;
		src_area.size.w = mLockWidth;
		src_area.size.h = mLockHeight;

		PhDCSetCurrent( mDrawContext );
		PgContextBlitAreaCx( mDrawContext, (PdOffscreenContext_t *) mLockDrawContext, &src_area, (PdOffscreenContext_t *) mDrawContext, &dst_area );
		PgFlushCx( mDrawContext );

		
		PhDCRelease( (PdDirectContext_t *) mLockDrawContext );
		mLockDrawContext = nsnull;
		}

	mLocked = PR_FALSE;
	return NS_OK;
	}

NS_IMETHODIMP nsDrawingSurfacePh :: Init( PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFlags ) {
	mWidth = aWidth;
	mHeight = aHeight;
	mFlags = aFlags;
	
	mIsOffscreen = PR_TRUE;

	
	mGC = PgCreateGC( 0 );

	mDrawContext = (PhDrawContext_t *)PdCreateOffscreenContext(0, mWidth, mHeight, Pg_OSC_MEM_PAGE_ALIGN);
	if( !mDrawContext ) return NS_ERROR_FAILURE;

	PgSetDrawBufferSizeCx( mDrawContext, 0xffff );

	nsresult rv;
	nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
	if (NS_SUCCEEDED(rv)) {
		prefs->RegisterCallback("browser.display.internaluse.graphics_changed", prefChanged, (void *)this);
		}

 	return NS_OK;
	}

int nsDrawingSurfacePh::prefChanged(const char *aPref, void *aClosure)
{
	nsDrawingSurfacePh *surface = (nsDrawingSurfacePh*)aClosure;
	
	if( surface->mLockDrawContext ) {
		PhDCRelease(surface->mLockDrawContext);
		surface->mLockDrawContext = nsnull;
		}

	if(surface->mIsOffscreen) {
		surface->mDrawContext->gc = nsnull; 
		PhDCRelease( surface->mDrawContext ); 
		surface->mDrawContext = (PhDrawContext_t *)PdCreateOffscreenContext(0, surface->mWidth, surface->mHeight, Pg_OSC_MEM_PAGE_ALIGN);
		if( !surface->mDrawContext ) return NS_ERROR_FAILURE;

		PgSetDrawBufferSizeCx( surface->mDrawContext, 0xffff );

		PgDestroyGC(surface->mDrawContext->gc);
		surface->mDrawContext->gc = surface->mGC;
		
	}
	return 0;
}
