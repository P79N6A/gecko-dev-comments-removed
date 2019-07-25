




































#ifndef nsScreenOS2_h___
#define nsScreenOS2_h___

#include "nsBaseScreen.h"

#define INCL_WIN
#define INCL_DOS
#include <os2.h>



class nsScreenOS2 : public nsBaseScreen
{
public:
  nsScreenOS2 ( );
  virtual ~nsScreenOS2();

  NS_IMETHOD GetRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
  NS_IMETHOD GetAvailRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
  NS_IMETHOD GetPixelDepth(PRInt32* aPixelDepth);
  NS_IMETHOD GetColorDepth(PRInt32* aColorDepth);

private:

};

#endif
