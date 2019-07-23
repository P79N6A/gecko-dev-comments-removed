




































#ifndef nsIRenderingContextWin_h___
#define nsIRenderingContextWin_h___

#include "nsIRenderingContext.h"
#include <windows.h>


#define NS_IRENDERING_CONTEXT_WIN_IID \
{ 0x0fcde820, 0x8ae2, 0x11d2, \
{ 0xa8, 0x48, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }


class nsIRenderingContextWin : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRENDERING_CONTEXT_WIN_IID)
  





  NS_IMETHOD CreateDrawingSurface(HDC aDC, nsIDrawingSurface* &aSurface) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRenderingContextWin,
                              NS_IRENDERING_CONTEXT_WIN_IID)

#endif 
