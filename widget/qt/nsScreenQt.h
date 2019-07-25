




#ifndef nsScreenQt_h___
#define nsScreenQt_h___

#include "nsBaseScreen.h"

#ifdef MOZ_ENABLE_QMSYSTEM2
namespace MeeGo
{
    class QmDisplayState;
}
#endif




class nsScreenQt : public nsBaseScreen
{
public:
  nsScreenQt (int aScreen);
  virtual ~nsScreenQt();

  NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth);

#ifdef MOZ_ENABLE_QMSYSTEM2
protected:
  virtual void ApplyMinimumBrightness(uint32_t aType) MOZ_OVERRIDE;
private:
  MeeGo::QmDisplayState* mDisplayState;
#endif
private:
  int mScreen;
};

#endif  
