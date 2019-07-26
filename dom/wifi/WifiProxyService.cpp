



#include "WifiProxyService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsXULAppAPI.h"
#include "WifiUtils.h"
#include "nsCxPusher.h"

#define NS_WIFIPROXYSERVICE_CID \
  { 0xc6c9be7e, 0x744f, 0x4222, {0xb2, 0x03, 0xcd, 0x55, 0xdf, 0xc8, 0xbc, 0x12} }

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {


StaticRefPtr<WifiProxyService> gWifiProxyService;


static nsAutoPtr<WpaSupplicant> gWpaSupplicant;


class WifiEventDispatcher : public nsRunnable
{
public:
  WifiEventDispatcher(nsAString& aEvent): mEvent(aEvent)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    gWifiProxyService->DispatchWifiEvent(mEvent);
    return NS_OK;
  }

private:
  nsString mEvent;
};


class EventRunnable : public nsRunnable
{
public:
  EventRunnable()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    nsAutoString event;
    gWpaSupplicant->WaitForEvent(event);
    if (!event.IsEmpty()) {
      nsCOMPtr<nsIRunnable> runnable = new WifiEventDispatcher(event);
      NS_DispatchToMainThread(runnable);
    }
    return NS_OK;
  }
};


class WifiResultDispatcher : public nsRunnable
{
public:
  WifiResultDispatcher(WifiResultOptions& aResult)
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    
#define COPY_FIELD(prop) mResult.prop = aResult.prop;

    COPY_FIELD(mId)
    COPY_FIELD(mStatus)
    COPY_FIELD(mReply)
    COPY_FIELD(mRoute)
    COPY_FIELD(mError)
    COPY_FIELD(mValue)
    COPY_FIELD(mIpaddr_str)
    COPY_FIELD(mGateway_str)
    COPY_FIELD(mBroadcast_str)
    COPY_FIELD(mDns1_str)
    COPY_FIELD(mDns2_str)
    COPY_FIELD(mMask_str)
    COPY_FIELD(mServer_str)
    COPY_FIELD(mVendor_str)
    COPY_FIELD(mLease)
    COPY_FIELD(mMask)
    COPY_FIELD(mIpaddr)
    COPY_FIELD(mGateway)
    COPY_FIELD(mDns1)
    COPY_FIELD(mDns2)
    COPY_FIELD(mServer)

#undef COPY_FIELD
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    gWifiProxyService->DispatchWifiResult(mResult);
    return NS_OK;
  }

private:
  WifiResultOptions mResult;
};


class ControlRunnable : public nsRunnable
{
public:
  ControlRunnable(CommandOptions aOptions) : mOptions(aOptions) {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    WifiResultOptions result;
    if (gWpaSupplicant->ExecuteCommand(mOptions, result)) {
      nsCOMPtr<nsIRunnable> runnable = new WifiResultDispatcher(result);
      NS_DispatchToMainThread(runnable);
    }
    return NS_OK;
  }
private:
   CommandOptions mOptions;
};

NS_IMPL_ISUPPORTS1(WifiProxyService, nsIWifiProxyService)

WifiProxyService::WifiProxyService()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!gWifiProxyService);
}

WifiProxyService::~WifiProxyService()
{
  MOZ_ASSERT(!gWifiProxyService);
}

already_AddRefed<WifiProxyService>
WifiProxyService::FactoryCreate()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return nullptr;
  }

  MOZ_ASSERT(NS_IsMainThread());

  if (!gWifiProxyService) {
    gWifiProxyService = new WifiProxyService();
    ClearOnShutdown(&gWifiProxyService);

    gWpaSupplicant = new WpaSupplicant();
    ClearOnShutdown(&gWpaSupplicant);
  }

  nsRefPtr<WifiProxyService> service = gWifiProxyService.get();
  return service.forget();
}

NS_IMETHODIMP
WifiProxyService::Start(nsIWifiEventListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);

  nsresult rv = NS_NewThread(getter_AddRefs(mEventThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create wifi event thread");
    return NS_ERROR_FAILURE;
  }

  rv = NS_NewThread(getter_AddRefs(mControlThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create wifi control thread");
    
    mEventThread->Shutdown();
    mEventThread = nullptr;
    return NS_ERROR_FAILURE;
  }

  mListener = aListener;

  return NS_OK;
}

NS_IMETHODIMP
WifiProxyService::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  mEventThread->Shutdown();
  mEventThread = nullptr;
  mControlThread->Shutdown();
  mControlThread = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
WifiProxyService::SendCommand(const JS::Value& aOptions, JSContext* aCx)
{
  MOZ_ASSERT(NS_IsMainThread());
  WifiCommandOptions options;

  if (!options.Init(aCx,
                    JS::Handle<JS::Value>::fromMarkedLocation(&aOptions))) {
    NS_WARNING("Bad dictionary passed to WifiProxyService::SendCommand");
    return NS_ERROR_FAILURE;
  }

  
  CommandOptions commandOptions(options);
  nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(commandOptions);
  mControlThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
  return NS_OK;
}

NS_IMETHODIMP
WifiProxyService::WaitForEvent()
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIRunnable> runnable = new EventRunnable();
  mEventThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
  return NS_OK;
}

void
WifiProxyService::DispatchWifiResult(const WifiResultOptions& aOptions)
{
  MOZ_ASSERT(NS_IsMainThread());

  mozilla::AutoSafeJSContext cx;
  JS::RootedValue val(cx);

  if (!aOptions.ToObject(cx, JS::NullPtr(), &val)) {
    return;
  }

  
  mListener->OnCommand(val);
}

void
WifiProxyService::DispatchWifiEvent(const nsAString& aEvent)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  mListener->OnWaitEvent(aEvent);
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(WifiProxyService,
                                         WifiProxyService::FactoryCreate)

NS_DEFINE_NAMED_CID(NS_WIFIPROXYSERVICE_CID);

static const mozilla::Module::CIDEntry kWifiProxyServiceCIDs[] = {
  { &kNS_WIFIPROXYSERVICE_CID, false, nullptr, WifiProxyServiceConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kWifiProxyServiceContracts[] = {
  { "@mozilla.org/wifi/service;1", &kNS_WIFIPROXYSERVICE_CID },
  { nullptr }
};

static const mozilla::Module kWifiProxyServiceModule = {
  mozilla::Module::kVersion,
  kWifiProxyServiceCIDs,
  kWifiProxyServiceContracts,
  nullptr
};

} 

NSMODULE_DEFN(WifiProxyServiceModule) = &kWifiProxyServiceModule;
