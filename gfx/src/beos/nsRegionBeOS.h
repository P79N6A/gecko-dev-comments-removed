




































#ifndef nsRegionBeOS_h___
#define nsRegionBeOS_h___

#include "nsIRegion.h"

#include <Region.h>

class nsRegionBeOS : public nsIRegion
{
public:
  nsRegionBeOS();
  virtual ~nsRegionBeOS();

    NS_DECL_ISUPPORTS

  virtual nsresult Init();

  virtual void SetTo(const nsIRegion &aRegion);
  virtual void SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  void SetTo(const nsRegionBeOS *aRegion);
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
  
  NS_IMETHOD GetNumRects(PRUint32 *aRects) const { *aRects = 0; return NS_OK; }

protected: 
  void         SetRegionType(void); 

private:
  BRegion				mRegion;
  nsRegionComplexity	mRegionType;
};

#endif  
