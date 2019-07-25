







































#ifndef nsScreenQt_h___
#define nsScreenQt_h___

#include "nsIScreen.h"

#ifdef MOZ_ENABLE_QMSYSTEM2
#include "WidgetUtils.h"
namespace MeeGo
{
    class QmDisplayState;
}
#endif




class nsScreenQt : public nsIScreen
#ifdef MOZ_ENABLE_QMSYSTEM2
                , public mozilla::widget::BrightnessLockingWidget
#endif
{
public:
  nsScreenQt (int aScreen);
  virtual ~nsScreenQt();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

#ifdef MOZ_ENABLE_QMSYSTEM2
  void ApplyMinimumBrightness(PRUint32 aType);
private:
  MeeGo::QmDisplayState* mDisplayState;
#endif
private:
  int mScreen;
};

#endif  
