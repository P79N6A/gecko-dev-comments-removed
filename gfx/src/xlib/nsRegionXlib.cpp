





































#include "prmem.h"
#include "nsRegionXlib.h"
#include "xregion.h"



#ifdef DEBUG_REGIONS
static int nRegions;
#endif

Region nsRegionXlib::copyRegion = 0;


nsRegionXlib::nsRegionXlib()
{
#ifdef DEBUG_REGIONS
    ++nRegions;
      printf("REGIONS+ = %i\n", nRegions);
#endif

  mRegion = nsnull;
}

nsRegionXlib::~nsRegionXlib()
{
#ifdef DEBUG_REGIONS
  --nRegions;
  printf("REGIONS- = %i\n", nRegions);
#endif
  
  if (mRegion)
    ::XDestroyRegion(mRegion);
  mRegion = nsnull;
}

NS_IMPL_ISUPPORTS1(nsRegionXlib, nsIRegion)

Region
nsRegionXlib::GetCopyRegion()
{
  if (!copyRegion)
    copyRegion = ::XCreateRegion();
  return copyRegion;
}

Region
nsRegionXlib::xlib_region_copy(Region region)
{
  Region nRegion;
  nRegion = XCreateRegion();

  XUnionRegion(region, GetCopyRegion(), nRegion);

  return nRegion;
}

Region nsRegionXlib::xlib_region_from_rect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  XRectangle rect;
  Region nRegion;

  rect.x = aX;
  rect.y = aY;
  rect.width = aWidth;
  rect.height = aHeight;

  nRegion = XCreateRegion();

  XUnionRectWithRegion(&rect, GetCopyRegion(), nRegion);

  return nRegion;
}

nsresult
nsRegionXlib::Init()
{
  if (mRegion) {
    ::XDestroyRegion(mRegion);
    mRegion = nsnull;
  }

  return NS_OK;
}

void
nsRegionXlib::SetTo(const nsIRegion &aRegion)
{
  Init();
  nsRegionXlib * pRegion = (nsRegionXlib *)&aRegion;

  mRegion = xlib_region_copy(pRegion->mRegion);
}

void
nsRegionXlib::SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  Init();
  mRegion = xlib_region_from_rect(aX, aY, aWidth, aHeight);
}

void
nsRegionXlib::Intersect(const nsIRegion &aRegion)
{
  nsRegionXlib * pRegion = (nsRegionXlib *)&aRegion;
  
  Region nRegion = XCreateRegion();
  ::XIntersectRegion(mRegion, pRegion->mRegion, nRegion);
  ::XDestroyRegion(mRegion);
  mRegion = nRegion;
}

void
nsRegionXlib::Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  Region tRegion = xlib_region_from_rect(aX, aY, aWidth, aHeight);
  
  Region nRegion = XCreateRegion();

  ::XIntersectRegion(mRegion, tRegion, nRegion);
  ::XDestroyRegion(tRegion);
  ::XDestroyRegion(mRegion);
  mRegion = nRegion;
}

void
nsRegionXlib::Union(const nsIRegion &aRegion)
{
  nsRegionXlib * pRegion = (nsRegionXlib *)&aRegion;

  if (pRegion->mRegion && !::XEmptyRegion(pRegion->mRegion)) {
    if (mRegion) {
      if (::XEmptyRegion(mRegion)) {
        ::XDestroyRegion(mRegion);
        mRegion = xlib_region_copy(pRegion->mRegion);
      } else {
        Region nRegion = ::XCreateRegion();
        ::XUnionRegion(mRegion, pRegion->mRegion, nRegion);
        ::XDestroyRegion(mRegion);
        mRegion = nRegion;
      }
    } else
      mRegion = xlib_region_copy(pRegion->mRegion);
  }
}

void
nsRegionXlib::Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  if (mRegion) {
    XRectangle rect;
    
    rect.x = aX;
    rect.y = aY;
    rect.width = aWidth;
    rect.height = aHeight;

    if (rect.width > 0 && rect.height > 0) {
      if (::XEmptyRegion(mRegion)) {
        ::XDestroyRegion(mRegion);
        mRegion = xlib_region_from_rect(aX, aY, aWidth, aHeight);
      } else {
        Region nRegion = ::XCreateRegion();
        ::XUnionRectWithRegion(&rect, mRegion, nRegion);
        ::XDestroyRegion(mRegion);
        mRegion = nRegion;
      }
    }
  } else {
    mRegion = xlib_region_from_rect(aX, aY, aWidth, aHeight);
  }
}

void
nsRegionXlib::Subtract(const nsIRegion &aRegion)
{
#ifdef DEBUG_REGIONS
  printf("nsRegionXlib::Subtract ");
#endif
  nsRegionXlib * pRegion = (nsRegionXlib *)&aRegion;

  if (pRegion->mRegion) {
    if (mRegion) {
#ifdef DEBUG_REGIONS
    printf("-");
#endif
      Region nRegion = ::XCreateRegion();
      ::XSubtractRegion(mRegion, pRegion->mRegion, nRegion);
      ::XDestroyRegion(mRegion);
      mRegion = nRegion;
    } else {
#ifdef DEBUG_REGIONS
    printf("+");
#endif
      mRegion = ::XCreateRegion();
      ::XSubtractRegion(GetCopyRegion(), pRegion->mRegion, mRegion);
    }
  }
#ifdef DEBUG_REGIONS
    printf("\n");
#endif
}

void
nsRegionXlib::Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  if (mRegion) {
    Region tRegion = xlib_region_from_rect(aX, aY, aWidth, aHeight);

    Region nRegion = ::XCreateRegion();
    ::XSubtractRegion(mRegion, tRegion, nRegion);
    ::XDestroyRegion(mRegion);
    ::XDestroyRegion(tRegion);
    mRegion = nRegion;
  } else {
    Region tRegion = xlib_region_from_rect(aX, aY, aWidth, aHeight);
    mRegion = XCreateRegion();
    ::XSubtractRegion(GetCopyRegion(), tRegion, mRegion);
    ::XDestroyRegion(tRegion);
  }
}

PRBool
nsRegionXlib::IsEmpty(void)
{
  if (!mRegion)
    return PR_TRUE;
  return ::XEmptyRegion(mRegion);
}

PRBool
nsRegionXlib::IsEqual(const nsIRegion &aRegion)
{
  nsRegionXlib *pRegion = (nsRegionXlib *)&aRegion;

  if (mRegion && pRegion->mRegion) {
    return ::XEqualRegion(mRegion, pRegion->mRegion);
  } else if (!mRegion && !pRegion->mRegion) {
    return PR_TRUE;
  } else if ((mRegion && !pRegion->mRegion) ||
      (!mRegion && pRegion->mRegion)) {
    return PR_FALSE;
  }

  return PR_FALSE;
}

void
nsRegionXlib::GetBoundingBox(PRInt32 *aX, PRInt32 *aY,
                             PRInt32 *aWidth, PRInt32 *aHeight)
{
  if (mRegion) {
    XRectangle r;

    ::XClipBox(mRegion, &r);

    *aX = r.x;
    *aY = r.y;
    *aWidth = r.width;
    *aHeight = r.height;
  } else {
    *aX = 0;
    *aY = 0;
    *aWidth = 0;
    *aHeight = 0;
  }
}

void
nsRegionXlib::Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
  if (mRegion) {
    ::XOffsetRegion(mRegion, aXOffset, aYOffset);
  }
}

PRBool
nsRegionXlib::ContainsRect(PRInt32 aX, PRInt32 aY,
                           PRInt32 aWidth, PRInt32 aHeight)
{
  return (::XRectInRegion(mRegion, aX, aY, aWidth, aHeight) == RectangleIn) ?
    PR_TRUE : PR_FALSE;
}

NS_IMETHODIMP
nsRegionXlib::GetRects(nsRegionRectSet **aRects)
{
  *aRects = nsnull;

  if (!mRegion)
    return NS_OK;

  nsRegionRectSet   *rects;
  int               nbox;
  BOX               *pbox;
  nsRegionRect      *rect;

  NS_ASSERTION(!(nsnull == aRects), "bad ptr");
 
  

  pbox = mRegion->rects;
  nbox = mRegion->numRects;
 
  rects = *aRects;

  if ((nsnull == rects) || (rects->mRectsLen < (PRUint32)nbox)) {
    void *buf = PR_Realloc(rects, sizeof(nsRegionRectSet) +
                           (sizeof(nsRegionRect) * (nbox - 1)));

    if (nsnull == buf) {
      if (nsnull != rects)
        rects->mNumRects = 0;
 
      return NS_OK;
    }

    rects = (nsRegionRectSet *)buf;
    rects->mRectsLen = nbox;
  }

  rects->mNumRects = nbox;
  rects->mArea = 0;
  rect = &rects->mRects[0];

  while (nbox--) {
    rect->x = pbox->x1;
    rect->width = (pbox->x2 - pbox->x1);
    rect->y = pbox->y1;
    rect->height = (pbox->y2 - pbox->y1);

    rects->mArea += rect->width * rect->height;

    pbox++;
    rect++;
  }

  *aRects = rects;

  return NS_OK;
}

NS_IMETHODIMP
nsRegionXlib::FreeRects(nsRegionRectSet *aRects)
{
  if (nsnull != aRects)
    PR_Free((void *)aRects);

  return NS_OK;
}

NS_IMETHODIMP
nsRegionXlib::GetNativeRegion(void *&aRegion) const
{
  aRegion = (void *)mRegion;
  return NS_OK;
}

NS_IMETHODIMP
nsRegionXlib::GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
  
  if (((nsRegionXlib*)this)->IsEmpty())
    aComplexity = eRegionComplexity_empty;
  else 
    aComplexity = eRegionComplexity_rect;
      
  return NS_OK;
}

void nsRegionXlib::SetRegionEmpty()
{
  if (!IsEmpty()) { 
    ::XDestroyRegion(mRegion);
  }
}

NS_IMETHODIMP
nsRegionXlib::GetNumRects(PRUint32 *aRects) const
{
  if (!mRegion)
    *aRects = 0;

  *aRects = mRegion->numRects;

  return NS_OK;
}
