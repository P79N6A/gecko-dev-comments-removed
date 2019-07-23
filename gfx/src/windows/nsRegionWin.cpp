




































#include "nsRegionWin.h"
#include "prmem.h"

nsRegionWin :: nsRegionWin()
{
  mRegion = NULL;
  mRegionType = NULLREGION;
  mData = NULL;
  mDataSize = 0;
}

nsRegionWin :: ~nsRegionWin()
{
  if (NULL != mRegion)
  {
    ::DeleteObject(mRegion);
    mRegion = NULL;
  }

  if (NULL != mData)
  {
    PR_Free(mData);

    mData = NULL;
    mDataSize = 0;
  }
}

NS_IMPL_ISUPPORTS1(nsRegionWin, nsIRegion)

nsresult nsRegionWin :: Init(void)
{
  if (NULL != mRegion) {
    ::SetRectRgn(mRegion, 0, 0, 0, 0);
    FreeRects(nsnull);
  }
  else {
    mRegion = ::CreateRectRgn(0, 0, 0, 0);
  }
  
  mRegionType = NULLREGION;

  return NS_OK;
}

void nsRegionWin :: SetTo(const nsIRegion &aRegion)
{
  nsRegionWin *pRegion = (nsRegionWin *)&aRegion;

  mRegionType = ::CombineRgn(mRegion, pRegion->mRegion, NULL, RGN_COPY);
}

void nsRegionWin :: SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  if (NULL != mRegion)
    ::SetRectRgn(mRegion, aX, aY, aX + aWidth, aY + aHeight);
  else
    mRegion = ::CreateRectRgn(aX, aY, aX + aWidth, aY + aHeight);

  if ((aWidth == 0) || (aHeight == 0))
    mRegionType = NULLREGION;
  else
    mRegionType = SIMPLEREGION;
}

void nsRegionWin :: Intersect(const nsIRegion &aRegion)
{
  nsRegionWin *pRegion = (nsRegionWin *)&aRegion;

  mRegionType = ::CombineRgn(mRegion, mRegion, pRegion->mRegion, RGN_AND);
}

void nsRegionWin :: Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  HRGN tRegion;

  tRegion = ::CreateRectRgn(aX, aY, aX + aWidth, aY + aHeight);
  mRegionType = ::CombineRgn(mRegion, mRegion, tRegion, RGN_AND);

  ::DeleteObject(tRegion);
}

void nsRegionWin :: Union(const nsIRegion &aRegion)
{
  nsRegionWin *pRegion = (nsRegionWin *)&aRegion;

  mRegionType = ::CombineRgn(mRegion, mRegion, pRegion->mRegion, RGN_OR);
}

void nsRegionWin :: Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  HRGN tRegion;

  tRegion = ::CreateRectRgn(aX, aY, aX + aWidth, aY + aHeight);
  mRegionType = ::CombineRgn(mRegion, mRegion, tRegion, RGN_OR);

  ::DeleteObject(tRegion);
}

void nsRegionWin :: Subtract(const nsIRegion &aRegion)
{
  nsRegionWin *pRegion = (nsRegionWin *)&aRegion;

  mRegionType = ::CombineRgn(mRegion, mRegion, pRegion->mRegion, RGN_DIFF);
}

void nsRegionWin :: Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  HRGN tRegion;

  tRegion = ::CreateRectRgn(aX, aY, aX + aWidth, aY + aHeight);
  mRegionType = ::CombineRgn(mRegion, mRegion, tRegion, RGN_DIFF);

  ::DeleteObject(tRegion);
}

PRBool nsRegionWin :: IsEmpty(void)
{
  return (mRegionType == NULLREGION) ? PR_TRUE : PR_FALSE;
}

PRBool nsRegionWin :: IsEqual(const nsIRegion &aRegion)
{
  nsRegionWin *pRegion = (nsRegionWin *)&aRegion;

  return ::EqualRgn(mRegion, pRegion->mRegion) ? PR_TRUE : PR_FALSE;
}

void nsRegionWin :: GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  RECT  bounds;

  if (mRegionType != NULLREGION)
  {
    ::GetRgnBox(mRegion, &bounds);

    *aX = bounds.left;
    *aY = bounds.top;
    *aWidth = bounds.right - bounds.left;
    *aHeight = bounds.bottom - bounds.top;
  }
  else
    *aX = *aY = *aWidth = *aHeight = 0;
}

void nsRegionWin :: Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
  ::OffsetRgn(mRegion, aXOffset, aYOffset);
}

PRBool nsRegionWin :: ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  RECT  trect;

  trect.left = aX;
  trect.top = aY;
  trect.right = aX + aWidth;
  trect.bottom = aY + aHeight;

  return ::RectInRegion(mRegion, &trect) ? PR_TRUE : PR_FALSE;
}

NS_IMETHODIMP nsRegionWin :: GetRects(nsRegionRectSet **aRects)
{
  nsRegionRectSet *rects;
  nsRegionRect    *rect;
	LPRECT          pRects;
	DWORD           dwCount, dwResult;
	unsigned int    num_rects;

  NS_ASSERTION(!(nsnull == aRects), "bad ptr");

  rects = *aRects;

  if (nsnull != rects)
    rects->mNumRects = 0;

  

	
	dwCount = GetRegionData(mRegion, 0, NULL);

	NS_ASSERTION(!(dwCount == 0), "bad region");

	if (dwCount == 0)
	  return NS_OK;

  if (dwCount > mDataSize)
  {
    if (NULL != mData)
      PR_Free(mData);

	  mData = (LPRGNDATA)PR_Malloc(dwCount);
  }

	NS_ASSERTION(!(nsnull == mData), "failed allocation");

	if (mData == NULL)
	  return NS_OK;

	dwResult = GetRegionData(mRegion, dwCount, mData);

	NS_ASSERTION(!(dwResult == 0), "get data failed");

	if (dwResult == 0)
		return NS_OK;

  if ((nsnull == rects) || (rects->mRectsLen < mData->rdh.nCount))
  {
    void *buf = PR_Realloc(rects, sizeof(nsRegionRectSet) + (sizeof(nsRegionRect) * (mData->rdh.nCount - 1)));

    if (nsnull == buf)
    {
      if (nsnull != rects)
        rects->mNumRects = 0;

      return NS_OK;
    }

    rects = (nsRegionRectSet *)buf;
    rects->mRectsLen = mData->rdh.nCount;
  }

  rects->mNumRects = mData->rdh.nCount;
  rects->mArea = 0;
  rect = &rects->mRects[0];

  for (pRects = (LPRECT)mData->Buffer, num_rects = 0; 
		   num_rects < mData->rdh.nCount; 
		   num_rects++, pRects++, rect++)
  {
		rect->x = pRects->left;
		rect->y = pRects->top;
		rect->width = pRects->right - rect->x;
		rect->height = pRects->bottom - rect->y;

    rects->mArea += rect->width * rect->height;
	}

  *aRects = rects;

  return NS_OK;
}

NS_IMETHODIMP nsRegionWin :: FreeRects(nsRegionRectSet *aRects)
{
  if (nsnull != aRects)
    PR_Free((void *)aRects);

  if (NULL != mData)
  {
    PR_Free(mData);

    mData = NULL;
    mDataSize = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP nsRegionWin :: GetNativeRegion(void *&aRegion) const
{
  aRegion = (void *)mRegion;
  return NS_OK;
}

NS_IMETHODIMP nsRegionWin :: GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
  switch (mRegionType)
  {
    case NULLREGION:
      aComplexity = eRegionComplexity_empty;
      break;

    case SIMPLEREGION:
      aComplexity = eRegionComplexity_rect;
      break;

    default:
    case COMPLEXREGION:
      aComplexity = eRegionComplexity_complex;
      break;
  }

  return NS_OK;
}
