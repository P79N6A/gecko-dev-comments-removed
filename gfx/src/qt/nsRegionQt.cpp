







































#include "nsRegionQt.h"
#include "prmem.h"
#include "nsRenderingContextQt.h"
#include <qregion.h>

#include "qtlog.h"

nsRegionQt::nsRegionQt() : mRegion()
{
}

nsRegionQt::~nsRegionQt()
{
}

NS_IMPL_ISUPPORTS1(nsRegionQt, nsIRegion)

nsresult nsRegionQt::Init(void)
{
    mRegion = QRegion();
    return NS_OK;
}

void nsRegionQt::SetTo(const nsIRegion &aRegion)
{
    nsRegionQt *pRegion = (nsRegionQt*)&aRegion;

    mRegion = pRegion->mRegion;
}

void nsRegionQt::SetTo(const nsRegionQt *aRegion)
{
    mRegion = aRegion->mRegion;
}

void nsRegionQt::SetTo(PRInt32 aX,PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight)
{
    mRegion = QRegion(aX, aY, aWidth, aHeight);;
}

void nsRegionQt::Intersect(const nsIRegion &aRegion)
{
    nsRegionQt *pRegion = (nsRegionQt*)&aRegion;

    mRegion = mRegion.intersect(pRegion->mRegion);
}

void nsRegionQt::Intersect(PRInt32 aX,PRInt32 aY,
                           PRInt32 aWidth,PRInt32 aHeight)
{
    mRegion = mRegion.intersect(QRect(aX, aY, aWidth, aHeight));
}

void nsRegionQt::Union(const nsIRegion &aRegion)
{
    nsRegionQt *pRegion = (nsRegionQt*)&aRegion;

    mRegion = mRegion.unite(pRegion->mRegion);
}

void nsRegionQt::Union(PRInt32 aX,PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight)
{
    mRegion = mRegion.unite(QRect(aX, aY, aWidth, aHeight));
}

void nsRegionQt::Subtract(const nsIRegion &aRegion)
{
    nsRegionQt *pRegion = (nsRegionQt*)&aRegion;

    mRegion = mRegion.subtract(pRegion->mRegion);
}

void nsRegionQt::Subtract(PRInt32 aX,PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight)
{
    mRegion = mRegion.subtract(QRect(aX, aY, aWidth, aHeight));
}

PRBool nsRegionQt::IsEmpty(void)
{
    return mRegion.isEmpty();
}

PRBool nsRegionQt::IsEqual(const nsIRegion &aRegion)
{
    nsRegionQt *pRegion = (nsRegionQt*)&aRegion;

    return (mRegion == pRegion->mRegion);
}

void nsRegionQt::GetBoundingBox(PRInt32 *aX,PRInt32 *aY,
                                PRInt32 *aWidth,PRInt32 *aHeight)
{
    QRect rect = mRegion.boundingRect();

    *aX = rect.x();
    *aY = rect.y();
    *aWidth = rect.width();
    *aHeight = rect.height();
}

void nsRegionQt::Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
    mRegion.translate(aXOffset, aYOffset);
}

PRBool nsRegionQt::ContainsRect(PRInt32 aX,PRInt32 aY,
                                PRInt32 aWidth,PRInt32 aHeight)
{
    return mRegion.contains(QRect(aX, aY, aWidth, aHeight));
}

NS_IMETHODIMP nsRegionQt::GetRects(nsRegionRectSet **aRects)
{
    NS_ASSERTION(!(nsnull == aRects), "bad ptr");

    QMemArray<QRect>   array = mRegion.rects();
    PRUint32        size  = array.size();
    nsRegionRect    *rect  = nsnull;
    nsRegionRectSet *rects = *aRects;

    if (nsnull == rects || rects->mRectsLen < (PRUint32)size) {
        void *buf = PR_Realloc(rects,
                               sizeof(nsRegionRectSet)
                               + (sizeof(nsRegionRect) * (size - 1)));

        if (nsnull == buf) {
            if (nsnull != rects)
                rects->mNumRects = 0;
            return NS_OK;
        }
        rects = (nsRegionRectSet*)buf;
        rects->mRectsLen = size;
    }
    rects->mNumRects = size;
    rects->mArea = 0;
    rect = &rects->mRects[0];

    for (PRUint32 i = 0; i < size; i++) {
        const QRect &qRect = array.at(i);

        rect->x = qRect.x();
        rect->y = qRect.y();
        rect->width = qRect.width();
        rect->height = qRect.height();

        rects->mArea += rect->width * rect->height;

        rect++;
    }
    *aRects = rects;
    return NS_OK;
}

NS_IMETHODIMP nsRegionQt::FreeRects(nsRegionRectSet *aRects)
{
    if (nsnull != aRects) {
        PR_Free((void*)aRects);
    }
    return NS_OK;
}

NS_IMETHODIMP nsRegionQt::GetNativeRegion(void *&aRegion) const
{
    aRegion = (void*)&mRegion;
    return NS_OK;
}

NS_IMETHODIMP nsRegionQt::GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
    
    if (mRegion.isEmpty()) {
        aComplexity = eRegionComplexity_empty;
    }
    else {
        aComplexity = eRegionComplexity_rect;
    }
    return NS_OK;
}

void nsRegionQt::SetRegionEmpty()
{
    mRegion = QRegion();
}

NS_IMETHODIMP nsRegionQt::GetNumRects(PRUint32 *aRects) const
{
  *aRects = mRegion.rects().size();

  return NS_OK;
}
