






































#include "Hal.h"
#include "mozilla/Util.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/Observer.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "mozilla/Services.h"
#include "nsIWebNavigation.h"
#include "nsITabChild.h"
#include "nsIDocShell.h"
#include "mozilla/ClearOnShutdown.h"
#include "WindowIdentifier.h"

using namespace mozilla::services;

#define PROXY_IF_SANDBOXED(_call)                 \
  do {                                            \
    if (InSandbox()) {                            \
      hal_sandbox::_call;                         \
    } else {                                      \
      hal_impl::_call;                            \
    }                                             \
  } while (0)

#define RETURN_PROXY_IF_SANDBOXED(_call)          \
  do {                                            \
    if (InSandbox()) {                            \
      return hal_sandbox::_call;                  \
    } else {                                      \
      return hal_impl::_call;                     \
    }                                             \
  } while (0)

namespace mozilla {
namespace hal {

PRLogModuleInfo *sHalLog = PR_LOG_DEFINE("hal");

namespace {

void
AssertMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

bool
InSandbox()
{
  return GeckoProcessType_Content == XRE_GetProcessType();
}

bool
WindowIsActive(nsIDOMWindow *window)
{
  NS_ENSURE_TRUE(window, false);

  nsCOMPtr<nsIDOMDocument> doc;
  window->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, false);

  bool hidden = true;
  doc->GetMozHidden(&hidden);
  return !hidden;
}

nsAutoPtr<WindowIdentifier::IDArrayType> gLastIDToVibrate;

void InitLastIDToVibrate()
{
  gLastIDToVibrate = new WindowIdentifier::IDArrayType();
  ClearOnShutdown(&gLastIDToVibrate);
}

} 

void
Vibrate(const nsTArray<uint32>& pattern, nsIDOMWindow* window)
{
  Vibrate(pattern, WindowIdentifier(window));
}

void
Vibrate(const nsTArray<uint32>& pattern, const WindowIdentifier &id)
{
  AssertMainThread();

  
  
  
  
  
  
  if (!id.HasTraveledThroughIPC() && !WindowIsActive(id.GetWindow())) {
    HAL_LOG(("Vibrate: Window is inactive, dropping vibrate."));
    return;
  }

  if (InSandbox()) {
    hal_sandbox::Vibrate(pattern, id);
  }
  else {
    if (!gLastIDToVibrate)
      InitLastIDToVibrate();
    *gLastIDToVibrate = id.AsArray();

    HAL_LOG(("Vibrate: Forwarding to hal_impl."));

    
    
    hal_impl::Vibrate(pattern, WindowIdentifier());
  }
}

void
CancelVibrate(nsIDOMWindow* window)
{
  CancelVibrate(WindowIdentifier(window));
}

void
CancelVibrate(const WindowIdentifier &id)
{
  AssertMainThread();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (InSandbox()) {
    hal_sandbox::CancelVibrate(id);
  }
  else if (*gLastIDToVibrate == id.AsArray()) {
    
    
    
    HAL_LOG(("CancelVibrate: Forwarding to hal_impl."));
    hal_impl::CancelVibrate(WindowIdentifier());
  }
}

class BatteryObserversManager
{
public:
  void AddObserver(BatteryObserver* aObserver) {
    if (!mObservers) {
      mObservers = new ObserverList<BatteryInformation>();
    }

    mObservers->AddObserver(aObserver);

    if (mObservers->Length() == 1) {
      PROXY_IF_SANDBOXED(EnableBatteryNotifications());
    }
  }

  void RemoveObserver(BatteryObserver* aObserver) {
    mObservers->RemoveObserver(aObserver);

    if (mObservers->Length() == 0) {
      PROXY_IF_SANDBOXED(DisableBatteryNotifications());

      delete mObservers;
      mObservers = 0;

      delete mBatteryInfo;
      mBatteryInfo = 0;
    }
  }

  void CacheBatteryInformation(const BatteryInformation& aBatteryInfo) {
    if (mBatteryInfo) {
      delete mBatteryInfo;
    }
    mBatteryInfo = new BatteryInformation(aBatteryInfo);
  }

  bool HasCachedBatteryInformation() const {
    return mBatteryInfo;
  }

  void GetCachedBatteryInformation(BatteryInformation* aBatteryInfo) const {
    *aBatteryInfo = *mBatteryInfo;
  }

  void Broadcast(const BatteryInformation& aBatteryInfo) {
    MOZ_ASSERT(mObservers);
    mObservers->Broadcast(aBatteryInfo);
  }

private:
  ObserverList<BatteryInformation>* mObservers;
  BatteryInformation*               mBatteryInfo;
};

static BatteryObserversManager sBatteryObservers;

void
RegisterBatteryObserver(BatteryObserver* aBatteryObserver)
{
  AssertMainThread();
  sBatteryObservers.AddObserver(aBatteryObserver);
}

void
UnregisterBatteryObserver(BatteryObserver* aBatteryObserver)
{
  AssertMainThread();
  sBatteryObservers.RemoveObserver(aBatteryObserver);
}




void
GetCurrentBatteryInformation(BatteryInformation* aBatteryInfo)
{
  AssertMainThread();

  if (sBatteryObservers.HasCachedBatteryInformation()) {
    sBatteryObservers.GetCachedBatteryInformation(aBatteryInfo);
  } else {
    PROXY_IF_SANDBOXED(GetCurrentBatteryInformation(aBatteryInfo));
    sBatteryObservers.CacheBatteryInformation(*aBatteryInfo);
  }
}

void NotifyBatteryChange(const BatteryInformation& aBatteryInfo)
{
  AssertMainThread();

  sBatteryObservers.CacheBatteryInformation(aBatteryInfo);
  sBatteryObservers.Broadcast(aBatteryInfo);
}

bool GetScreenEnabled()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetScreenEnabled());
}

void SetScreenEnabled(bool enabled)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetScreenEnabled(enabled));
}

double GetScreenBrightness()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetScreenBrightness());
}

void SetScreenBrightness(double brightness)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetScreenBrightness(clamped(brightness, 0.0, 1.0)));
}

} 
} 
