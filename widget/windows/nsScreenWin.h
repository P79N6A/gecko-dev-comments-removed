




#ifndef nsScreenWin_h___
#define nsScreenWin_h___

#include <windows.h>
#include "nsBaseScreen.h"



class nsScreenWin : public nsBaseScreen
{
public:
  nsScreenWin ( HMONITOR inScreen );
  ~nsScreenWin();

  NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth);

private:

  HMONITOR mScreen;
};

#endif  
