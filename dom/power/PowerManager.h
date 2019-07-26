



#ifndef mozilla_dom_power_PowerManager_h
#define mozilla_dom_power_PowerManager_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDOMWindow.h"
#include "nsWeakReference.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

class nsPIDOMWindow;

namespace mozilla {
class ErrorResult;

namespace dom {

class PowerManager MOZ_FINAL : public nsIDOMMozWakeLockListener
                             , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(PowerManager)
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER

  PowerManager()
  {
    SetIsDOMBinding();
  }

  nsresult Init(nsIDOMWindow *aWindow);
  nsresult Shutdown();

  static bool CheckPermission(nsPIDOMWindow*);

  static already_AddRefed<PowerManager> CreateInstance(nsPIDOMWindow*);

  
  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  void Reboot(ErrorResult& aRv);
  void FactoryReset();
  void PowerOff(ErrorResult& aRv);
  void AddWakeLockListener(nsIDOMMozWakeLockListener* aListener);
  void RemoveWakeLockListener(nsIDOMMozWakeLockListener* aListener);
  void GetWakeLockState(const nsAString& aTopic, nsAString& aState,
                        ErrorResult& aRv);
  bool ScreenEnabled();
  void SetScreenEnabled(bool aEnabled);
  double ScreenBrightness();
  void SetScreenBrightness(double aBrightness, ErrorResult& aRv);
  bool CpuSleepAllowed();
  void SetCpuSleepAllowed(bool aAllowed);

private:
  nsCOMPtr<nsIDOMWindow> mWindow;
  nsTArray<nsCOMPtr<nsIDOMMozWakeLockListener> > mListeners;
};

} 
} 

#endif 
