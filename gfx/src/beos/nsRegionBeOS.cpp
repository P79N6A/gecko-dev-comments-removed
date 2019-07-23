




































#include <Region.h>

#include "nsRegionBeOS.h"
#include "prmem.h"

#ifdef DEBUG_REGIONS 
static int nRegions; 
#endif 


nsRegionBeOS :: nsRegionBeOS()
{
#ifdef DEBUG_REGIONS 
  ++nRegions; 
  printf("REGIONS+ = %i\n", nRegions); 
#endif 
 
  mRegion.MakeEmpty();
  mRegionType = eRegionComplexity_empty;
}

nsRegionBeOS :: ~nsRegionBeOS()
{
#ifdef DEBUG_REGIONS 
  --nRegions; 
  printf("REGIONS- = %i\n", nRegions); 
#endif 
 
  mRegion.MakeEmpty();
}

NS_IMPL_ISUPPORTS1(nsRegionBeOS, nsIRegion)

nsresult nsRegionBeOS :: Init(void)
{
  mRegion.MakeEmpty();
  mRegionType = eRegionComplexity_empty;
  return NS_OK;
}

void nsRegionBeOS :: SetTo(const nsIRegion &aRegion)
{
  Init();
  
  nsRegionBeOS *pRegion = (nsRegionBeOS *)&aRegion;

  mRegion = pRegion->mRegion;
  SetRegionType();
}

void nsRegionBeOS :: SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  Init();
  
	mRegion.Set(BRect(aX, aY, aX + aWidth - 1, aY + aHeight - 1));
	SetRegionType();
}

void nsRegionBeOS :: Intersect(const nsIRegion &aRegion)
{
	nsRegionBeOS *pRegion = (nsRegionBeOS*)&aRegion;

	mRegion.IntersectWith(&pRegion->mRegion);
	SetRegionType();
}

void nsRegionBeOS :: Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  BRegion tRegion;
  tRegion.Set( BRect( aX, aY, aX + aWidth - 1, aY + aHeight - 1 ) );
  mRegion.IntersectWith(&tRegion);
  SetRegionType();
}

void nsRegionBeOS :: Union(const nsIRegion &aRegion)
{
	nsRegionBeOS *pRegion = (nsRegionBeOS*)&aRegion;

	mRegion.Include(&pRegion->mRegion);
	SetRegionType();
}

void nsRegionBeOS :: Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	mRegion.Include(BRect(aX, aY, aX + aWidth - 1, aY + aHeight - 1));
	SetRegionType();
}

void nsRegionBeOS :: Subtract(const nsIRegion &aRegion)
{
  nsRegionBeOS *pRegion = (nsRegionBeOS*)&aRegion;

  mRegion.Exclude(&pRegion->mRegion);
  SetRegionType();
}

void nsRegionBeOS :: Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	mRegion.Exclude(BRect(aX, aY, aX + aWidth - 1, aY + aHeight - 1));
	SetRegionType();
}

PRBool nsRegionBeOS :: IsEmpty(void)
{
  if( mRegionType == eRegionComplexity_empty )
    return PR_TRUE;
  return PR_FALSE;
}

PRBool nsRegionBeOS :: IsEqual(const nsIRegion &aRegion)
{
#ifdef DEBUG
  printf(" - nsRegionBeOS :: IsEqual not implemented!\n");
#endif
  return PR_FALSE;
}

void nsRegionBeOS :: GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  if( mRegion.CountRects() ) {
    BRect rect = mRegion.Frame();
    *aX = nscoord(rect.left);
    *aY = nscoord(rect.top);
    *aWidth = nscoord(rect.Width()+1);
    *aHeight = nscoord(rect.Height()+1);
  }
  else
  {
  	*aX = *aY = *aWidth = *aHeight = 0;
  }
}

void nsRegionBeOS :: Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
	mRegion.OffsetBy( aXOffset, aYOffset );
}

void nsRegionBeOS :: SetRegionType(void)
{
  if(mRegion.CountRects() == 1)
    mRegionType = eRegionComplexity_rect ;
  else if(mRegion.CountRects() > 1)
    mRegionType = eRegionComplexity_complex ;
  else
    mRegionType = eRegionComplexity_empty;
}

PRBool nsRegionBeOS :: ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	return mRegion.Intersects(BRect(aX, aY, aX + aWidth - 1, aY + aHeight - 1));
}

NS_IMETHODIMP nsRegionBeOS :: GetRects(nsRegionRectSet **aRects)
{
	nsRegionRectSet   *rects;
	int               nbox;
	nsRegionRect      *rect;
	
	NS_ASSERTION(!(nsnull == aRects), "bad ptr");
	
	
	
	nbox = mRegion.CountRects();
	
	rects = *aRects;
	
	if ((nsnull == rects) || (rects->mRectsLen < (PRUint32)nbox))
	{
		void *buf = PR_Realloc(rects, sizeof(nsRegionRectSet) + (sizeof(nsRegionRect) * (nbox - 1)));

		if(nsnull == buf)
		{
			if(nsnull != rects)
				rects->mNumRects = 0;

			return NS_OK;
		}

		rects = (nsRegionRectSet *)buf;
		rects->mRectsLen = nbox;
	}

	rects->mNumRects = nbox;
	rects->mArea = 0;
	rect = &rects->mRects[0];

	for(int32 i = 0; i < nbox; i++)
	{
		BRect r = mRegion.RectAt(i);
    rect->x = nscoord(r.left); 
    rect->width = nscoord(r.right - r.left + 1); 
    rect->y = nscoord(r.top); 
    rect->height = nscoord(r.bottom - r.top + 1); 

		rects->mArea += rect->width * rect->height;

		rect++;
	}

	*aRects = rects;

	return NS_OK;
}

NS_IMETHODIMP nsRegionBeOS :: FreeRects(nsRegionRectSet *aRects)
{
	if(nsnull != aRects)
		PR_Free((void *)aRects);

	return NS_OK;
}

NS_IMETHODIMP nsRegionBeOS :: GetNativeRegion(void *&aRegion) const
{
	aRegion = (void *)&mRegion;
	return NS_OK;
}

NS_IMETHODIMP nsRegionBeOS :: GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
	aComplexity = mRegionType;
	return NS_OK;
}
