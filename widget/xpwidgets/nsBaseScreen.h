







































#ifndef nsBaseScreen_h
#define nsBaseScreen_h

#include "mozilla/Attributes.h"
#include "nsIScreen.h"

class nsBaseScreen : public nsIScreen
{
public:
  nsBaseScreen();
  virtual ~nsBaseScreen();
  
  NS_DECL_ISUPPORTS

  

  



  NS_IMETHOD LockMinimumBrightness(PRUint32 aBrightness);
  NS_IMETHOD UnlockMinimumBrightness(PRUint32 aBrightness);

  NS_IMETHOD GetRotation(PRUint32* aRotation) {
    *aRotation = nsIScreen::ROTATION_0_DEG;
    return NS_OK;
  }
  NS_IMETHOD SetRotation(PRUint32 aRotation) { return NS_ERROR_NOT_AVAILABLE; }

protected:
  

















  virtual void ApplyMinimumBrightness(PRUint32 aBrightness) { }

private:
  



  void CheckMinimumBrightness();

  PRUint32 mBrightnessLocks[nsIScreen::BRIGHTNESS_LEVELS];
};

#endif 
