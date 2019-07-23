




































#ifndef nsIRenderingContextOS2_h___
#define nsIRenderingContextOS2_h___

#include "nsIRenderingContext.h"
#include <os2.h>


#define NS_IRENDERING_CONTEXT_OS2_IID \
{ 0x0fcde820, 0x8ae2, 0x11d2, \
{ 0xa8, 0x48, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }


class nsIRenderingContextOS2 : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRENDERING_CONTEXT_OS2_IID)
  





  NS_IMETHOD CreateDrawingSurface(HPS aPS, nsIDrawingSurface* &aSurface, nsIWidget *aWidget) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRenderingContextOS2,
                              NS_IRENDERING_CONTEXT_OS2_IID)

#endif 
