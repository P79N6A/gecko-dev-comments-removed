






































#include "nsThebesRegion.h"

NS_IMPL_ISUPPORTS1(nsThebesRegion, nsIRegion)

nsThebesRegion::nsThebesRegion() 
{  
  NS_INIT_ISUPPORTS();
}

nsresult nsThebesRegion::Init (void)
{
  mRegion.SetEmpty();
  return NS_OK;
}

void nsThebesRegion::SetTo (const nsIRegion &aRegion)
{
  const nsThebesRegion* pRegion = NS_STATIC_CAST (const nsThebesRegion*, &aRegion);
  mRegion = pRegion->mRegion;
}

void nsThebesRegion::SetTo (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion = nsRect (aX, aY, aWidth, aHeight);
}

void nsThebesRegion::Intersect (const nsIRegion &aRegion)
{
  const nsThebesRegion* pRegion = NS_STATIC_CAST (const nsThebesRegion*, &aRegion);
  mRegion.And (mRegion, pRegion->mRegion);
}

void nsThebesRegion::Intersect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion.And (mRegion, nsRect (aX, aY, aWidth, aHeight));
}

void nsThebesRegion::Union (const nsIRegion &aRegion)
{
  const nsThebesRegion* pRegion = NS_STATIC_CAST (const nsThebesRegion*, &aRegion);
  mRegion.Or (mRegion, pRegion->mRegion);
}

void nsThebesRegion::Union (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion.Or (mRegion, nsRect (aX, aY, aWidth, aHeight));
}

void nsThebesRegion::Subtract (const nsIRegion &aRegion)
{
  const nsThebesRegion* pRegion = NS_STATIC_CAST (const nsThebesRegion*, &aRegion);
  mRegion.Sub (mRegion, pRegion->mRegion);
}

void nsThebesRegion::Subtract (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion.Sub (mRegion, nsRect (aX, aY, aWidth, aHeight));
}

PRBool nsThebesRegion::IsEmpty (void)
{
  return mRegion.IsEmpty ();
}

PRBool nsThebesRegion::IsEqual (const nsIRegion &aRegion)
{
  const nsThebesRegion* pRegion = NS_STATIC_CAST (const nsThebesRegion*, &aRegion);
  return mRegion.IsEqual (pRegion->mRegion);
}

void nsThebesRegion::GetBoundingBox (PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  nsRect BoundRect;
  BoundRect = mRegion.GetBounds();
  *aX = BoundRect.x;
  *aY = BoundRect.y;
  *aWidth  = BoundRect.width;
  *aHeight = BoundRect.height;
}

void nsThebesRegion::Offset (PRInt32 aXOffset, PRInt32 aYOffset)
{
  mRegion.MoveBy (aXOffset, aYOffset);
}

PRBool nsThebesRegion::ContainsRect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  nsRegion TmpRegion;
  TmpRegion.And (mRegion, nsRect (aX, aY, aWidth, aHeight));
  return (!TmpRegion.IsEmpty ());
}

NS_IMETHODIMP
nsThebesRegion::GetRects (nsRegionRectSet **aRects)
{
  if (!aRects)
    return NS_ERROR_NULL_POINTER;

  nsRegionRectSet* pRegionSet = *aRects;
  PRUint32 NumRects = mRegion.GetNumRects ();

  if (pRegionSet == nsnull)                 
  {
    PRUint8* pBuf = new PRUint8 [sizeof (nsRegionRectSet) + NumRects * sizeof (nsRegionRect)];
    pRegionSet = NS_REINTERPRET_CAST (nsRegionRectSet*, pBuf);
    pRegionSet->mRectsLen = NumRects + 1;
  } else                                    
  {
    if (NumRects > pRegionSet->mRectsLen)   
    {
      delete [] NS_REINTERPRET_CAST (PRUint8*, pRegionSet);
      PRUint8* pBuf = new PRUint8 [sizeof (nsRegionRectSet) + NumRects * sizeof (nsRegionRect)];
      pRegionSet = NS_REINTERPRET_CAST (nsRegionRectSet*, pBuf);
      pRegionSet->mRectsLen = NumRects + 1;
    }
  }
  pRegionSet->mNumRects = NumRects;
  *aRects = pRegionSet;


  nsRegionRectIterator ri (mRegion);
  nsRegionRect* pDest = &pRegionSet->mRects [0];
  const nsRect* pSrc;

  while ((pSrc = ri.Next ()))
  {
    pDest->x = pSrc->x;
    pDest->y = pSrc->y;
    pDest->width  = pSrc->width;
    pDest->height = pSrc->height;

    pDest++;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThebesRegion::FreeRects (nsRegionRectSet *aRects)
{
  if (!aRects)
    return NS_ERROR_NULL_POINTER;

  delete [] NS_REINTERPRET_CAST (PRUint8*, aRects);
  return NS_OK;
}

NS_IMETHODIMP
nsThebesRegion::GetNativeRegion (void *&aRegion) const
{
  aRegion = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsThebesRegion::GetRegionComplexity (nsRegionComplexity &aComplexity) const
{
  switch (mRegion.GetNumRects ())
  {
    case 0:   aComplexity = eRegionComplexity_empty;    break;
    case 1:   aComplexity = eRegionComplexity_rect;     break;
    default:  aComplexity = eRegionComplexity_complex;  break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThebesRegion::GetNumRects (PRUint32 *aRects) const
{
  *aRects = mRegion.GetNumRects ();
  return NS_OK;
}
