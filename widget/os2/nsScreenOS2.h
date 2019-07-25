




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

  NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth);

private:

};

#endif
