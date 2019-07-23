




































#ifndef nsIDrawingSurfacePh_h___
#define nsIDrawingSurfacePh_h___

#include "nsIDrawingSurface.h"
#include <Pt.h>



#define NS_IDRAWING_SURFACE_PH_IID   \
{ 0x1ed958b0, 0xcab6, 0x11d2, \
{ 0xa8, 0x49, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

class nsIDrawingSurfacePh : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAWING_SURFACE_PH_IID)

  






  NS_IMETHOD Init( PhDrawContext_t *aDC, PhGC_t *aGC ) = 0;

  












  NS_IMETHOD Init( PRUint32 aWidth, PRUint32 aHeight,
                  PRUint32 aFlags) = 0;
				  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDrawingSurfacePh, NS_IDRAWING_SURFACE_PH_IID)

#endif  
