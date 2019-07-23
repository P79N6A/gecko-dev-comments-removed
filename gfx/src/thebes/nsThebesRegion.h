






































#ifndef NSTHEBESREGION__H__
#define NSTHEBESREGION__H__

#include "nsIRegion.h"
#include "nsRegion.h"

class nsThebesRegion : public nsIRegion
{
public:
    nsThebesRegion();

    NS_DECL_ISUPPORTS

    
    nsresult Init (void);
    void SetTo (const nsIRegion &aRegion);
    void SetTo (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    void Intersect (const nsIRegion &aRegion);
    void Intersect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    void Union (const nsIRegion &aRegion);
    void Union (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    void Subtract (const nsIRegion &aRegion);
    void Subtract (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    PRBool IsEmpty (void);
    PRBool IsEqual (const nsIRegion &aRegion);
    void GetBoundingBox (PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight);
    void Offset (PRInt32 aXOffset, PRInt32 aYOffset);
    PRBool ContainsRect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    NS_IMETHOD GetRects (nsRegionRectSet **aRects);
    NS_IMETHOD FreeRects (nsRegionRectSet *aRects);
    NS_IMETHOD GetNativeRegion (void *&aRegion) const;
    NS_IMETHOD GetRegionComplexity (nsRegionComplexity &aComplexity) const;
    NS_IMETHOD GetNumRects (PRUint32 *aRects) const;

protected:
    nsRegion mRegion;
};

#endif
