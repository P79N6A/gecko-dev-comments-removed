




































#ifndef nsIRegion_h___
#define nsIRegion_h___

#include "nscore.h"
#include "nsISupports.h"
#include "nsRect.h"

enum nsRegionComplexity
{
  eRegionComplexity_empty = 0,
  eRegionComplexity_rect = 1,
  eRegionComplexity_complex = 2
};

typedef struct
{
  PRInt32   x;
  PRInt32   y;
  PRUint32  width;
  PRUint32  height;
} nsRegionRect;

typedef struct
{
  PRUint32      mNumRects;    
  PRUint32      mRectsLen;    
  PRUint32      mArea;        
  nsRegionRect  mRects[1];
} nsRegionRectSet;






#define NS_IREGION_IID   \
{ 0x8ef366e0, 0xee94, 0x11d1,    \
{ 0xa8, 0x2a, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

class nsIRegion : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IREGION_IID)

  virtual nsresult Init(void) = 0;

  







  virtual void SetTo(const nsIRegion &aRegion) = 0;

  










  virtual void SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) = 0;

  







  virtual void Intersect(const nsIRegion &aRegion) = 0;

  










  virtual void Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) = 0;

  







  virtual void Union(const nsIRegion &aRegion) = 0;

  










  virtual void Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) = 0;

  







  virtual void Subtract(const nsIRegion &aRegion) = 0;

  










  virtual void Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) = 0;
  
  







  virtual PRBool IsEmpty(void) = 0;

  








  virtual PRBool IsEqual(const nsIRegion &aRegion) = 0;

  










  virtual void GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight) = 0;

  







  virtual void Offset(PRInt32 aXOffset, PRInt32 aYOffset) = 0;

  







  virtual PRBool ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) = 0;
  
  










  NS_IMETHOD GetRects(nsRegionRectSet **aRects) = 0;

  






  NS_IMETHOD FreeRects(nsRegionRectSet *aRects) = 0;

  




  NS_IMETHOD GetNativeRegion(void *&aRegion) const = 0;

  





  NS_IMETHOD GetRegionComplexity(nsRegionComplexity &aComplexity) const = 0;

  







  NS_IMETHOD GetNumRects(PRUint32 *aRects) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRegion, NS_IREGION_IID)

#endif  
