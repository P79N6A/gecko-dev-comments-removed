






































#include "nsIInterfaceRequestorUtils.h" 
#include "nsIServiceManager.h"
#include "nsRenderingContextMac.h"
#include "nsDeviceContextMac.h"
#include "nsFontMetricsMac.h"
#include "nsIRegion.h"
#include "nsIEnumerator.h"
#include "nsRegionMac.h"
#include "nsGraphicState.h"

#include "nsTransform2D.h"
#include "nsVoidArray.h"
#include "nsGfxCIID.h"
#include "nsGfxUtils.h"
#include "nsCOMPtr.h"

#include "plhash.h"

#include <FixMath.h>
#include <Gestalt.h>
#include <Quickdraw.h>

#include "nsRegionPool.h"
#include "nsFontUtils.h"

#include "nsCarbonHelpers.h"

#define STACK_THRESHOLD 1000




nsRenderingContextMac::nsRenderingContextMac()
: mP2T(1.0f)
, mContext(nsnull)
, mCurrentSurface(nsnull)
, mPort(nsnull)
, mGS(nsnull)
, mChanges(kEverythingChanged)
, mRightToLeftText(PR_FALSE)
{
	mFrontSurface				= new nsDrawingSurfaceMac();
	NS_IF_ADDREF(mFrontSurface);
}




nsRenderingContextMac::~nsRenderingContextMac()
{
	
	NS_IF_RELEASE(mContext);

	
	NS_IF_RELEASE(mFrontSurface);
	NS_IF_RELEASE(mCurrentSurface);

	mPort = nsnull;
	mGS = nsnull;

	
  PRInt32 cnt = mGSStack.Count();
	  for (PRInt32 i = 0; i < cnt; i ++) {
    nsGraphicState* gs = (nsGraphicState*)mGSStack.ElementAt(i);
    	if (gs != nsnull)
    		sGraphicStatePool.ReleaseGS(gs); 
	  }

	NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");
}


NS_IMPL_ISUPPORTS1(nsRenderingContextMac, nsIRenderingContext)




NS_IMETHODIMP nsRenderingContextMac::Init(nsIDeviceContext* aContext, nsIWidget* aWindow)
{
	NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");

	
	if (nsnull == mFrontSurface)
		return NS_ERROR_OUT_OF_MEMORY;
		
	if (nsnull == aWindow->GetNativeData(NS_NATIVE_WINDOW))
		return NS_ERROR_NOT_INITIALIZED;

	if (aContext != mContext) {
		NS_IF_RELEASE(mContext);
		mContext = aContext;
		NS_IF_ADDREF(mContext);
	}

 	
	mFrontSurface->Init(aWindow);
	SelectDrawingSurface(mFrontSurface);

	
	
	
	
	
	

	return NS_OK;
}




NS_IMETHODIMP nsRenderingContextMac::Init(nsIDeviceContext* aContext, nsIDrawingSurface* aSurface)
{
	
	if (nsnull == mFrontSurface)
		return NS_ERROR_OUT_OF_MEMORY;
		
	mContext = aContext;
	NS_IF_ADDREF(mContext);

	
	nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);
	SelectDrawingSurface(surface);

	return NS_OK;
}




nsresult nsRenderingContextMac::Init(nsIDeviceContext* aContext, CGrafPtr aPort)
{
	
	if (nsnull == mFrontSurface)
		return NS_ERROR_OUT_OF_MEMORY;
		
	mContext = aContext;
	NS_IF_ADDREF(mContext);

 	
	mFrontSurface->Init(aPort);
	SelectDrawingSurface(mFrontSurface);

	return NS_OK;
}



void nsRenderingContextMac::SelectDrawingSurface(nsDrawingSurfaceMac* aSurface, PRUint32 aChanges)
{
  NS_PRECONDITION(aSurface != nsnull, "null surface");
	if (! aSurface)
		return;

  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");

	
	if (mCurrentSurface != aSurface)
	{
		aChanges = kEverythingChanged;

		NS_IF_RELEASE(mCurrentSurface);
		mCurrentSurface = aSurface;
		NS_IF_ADDREF(mCurrentSurface);
	}
	
	CGrafPtr    newPort;
	aSurface->GetGrafPtr(&newPort);
	mPort = newPort;
	mGS = aSurface->GetGS();
	mTranMatrix = &(mGS->mTMatrix);

  nsGraphicsUtils::SafeSetPort(mPort);

#ifndef MOZ_WIDGET_COCOA
  
  
  
	::SetOrigin(-mGS->mOffx, -mGS->mOffy);		
#endif

	if (aChanges & kClippingChanged)
		::SetClip(mGS->mClipRegion);			

	::PenNormal();
	::PenMode(patCopy);
	::TextMode(srcOr);

	if (aChanges & kColorChanged)
		SetColor(mGS->mColor);

	if (mGS->mFontMetrics && (aChanges & kFontChanged))
		SetFont(mGS->mFontMetrics);
	
	if (!mContext) return;
	
	
#if 0
	((nsDeviceContextMac *)mContext)->InstallColormap();
#endif

	mP2T = mContext->DevUnitsToAppUnits();

	if (mGS->mTMatrix.GetType() == MG_2DIDENTITY) {
		
		float app2dev;
		app2dev = mContext->AppUnitsToDevUnits();
		mGS->mTMatrix.AddScale(app2dev, app2dev);
	}

  NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");
}




nsresult nsRenderingContextMac::SetPortTextState()
{
	NS_PRECONDITION(mGS->mFontMetrics != nsnull, "No font metrics in SetPortTextState");
	
	if (nsnull == mGS->mFontMetrics)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(mContext != nsnull, "No device context in SetPortTextState");
	
	if (nsnull == mContext)
		return NS_ERROR_NULL_POINTER;

	TextStyle		theStyle;
	nsFontUtils::GetNativeTextStyle(*mGS->mFontMetrics, *mContext, theStyle);

	::TextFont(theStyle.tsFont);
	::TextSize(theStyle.tsSize);
	::TextFace(theStyle.tsFace);

	return NS_OK;
}




void nsRenderingContextMac::SetupPortState()
{
  nsGraphicsUtils::SafeSetPort(mPort);
#ifndef MOZ_WIDGET_COCOA
	::SetOrigin(-mGS->mOffx, -mGS->mOffy);
#endif
	::SetClip(mGS->mClipRegion);
}

#pragma mark -



NS_IMETHODIMP nsRenderingContextMac::PushState(void)
{
	
	nsGraphicState * gs = sGraphicStatePool.GetNewGS();
	if (!gs)
		return NS_ERROR_OUT_OF_MEMORY;

	
	mGS->SetChanges(mChanges);

	
	gs->Duplicate(mGS);

	
	mGSStack.AppendElement(gs);
  
	
	mChanges = 0;

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::PopState(void)
{
	NS_ASSERTION(ValidateDrawingState(), "Bad drawing state");

	PRInt32 count = mGSStack.Count();
	NS_ASSERTION(count > 0, "No state to pop");
	if (count > 0) {
		PRInt32 index = count - 1;
	
		
		nsGraphicState* gs = (nsGraphicState *)mGSStack.ElementAt(index);

		
		mGS->Duplicate(gs);
		
		SelectDrawingSurface(mCurrentSurface, mChanges);
		
		
		mChanges = mGS->GetChanges();

		
		mGSStack.RemoveElementAt(index);
		sGraphicStatePool.ReleaseGS(gs);
		
		
		mTranMatrix = &(mGS->mTMatrix);
	}

	return NS_OK;
}

#pragma mark -



NS_IMETHODIMP nsRenderingContextMac::LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                                          PRUint32 aWidth, PRUint32 aHeight,
                                                          void **aBits, PRInt32 *aStride,
                                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  PushState();

  return mCurrentSurface->Lock(aX, aY, aWidth, aHeight,
  			aBits, aStride, aWidthBytes, aFlags);
}



NS_IMETHODIMP nsRenderingContextMac::UnlockDrawingSurface(void)
{
  PopState();

  mCurrentSurface->Unlock();
  
  return NS_OK;
}




NS_IMETHODIMP nsRenderingContextMac::SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{  
	nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);

	if (surface != nsnull)
		SelectDrawingSurface(surface);				
	else
		SelectDrawingSurface(mFrontSurface);		

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetDrawingSurface(nsIDrawingSurface* *aSurface)
{  
	*aSurface = mCurrentSurface;
	
	
	
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
                                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                                         const nsRect &aDestBounds,
                                                         PRUint32 aCopyFlags)
{
	
	
	if ((aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER) == 0)
		if (::EmptyRgn(mFrontSurface->GetGS()->mMainRegion))
			return NS_OK;

	
	nsDrawingSurfaceMac* srcSurface = static_cast<nsDrawingSurfaceMac*>(aSrcSurf);
	CGrafPtr srcPort;
	srcSurface->GetGrafPtr(&srcPort);

	
	PRInt32 x = aSrcX;
	PRInt32 y = aSrcY;
	if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
		mGS->mTMatrix.TransformCoord(&x, &y);

	nsRect dstRect = aDestBounds;
	if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
		mGS->mTMatrix.TransformCoord(&dstRect.x, &dstRect.y, &dstRect.width, &dstRect.height);

	
	Rect macSrcRect, macDstRect;
	::SetRect(&macSrcRect, x, y, x + dstRect.width, y + dstRect.height);
	::SetRect(&macDstRect, dstRect.x, dstRect.y, dstRect.x + dstRect.width, dstRect.y + dstRect.height);
  
	
	StRegionFromPool clipRgn;
	if (!clipRgn) return NS_ERROR_OUT_OF_MEMORY;
		
	if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION) {
		::GetPortClipRegion(srcPort, clipRgn);
	} else
		::CopyRgn(mGS->mMainRegion, clipRgn);

	
	CGrafPtr destPort;
	nsDrawingSurfaceMac* destSurface;
	if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER) {
		destSurface	= mCurrentSurface;
		destPort		= mPort;
		NS_ASSERTION((destPort != nsnull), "no back buffer");
	} else {
		destSurface	= mFrontSurface;
		mFrontSurface->GetGrafPtr(&destPort);
	}

  
  SelectDrawingSurface(destSurface);
  
	
	RGBColor foreColor;
	Boolean changedForeColor = false;
	::GetForeColor(&foreColor);
	if ((foreColor.red != 0x0000) || (foreColor.green != 0x0000) || (foreColor.blue != 0x0000)) {
		RGBColor rgbBlack = {0x0000,0x0000,0x0000};
		::RGBForeColor(&rgbBlack);
		changedForeColor = true;
	}

	RGBColor backColor;
	Boolean changedBackColor = false;
	::GetBackColor(&backColor);
	if ((backColor.red != 0xFFFF) || (backColor.green != 0xFFFF) || (backColor.blue != 0xFFFF)) {
		RGBColor rgbWhite = {0xFFFF,0xFFFF,0xFFFF};
		::RGBBackColor(&rgbWhite);
		changedBackColor = true;
	}

	
	::CopyBits(
          ::GetPortBitMapForCopyBits(srcPort),
          ::GetPortBitMapForCopyBits(destPort),
		  &macSrcRect,
		  &macDstRect,
		  srcCopy,
		  clipRgn);

	
	if (changedForeColor)
		::RGBForeColor(&foreColor);
	if (changedBackColor)
		::RGBBackColor(&backColor);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::CreateDrawingSurface(const nsRect& aBounds, PRUint32 aSurfFlags, nsIDrawingSurface* &aSurface)
{
	aSurface = nsnull;

	PRUint32 depth = 8;
	if (mContext)
		mContext->GetDepth(depth);

	
	Rect macRect;
  
  ::SetRect(&macRect, aBounds.x, aBounds.y, aBounds.XMost(), aBounds.YMost());
  
	nsDrawingSurfaceMac* surface = new nsDrawingSurfaceMac();
	if (!surface)
		return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(surface);

	nsresult rv = surface->Init(depth, macRect.right, macRect.bottom, aSurfFlags);
	if (NS_SUCCEEDED(rv))
		aSurface = surface;
	else
		delete surface;

	return rv;
}



NS_IMETHODIMP nsRenderingContextMac::DestroyDrawingSurface(nsIDrawingSurface* aSurface)
{
	if (!aSurface)
		return NS_ERROR_FAILURE;

	
	if (aSurface == mCurrentSurface)
	{
	  NS_ASSERTION(mCurrentSurface != mFrontSurface, "Nuking the front surface");
		SelectDrawingSurface(mFrontSurface);
  }
	
	nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);
	NS_IF_RELEASE(surface);

	return NS_OK;
}


#pragma mark -


NS_IMETHODIMP nsRenderingContextMac::GetHints(PRUint32& aResult)
{
	PRUint32 result = 0;

	
	
	result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;
	aResult = result;
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::Reset(void)
{
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetDeviceContext(nsIDeviceContext *&aContext)
{
	if (mContext) {
		aContext = mContext;
		NS_ADDREF(aContext);
	} else {
		aContext = nsnull;
	}
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
	aVisible = PR_TRUE;
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
	nsRect  trect = aRect;

	mGS->mTMatrix.TransformCoord(&trect.x, &trect.y, &trect.width, &trect.height);

	Rect macRect;
	::SetRect(&macRect, trect.x, trect.y, trect.x + trect.width, trect.y + trect.height);

	StRegionFromPool rectRgn;
	RgnHandle clipRgn = mGS->mClipRegion;
	if (!clipRgn || !rectRgn)
		return NS_ERROR_OUT_OF_MEMORY;

	::RectRgn(rectRgn, &macRect);

	switch (aCombine) {
	case nsClipCombine_kIntersect:
		::SectRgn(clipRgn, rectRgn, clipRgn);
		break;

	case nsClipCombine_kUnion:
		::UnionRgn(clipRgn, rectRgn, clipRgn);
		break;

	case nsClipCombine_kSubtract:
		::DiffRgn(clipRgn, rectRgn, clipRgn);
		break;

	case nsClipCombine_kReplace:

		::SectRgn(rectRgn, mGS->mMainRegion, clipRgn);
		break;
	}

	{
		StPortSetter setter(mPort);
		::SetClip(clipRgn);
	}

	mGS->mClipRegion = clipRgn;
	
	
	mChanges |= kClippingChanged;

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
	Rect	cliprect;

	if (mGS->mClipRegion != nsnull) {
		::GetRegionBounds(mGS->mClipRegion, &cliprect);
		aRect.SetRect(cliprect.left, cliprect.top, cliprect.right - cliprect.left, cliprect.bottom - cliprect.top);
		aClipValid = PR_TRUE;
 	} else {
		aRect.SetRect(0,0,0,0);
		aClipValid = PR_FALSE;
 	}

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
	RgnHandle regionH;
	aRegion.GetNativeRegion((void*&)regionH);

	RgnHandle clipRgn = mGS->mClipRegion;
	if (!clipRgn) return NS_ERROR_OUT_OF_MEMORY;

	switch (aCombine) {
	case nsClipCombine_kIntersect:
		::SectRgn(clipRgn, regionH, clipRgn);
		break;

	case nsClipCombine_kUnion:
		::UnionRgn(clipRgn, regionH, clipRgn);
		break;

	case nsClipCombine_kSubtract:
		::DiffRgn(clipRgn, regionH, clipRgn);
		break;

	case nsClipCombine_kReplace:

		::SectRgn(regionH, mGS->mMainRegion, clipRgn);
		break;
	}

		
	{
		StPortSetter setter(mPort);
		::SetClip(clipRgn);
	}

	mGS->mClipRegion = clipRgn;

	
	mChanges |= kClippingChanged;

	return NS_OK;
}




NS_IMETHODIMP nsRenderingContextMac::CopyClipRegion(nsIRegion &aRegion)
{
	nsRegionMac* macRegion = (nsRegionMac*)&aRegion;
	macRegion->SetNativeRegion(mGS->mClipRegion);
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetClipRegion(nsIRegion **aRegion)
{
	nsresult  rv = NS_OK;

	NS_ASSERTION(!(nsnull == aRegion), "no region ptr");

	if (nsnull == *aRegion) {
		nsRegionMac *rgn = new nsRegionMac();

		if (nsnull != rgn) {
			NS_ADDREF(rgn);
			rv = rgn->Init();

			if (NS_OK != rv)
				NS_RELEASE(rgn);
			else
				*aRegion = rgn;
		} else
			rv = NS_ERROR_OUT_OF_MEMORY;
	}

	if (rv == NS_OK) {
		nsRegionMac* macRegion = *(nsRegionMac**)aRegion;
		macRegion->SetNativeRegion(mGS->mClipRegion);
	}

	return rv;
}



NS_IMETHODIMP nsRenderingContextMac::SetColor(nscolor aColor)
{
	SetupPortState();

	#define COLOR8TOCOLOR16(color8)	 ((color8 << 8) | color8)

	RGBColor color;
	color.red   = COLOR8TOCOLOR16(NS_GET_R(aColor));
	color.green = COLOR8TOCOLOR16(NS_GET_G(aColor));
	color.blue  = COLOR8TOCOLOR16(NS_GET_B(aColor));
	::RGBForeColor(&color);
	mGS->mColor = aColor ;

	mChanges |= kColorChanged;
  	
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetColor(nscolor &aColor) const
{
	aColor = mGS->mColor;
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::SetLineStyle(nsLineStyle aLineStyle)
{
  mGS->mLineStyle = aLineStyle;
  return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetLineStyle(nsLineStyle &aLineStyle)
{
  aLineStyle = mGS->mLineStyle;
  return NS_OK;
}




NS_IMETHODIMP nsRenderingContextMac::SetFont(const nsFont& aFont, nsIAtom* aLangGroup)
{
	NS_IF_RELEASE(mGS->mFontMetrics);

	if (mContext)
		mContext->GetMetricsFor(aFont, aLangGroup, mGS->mFontMetrics);

	mChanges |= kFontChanged;
		
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::SetFont(nsIFontMetrics *aFontMetrics)
{
	NS_IF_RELEASE(mGS->mFontMetrics);
	mGS->mFontMetrics = aFontMetrics;
	NS_IF_ADDREF(mGS->mFontMetrics);
	mChanges |= kFontChanged;
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
	NS_IF_ADDREF(mGS->mFontMetrics);
	aFontMetrics = mGS->mFontMetrics;
	return NS_OK;
}




NS_IMETHODIMP nsRenderingContextMac::Translate(nscoord aX, nscoord aY)
{
	mGS->mTMatrix.AddTranslation((float)aX,(float)aY);
	return NS_OK;
}




NS_IMETHODIMP nsRenderingContextMac::Scale(float aSx, float aSy)
{
	mGS->mTMatrix.AddScale(aSx, aSy);
	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetCurrentTransform(nsTransform2D *&aTransform)
{
	aTransform = &mGS->mTMatrix;
	return NS_OK;
}


#pragma mark -










static const Pattern dottedPattern = {0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55};










static const Pattern dashedPattern = {0xf0,0x78,0x3c,0x1e,0x0f,0x87,0xc3,0xe1};



NS_IMETHODIMP nsRenderingContextMac::DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  if (mGS->mLineStyle == nsLineStyle_kNone)
    return NS_OK;

  SetupPortState();

  PenState savedPenState;
  RGBColor savedBGColor;
  if (mGS->mLineStyle == nsLineStyle_kDotted ||
      mGS->mLineStyle == nsLineStyle_kDashed) {
    ::GetPenState(&savedPenState);
    ::GetBackColor(&savedBGColor);

    ::PenMode(transparent);
    if (mGS->mLineStyle == nsLineStyle_kDashed)
      ::PenPat(&dashedPattern);
    else
      ::PenPat(&dottedPattern);
    RGBColor invertedForeColor;
    ::GetForeColor(&invertedForeColor);
    ::InvertColor(&invertedForeColor);
    ::RGBBackColor(&invertedForeColor);
  }

	mGS->mTMatrix.TransformCoord(&aX0,&aY0);
	mGS->mTMatrix.TransformCoord(&aX1,&aY1);

	
	nscoord diffX = aX1 - aX0;
	if (diffX)
		diffX -= (diffX > 0 ? 1 : -1);

	nscoord diffY = aY1 - aY0;
	if (diffY)
		diffY -= (diffY > 0 ? 1 : -1);

  ::MoveTo(aX0, aY0);
  ::Line(diffX, diffY);

  if (mGS->mLineStyle == nsLineStyle_kDotted ||
      mGS->mLineStyle == nsLineStyle_kDashed) {
    ::PenMode(savedPenState.pnMode);
    ::PenPat(&savedPenState.pnPat);
    ::RGBBackColor(&savedBGColor);
  }

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  if (mGS->mLineStyle == nsLineStyle_kNone)
    return NS_OK;

  SetupPortState();

  PenState savedPenState;
  RGBColor savedBGColor;
  if (mGS->mLineStyle == nsLineStyle_kDotted ||
      mGS->mLineStyle == nsLineStyle_kDashed) {
    ::GetPenState(&savedPenState);
    ::GetBackColor(&savedBGColor);

    ::PenMode(transparent);
    if (mGS->mLineStyle == nsLineStyle_kDashed)
      ::PenPat(&dashedPattern);
    else
      ::PenPat(&dottedPattern);
    RGBColor invertedForeColor;
    ::GetForeColor(&invertedForeColor);
    ::InvertColor(&invertedForeColor);
    ::RGBBackColor(&invertedForeColor);
  }

  PRInt32 x,y;

  x = aPoints[0].x;
  y = aPoints[0].y;
  mGS->mTMatrix.TransformCoord((PRInt32*)&x,(PRInt32*)&y);

  ::MoveTo(x,y);
  for (PRInt32 i = 1; i < aNumPoints; i++) {
    x = aPoints[i].x;
    y = aPoints[i].y;
    mGS->mTMatrix.TransformCoord((PRInt32*)&x,(PRInt32*)&y);
    ::LineTo(x,y);
  }

  if (mGS->mLineStyle == nsLineStyle_kDotted ||
      mGS->mLineStyle == nsLineStyle_kDashed) {
    ::PenMode(savedPenState.pnMode);
    ::PenPat(&savedPenState.pnPat);
    ::RGBBackColor(&savedBGColor);
  }

	return NS_OK;
}

	







	inline short pinToShort(nscoord value)
	{
		if (value < -32768)
			return -32768;
		if (value > 32767)
			return 32767;
		return (short) value;
	}




NS_IMETHODIMP nsRenderingContextMac::DrawRect(const nsRect& aRect)
{
	return DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}



NS_IMETHODIMP nsRenderingContextMac::DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	SetupPortState();
	
	nscoord x,y,w,h;
	Rect	therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	mGS->mTMatrix.TransformCoord(&x, &y, &w, &h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::FrameRect(&therect);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::FillRect(const nsRect& aRect)
{
	return FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}



NS_IMETHODIMP nsRenderingContextMac::FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	SetupPortState();

	nscoord x,y,w,h;
	Rect	therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	
	mGS->mTMatrix.TransformCoord(&x, &y, &w, &h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::PaintRect(&therect);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
	SetupPortState();

	PolyHandle thepoly;
	PRInt32    x,y;

	thepoly = ::OpenPoly();
	if (nsnull == thepoly) {
		return NS_ERROR_OUT_OF_MEMORY;
	}

	x = aPoints[0].x;
	y = aPoints[0].y;
	mGS->mTMatrix.TransformCoord((PRInt32*)&x,(PRInt32*)&y);
	::MoveTo(x,y);

	for (PRInt32 i = 1; i < aNumPoints; i++) {
		x = aPoints[i].x;
		y = aPoints[i].y;
		
		mGS->mTMatrix.TransformCoord((PRInt32*)&x,(PRInt32*)&y);
		::LineTo(x,y);
	}

	::ClosePoly();

	::FramePoly(thepoly);
	::KillPoly(thepoly);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
	SetupPortState();

	PolyHandle thepoly;
	PRInt32    x,y;

	thepoly = ::OpenPoly();
	if (nsnull == thepoly) {
		return NS_ERROR_OUT_OF_MEMORY;
	}

	x = aPoints[0].x;
	y = aPoints[0].y;
	mGS->mTMatrix.TransformCoord((PRInt32*)&x,(PRInt32*)&y);
	::MoveTo(x,y);

	for (PRInt32 i = 1; i < aNumPoints; i++) {
		x = aPoints[i].x;
		y = aPoints[i].y;
		mGS->mTMatrix.TransformCoord((PRInt32*)&x,(PRInt32*)&y);
		::LineTo(x,y);
	}

	::ClosePoly();
	::PaintPoly(thepoly);
	::KillPoly(thepoly);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::DrawEllipse(const nsRect& aRect)
{
	return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}



NS_IMETHODIMP nsRenderingContextMac::DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	SetupPortState();

	nscoord x,y,w,h;
	Rect    therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	mGS->mTMatrix.TransformCoord(&x,&y,&w,&h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::FrameOval(&therect);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::FillEllipse(const nsRect& aRect)
{
	return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}



NS_IMETHODIMP nsRenderingContextMac::FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	SetupPortState();

	nscoord x,y,w,h;
	Rect    therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	mGS->mTMatrix.TransformCoord(&x,&y,&w,&h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::PaintOval(&therect);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
	return DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}



NS_IMETHODIMP nsRenderingContextMac::DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
	SetupPortState();

	nscoord x,y,w,h;
	Rect    therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	mGS->mTMatrix.TransformCoord(&x,&y,&w,&h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::FrameArc(&therect, (short)aStartAngle, (short)aEndAngle);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
	return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}



NS_IMETHODIMP nsRenderingContextMac::FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
	SetupPortState();

	nscoord x,y,w,h;
	Rect	therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	mGS->mTMatrix.TransformCoord(&x,&y,&w,&h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::PaintArc(&therect, (short)aStartAngle, (short)aEndAngle);

	return NS_OK;
}

PRInt32 nsRenderingContextMac::GetMaxStringLength()
{
  if (!mGS->mFontMetrics)
    return 1;
  return static_cast<nsFontMetricsMac*>(mGS->mFontMetrics)->GetMaxStringLength();
}

#pragma mark -


NS_IMETHODIMP nsRenderingContextMac::GetWidth(char ch, nscoord &aWidth)
{
	if (ch == ' ' && mGS->mFontMetrics) {
		return mGS->mFontMetrics->GetSpaceWidth(aWidth);
	}

	char buf[1];
	buf[0] = ch;
	return GetWidth(buf, 1, aWidth);
}



NS_IMETHODIMP nsRenderingContextMac::GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
	if (ch == ' ' && mGS->mFontMetrics) {
		return mGS->mFontMetrics->GetSpaceWidth(aWidth);
	}

	PRUnichar buf[1];
	buf[0] = ch;
	return GetWidth(buf, 1, aWidth, aFontID);
}



NS_IMETHODIMP
nsRenderingContextMac::GetWidthInternal(const char* aString, PRUint32 aLength, nscoord& aWidth)
{
	SetupPortState();

	
	SetPortTextState();



  
	
	short textWidth = ::TextWidth(aString, 0, aLength);
	aWidth = NSToCoordRound(float(textWidth) * mP2T);

	return NS_OK;
}



NS_IMETHODIMP nsRenderingContextMac::GetWidthInternal(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth, PRInt32 *aFontID)
{
	SetupPortState();
	
 	nsresult rv = SetPortTextState();
 	if (NS_FAILED(rv))
 		return rv;

  PRBool rtlText = mRightToLeftText;

	rv = mUnicodeRenderingToolkit.PrepareToDraw(mP2T, mContext, mGS, mPort, rtlText);
	if (NS_SUCCEEDED(rv))
    rv = mUnicodeRenderingToolkit.GetWidth(aString, aLength, aWidth, aFontID);
    
	return rv;
}



NS_IMETHODIMP
nsRenderingContextMac::GetTextDimensionsInternal(const char* aString, PRUint32 aLength,
                                                 nsTextDimensions& aDimensions)
{
  nsresult rv= GetWidth(aString, aLength, aDimensions.width);
  if (NS_SUCCEEDED(rv) && (mGS->mFontMetrics))
  {
    mGS->mFontMetrics->GetMaxAscent(aDimensions.ascent);
    mGS->mFontMetrics->GetMaxDescent(aDimensions.descent);
  }
  return rv;
}

NS_IMETHODIMP
nsRenderingContextMac::GetTextDimensionsInternal(const PRUnichar* aString, PRUint32 aLength,
                                                 nsTextDimensions& aDimensions, PRInt32* aFontID)
{
  SetupPortState();
  
  nsresult rv = SetPortTextState();
  if (NS_FAILED(rv))
    return rv;

  PRBool rtlText = mRightToLeftText;

  rv = mUnicodeRenderingToolkit.PrepareToDraw(mP2T, mContext, mGS, mPort, rtlText);
	if (NS_SUCCEEDED(rv))
    rv = mUnicodeRenderingToolkit.GetTextDimensions(aString, aLength, aDimensions, aFontID);
    
  return rv;
}

#pragma mark -


NS_IMETHODIMP nsRenderingContextMac::DrawStringInternal(const char *aString, PRUint32 aLength,
                                                        nscoord aX, nscoord aY,
                                                        const nscoord* aSpacing)
{
	SetupPortState();

	PRInt32 x = aX;
	PRInt32 y = aY;
	
	if (mGS->mFontMetrics) {
		
		SetPortTextState();
	}

	mGS->mTMatrix.TransformCoord(&x,&y);

	::MoveTo(x,y);
	if ( aSpacing == NULL )
		::DrawText(aString,0,aLength);
	else
	{
		int buffer[STACK_THRESHOLD];
		int* spacing = (aLength <= STACK_THRESHOLD ? buffer : new int[aLength]);
		if (spacing)
		{
			mGS->mTMatrix.ScaleXCoords(aSpacing, aLength, spacing);
			PRInt32 currentX = x;
			for (PRUint32 i = 0; i < aLength; i++)
			{
				::DrawChar(aString[i]);
				currentX += spacing[i];
				::MoveTo(currentX, y);
			}
			if (spacing != buffer)
				delete[] spacing;
		}
		else
			return NS_ERROR_OUT_OF_MEMORY;
	}

	return NS_OK;
}





NS_IMETHODIMP nsRenderingContextMac::DrawStringInternal(const PRUnichar *aString, PRUint32 aLength,
                                                        nscoord aX, nscoord aY, PRInt32 aFontID,
                                                        const nscoord* aSpacing)
{
	SetupPortState();

 	nsresult rv = SetPortTextState();
 	if (NS_FAILED(rv))
 		return rv;

	NS_PRECONDITION(mGS->mFontMetrics != nsnull, "No font metrics in SetPortTextState");
	
	if (nsnull == mGS->mFontMetrics)
		return NS_ERROR_NULL_POINTER;

  PRBool rtlText = mRightToLeftText;

	rv = mUnicodeRenderingToolkit.PrepareToDraw(mP2T, mContext, mGS, mPort, rtlText);
	if (NS_SUCCEEDED(rv))
		rv = mUnicodeRenderingToolkit.DrawString(aString, aLength, aX, aY, aFontID, aSpacing);

	return rv;        
}


#pragma mark -


NS_IMETHODIMP nsRenderingContextMac::InvertRect(const nsRect& aRect)
{
	return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextMac::InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	SetupPortState();

	nscoord x,y,w,h;
	Rect	therect;

	x = aX;
	y = aY;
	w = aWidth;
	h = aHeight;

	mGS->mTMatrix.TransformCoord(&x, &y, &w, &h);
	::SetRect(&therect, pinToShort(x), pinToShort(y), pinToShort(x + w), pinToShort(y + h));
	::InvertRect(&therect);

	return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextMac::FlushRect(const nsRect& aRect)
{
  return FlushRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextMac::FlushRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  return NS_OK;
}

#ifdef MOZ_MATHML

NS_IMETHODIMP
nsRenderingContextMac::GetBoundingMetricsInternal(const char*        aString, 
                                                  PRUint32           aLength,
                                                  nsBoundingMetrics& aBoundingMetrics)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRenderingContextMac::GetBoundingMetricsInternal(const PRUnichar*   aString, 
                                                  PRUint32           aLength,
                                                  nsBoundingMetrics& aBoundingMetrics,
                                                  PRInt32*           aFontID)
{
  SetupPortState();
  
  nsresult rv = SetPortTextState();
  if(NS_FAILED(rv))
    return rv;
  
  PRBool rtlText = mRightToLeftText;

  rv = mUnicodeRenderingToolkit.PrepareToDraw(mP2T, mContext, mGS, mPort, rtlText);
  if(NS_SUCCEEDED(rv))
    rv = mUnicodeRenderingToolkit.GetTextBoundingMetrics(aString, aLength, aBoundingMetrics, aFontID);
  
  return rv;
}


#endif 


NS_IMETHODIMP
nsRenderingContextMac::SetRightToLeftText(PRBool aIsRTL)
{
  mRightToLeftText = aIsRTL;
	return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextMac::GetRightToLeftText(PRBool* aIsRTL)
{
  *aIsRTL = mRightToLeftText;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextMac::DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect)
{
  SetupPortState();
  return nsRenderingContextImpl::DrawImage(aImage, aSrcRect, aDestRect);
}

NS_IMETHODIMP
nsRenderingContextMac::DrawTile(imgIContainer *aImage,
                    nscoord aXImageStart, nscoord aYImageStart,
                    const nsRect * aTargetRect)
{
  SetupPortState();
  return nsRenderingContextImpl::DrawTile(aImage, aXImageStart, aYImageStart, aTargetRect);
}

#pragma mark -

PRBool
nsRenderingContextMac::OnTigerOrLater()
{
  static PRBool sInitVer = PR_FALSE;
  static PRBool sOnTigerOrLater = PR_FALSE;
  if (!sInitVer) {
    long version;
    OSErr err = ::Gestalt(gestaltSystemVersion, &version);
    sOnTigerOrLater = ((err == noErr) && (version >= 0x00001040));
    sInitVer = PR_TRUE;
  }
  return sOnTigerOrLater;
}

void*
nsRenderingContextMac::GetNativeGraphicData(GraphicDataType aType)
{
  if (aType == NATIVE_MAC_THING) {
    return mPort;
  }

  return nsnull;
}
