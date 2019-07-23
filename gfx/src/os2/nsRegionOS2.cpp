



































#include "nsRegionOS2.h"
#include "nsGfxDefs.h"

nsRegionOS2::nsRegionOS2() 
{  
}

NS_IMPL_ISUPPORTS1(nsRegionOS2, nsIRegion)

PRUint32 nsRegionOS2::NumOfRects (HPS aPS, HRGN aRegion) const
{
  RGNRECT rgnRect;
  rgnRect.ircStart = 1;
  rgnRect.crc = 0xFFFFFFFF;
  rgnRect.crcReturned = 0;
  rgnRect.ulDirection = RECTDIR_LFRT_TOPBOT;

  GFX (::GpiQueryRegionRects (aPS, aRegion, NULL, &rgnRect, NULL), FALSE);
 
  return rgnRect.crcReturned;
}

HRGN nsRegionOS2::GetHRGN (PRUint32 DestHeight, HPS DestPS)
{
  PRUint32 NumRects = mRegion.GetNumRects ();

  if (NumRects > 0)
  {
    PRECTL pRects = new RECTL [NumRects];

    nsRegionRectIterator ri (mRegion);
    const nsRect* pSrc;
    PRECTL pDest = pRects;

    while ((pSrc = ri.Next()))
    {
      pDest->xLeft    = pSrc->x;
      pDest->xRight   = pSrc->XMost ();
      pDest->yTop     = DestHeight - pSrc->y;
      pDest->yBottom  = pDest->yTop - pSrc->height;
      pDest++;
    }

    HRGN rgn = GFX (::GpiCreateRegion (DestPS, NumRects, pRects), RGN_ERROR);
    delete [] pRects;

    return rgn;
  } else
  {
    return GFX (::GpiCreateRegion (DestPS, 0, NULL), RGN_ERROR);
  }
}


nsresult nsRegionOS2::InitWithHRGN (HRGN SrcRegion, PRUint32 SrcHeight, HPS SrcPS)
{
  PRUint32 NumRects = NumOfRects (SrcPS, SrcRegion);
  mRegion.SetEmpty ();

  if (NumRects > 0)
  {
    RGNRECT RgnControl = { 1, NumRects, 0, RECTDIR_LFRT_TOPBOT };
    PRECTL  pRects = new RECTL [NumRects];

    GFX (::GpiQueryRegionRects (SrcPS, SrcRegion, NULL, &RgnControl, pRects), FALSE);

    for (PRUint32 cnt = 0 ; cnt < NumRects ; cnt++)
      mRegion.Or (mRegion, nsRect ( pRects [cnt].xLeft, SrcHeight - pRects [cnt].yTop, 
                  pRects [cnt].xRight - pRects [cnt].xLeft, pRects [cnt].yTop - pRects [cnt].yBottom));

    delete [] pRects;
  }

  return NS_OK;
}

nsresult nsRegionOS2::Init (void)
{
  mRegion.SetEmpty ();
  return NS_OK;
}

void nsRegionOS2::SetTo (const nsIRegion &aRegion)
{
  const nsRegionOS2* pRegion = static_cast<const nsRegionOS2*>(&aRegion);
  mRegion = pRegion->mRegion;
}

void nsRegionOS2::SetTo (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion = nsRect (aX, aY, aWidth, aHeight);
}

void nsRegionOS2::Intersect (const nsIRegion &aRegion)
{
  const nsRegionOS2* pRegion = static_cast<const nsRegionOS2*>(&aRegion);
  mRegion.And (mRegion, pRegion->mRegion);
}

void nsRegionOS2::Intersect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion.And (mRegion, nsRect (aX, aY, aWidth, aHeight));
}

void nsRegionOS2::Union (const nsIRegion &aRegion)
{
  const nsRegionOS2* pRegion = static_cast<const nsRegionOS2*>(&aRegion);
  mRegion.Or (mRegion, pRegion->mRegion);
}

void nsRegionOS2::Union (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion.Or (mRegion, nsRect (aX, aY, aWidth, aHeight));
}

void nsRegionOS2::Subtract (const nsIRegion &aRegion)
{
  const nsRegionOS2* pRegion = static_cast<const nsRegionOS2*>(&aRegion);
  mRegion.Sub (mRegion, pRegion->mRegion);
}

void nsRegionOS2::Subtract (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  mRegion.Sub (mRegion, nsRect (aX, aY, aWidth, aHeight));
}

PRBool nsRegionOS2::IsEmpty (void)
{
  return mRegion.IsEmpty ();
}

PRBool nsRegionOS2::IsEqual (const nsIRegion &aRegion)
{
  const nsRegionOS2* pRegion = static_cast<const nsRegionOS2*>(&aRegion);
  return mRegion.IsEqual (pRegion->mRegion);
}

void nsRegionOS2::GetBoundingBox (PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  const nsRect& BoundRect = mRegion.GetBounds();
  *aX = BoundRect.x;
  *aY = BoundRect.y;
  *aWidth  = BoundRect.width;
  *aHeight = BoundRect.height;
}

void nsRegionOS2::Offset (PRInt32 aXOffset, PRInt32 aYOffset)
{
  mRegion.MoveBy (aXOffset, aYOffset);
}

PRBool nsRegionOS2::ContainsRect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  nsRegion TmpRegion;
  TmpRegion.And (mRegion, nsRect (aX, aY, aWidth, aHeight));
  return (!TmpRegion.IsEmpty ());
}

nsresult nsRegionOS2::GetRects (nsRegionRectSet **aRects)
{
  if (!aRects)
    return NS_ERROR_NULL_POINTER;

  nsRegionRectSet* pRegionSet = *aRects;
  PRUint32 NumRects = mRegion.GetNumRects ();

  if (!pRegionSet)                          
  {
    PRUint8* pBuf = new PRUint8 [sizeof (nsRegionRectSet) + NumRects * sizeof (nsRegionRect)];
    pRegionSet = reinterpret_cast<nsRegionRectSet*>(pBuf);
    pRegionSet->mRectsLen = NumRects + 1;
  } else                                    
  {
    if (NumRects > pRegionSet->mRectsLen)   
    {
      delete [] reinterpret_cast<PRUint8*>(pRegionSet);
      PRUint8* pBuf = new PRUint8 [sizeof (nsRegionRectSet) + NumRects * sizeof (nsRegionRect)];
      pRegionSet = reinterpret_cast<nsRegionRectSet*>(pBuf);
      pRegionSet->mRectsLen = NumRects + 1;
    }
  }
  pRegionSet->mNumRects = NumRects;
  *aRects = pRegionSet;


  nsRegionRectIterator ri (mRegion);
  nsRegionRect* pDest = &pRegionSet->mRects [0];
  const nsRect* pSrc;

  while ((pSrc = ri.Next ()) != nsnull)
  {
    pDest->x = pSrc->x;
    pDest->y = pSrc->y;
    pDest->width  = pSrc->width;
    pDest->height = pSrc->height;

    ++pDest;
  }

  return NS_OK;
}

nsresult nsRegionOS2::FreeRects (nsRegionRectSet *aRects)
{
  if (!aRects)
    return NS_ERROR_NULL_POINTER;

  delete [] reinterpret_cast<PRUint8*>(aRects);
  return NS_OK;
}

nsresult nsRegionOS2::GetNativeRegion (void *&aRegion) const
{
  aRegion = RGN_ERROR;
  return NS_OK;
}

nsresult nsRegionOS2::GetRegionComplexity (nsRegionComplexity &aComplexity) const
{
  switch (mRegion.GetNumRects ())
  {
    case 0:   aComplexity = eRegionComplexity_empty;    break;
    case 1:   aComplexity = eRegionComplexity_rect;     break;
    default:  aComplexity = eRegionComplexity_complex;  break;
  }

  return NS_OK;
}

nsresult nsRegionOS2::GetNumRects (PRUint32 *aRects) const
{
  *aRects = mRegion.GetNumRects ();
  return NS_OK;
}
