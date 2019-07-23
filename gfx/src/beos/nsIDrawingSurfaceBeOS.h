




































#ifndef nsIDrawingSurfaceBeOS_h___
#define nsIDrawingSurfaceBeOS_h___

#include "nsIDrawingSurface.h"

#include <View.h>



#define NS_IDRAWING_SURFACE_BEOS_IID   \
{ 0x1ed958b0, 0xcab6, 0x11d2, \
{ 0xa8, 0x49, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

class nsIDrawingSurfaceBeOS : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAWING_SURFACE_BEOS_IID)

  






  NS_IMETHOD Init(BView *aView) = 0;

  












  NS_IMETHOD Init(BView *aView, PRUint32 aWidth, PRUint32 aHeight,
                  PRUint32 aFlags) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDrawingSurfaceBeOS,
                              NS_IDRAWING_SURFACE_BEOS_IID)

#endif  
