






#ifndef nsBaseScreen_h
#define nsBaseScreen_h

#include "mozilla/Attributes.h"
#include "nsIScreen.h"

class nsBaseScreen : public nsIScreen
{
public:
  nsBaseScreen();

  NS_DECL_ISUPPORTS

  

  
  
  
  NS_IMETHOD GetRectDisplayPix(int32_t *outLeft,  int32_t *outTop,
                               int32_t *outWidth, int32_t *outHeight) override;
  NS_IMETHOD GetAvailRectDisplayPix(int32_t *outLeft,  int32_t *outTop,
                                    int32_t *outWidth, int32_t *outHeight) override;

  



  NS_IMETHOD LockMinimumBrightness(uint32_t aBrightness) override;
  NS_IMETHOD UnlockMinimumBrightness(uint32_t aBrightness) override;

  NS_IMETHOD GetRotation(uint32_t* aRotation) override {
    *aRotation = nsIScreen::ROTATION_0_DEG;
    return NS_OK;
  }
  NS_IMETHOD SetRotation(uint32_t aRotation) override { return NS_ERROR_NOT_AVAILABLE; }

  NS_IMETHOD GetContentsScaleFactor(double* aContentsScaleFactor) override;

protected:
  virtual ~nsBaseScreen();

  

















  virtual void ApplyMinimumBrightness(uint32_t aBrightness) { }

private:
  



  void CheckMinimumBrightness();

  uint32_t mBrightnessLocks[nsIScreen::BRIGHTNESS_LEVELS];
};

#endif 
