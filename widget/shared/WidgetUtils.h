







































#ifndef __mozilla_widget_WidgetUtils_h__
#define __mozilla_widget_WidgetUtils_h__

#include "nsCOMPtr.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindow.h"
#include "nsIScreen.h"

namespace mozilla {
namespace widget {

class WidgetUtils
{
public:

  



  static already_AddRefed<nsIWidget> DOMWindowToWidget(nsIDOMWindow *aDOMWindow);
};





class BrightnessLockingWidget : public nsIScreen_MOZILLA_2_0_BRANCH
{
public:
  BrightnessLockingWidget();

  NS_IMETHOD LockMinimumBrightness(PRUint32 aBrightness);
  NS_IMETHOD UnlockMinimumBrightness(PRUint32 aBrightness);

protected:
  















  virtual void ApplyMinimumBrightness(PRUint32 aBrightness) = 0;

  



  void CheckMinimumBrightness();

  PRUint32 mBrightnessLocks[nsIScreen_MOZILLA_2_0_BRANCH::BRIGHTNESS_LEVELS];
};

} 
} 

#endif
