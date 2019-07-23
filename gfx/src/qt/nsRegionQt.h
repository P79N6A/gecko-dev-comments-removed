







































#ifndef nsRegionQt_h___
#define nsRegionQt_h___

#include "nsIRegion.h"

#include <qregion.h>

class nsRegionQt : public nsIRegion
{
public:
    nsRegionQt();
    virtual ~nsRegionQt();

    NS_DECL_ISUPPORTS

    virtual nsresult Init();

    virtual void SetTo(const nsIRegion &aRegion);
    virtual void SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    void SetTo(const nsRegionQt *aRegion);
    virtual void Intersect(const nsIRegion &aRegion);
    virtual void Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    virtual void Union(const nsIRegion &aRegion);
    virtual void Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    virtual void Subtract(const nsIRegion &aRegion);
    virtual void Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    virtual PRBool IsEmpty(void);
    virtual PRBool IsEqual(const nsIRegion &aRegion);
    virtual void GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight);
    virtual void Offset(PRInt32 aXOffset, PRInt32 aYOffset);
    virtual PRBool ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
    NS_IMETHOD GetRects(nsRegionRectSet **aRects);
    NS_IMETHOD FreeRects(nsRegionRectSet *aRects);
    NS_IMETHOD GetNativeRegion(void *&aRegion) const;
    NS_IMETHOD GetRegionComplexity(nsRegionComplexity &aComplexity) const;
    NS_IMETHOD GetNumRects(PRUint32* aRects) const;

private:
    virtual void SetRegionEmpty();

private:
    QRegion  mRegion;
};

#endif  
