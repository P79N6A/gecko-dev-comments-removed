




































#include "nsGraphicState.h"
#include "nsDrawingSurfaceMac.h"
#include "nsRegionMac.h"
#include "nsIFontMetrics.h"
#include "nsCarbonHelpers.h"
#include "nsRegionPool.h"

nsGraphicStatePool sGraphicStatePool;




nsGraphicStatePool::nsGraphicStatePool()
{
	mFreeList = nsnull;
}



nsGraphicStatePool::~nsGraphicStatePool()
{
	nsGraphicState* gs = mFreeList;
	while (gs != nsnull) {
		nsGraphicState* next = gs->mNext;
		delete gs;
		gs = next;
	}
}



nsGraphicState* nsGraphicStatePool::GetNewGS()
{
	nsGraphicState* gs = mFreeList;
	if (gs != nsnull) {
		mFreeList = gs->mNext;
		return gs;
	}
	return new nsGraphicState;
}



void nsGraphicStatePool::ReleaseGS(nsGraphicState* aGS)
{
	
	
	
	aGS->Clear();
	aGS->mNext = mFreeList;
	mFreeList = aGS;
}


#pragma mark -


nsGraphicState::nsGraphicState()
{
	
}



nsGraphicState::~nsGraphicState()
{
	Clear();
}



void nsGraphicState::Clear()
{
	mTMatrix.SetToIdentity();

	if (mMainRegion) {
		sNativeRegionPool.ReleaseRegion(mMainRegion); 
		mMainRegion = nsnull;
	}

	if (mClipRegion) {
		sNativeRegionPool.ReleaseRegion(mClipRegion); 
		mClipRegion = nsnull;
	}

  NS_IF_RELEASE(mFontMetrics);

  mOffx						= 0;
  mOffy						= 0;
  mColor 					= NS_RGB(255,255,255);
	mFont						= 0;
  mFontMetrics		= nsnull;
  mCurrFontHandle	= 0;
  mLineStyle = nsLineStyle_kSolid;
}



void nsGraphicState::Init(nsIDrawingSurface* aSurface)
{
	
	nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);
	CGrafPtr port;
	surface->GetGrafPtr(&port);

	
	Init(port);
}



void nsGraphicState::Init(CGrafPtr aPort)
{
	
	Clear();

	
	RgnHandle	rgn = sNativeRegionPool.GetNewRegion(); 
	if ( rgn ) {
		Rect bounds;
		::RectRgn(rgn, ::GetPortBounds(aPort, &bounds));
	}

  Rect    portBounds;
  ::GetPortBounds(aPort, &portBounds);
  mOffx = -portBounds.left;
  mOffy = -portBounds.top;
  
  mMainRegion					= rgn;
  mClipRegion					= DuplicateRgn(rgn);
}



void nsGraphicState::Init(nsIWidget* aWindow)
{
	
	Clear();

	
  mOffx						= (PRInt32)aWindow->GetNativeData(NS_NATIVE_OFFSETX);
  mOffy						= (PRInt32)aWindow->GetNativeData(NS_NATIVE_OFFSETY);

	RgnHandle widgetRgn = (RgnHandle)aWindow->GetNativeData(NS_NATIVE_REGION);
	mMainRegion					= DuplicateRgn(widgetRgn);
  mClipRegion					= DuplicateRgn(widgetRgn);
}



void nsGraphicState::Duplicate(nsGraphicState* aGS)
{
	
	

	
	mTMatrix.SetMatrix(&aGS->mTMatrix);

	mOffx						= aGS->mOffx;
	mOffy						= aGS->mOffy;

	mMainRegion			= DuplicateRgn(aGS->mMainRegion, mMainRegion);
	mClipRegion			= DuplicateRgn(aGS->mClipRegion, mClipRegion);

	mColor					= aGS->mColor;
	mFont						= aGS->mFont;
	
	NS_IF_RELEASE(mFontMetrics);
	mFontMetrics		= aGS->mFontMetrics;
	NS_IF_ADDREF(mFontMetrics);

	mCurrFontHandle	= aGS->mCurrFontHandle;

	mLineStyle = aGS->mLineStyle;
	
	mChanges				= aGS->mChanges;
}



RgnHandle nsGraphicState::DuplicateRgn(RgnHandle aRgn, RgnHandle aDestRgn)
{
	RgnHandle dupRgn = aDestRgn;
	if (aRgn)	{
		if (nsnull == dupRgn)
			dupRgn = sNativeRegionPool.GetNewRegion(); 
		if (dupRgn)
			::CopyRgn(aRgn, dupRgn);
	}
	return dupRgn;
}
