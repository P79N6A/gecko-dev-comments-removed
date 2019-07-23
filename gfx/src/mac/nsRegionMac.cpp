




































#include "nsRegionMac.h"
#include "prmem.h"
#include "nsCarbonHelpers.h"

#include "nsRegionPool.h"

static RegionToRectsUPP sAddRectToArrayProc;
static RegionToRectsUPP sCountRectProc;

static OSStatus
AddRectToArrayProc(UInt16 message, RgnHandle rgn, const Rect *inRect, void *inArray)
{
  if (message == kQDRegionToRectsMsgParse) {
    nsRegionRectSet* rects = reinterpret_cast<nsRegionRectSet*>(inArray);
    nsRegionRect* rect = &rects->mRects[rects->mNumRects++];
    rect->x = inRect->left;
    rect->y = inRect->top;
    rect->width = inRect->right - inRect->left;
    rect->height = inRect->bottom - inRect->top;
    rects->mArea += rect->width * rect->height;
  }

  return noErr;
}

static OSStatus
CountRectProc(UInt16 message, RgnHandle rgn, const Rect* inRect, void* rectCount)
{
  if (message == kQDRegionToRectsMsgParse)
    ++(*reinterpret_cast<long*>(rectCount));

  return noErr;
}



nsRegionMac::nsRegionMac()
{
	mRegion = nsnull;
	mRegionType = eRegionComplexity_empty;
  if (!sAddRectToArrayProc) {
    sAddRectToArrayProc = NewRegionToRectsUPP(AddRectToArrayProc);
    sCountRectProc = NewRegionToRectsUPP(CountRectProc);
  }
}



nsRegionMac::~nsRegionMac()
{
	if (mRegion != nsnull) {
		sNativeRegionPool.ReleaseRegion(mRegion);
		mRegion = nsnull;
	}
}

NS_IMPL_ISUPPORTS1(nsRegionMac, nsIRegion)



nsresult nsRegionMac::Init(void)
{
	if (mRegion != nsnull)
		::SetEmptyRgn(mRegion);
	else
		mRegion = sNativeRegionPool.GetNewRegion();
	if (mRegion != nsnull) {
		mRegionType = eRegionComplexity_empty;
		return NS_OK;
	}
	return NS_ERROR_OUT_OF_MEMORY;
}



void nsRegionMac::SetTo(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	::CopyRgn(pRegion->mRegion, mRegion);
	SetRegionType();
}



void nsRegionMac::SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	::SetRectRgn(mRegion, aX, aY, aX + aWidth, aY + aHeight);
	SetRegionType();
}



void nsRegionMac::Intersect(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	::SectRgn(mRegion, pRegion->mRegion, mRegion);
	SetRegionType();
}



void nsRegionMac::Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = sNativeRegionPool.GetNewRegion();
	if (rectRgn != nsnull) {
		::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
		::SectRgn(mRegion, rectRgn, mRegion);
		sNativeRegionPool.ReleaseRegion(rectRgn);
		SetRegionType();
	}
}



void nsRegionMac::Union(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	::UnionRgn(mRegion, pRegion->mRegion, mRegion);
	SetRegionType();
}



void nsRegionMac::Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = sNativeRegionPool.GetNewRegion();
	if (rectRgn != nsnull) {
		::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
		::UnionRgn(mRegion, rectRgn, mRegion);
		sNativeRegionPool.ReleaseRegion(rectRgn);
		SetRegionType();
	}
}



void nsRegionMac::Subtract(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	::DiffRgn(mRegion, pRegion->mRegion, mRegion);
	SetRegionType();
}



void nsRegionMac::Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = sNativeRegionPool.GetNewRegion();
	if (rectRgn != nsnull) {
		::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
		::DiffRgn(mRegion, rectRgn, mRegion);
		sNativeRegionPool.ReleaseRegion(rectRgn);
		SetRegionType();
	}
}



PRBool nsRegionMac::IsEmpty(void)
{
	if (mRegionType == eRegionComplexity_empty)
    	return PR_TRUE;
	else
		return PR_FALSE;
}



PRBool nsRegionMac::IsEqual(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	return(::EqualRgn(mRegion, pRegion->mRegion));
}



void nsRegionMac::GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
	Rect macRect;
	::GetRegionBounds (mRegion, &macRect);

	*aX = macRect.left;
	*aY = macRect.top;
	*aWidth  = macRect.right - macRect.left;
	*aHeight = macRect.bottom - macRect.top;
}



void nsRegionMac::Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
	::OffsetRgn(mRegion, aXOffset, aYOffset);
}



PRBool nsRegionMac::ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	Rect macRect;
	::SetRect(&macRect, aX, aY, aX + aWidth, aY + aHeight);
	return(::RectInRgn(&macRect, mRegion));
}



NS_IMETHODIMP nsRegionMac::GetRects(nsRegionRectSet **aRects)
{
  NS_ASSERTION(aRects, "bad ptr");

  nsRegionRectSet* rects = *aRects;
  long numrects = 0;
  QDRegionToRects(mRegion, kQDParseRegionFromTopLeft, sCountRectProc, &numrects);

  if (!rects || rects->mRectsLen < (PRUint32) numrects) {
    void* buf = PR_Realloc(rects, sizeof(nsRegionRectSet) + sizeof(nsRegionRect) * (numrects - 1));

    if (!buf) {
      if (rects)
        rects->mNumRects = 0;

      return NS_ERROR_OUT_OF_MEMORY;
    }

    rects = (nsRegionRectSet*) buf;
    rects->mRectsLen = numrects;
  }

  rects->mNumRects = 0;
  rects->mArea = 0;
  QDRegionToRects(mRegion, kQDParseRegionFromTopLeft, sAddRectToArrayProc, rects);

  *aRects = rects;

  return NS_OK;
}



NS_IMETHODIMP nsRegionMac::FreeRects(nsRegionRectSet *aRects)
{
	if (nsnull != aRects)
		PR_Free((void *)aRects);

	return NS_OK;
}




NS_IMETHODIMP nsRegionMac::GetNativeRegion(void *&aRegion) const
{
	aRegion = (void *)mRegion;
	return NS_OK;
}


nsresult nsRegionMac::SetNativeRegion(void *aRegion)
{
	if (aRegion) {
		::CopyRgn((RgnHandle)aRegion, mRegion);
    	SetRegionType();
	} else {
		Init();
  	}
	return NS_OK;
}



NS_IMETHODIMP nsRegionMac::GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
	aComplexity = mRegionType;
	return NS_OK;
}




void nsRegionMac::SetRegionType()
{
	if (::EmptyRgn(mRegion) == PR_TRUE)
    	mRegionType = eRegionComplexity_empty;
	else
	if ( ::IsRegionRectangular(mRegion) )
		mRegionType = eRegionComplexity_rect;
	else
		mRegionType = eRegionComplexity_complex;
}





void nsRegionMac::SetRegionEmpty()
{
	::SetEmptyRgn(mRegion);
	SetRegionType();
}



RgnHandle nsRegionMac::CreateRectRegion(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = sNativeRegionPool.GetNewRegion();
	if (rectRgn != nsnull)
		::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
	return rectRgn;
}
