



#ifndef nsScreen_h___
#define nsScreen_h___

#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Hal.h"
#include "nsIDOMScreen.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsCOMPtr.h"
#include "nsDOMEventTargetHelper.h"

class nsIDocShell;
class nsDeviceContext;
struct nsRect;


class nsScreen : public nsDOMEventTargetHelper
               , public nsIDOMScreen
               , public mozilla::hal::ScreenConfigurationObserver
{
  typedef mozilla::ErrorResult ErrorResult;
public:
  static already_AddRefed<nsScreen> Create(nsPIDOMWindow* aWindow);

  void Reset();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSCREEN
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  bool MozLockOrientation(const nsAString& aOrientation, ErrorResult& aRv);
  bool MozLockOrientation(const mozilla::dom::Sequence<nsString>& aOrientations, ErrorResult& aRv);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsScreen,
                                           nsDOMEventTargetHelper)

  void Notify(const mozilla::hal::ScreenConfiguration& aConfiguration);

protected:
  nsDeviceContext* GetDeviceContext();
  nsresult GetRect(nsRect& aRect);
  nsresult GetAvailRect(nsRect& aRect);

  mozilla::dom::ScreenOrientation mOrientation;

private:
  class FullScreenEventListener MOZ_FINAL : public nsIDOMEventListener
  {
  public:
    FullScreenEventListener() {};

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER
  };

  nsScreen();
  virtual ~nsScreen();

  enum LockPermission {
    LOCK_DENIED,
    FULLSCREEN_LOCK_ALLOWED,
    LOCK_ALLOWED
  };

  LockPermission GetLockOrientationPermission() const;

  nsRefPtr<FullScreenEventListener> mEventListener;
};

#endif
