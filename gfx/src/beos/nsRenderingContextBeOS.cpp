






































#include "nsFontMetricsBeOS.h"
#include "nsRenderingContextBeOS.h"
#include "nsRegionBeOS.h"
#include "nsImageBeOS.h"
#include "nsGraphicsStateBeOS.h"
#include "nsICharRepresentable.h"
#include "prenv.h"
#include <Polygon.h>
#include <math.h>

static const pattern NS_BEOS_DASHED = { {0xc7, 0x8f, 0x1f, 0x3e, 0x7c, 0xf8, 0xf1, 0xe3} };
static const pattern NS_BEOS_DOTTED = { {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa} };

NS_IMPL_ISUPPORTS1(nsRenderingContextBeOS, nsIRenderingContext)

static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

nsRenderingContextBeOS::nsRenderingContextBeOS()
{
	mOffscreenSurface = nsnull;
	mSurface = nsnull;
	mContext = nsnull;
	mFontMetrics = nsnull;
	mClipRegion = nsnull;
	mStateCache = new nsVoidArray();
	mView = nsnull;	
	mCurrentColor = NS_RGB(255, 255, 255);
	mCurrentBFont = nsnull;
	mCurrentLineStyle = nsLineStyle_kSolid;
	mCurrentLinePattern = B_SOLID_HIGH;
	mP2T = 1.0f;
	mTranMatrix = nsnull;

	PushState();
}

nsRenderingContextBeOS::~nsRenderingContextBeOS()
{
	
	if (mStateCache)
	{
		PRInt32 cnt = mStateCache->Count();
		while (--cnt >= 0)
			PopState();
		delete mStateCache;
		mStateCache = nsnull;
	}
	
	delete mTranMatrix;
	NS_IF_RELEASE(mOffscreenSurface);
	NS_IF_RELEASE(mFontMetrics);
	NS_IF_RELEASE(mContext);
}


NS_IMETHODIMP nsRenderingContextBeOS::Init(nsIDeviceContext *aContext, nsIWidget *aWindow)
{	
	if (!aContext || !aWindow)
		return NS_ERROR_NULL_POINTER;

	BView *view = (BView *)aWindow->GetNativeData(NS_NATIVE_GRAPHIC);
	if (!view)
		return NS_ERROR_FAILURE;
	
	mSurface = new nsDrawingSurfaceBeOS();
	if (!mSurface)
		return NS_ERROR_OUT_OF_MEMORY;
	
	mContext = aContext;
	NS_IF_ADDREF(mContext);

	mSurface->Init(view);
	mOffscreenSurface = mSurface;
	NS_ADDREF(mSurface);
	return CommonInit();
}

NS_IMETHODIMP nsRenderingContextBeOS::Init(nsIDeviceContext *aContext, nsIDrawingSurface* aSurface)
{
	if (!aContext || !aSurface)
		return NS_ERROR_NULL_POINTER;
		
	mContext = aContext;
	NS_IF_ADDREF(mContext);
	mSurface = (nsDrawingSurfaceBeOS *) aSurface;
	mOffscreenSurface = mSurface;
	NS_ADDREF(mSurface);
	return CommonInit();
}

NS_IMETHODIMP nsRenderingContextBeOS::CommonInit()
{
	if (!mTranMatrix)
		return NS_ERROR_OUT_OF_MEMORY;

	mP2T = mContext->DevUnitsToAppUnits();
	float app2dev;
	app2dev = mContext->AppUnitsToDevUnits();
 	mTranMatrix->AddScale(app2dev, app2dev);
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextBeOS::GetHints(PRUint32 &aResult)
{
	aResult = 0;
	if (!PR_GetEnv("MOZILLA_GFX_DISABLE_FAST_MEASURE"))
		aResult = NS_RENDERING_HINT_FAST_MEASURE;	
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::LockDrawingSurface(PRInt32 aX, PRInt32 aY, PRUint32 aWidth,
	PRUint32 aHeight, void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes, PRUint32 aFlags)
{
	PushState();
	return mSurface->Lock(aX, aY, aWidth, aHeight, aBits, aStride, aWidthBytes, aFlags);
}

NS_IMETHODIMP nsRenderingContextBeOS::UnlockDrawingSurface()
{
	PopState();
	mSurface->Unlock();
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{
	if (nsnull == aSurface)
		mSurface = mOffscreenSurface;
	else
		mSurface = (nsDrawingSurfaceBeOS *)aSurface;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetDrawingSurface(nsIDrawingSurface* *aSurface)
{
	*aSurface = mSurface;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::Reset()
{
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetDeviceContext(nsIDeviceContext *&aContext)
{
	NS_IF_ADDREF(mContext);
	aContext = mContext;
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextBeOS::PushState()
{
#ifdef USE_GS_POOL
	nsGraphicsState *state = nsGraphicsStatePool::GetNewGS();
#else
	nsGraphicsState *state = new nsGraphicsState;
#endif
	
	if (!state)
		return NS_ERROR_OUT_OF_MEMORY;

	nsTransform2D *tranMatrix;
	if (nsnull == mTranMatrix)
		tranMatrix = new nsTransform2D();
	else
		tranMatrix = new nsTransform2D(mTranMatrix);

	if (!tranMatrix)
	{
#ifdef USE_GS_POOL
		nsGraphicsStatePool::ReleaseGS(state);
#else
		delete state;
#endif
		return NS_ERROR_OUT_OF_MEMORY;
	}
	state->mMatrix = mTranMatrix;
	mTranMatrix = tranMatrix;

	
	state->mClipRegion = mClipRegion;

	NS_IF_ADDREF(mFontMetrics);
	state->mFontMetrics = mFontMetrics;
	state->mColor = mCurrentColor;
	state->mLineStyle = mCurrentLineStyle;

	mStateCache->AppendElement(state);
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::PopState(void)
{
	PRUint32 cnt = mStateCache->Count();
	nsGraphicsState *state;
	
	if (cnt > 0)
	{
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
		mStateCache->RemoveElementAt(cnt - 1);
		
		
		if (state->mMatrix)
		{
			delete mTranMatrix;
			mTranMatrix = state->mMatrix;
		}
		
		mClipRegion = state->mClipRegion;
		if (state->mFontMetrics && (mFontMetrics != state->mFontMetrics))
			SetFont(state->mFontMetrics);
		if (state->mColor != mCurrentColor)
			SetColor(state->mColor);
		if (state->mLineStyle != mCurrentLineStyle)
			SetLineStyle(state->mLineStyle);

		
#ifdef USE_GS_POOL
		nsGraphicsStatePool::ReleaseGS(state);
#else
		delete state;
#endif
	}
	
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
	aVisible = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
	if (!mClipRegion)
		return NS_ERROR_FAILURE;
	if (!mClipRegion->IsEmpty())
	{
		PRInt32 x, y, w, h;
		mClipRegion->GetBoundingBox(&x, &y, &w, &h);
		aRect.SetRect(x, y, w, h);
		aClipValid = PR_TRUE;
	}
	else
	{
		aRect.SetRect(0, 0, 0, 0);
		aClipValid = PR_FALSE;
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
	
	PRUint32 cnt = mStateCache->Count();
	nsGraphicsState *state = nsnull;
	if (cnt > 0)
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
	
	if (state && mClipRegion && state->mClipRegion == mClipRegion)
	{
		nsCOMPtr<nsIRegion> region;
		GetClipRegion(getter_AddRefs(region));
		mClipRegion = region;
	}
	
	CreateClipRegion();
	nsRect trect = aRect;
	mTranMatrix->TransformCoord(&trect.x, &trect.y, &trect.width, &trect.height);

	switch (aCombine)
	{
		case nsClipCombine_kIntersect:
			mClipRegion->Intersect(trect.x, trect.y, trect.width, trect.height);
			break;
		case nsClipCombine_kUnion:
			mClipRegion->Union(trect.x, trect.y, trect.width, trect.height);
			break;
		case nsClipCombine_kSubtract:
			mClipRegion->Subtract(trect.x, trect.y, trect.width, trect.height);
			break;
		case nsClipCombine_kReplace:
			mClipRegion->SetTo(trect.x, trect.y, trect.width, trect.height);
			break;
	}
	return NS_OK;
} 





bool nsRenderingContextBeOS::LockAndUpdateView() 
{
	bool rv = false;
	if (!mSurface)
		return rv;
	if (mView) 
		mView = nsnull;
	mSurface->AcquireView(&mView);
	if (!mView)
		return rv; 

	
	if (mSurface->LockDrawable()) 
	{
		
		if (mCurrentBFont == nsnull)
		{ 
			if (mFontMetrics)
				mFontMetrics->GetFontHandle((nsFontHandle&)mCurrentBFont);

			if (mCurrentBFont)
				mView->SetFont(mCurrentBFont);
			else
				mView->SetFont(be_plain_font); 
		}

		if (mClipRegion) 
		{
			BRegion *region = nsnull;
			mClipRegion->GetNativeRegion((void *&)region);
			mView->ConstrainClippingRegion(region);
		}
		else
		{
			mView->ConstrainClippingRegion(0);
		}
		rv = true;
	}
	return rv;
}

void nsRenderingContextBeOS::UnlockView()
{
	mSurface->UnlockDrawable();
}
	
void nsRenderingContextBeOS::CreateClipRegion()
{
	
	
	
	

	if (mClipRegion)
	{
		PRUint32 cnt = mStateCache->Count();

		if (cnt == 0) 
			return;

		nsGraphicsState *state;
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);

		if (state->mClipRegion != mClipRegion)
			return;
			
		mClipRegion = new nsRegionBeOS;
		if (mClipRegion)
			mClipRegion->SetTo(*state->mClipRegion);
	}
	else
	{
		PRUint32 w, h;
		mSurface->GetSize(&w, &h);

		mClipRegion = new nsRegionBeOS;
		if (mClipRegion)
		{
			mClipRegion->Init();
			mClipRegion->SetTo(0, 0, w, h);
		}
	}
}
	
NS_IMETHODIMP nsRenderingContextBeOS::SetClipRegion(const nsIRegion &aRegion, nsClipCombine aCombine)
{
	
	PRUint32 cnt = mStateCache->Count();
	nsGraphicsState *state = nsnull;
	if (cnt > 0)
		state = (nsGraphicsState *)mStateCache->ElementAt(cnt - 1);
	
	if (state && mClipRegion && state->mClipRegion == mClipRegion)
	{
		nsCOMPtr<nsIRegion> region;
		GetClipRegion(getter_AddRefs(region));
		mClipRegion = region;
	}
	
	CreateClipRegion();
	switch (aCombine)
	{
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

NS_IMETHODIMP nsRenderingContextBeOS::CopyClipRegion(nsIRegion &aRegion)
{
	if (!mClipRegion)
		return NS_ERROR_FAILURE;
	aRegion.SetTo(*mClipRegion);
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetClipRegion(nsIRegion **aRegion)
{
	if (!aRegion || !mClipRegion)
		return NS_ERROR_NULL_POINTER;
	
	if (*aRegion) 
	{
		(*aRegion)->SetTo(*mClipRegion);
	}
	else
	{
		nsCOMPtr<nsIRegion> newRegion = new nsRegionBeOS();
		newRegion->Init();
		newRegion->SetTo(*mClipRegion);
		NS_ADDREF(*aRegion = newRegion);
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::SetColor(nscolor aColor) 
{
	if (nsnull == mContext)
		return NS_ERROR_FAILURE;
	mCurrentColor = aColor;
	mRGB_color.red = NS_GET_R(mCurrentColor);
	mRGB_color.green = NS_GET_G(mCurrentColor);
	mRGB_color.blue = NS_GET_B(mCurrentColor);
	mRGB_color.alpha = 255;	
	if (LockAndUpdateView())
	{
		mView->SetHighColor(mRGB_color);
		UnlockView();
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetColor(nscolor &aColor) const 
{
	aColor = mCurrentColor;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::SetFont(const nsFont &aFont, nsIAtom* aLangGroup)
{
	nsCOMPtr<nsIFontMetrics> newMetrics;
	nsresult rv = mContext->GetMetricsFor(aFont, aLangGroup, *getter_AddRefs(newMetrics));
	if (NS_SUCCEEDED(rv)) 
		rv = SetFont(newMetrics);
	return rv;
}

NS_IMETHODIMP nsRenderingContextBeOS::SetFont(nsIFontMetrics *aFontMetrics)
{
	mCurrentBFont = nsnull;
	NS_IF_RELEASE(mFontMetrics);
	mFontMetrics = aFontMetrics;
	NS_IF_ADDREF(mFontMetrics);
	
	if (LockAndUpdateView())
		UnlockView();
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::SetLineStyle(nsLineStyle aLineStyle)
{
	switch(aLineStyle)
	{
		case nsLineStyle_kDashed:
			mCurrentLinePattern = NS_BEOS_DASHED;
			break;
		case nsLineStyle_kDotted:
			mCurrentLinePattern = NS_BEOS_DOTTED;
			break;
		case nsLineStyle_kSolid:
		default:
			mCurrentLinePattern = B_SOLID_HIGH;
		break;
	}
	mCurrentLineStyle = aLineStyle ;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetLineStyle(nsLineStyle &aLineStyle)
{
	aLineStyle = mCurrentLineStyle;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
	NS_IF_ADDREF(mFontMetrics);
	aFontMetrics = mFontMetrics;
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextBeOS::Translate(nscoord aX, nscoord aY)
{
	mTranMatrix->AddTranslation((float)aX, (float)aY);
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextBeOS::Scale(float aSx, float aSy)
{
	mTranMatrix->AddScale(aSx, aSy);
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetCurrentTransform(nsTransform2D *&aTransform)
{
	aTransform = mTranMatrix;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::CreateDrawingSurface(const nsRect& aBounds, PRUint32 aSurfFlags,
	nsIDrawingSurface* &aSurface)
{
	
	if (nsnull == mSurface)
	{
		aSurface = nsnull;
		return NS_ERROR_FAILURE;
	}
	
	if ((aBounds.width <= 0) || (aBounds.height <= 0))
		return NS_ERROR_FAILURE;
	
	nsDrawingSurfaceBeOS *surf = new nsDrawingSurfaceBeOS();
	if (!surf)
	{
		aSurface = nsnull;
		return NS_ERROR_FAILURE;
	}

	NS_ADDREF(surf);
	
	if (LockAndUpdateView())
		UnlockView();
		
	surf->Init(mView, aBounds.width, aBounds.height, aSurfFlags);
	aSurface = surf;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DestroyDrawingSurface(nsIDrawingSurface* aDS)
{
	nsDrawingSurfaceBeOS *surf = (nsDrawingSurfaceBeOS *)aDS;
	if (surf == nsnull)
		return NS_ERROR_FAILURE;
	NS_IF_RELEASE(surf);
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1) {
	if (mTranMatrix == nsnull)
		return NS_ERROR_FAILURE;
	if (mSurface == nsnull)
		return NS_ERROR_FAILURE;
	
	mTranMatrix->TransformCoord(&aX0, &aY0);
	mTranMatrix->TransformCoord(&aX1, &aY1);
	nscoord diffX = aX1 - aX0;
	nscoord diffY = aY1 - aY0;
	
	if (0 != diffX)
		diffX = (diffX > 0) ? 1 : -1;
	if (0 != diffY)
		diffY = (diffY > 0) ? 1 : -1;
	
	
	if (LockAndUpdateView())
	{
		mView->StrokeLine(BPoint(aX0, aY0), BPoint(aX1 - diffX, aY1 - diffY), mCurrentLinePattern);
		UnlockView();
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
	if (mTranMatrix == nsnull)
		return NS_ERROR_FAILURE;
	if (mSurface == nsnull)
		return NS_ERROR_FAILURE;
	BPoint *pts;
	BPolygon poly;
	BRect r;
	PRInt32 w, h;
	
	BPoint bpointbuf[64];
	pts = bpointbuf;
	if (aNumPoints>64)
		pts = new BPoint[aNumPoints];
	for (int i = 0; i < aNumPoints; ++i)
	{
		nsPoint p = aPoints[i];
		mTranMatrix->TransformCoord(&p.x, &p.y);
		pts[i].x = p.x;
		pts[i].y = p.y;
#ifdef DEBUG
		printf("polyline(%i,%i)\n", p.x, p.y);
#endif
	}
	poly.AddPoints(pts, aNumPoints);
	r = poly.Frame();	
	w = r.IntegerWidth();
	h = r.IntegerHeight();

	if (w && h)
	{
		if (LockAndUpdateView())
		{
			if (1 == h) 
			{
				mView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left + w - 1, r.top), mCurrentLinePattern);
			}
			else if (1 == w)
			{
				mView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.top + h -1), mCurrentLinePattern);
			}
			else
			{
				poly.MapTo(r,BRect(r.left, r.top, r.left + w -1, r.top + h - 1));
				mView->StrokePolygon(&poly, false, mCurrentLinePattern);
			}
			UnlockView();
		}		
	}	
	if (pts!=bpointbuf)
		delete [] pts;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawRect(const nsRect& aRect)
{
	return DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawRect(nscoord aX, nscoord aY, nscoord aWidth,
	nscoord aHeight)
{
	
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;

	
	
	
	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	ConditionRect(x, y, w, h);
	
	
	
	if (w && h)
	{
		if (LockAndUpdateView())
		{
			
			if (1 == h)
				mView->StrokeLine(BPoint(x, y), BPoint(x + w - 1, y), mCurrentLinePattern);
			else
				mView->StrokeRect(BRect(x, y, x + w - 1, y + h - 1), mCurrentLinePattern);
			UnlockView();
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::FillRect(const nsRect &aRect)
{
	return FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextBeOS::FillRect(nscoord aX, nscoord aY, nscoord aWidth,
	nscoord aHeight)
{
	
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;
	
	
	
	
	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	ConditionRect(x, y, w, h);
	
	if (w && h)
	{
		if (LockAndUpdateView())
		{
			
			if (1 == h)
				mView->StrokeLine(BPoint(x, y), BPoint(x + w - 1, y));
			else
				mView->FillRect(BRect(x, y, x + w - 1, y + h - 1), B_SOLID_HIGH);
			UnlockView();
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::InvertRect(const nsRect &aRect)
{
	return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextBeOS::InvertRect(nscoord aX, nscoord aY, nscoord aWidth,
	nscoord aHeight)
{
	
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;
	
	
	
	
	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	ConditionRect(x, y, w, h);
	
	if (w && h)
	{
		if (LockAndUpdateView())
		{
			
			BRegion tmpClip = BRegion(BRect(x, y, x + w - 1, y + h - 1));
			mView->ConstrainClippingRegion(&tmpClip);
			mView->InvertRect(BRect(x, y, x + w - 1, y + h - 1));
			mView->Sync();
			UnlockView();
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;
	BPoint *pts;
	BPolygon poly;
	BRect r;
	PRInt32 w, h;
	
	BPoint bpointbuf[64];
	pts = bpointbuf;
	if (aNumPoints>64)
		pts = new BPoint[aNumPoints];
	for (int i = 0; i < aNumPoints; ++i)
	{
		nsPoint p = aPoints[i];
		mTranMatrix->TransformCoord(&p.x, &p.y);
		pts[i].x = p.x;
		pts[i].y = p.y;
	}
	poly.AddPoints(pts, aNumPoints);
	r = poly.Frame();	
	w = r.IntegerWidth();
	h = r.IntegerHeight();

	if (w && h)
	{
		if (LockAndUpdateView())
		{
			if (1 == h)
			{
				mView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left + w - 1, r.top), mCurrentLinePattern);
			}
			else if (1 == w)
			{
				mView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.top + h -1), mCurrentLinePattern);
			}
			else
			{
				poly.MapTo(r,BRect(r.left, r.top, r.left + w -1, r.top + h - 1));
				mView->StrokePolygon(&poly, true, mCurrentLinePattern);
			}
			UnlockView();
		}		
	}		
	if (pts!=bpointbuf)
		delete [] pts;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;
	
	BPoint *pts;
	BPolygon poly;
	BRect r;
	PRInt32 w, h;
	BPoint bpointbuf[64];
	pts = bpointbuf;
	if (aNumPoints>64)
		pts = new BPoint[aNumPoints];
	for (int i = 0; i < aNumPoints; ++i)
	{
		nsPoint p = aPoints[i];
		mTranMatrix->TransformCoord(&p.x, &p.y);
		pts[i].x = p.x;
		pts[i].y = p.y;
	}
	poly.AddPoints(pts, aNumPoints);
	r = poly.Frame();
	w = r.IntegerWidth();
	h = r.IntegerHeight();

	if (w && h)
	{
		if (LockAndUpdateView())
		{
			if (1 == h)
			{
				mView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left + w - 1, r.top));
			}
			else if (1 == w)
			{
				mView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.top + h -1));
			}
			else
			{
				poly.MapTo(r,BRect(r.left, r.top, r.left + w -1, r.top + h - 1));
				mView->FillPolygon(&poly, B_SOLID_HIGH);
			}
			UnlockView();
		}		
	}
	if (pts!=bpointbuf)
		delete [] pts;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawEllipse(const nsRect &aRect)
{
	return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;

	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	
	if (LockAndUpdateView())
	{
		mView->StrokeEllipse(BRect(x, y, x + w - 1, y + h - 1), mCurrentLinePattern);
		UnlockView();
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::FillEllipse(const nsRect &aRect)
{
	return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextBeOS::FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;

	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	
	if (LockAndUpdateView())
	{
		mView->FillEllipse(BRect(x, y, x + w - 1, y + h - 1));
		UnlockView();
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawArc(const nsRect& aRect, float aStartAngle, float aEndAngle)
{
	return DrawArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
	float aStartAngle, float aEndAngle)
{
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;

	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	
	if (LockAndUpdateView())
	{
		
		mView->StrokeArc(BRect(x, y, x + w - 1, y + h - 1), 
						aStartAngle, aEndAngle - aStartAngle, mCurrentLinePattern);
		UnlockView();
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::FillArc(const nsRect &aRect, float aStartAngle, float aEndAngle)
{
	return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP nsRenderingContextBeOS::FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
	float aStartAngle, float aEndAngle)
{
	if (nsnull == mTranMatrix || nsnull == mSurface)
		return NS_ERROR_FAILURE;

	nscoord x = aX, y = aY, w = aWidth, h = aHeight;
	mTranMatrix->TransformCoord(&x, &y, &w, &h);
	
	if (LockAndUpdateView())
	{
		mView->FillArc(BRect(x, y, x + w - 1, y + h - 1), aStartAngle, aEndAngle - aStartAngle);
		UnlockView();
	}
	return NS_OK;
}







inline uint32 utf8_char_len(uchar byte) 
{
	return (((0xE5000000 >> ((byte >> 3) & 0x1E)) & 3) + 1);
}



#define BEGINS_CHAR(byte) ((byte & 0xc0) != 0x80)

inline uint32 utf8_str_len(const char* ustring) 
{
	uint32 cnt = 0;
	while ( *ustring != '\0')
	{
		if ( BEGINS_CHAR( *ustring ) )
			++cnt;
			++ustring;
	}
	return cnt;       
}





#define convert_to_utf8(str, uni_str) {\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}



NS_IMETHODIMP nsRenderingContextBeOS::GetWidth(char aC, nscoord &aWidth)
{
	return GetWidth(&aC, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextBeOS::GetWidth(PRUnichar aC, nscoord &aWidth, PRInt32 *aFontID)
{
	return GetWidth(&aC, 1, aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextBeOS::GetWidth(const nsString &aString, nscoord& aWidth, PRInt32 *aFontID)
{
	return GetWidth(aString.get(), aString.Length(), aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextBeOS::GetWidth(const char *aString, nscoord &aWidth)
{
	return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP nsRenderingContextBeOS::GetWidth(const char *aString, PRUint32 aLength, nscoord &aWidth)
{
	if (0 == aLength)
	{
		aWidth = 0;
	}
	else
	{
		if (aString == nsnull)
			return NS_ERROR_FAILURE;
		
		aWidth  = nscoord(((nsFontMetricsBeOS *)mFontMetrics)->GetStringWidth((char *)aString, aLength) * mP2T);
	}
	return NS_OK;
}


NS_IMETHODIMP nsRenderingContextBeOS::GetWidth(const PRUnichar *aString, PRUint32 aLength,
	nscoord &aWidth, PRInt32 *aFontID)
{
	
	uint8 *utf8str = new uint8[aLength * 4 + 1];
	uint8 *utf8ptr = utf8str;
	const PRUnichar *uniptr = aString;
	
	for (PRUint32 i = 0; i < aLength; i++)
	{
		convert_to_utf8(utf8ptr, uniptr);
	}
	
	*utf8ptr = '\0';
	uint32 utf8str_len = strlen((char *)utf8str);
	GetWidth((char *)utf8str, utf8str_len, aWidth);
	delete [] utf8str;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetTextDimensions(const char *aString, PRUint32 aLength,
	nsTextDimensions &aDimensions)
{
	if (mFontMetrics)
	{
		mFontMetrics->GetMaxAscent(aDimensions.ascent);
		mFontMetrics->GetMaxDescent(aDimensions.descent);
	}	
	return GetWidth(aString, aLength, aDimensions.width);
}

NS_IMETHODIMP nsRenderingContextBeOS::GetTextDimensions(const PRUnichar *aString, PRUint32 aLength,
	nsTextDimensions &aDimensions, PRInt32 *aFontID)
{
	if (mFontMetrics)
	{
		mFontMetrics->GetMaxAscent(aDimensions.ascent);
		mFontMetrics->GetMaxDescent(aDimensions.descent);
	}	
	return GetWidth(aString, aLength, aDimensions.width, aFontID);
}









NS_IMETHODIMP nsRenderingContextBeOS::GetTextDimensions(const PRUnichar* aString,
	PRInt32 aLength, PRInt32 aAvailWidth, PRInt32* aBreaks, PRInt32 aNumBreaks,
	nsTextDimensions& aDimensions, PRInt32& aNumCharsFit, nsTextDimensions& aLastWordDimensions,
	PRInt32* aFontID = nsnull)
{
	nsresult ret_code = NS_ERROR_FAILURE;	
	uint8 utf8buf[1024];
	uint8* utf8str = nsnull;
	
	PRUint32 slength = aLength * 4 + 1;
	
	
	
	
	if (slength < 1024) 
		utf8str = utf8buf;
	else 
		utf8str = new uint8[slength];

	uint8 *utf8ptr = utf8str;
	const PRUnichar *uniptr = aString;
	
	for (PRUint32 i = 0; i < aLength; ++i) 
		convert_to_utf8(utf8ptr, uniptr);
	
	*utf8ptr = '\0';
	ret_code = GetTextDimensions((char *)utf8str, utf8ptr-utf8str, aAvailWidth, aBreaks, aNumBreaks,
                               aDimensions, aNumCharsFit, aLastWordDimensions, aFontID);
	
	if (utf8str != utf8buf)
			delete [] utf8str;
	return ret_code;
}

NS_IMETHODIMP nsRenderingContextBeOS::GetTextDimensions(const char* aString, PRInt32 aLength,
	PRInt32 aAvailWidth,PRInt32* aBreaks, PRInt32 aNumBreaks, nsTextDimensions& aDimensions,
	PRInt32& aNumCharsFit, nsTextDimensions& aLastWordDimensions, PRInt32* aFontID = nsnull)
{
	
	
	char * utf8ptr = (char *)aString;
	PRInt32 *utf8pos =0;	
	PRInt32 num_of_glyphs = 0; 
	PRInt32 utf8posbuf[1025];
	if (aLength < 1025) 
		utf8pos = utf8posbuf;
	else 
		utf8pos = new PRInt32[aLength + 1];

	
	
	PRInt32 i = 0;
	while (i < aLength)
	{
		if ( BEGINS_CHAR( utf8ptr[i] ) )
		{
			utf8pos[num_of_glyphs] = i;
			++num_of_glyphs;
		}
		i++;
	}	
	utf8pos[num_of_glyphs] = i; 
	NS_PRECONDITION(aBreaks[aNumBreaks - 1] == num_of_glyphs, "invalid break array");
	
	
	
	PRInt32 prevBreakState_BreakIndex = -1; 
	nscoord prevBreakState_Width = 0; 
	mFontMetrics->GetMaxAscent(aLastWordDimensions.ascent);
	mFontMetrics->GetMaxDescent(aLastWordDimensions.descent);
	aLastWordDimensions.width = -1;
	aNumCharsFit = 0;
	
	nscoord width = 0;
	PRInt32 start = 0;
	nscoord aveCharWidth;
	PRInt32 numBytes = 0;
	
	
	
	mFontMetrics->GetAveCharWidth(aveCharWidth);


	while (start < num_of_glyphs) 
	{
		
		
		
		PRInt32 estimatedNumChars = 0;
		if (aveCharWidth > 0) 
			estimatedNumChars = (aAvailWidth - width) / aveCharWidth;

		if (estimatedNumChars < 1) 
			estimatedNumChars = 1;

		
		PRInt32 estimatedBreakOffset = start + estimatedNumChars;
		PRInt32 breakIndex;
		nscoord numChars;
		if (num_of_glyphs <= estimatedBreakOffset) 
		{
			
			numChars = num_of_glyphs - start;
			
			numBytes = aLength - utf8pos[start];
			breakIndex = aNumBreaks - 1;
		}
		else 
		{
			breakIndex = prevBreakState_BreakIndex;
			while (((breakIndex + 1) < aNumBreaks) 
					&& (aBreaks[breakIndex + 1] <= estimatedBreakOffset)) 
			{
				++breakIndex;
			}
			
			if (breakIndex == prevBreakState_BreakIndex) 
				++breakIndex; 

			numChars = aBreaks[breakIndex] - start;
			numBytes = utf8pos[aBreaks[breakIndex]] - utf8pos[start];
		}
		nscoord twWidth = 0;
		if ((1 == numChars) && (aString[utf8pos[start]] == ' ')) 
		{
			mFontMetrics->GetSpaceWidth(twWidth);
		} 
		else if (numChars > 0) 
		{
			GetWidth(&aString[utf8pos[start]], numBytes, twWidth);
		} 

		
		PRBool  textFits = (twWidth + width) <= aAvailWidth;
		
		
		if (textFits) 
		{
			aNumCharsFit += numChars;
			width += twWidth;
			start += numChars;

			
			
			prevBreakState_BreakIndex = breakIndex;
			prevBreakState_Width = width;
		}
		else 
		{
			
			
			if (prevBreakState_BreakIndex > 0) 
			{
				
				
				if (prevBreakState_BreakIndex == (breakIndex - 1)) 
				{
					aNumCharsFit = aBreaks[prevBreakState_BreakIndex];
					width = prevBreakState_Width;
					break;
				}
			}
			
			if (0 == breakIndex)
			{
				
				
				aNumCharsFit += numChars;
				width += twWidth;
				break;
			}       
			
			
			width += twWidth;
			while ((breakIndex >= 1) && (width > aAvailWidth)) 
			{
				twWidth = 0;
				start = aBreaks[breakIndex - 1];
				numChars = aBreaks[breakIndex] - start;
				numBytes = utf8pos[aBreaks[breakIndex]] - utf8pos[start];
				if ((1 == numChars) && (aString[utf8pos[start]] == ' ')) 
				{
					mFontMetrics->GetSpaceWidth(twWidth);
				}
				else if (numChars > 0) 
				{
					GetWidth(&aString[utf8pos[start]], numBytes, twWidth);
				}
				width -= twWidth;
				aNumCharsFit = start;
				--breakIndex;
			}
		break;   
		}	       
	}
	aDimensions.width = width;
	mFontMetrics->GetMaxAscent(aDimensions.ascent);
	mFontMetrics->GetMaxDescent(aDimensions.descent);
	
	if(utf8pos != utf8posbuf) 
		delete utf8pos;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawString(const nsString &aString,
	nscoord aX, nscoord aY, PRInt32 aFontID, const nscoord *aSpacing)
{
	return DrawString(aString.get(), aString.Length(), aX, aY, aFontID, aSpacing);
}




NS_IMETHODIMP nsRenderingContextBeOS::DrawString(const char *aString, PRUint32 aLength,
	nscoord aX, nscoord aY, const nscoord *aSpacing)
{
	
	if (0 == aLength)
		return NS_OK; 

	if (mTranMatrix == nsnull)
		return NS_ERROR_FAILURE;
	if (mSurface == nsnull)
		return NS_ERROR_FAILURE;
	if (aString == nsnull)
		return NS_ERROR_FAILURE;

	nscoord xx = aX, yy = aY, y=aY;
	
	if (LockAndUpdateView())  
	{
		PRBool doEmulateBold = PR_FALSE;
		
		if (mFontMetrics) 
		{
			doEmulateBold = ((nsFontMetricsBeOS *)mFontMetrics)->IsBold() && !(mCurrentBFont->Face() & B_BOLD_FACE);
		}
		
		
		PRBool offscreen;
		mSurface->IsOffscreen(&offscreen);
		mView->SetDrawingMode( offscreen ? B_OP_OVER : B_OP_COPY);
		if (nsnull == aSpacing || utf8_char_len((uchar)aString[0])==aLength) 
		{
			mTranMatrix->TransformCoord(&xx, &yy);
			mView->DrawString(aString, aLength, BPoint(xx, yy));
			if (doEmulateBold)
				mView->DrawString(aString, aLength, BPoint(xx + 1.0, yy));
		}
		else 
		{
			char *wpoint =0;
			int32 unichnum=0,  position=aX, ch_len=0;
			for (PRUint32 i =0; i <= aLength; i += ch_len)
			{
				ch_len = utf8_char_len((uchar)aString[i]);
				wpoint = (char *)(aString + i);
				xx = position; 
				yy = y;
				mTranMatrix->TransformCoord(&xx, &yy);
				
				mView->DrawString((char *)(wpoint), ch_len, BPoint(xx, yy));
				if (doEmulateBold)
					mView->DrawString((char *)(wpoint), ch_len, BPoint(xx + 1.0, yy));
				position += aSpacing[unichnum++];
			}
		}
		mView->SetDrawingMode(B_OP_COPY);
		UnlockView();
	}
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::DrawString(const PRUnichar *aString, PRUint32 aLength,
	nscoord aX, nscoord aY, PRInt32 aFontID, const nscoord *aSpacing)
{
	
	
	uint8 *utf8str = new uint8[aLength * 4 + 1];
	uint8 *utf8ptr = utf8str;
	const PRUnichar *uniptr = aString;
	
	for (PRUint32 i = 0; i < aLength; i++) 
		convert_to_utf8(utf8ptr, uniptr);
	
	*utf8ptr = '\0';
	uint32 utf8str_len = strlen((char *)utf8str);
	DrawString((char *)utf8str, utf8str_len, aX, aY, aSpacing);
	delete [] utf8str;
	return NS_OK;
}

NS_IMETHODIMP nsRenderingContextBeOS::CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
	PRInt32 aSrcX, PRInt32 aSrcY, const nsRect &aDestBounds, PRUint32 aCopyFlags)
{
	
	PRInt32 srcX = aSrcX;
	PRInt32 srcY = aSrcY;
	nsRect drect = aDestBounds;
	
	if (aSrcSurf == nsnull)
		return NS_ERROR_FAILURE;
	if (mTranMatrix == nsnull)
		return NS_ERROR_FAILURE;
	if (mSurface == nsnull)
		return NS_ERROR_FAILURE;
		
	BView *srcview = nsnull;
	BView *destview = nsnull;
	BBitmap *srcbitmap = nsnull;
	nsDrawingSurfaceBeOS *srcsurf = nsnull;
	nsDrawingSurfaceBeOS *destsurf = nsnull;
	
	srcsurf = (nsDrawingSurfaceBeOS *)aSrcSurf;
	srcsurf->AcquireView(&srcview);
	srcsurf->AcquireBitmap(&srcbitmap);
	
	
	if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER) 
	{
		NS_ASSERTION(nsnull != mSurface, "no back buffer");
		destsurf = mSurface;
	} 
	else 
	{
		destsurf = mOffscreenSurface;
	}
			
	if (!srcbitmap && srcsurf != mOffscreenSurface)
	{
#ifdef DEBUG
    		printf("nsRenderingContextBeOS::CopyOffScreenBits - FIXME: should render from surface without bitmap!?!?!\n");
#endif
		if (srcview)
			srcsurf->ReleaseView();
		return NS_OK;
	}
	
	destsurf->AcquireView(&destview);

	if (!destview)
	{
#ifdef DEBUG
    		printf("nsRenderingContextBeOS::CopyOffScreenBits - FIXME: no BView to draw!?!?!\n");
#endif
    		return NS_OK;
	}

	if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES) 
		mTranMatrix->TransformCoord(&srcX, &srcY);
 	
	if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
					mTranMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);

	if (!LockAndUpdateView())
	{
#ifdef DEBUG
		printf("nsRenderingContextBeOS::CopyOffScreenBits - FIXME: no mVviewView - LockAndUpdate failed!\n");
#endif
    		return NS_OK;
	}
	
	if (srcsurf != mSurface)
		srcsurf->LockDrawable();
	if (destsurf != mSurface)
		destsurf->LockDrawable();
	
	
	if (srcview)
		srcview->Sync();

	if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION) 
	{
		BRegion *region = nsnull;
		if(mClipRegion && mSurface == aSrcSurf)
			mClipRegion->GetNativeRegion((void *&)region);
		
		
		
		else if(srcview->Bounds() == destview->Bounds())
			srcview->GetClippingRegion(region);
		destview->ConstrainClippingRegion(region);
	}
				
				
				
	
	destview->DrawBitmap(srcbitmap, BRect(srcX, srcY, srcX + drect.width - 1, srcY + drect.height - 1),
		BRect(drect.x, drect.y, drect.x + drect.width - 1, drect.y + drect.height - 1));
	
	if (destsurf != mSurface)
		destsurf->UnlockDrawable();
	if (srcsurf != mSurface)
		srcsurf->UnlockDrawable();
	UnlockView();
	
	destsurf->ReleaseView();	
	srcsurf->ReleaseBitmap();	
	srcsurf->ReleaseView();
	return NS_OK;
}

#ifdef MOZ_MATHML
  


NS_IMETHODIMP 
nsRenderingContextBeOS::GetBoundingMetrics(const char* aString, PRUint32 aLength, nsBoundingMetrics& aBoundingMetrics)
{
	aBoundingMetrics.Clear();
	if (0 >= aLength || !aString || !mCurrentBFont)
		return NS_ERROR_FAILURE;

	BRect rect;
	escapement_delta delta;
	delta.nonspace = 0;
	delta.space = 0;
	
	mCurrentBFont->GetBoundingBoxesForStrings(&aString, 1, B_PRINTING_METRIC, &delta, &rect);


	GetWidth(aString, aLength, aBoundingMetrics.width );
	
	aBoundingMetrics.leftBearing = NSToCoordRound(rect.left * mP2T);
	aBoundingMetrics.rightBearing = NSToCoordRound(rect.right * mP2T);

	
	
	
	
	
	aBoundingMetrics.ascent = NSToCoordRound((-rect.top) * mP2T);
	aBoundingMetrics.descent = NSToCoordRound(rect.bottom * mP2T);

	return NS_OK;
}

  


NS_IMETHODIMP 
nsRenderingContextBeOS::GetBoundingMetrics(const PRUnichar* aString, PRUint32 aLength,
	nsBoundingMetrics& aBoundingMetrics, PRInt32* aFontID) 
{
	aBoundingMetrics.Clear();
	nsresult r = NS_OK;
	if (0 < aLength)
	{
		if (aString == NULL)
			return NS_ERROR_FAILURE;

		
		
		uint8 utf8buf[1024];
		uint8* utf8str = (uint8*)&utf8buf;
		if (aLength * 4 + 1 > 1024)
			utf8str = new uint8[aLength * 4 + 1];
		uint8 *utf8ptr = utf8str;
		const PRUnichar *uniptr = aString;
	
		for (PRUint32 i = 0; i < aLength; i++)
		{
			convert_to_utf8(utf8ptr, uniptr);
		}
	
		*utf8ptr = '\0';
		uint32 utf8str_len = strlen((char *)utf8str);
		r = GetBoundingMetrics((char *)utf8str, utf8str_len, aBoundingMetrics);
		if (utf8str != utf8buf)
			delete [] utf8str;
		if (nsnull != aFontID)
			*aFontID = 0;
	}		
	return r;
}
#endif 
