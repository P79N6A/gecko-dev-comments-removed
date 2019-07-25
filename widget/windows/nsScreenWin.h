




































#ifndef nsScreenWin_h___
#define nsScreenWin_h___

#include <windows.h>
#include "nsBaseScreen.h"



class nsScreenWin : public nsBaseScreen
{
public:
  nsScreenWin ( HMONITOR inScreen );
  ~nsScreenWin();

  NS_IMETHOD GetRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
  NS_IMETHOD GetAvailRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
  NS_IMETHOD GetPixelDepth(PRInt32* aPixelDepth);
  NS_IMETHOD GetColorDepth(PRInt32* aColorDepth);

private:

  HMONITOR mScreen;
};

#endif  
