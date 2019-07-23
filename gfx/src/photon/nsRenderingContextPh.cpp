




































#include "nsFontMetricsPh.h"
#include "nsGraphicsStatePh.h"
#include "nsGfxCIID.h"
#include "nsRegionPh.h"
#include "nsRenderingContextPh.h"
#include "nsICharRepresentable.h"
#include "nsDeviceContextPh.h"
#include "prprf.h"
#include "nsDrawingSurfacePh.h"

#include <stdlib.h>
#include <mem.h>
#include "nsReadableUtils.h"

static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);


#include <prlog.h>
PRLogModuleInfo *PhGfxLog = PR_NewLogModule("PhGfxLog");
#include "nsPhGfxLog.h"

NS_IMPL_ISUPPORTS1(nsRenderingContextPh, nsIRenderingContext)


static unsigned char sLineStyle[4][2] =
  {
  { 0 },     	
  { 0 },     	
  { 10, 4 },  
  { 1 }				
  };

static unsigned char sLineStyleLen[4] =
  {
  0,        
  0,      
  2,      
  1       
  };

nsRenderingContextPh :: nsRenderingContextPh() 
{
	mGC               = nsnull;
	mTranMatrix       = nsnull;
	mClipRegion       = nsnull;
	mFontMetrics      = nsnull;
	mSurface          = nsnull;
	mOffscreenSurface = nsnull;
	mContext          = nsnull;
	mP2T              = 1.0f;
	mPhotonFontName   = nsnull;
	mCurrentColor     = NS_RGB(255, 255, 255);
	mCurrentLineStyle = nsLineStyle_kSolid;
	mStateCache       = new nsVoidArray();
	mOwner						= PR_FALSE;
	
	PushState();
}

nsRenderingContextPh :: ~nsRenderingContextPh() 
{

	
	if( mStateCache ) {
	  PRInt32 cnt = mStateCache->Count();
		
		while( cnt > 0 ) {
			
			
			nsGraphicsState *state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
			mStateCache->RemoveElementAt(cnt - 1);
			if ( state->mMatrix) delete state->mMatrix;
		
    	
#ifdef USE_GS_POOL
    	nsGraphicsStatePool::ReleaseGS(state);
#else
    	delete state;
#endif	
			cnt--;
		}
		delete mStateCache;
		mStateCache = nsnull;
	}

	if( mTranMatrix ) 
		delete mTranMatrix;
	
	NS_IF_RELEASE( mOffscreenSurface );		
	NS_IF_RELEASE( mFontMetrics );
	NS_IF_RELEASE( mContext );

	
	PgSetGC( NULL );

	if( mPhotonFontName ) 
		delete [] mPhotonFontName;

	if( mOwner ) {
		PgDestroyGC( mGC );
		}
}


NS_IMETHODIMP nsRenderingContextPh :: Init( nsIDeviceContext* aContext, nsIWidget *aWindow ) 
{
	nsresult res;
	
	mContext = aContext;
	NS_IF_ADDREF(mContext);
	
	PtWidget_t *widget = (PtWidget_t*) aWindow->GetNativeData( NS_NATIVE_WIDGET );
	
	if( !widget ) {
		NS_IF_RELEASE( mContext ); 
		NS_ASSERTION(widget,"nsRenderingContext::Init (with a widget) widget is NULL!");
		return NS_ERROR_FAILURE;
	}
	
	mRegionID = PtWidgetRid( widget );
	if( mRegionID ) {
		mSurface = new nsDrawingSurfacePh();
		if( mSurface ) {

			mGC = PgCreateGC( 0 );
			mOwner = PR_TRUE;

			
			res = mSurface->Init( _Ph_->dflt_draw_context, mGC );
			if( res != NS_OK )
				return NS_ERROR_FAILURE;
			
			mOffscreenSurface = mSurface;
			NS_ADDREF( mSurface );

			mSurfaceDC = ((nsDrawingSurfacePh*)mSurface)->GetDC();
		}
		else 
			return NS_ERROR_FAILURE;
	}

	return CommonInit();
}

NS_IMETHODIMP nsRenderingContextPh :: PushState( void ) 
{
	
#ifdef USE_GS_POOL
	nsGraphicsState *state = nsGraphicsStatePool::GetNewGS();
#else
	nsGraphicsState *state = new nsGraphicsState;
#endif

	
	if( !state ) {
		NS_ASSERTION(0, "nsRenderingContextPh::PushState Failed to create a new Graphics State");
		return NS_ERROR_FAILURE;
	}
	
	state->mMatrix = mTranMatrix;
	
	if( nsnull == mTranMatrix ) 
		mTranMatrix = new nsTransform2D();
	else 
		mTranMatrix = new nsTransform2D(mTranMatrix);
	
	
	
	state->mClipRegion = mClipRegion;

	NS_IF_ADDREF(mFontMetrics);
	state->mFontMetrics = mFontMetrics;
	
	state->mColor = mCurrentColor;
	state->mLineStyle = mCurrentLineStyle;
	
	mStateCache->AppendElement(state);
	
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: PopState(void) 
{
	PRUint32 cnt = mStateCache->Count();
	nsGraphicsState * state;
	
	if (cnt > 0) {
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
		mStateCache->RemoveElementAt(cnt - 1);
		
		
		if ( state->mMatrix) {
			if (mTranMatrix)
				delete mTranMatrix;
			mTranMatrix = state->mMatrix;
		}
		
		
		mClipRegion = state->mClipRegion;

		if( state->mFontMetrics && mFontMetrics != state->mFontMetrics ) 
			SetFont( state->mFontMetrics );
		
		
		mCurrentColor = state->mColor;
		mCurrentLineStyle = state->mLineStyle;
		
		
#ifdef USE_GS_POOL
		nsGraphicsStatePool::ReleaseGS(state);
#else
		delete state;
#endif
	}
	
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetClipRect( nsRect &aRect, PRBool &aClipValid ) 
{
	PRInt32 x, y, w, h;
	
	if ( !mClipRegion )
		return NS_ERROR_FAILURE;
	
	if( !mClipRegion->IsEmpty() ) {
		mClipRegion->GetBoundingBox( &x, &y, &w, &h );
		aRect.SetRect( x, y, w, h );
		aClipValid = PR_TRUE;
	}
	else {
		aRect.SetRect(0,0,0,0);
		aClipValid = PR_FALSE;
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: SetClipRect( const nsRect& aRect, nsClipCombine aCombine ) 
{
	nsresult   res = NS_ERROR_FAILURE;
	nsRect     trect = aRect;
	PRUint32 cnt = mStateCache->Count();
	nsGraphicsState *state = nsnull;
	
	if (cnt > 0) {
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
	}
	
	if (state) {
		if (state->mClipRegion) {
			if (state->mClipRegion == mClipRegion) {
				nsCOMPtr<nsIRegion> tmpRgn;
				GetClipRegion(getter_AddRefs(tmpRgn));
				mClipRegion = tmpRgn;
			}
		}
	}
	
	
  if( !mClipRegion ) {
    PRUint32 w, h;
    mSurface->GetSize(&w, &h);

    mClipRegion = do_CreateInstance(kRegionCID);
    if( mClipRegion ) {
      mClipRegion->Init();
      mClipRegion->SetTo(0,0,w,h);
      }
    }
	
	if( mTranMatrix && mClipRegion ) {
		mTranMatrix->TransformCoord( &trect.x, &trect.y,&trect.width, &trect.height );
		switch( aCombine ) {
			case nsClipCombine_kIntersect:
				mClipRegion->Intersect(trect.x,trect.y,trect.width,trect.height);
				break;
			case nsClipCombine_kUnion:
				mClipRegion->Union(trect.x,trect.y,trect.width,trect.height);
				break;
			case nsClipCombine_kSubtract:
				mClipRegion->Subtract(trect.x,trect.y,trect.width,trect.height);
				break;
			case nsClipCombine_kReplace:
				mClipRegion->SetTo(trect.x,trect.y,trect.width,trect.height);
				break;
			default:
				break;
		}
		
		res = NS_OK;
	}
	
	return res;
}

NS_IMETHODIMP nsRenderingContextPh :: SetClipRegion( const nsIRegion& aRegion, nsClipCombine aCombine ) 
{
	PRUint32 cnt = mStateCache->Count();
	nsGraphicsState *state = nsnull;
	
	if (cnt > 0) {
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
	}
	
	if (state) {
		if (state->mClipRegion) {
			if (state->mClipRegion == mClipRegion) {
				nsCOMPtr<nsIRegion> tmpRgn;
				GetClipRegion(getter_AddRefs(tmpRgn));
				mClipRegion = tmpRgn;
			}
		}
	}
	
	
  if( !mClipRegion ) {
    PRUint32 w, h;
    mSurface->GetSize(&w, &h);

    mClipRegion = do_CreateInstance(kRegionCID);
    if( mClipRegion ) {
      mClipRegion->Init();
      mClipRegion->SetTo(0,0,w,h);
      }
    }
	
	switch( aCombine ) {
		case nsClipCombine_kIntersect:
			mClipRegion->Intersect(aRegion);
			break;
		case nsClipCombine_kUnion:
			mClipRegion->Union(aRegion);
			break;
		case nsClipCombine_kSubtract:
			mClipRegion->Subtract(aRegion);
			break;
		case nsClipCombine_kReplace:
			mClipRegion->SetTo(aRegion);
			break;
	}
	
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: GetClipRegion( nsIRegion **aRegion ) 
{
	nsresult rv = NS_ERROR_FAILURE;
	
	if (!aRegion || !mClipRegion)
		return NS_ERROR_NULL_POINTER;
	
	if (mClipRegion) {
		if (*aRegion) { 
			  (*aRegion)->SetTo(*mClipRegion);
			  rv = NS_OK;
		  } else {
				  nsCOMPtr<nsIRegion> newRegion = do_CreateInstance(kRegionCID, &rv);
				  if (NS_SUCCEEDED(rv)) {
					  newRegion->Init();
					  newRegion->SetTo(*mClipRegion);
					  NS_ADDREF(*aRegion = newRegion);
				  }
			  }
	} else {
		rv = NS_ERROR_FAILURE;
	}
	return rv;
}

NS_IMETHODIMP nsRenderingContextPh :: SetFont( nsIFontMetrics *aFontMetrics ) 
{
	if( mFontMetrics == aFontMetrics ) return NS_OK;
	
	nsFontHandle  fontHandle;			
	char      *pFontHandle;
	
	NS_IF_RELEASE(mFontMetrics);
	mFontMetrics = aFontMetrics;
	NS_IF_ADDREF(mFontMetrics);
	
  if( mFontMetrics == nsnull ) return NS_OK;
	
	mFontMetrics->GetFontHandle( fontHandle );
	pFontHandle = (char *) fontHandle;
    
	if( pFontHandle ) {
		if( mPhotonFontName ) free( mPhotonFontName );
		mPhotonFontName = strdup( pFontHandle );
		}
	
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: CreateDrawingSurface( const nsRect &aBounds, PRUint32 aSurfFlags, nsIDrawingSurface* &aSurface ) 
{
	if( mSurface ) {
		nsDrawingSurfacePh *surf = new nsDrawingSurfacePh();
		if( surf ) {
			NS_ADDREF(surf);
			
			if( surf->Init( aBounds.width, aBounds.height, aSurfFlags ) == NS_OK ) {
				aSurface = surf;
				return NS_OK;
				}
			}
		}
	aSurface = nsnull;
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextPh :: DrawLine( nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1 ) 
{
	nscoord diffX, diffY;

	mTranMatrix->TransformCoord( &aX0, &aY0 );
	mTranMatrix->TransformCoord( &aX1, &aY1 );

	diffX = aX1 - aX0;
	diffY = aY1 - aY0;

	if( diffX != 0 ) diffX = ( diffX > 0 ? 1 : -1 );
	if( diffY != 0 ) diffY = ( diffY > 0 ? 1 : -1 );
	
	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );
	
	PgDrawILineCx( mSurfaceDC, aX0, aY0, aX1-diffX, aY1-diffY );
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: DrawRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight ) 
{
	nscoord x,y,w,h;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	mTranMatrix->TransformCoord( &x, &y, &w, &h );
	
	UpdateGC();	
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );

	ConditionRect(x,y,w,h);	
	if( w && h )
		PgDrawIRectCx( mSurfaceDC, x, y, x + w - 1, y + h - 1, Pg_DRAW_STROKE );
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: FillRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight ) 
{
	nscoord x,y,w,h;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	
	mTranMatrix->TransformCoord( &x, &y, &w, &h );

	PgSetGC( NULL );
 PhDCSetCurrent( mSurfaceDC );
	
	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );
	PgSetFillColorCx( mGC, mCurrentColor );

	ConditionRect(x,y,w,h);	
	if( w && h ) {
		int y2 = y + h - 1;
		if( y < SHRT_MIN ) y = SHRT_MIN;			
		if( y2 >= SHRT_MAX ) y2 = SHRT_MAX;		
		
		PgDrawIRectCx( mSurfaceDC, x, y, x + w - 1, y2, Pg_DRAW_FILL );
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: InvertRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight ) 
{
	if( nsnull == mTranMatrix || nsnull == mSurface ) return NS_ERROR_FAILURE; 
	nscoord x,y,w,h;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	mTranMatrix->TransformCoord(&x,&y,&w,&h);
	
	if( !w || !h ) return NS_OK;
	
	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );
	PgSetFillColorCx( mGC, mCurrentColor );

	ConditionRect(x,y,w,h);
	PgSetFillColorCx( mGC, Pg_INVERT_COLOR );
	PgSetDrawModeCx( mGC, Pg_DRAWMODE_XOR );
	PgDrawIRectCx( mSurfaceDC, x, y, x + w - 1, y + h - 1, Pg_DRAW_FILL );
	PgSetDrawModeCx( mGC, Pg_DRAWMODE_OPAQUE );

	
	PgFlushCx( mSurfaceDC );

	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: DrawPolygon( const nsPoint aPoints[], PRInt32 aNumPoints ) 
{
	PhPoint_t *pts;
	
	if( !aNumPoints ) return NS_OK;
	
	if( (pts = new PhPoint_t [aNumPoints]) != NULL ) {
		PhPoint_t pos = {0,0};
		PRInt32 i;
		int x,y;
		
		for( i = 0; i < aNumPoints; i++ ) {
			x = aPoints[i].x;
			y = aPoints[i].y;
			mTranMatrix->TransformCoord(&x, &y);
			pts[i].x = x;
			pts[i].y = y;
		}

		UpdateGC();
		PgSetStrokeColorCx( mGC, mCurrentColor );
		PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );

		if( aNumPoints == 4 ) {
			
			int dx = pts[1].x - pts[0].x;
			int dy = pts[1].y - pts[0].y;
			if( !dx ) {
				
				pts[0].y++;
				pts[1].y--;
				int diff = pts[3].x > pts[0].x ? 1 : -1;
				pts[0].x += diff;
				pts[1].x += diff;
				}
			if( !dy ) {
				
				pts[2].x++;
				pts[3].x--;
				int diff = pts[3].y > pts[0].y ? -1 : 1;
				pts[2].y += diff;
				pts[3].y += diff;
				}
			}


		PgDrawPolygonCx( mSurfaceDC, pts, aNumPoints, &pos, Pg_DRAW_STROKE );
		delete [] pts;
	}
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillPolygon( const nsPoint aPoints[], PRInt32 aNumPoints ) 
{
	PhPoint_t *pts;
	
	if( !aNumPoints ) return NS_OK;
	
	if( (pts = new PhPoint_t [aNumPoints]) != NULL ) {
		PhPoint_t pos = {0,0};
		PRInt32 i;
		int x,y;
		
		for( i = 0; i < aNumPoints; i++ ) {
			x = aPoints[i].x;
			y = aPoints[i].y;
			mTranMatrix->TransformCoord(&x, &y);
			pts[i].x = x;
			pts[i].y = y;
		}

		UpdateGC();
		PgSetStrokeColorCx( mGC, mCurrentColor );
		PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );
		PgSetFillColorCx( mGC, mCurrentColor );

		if( aNumPoints == 4 ) {
			
			int dx = pts[1].x - pts[0].x;
			int dy = pts[1].y - pts[0].y;
			if( !dx ) {
				
				pts[0].y++;
				pts[1].y--;
				int diff = pts[3].x > pts[0].x ? 1 : -1;
				pts[0].x += diff;
				pts[1].x += diff;
				}
			if( !dy ) {
				
				pts[2].x++;
				pts[3].x--;
				int diff = pts[3].y > pts[0].y ? -1 : 1;
				pts[2].y += diff;
				pts[3].y += diff;
				}
			}


		PgDrawPolygonCx( mSurfaceDC, pts, aNumPoints, &pos, Pg_DRAW_FILL );
		delete [] pts;
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: DrawEllipse( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight ) 
{
	nscoord x,y,w,h;
	PhPoint_t center;
	PhPoint_t radii;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	mTranMatrix->TransformCoord( &x, &y, &w, &h );
	
	center.x = x;
	center.y = y;
	radii.x = x+w-1;
	radii.y = y+h-1;

	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );

	PgDrawEllipseCx( mSurfaceDC, &center, &radii, Pg_EXTENT_BASED | Pg_DRAW_STROKE );
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: FillEllipse( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight ) 
{
	nscoord x,y,w,h;
	PhPoint_t center;
	PhPoint_t radii;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	mTranMatrix->TransformCoord(&x,&y,&w,&h);
	
	center.x = x;
	center.y = y;
	radii.x = x+w-1;
	radii.y = y+h-1;
	
	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );
	PgSetFillColorCx( mGC, mCurrentColor );

	PgDrawEllipseCx( mSurfaceDC, &center, &radii, Pg_EXTENT_BASED | Pg_DRAW_FILL );
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight, float aStartAngle, float aEndAngle ) 
{
	nscoord x,y,w,h;
	PhPoint_t center;
	PhPoint_t radii;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	
	mTranMatrix->TransformCoord(&x,&y,&w,&h);
	
	center.x = x;
	center.y = y;
	radii.x = x+w-1;
	radii.y = y+h-1;
	
	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );

	PgDrawArcCx( mSurfaceDC, &center, &radii, (unsigned int)aStartAngle, (unsigned int)aEndAngle, Pg_EXTENT_BASED | Pg_DRAW_STROKE );
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: FillArc( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight, float aStartAngle, float aEndAngle ) 
{
	nscoord x,y,w,h;
	PhPoint_t center;
	PhPoint_t radii;
	
	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;
	mTranMatrix->TransformCoord(&x,&y,&w,&h);
	
	center.x = x;
	center.y = y;
	radii.x = x+w-1;
	radii.y = y+h-1;
	
	UpdateGC();
	PgSetStrokeColorCx( mGC, mCurrentColor );
	PgSetStrokeDashCx( mGC, sLineStyle[mCurrentLineStyle], sLineStyleLen[mCurrentLineStyle], 0x10000 );
	PgSetFillColorCx( mGC, mCurrentColor );

	PgDrawArcCx( mSurfaceDC, &center, &radii, (unsigned int)aStartAngle, (unsigned int)aEndAngle, Pg_EXTENT_BASED | Pg_DRAW_FILL );
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth ) 
{
	PhRect_t extent;

	
	if( aString[0] == ' ' && aLength == 1 )
		return mFontMetrics->GetSpaceWidth(aWidth);

	PfExtent( &extent, NULL, mPhotonFontName, 0L, 0L, aString, aLength, PF_SIMPLE_METRICS, NULL );
	aWidth = NSToCoordRound((int) ((extent.lr.x - extent.ul.x + 1) * mP2T));
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: GetWidth( const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth, PRInt32 *aFontID ) 
{
	NS_ConvertUTF16toUTF8    theUnicodeString (aString, aLength);
	const char *s = theUnicodeString.get();
	return GetWidth( s, strlen(s), aWidth );
}

NS_IMETHODIMP nsRenderingContextPh::GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
													  nsTextDimensions& aDimensions, PRInt32* aFontID)
{
	mFontMetrics->GetMaxAscent(aDimensions.ascent);
	mFontMetrics->GetMaxDescent(aDimensions.descent);
		
	NS_ConvertUTF16toUTF8    theUnicodeString (aString, aLength);
	const char *s = theUnicodeString.get();
	return GetWidth( s, strlen(s), aDimensions.width );
}

NS_IMETHODIMP nsRenderingContextPh::DrawString(const char *aString, PRUint32 aLength,
												nscoord aX, nscoord aY,
												const nscoord* aSpacing)
{
	if ( aLength == 0 )
		return NS_OK;

	UpdateGC();
	PgSetTextColorCx( mGC, mCurrentColor );
	
	PgSetFontCx( mGC, mPhotonFontName );
	PgSetExtendedTextFlagsCx( mGC, Pg_TEXT_SIMPLE_METRICS );

	if( !aSpacing ) {
		mTranMatrix->TransformCoord( &aX, &aY );
		PhPoint_t pos = { aX, aY };
		PgDrawTextCharsCx( mSurfaceDC, aString, aLength, &pos, Pg_TEXT_LEFT);
		}
	else {
    nscoord x = aX;
    nscoord y = aY;
    const char* end = aString + aLength;
    while( aString < end ) {
			const char *ch = aString;
			int charlen = utf8len( aString, aLength );
			if( charlen <= 0 )
				break;

			aString += charlen;
			aLength -= charlen;

      nscoord xx = x;
      nscoord yy = y;
      mTranMatrix->TransformCoord(&xx, &yy);
      PhPoint_t pos = { xx, yy };
			PgDrawTextCx( mSurfaceDC, ch, charlen, &pos, Pg_TEXT_LEFT);
			x += *aSpacing++;
			}
		}

	PgSetExtendedTextFlagsCx( mGC, 0 );

	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh::DrawImage( nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect ) 
{
	nsRect    sr,dr;
	
	sr = aSRect;
	mTranMatrix->TransformCoord(&sr.x, &sr.y, &sr.width, &sr.height);
	sr.x -= mTranMatrix->GetXTranslationCoord();
	sr.y -= mTranMatrix->GetYTranslationCoord();
	
	dr = aDRect;
	mTranMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);
	
	return aImage->Draw(*this, mSurface,
						sr.x, sr.y,
						sr.width, sr.height,
						dr.x, dr.y,
						dr.width, dr.height);
}





NS_IMETHODIMP nsRenderingContextPh :: CopyOffScreenBits( nsIDrawingSurface* aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY, const nsRect &aDestBounds, PRUint32 aCopyFlags ) 
{
	PhArea_t              darea, sarea;
	PRInt32               srcX = aSrcX;
	PRInt32               srcY = aSrcY;
	nsRect                drect = aDestBounds;
	nsDrawingSurfacePh    *destsurf;

	if( !aSrcSurf || !mTranMatrix || !mSurface ) return NS_ERROR_FAILURE;
	
	if( aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER ) {
		NS_ASSERTION(!(nsnull == mSurface), "no back buffer");
		destsurf = mSurface;
	}
	else 
		destsurf = mOffscreenSurface;
	
	if( aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES )	
		mTranMatrix->TransformCoord( &srcX, &srcY );
	if( aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES )
		mTranMatrix->TransformCoord( &drect.x, &drect.y, &drect.width, &drect.height );

	mSurfaceDC = destsurf->Select();
	UpdateGC();

	sarea.pos.x = srcX;
	sarea.pos.y = srcY;
	sarea.size.w = drect.width;
	sarea.size.h = drect.height;
	darea.pos.x = drect.x;
	darea.pos.y = drect.y;
	darea.size.w = sarea.size.w;
	darea.size.h = sarea.size.h;

	PgContextBlitAreaCx( mSurfaceDC, ((nsDrawingSurfacePh *)aSrcSurf)->GetDC(), &sarea, mSurfaceDC, &darea );
	PgFlushCx( mSurfaceDC );

	return NS_OK;
}

#ifdef MOZ_MATHML
  


NS_IMETHODIMP nsRenderingContextPh::GetBoundingMetrics(const char*        aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics)
{
	return NS_ERROR_FAILURE;
}

  


NS_IMETHODIMP nsRenderingContextPh::GetBoundingMetrics(const PRUnichar*   aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics,
                                PRInt32*           aFontID = nsnull)
{
	return NS_ERROR_FAILURE;
}

#endif 


void nsRenderingContextPh::ApplyClipping( PhGC_t *gc ) 
{
	
	if( mClipRegion ) {
		PhTile_t    *tiles = nsnull;
		PhRect_t    *rects = nsnull;
		int         rect_count;
		
		
		mClipRegion->GetNativeRegion((void*&)tiles);
		
		if( tiles != nsnull ) {
			rects = PhTilesToRects( tiles, &rect_count );
			PgSetMultiClipCx( gc, rect_count, rects );
			free( rects );
		}
		else PgSetMultiClipCx( gc, 0, NULL );
	}
}
