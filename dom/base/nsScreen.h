



































#ifndef nsScreen_h___
#define nsScreen_h___

#include "mozilla/Hal.h"
#include "nsIDOMScreen.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/Observer.h"

class nsIDocShell;
class nsDeviceContext;
struct nsRect;


class nsScreen : public nsDOMEventTargetHelper
               , public nsIDOMScreen
               , public mozilla::hal::ScreenOrientationObserver
{
public:
  static already_AddRefed<nsScreen> Create(nsPIDOMWindow* aWindow);

  void Invalidate();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSCREEN
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsScreen,
                                           nsDOMEventTargetHelper)

  void Notify(const mozilla::dom::ScreenOrientationWrapper& aOrientation);

protected:
  nsDeviceContext* GetDeviceContext();
  nsresult GetRect(nsRect& aRect);
  nsresult GetAvailRect(nsRect& aRect);

  bool mIsChrome;

  mozilla::dom::ScreenOrientation mOrientation;

private:
  nsScreen();
  virtual ~nsScreen();

  static bool sInitialized;
  static bool sAllowScreenEnabledProperty;
  static bool sAllowScreenBrightnessProperty;

  static void Initialize();

  NS_DECL_EVENT_HANDLER(mozorientationchange)
};

#endif
