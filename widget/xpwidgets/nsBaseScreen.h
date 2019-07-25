







































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

protected:
  

















  virtual void ApplyMinimumBrightness(PRUint32 aBrightness) { }

private:
  



  void CheckMinimumBrightness();

  PRUint32 mBrightnessLocks[nsIScreen::BRIGHTNESS_LEVELS];
};

#endif 
