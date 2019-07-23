




































#ifndef nsIDrawingSurfaceWin_h___
#define nsIDrawingSurfaceWin_h___

#include "nsIDrawingSurface.h"
#include <windows.h>



#define NS_IDRAWING_SURFACE_WIN_IID   \
{ 0x1ed958b0, 0xcab6, 0x11d2, \
{ 0xa8, 0x49, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

class nsIDrawingSurfaceWin : public nsISupports
{
public:
  






  NS_IMETHOD Init(HDC aDC) = 0;

  












  NS_IMETHOD Init(HDC aDC, PRUint32 aWidth, PRUint32 aHeight,
                  PRUint32 aFlags) = 0;

  






  NS_IMETHOD GetDC(HDC *aDC) = 0;

  




  NS_IMETHOD ReleaseDC(void) = 0;

  





  NS_IMETHOD IsReleaseDCDestructive(PRBool *aDestructive) = 0;
};

#endif  
