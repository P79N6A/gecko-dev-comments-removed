




#ifndef nsScreenQt_h___
#define nsScreenQt_h___

#include "nsBaseScreen.h"



class nsScreenQt : public nsBaseScreen
{
public:
  nsScreenQt (int aScreen);
  virtual ~nsScreenQt();

  NS_IMETHOD GetId(uint32_t* aId);
  NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth);

private:
  int mScreen;
};

#endif  
