



#include "WifiProxyService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsXULAppAPI.h"
#include "WifiUtils.h"

#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
using namespace mozilla::tasktracer;
#endif

#define NS_WIFIPROXYSERVICE_CID \
  { 0xc6c9be7e, 0x744f, 0x4222, {0xb2, 0x03, 0xcd, 0x55, 0xdf, 0xc8, 0xbc, 0x12} }

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {


static StaticRefPtr<WifiProxyService> gWifiProxyService;


static nsAutoPtr<WpaSupplicant> gWpaSupplicant;


class WifiEventDispatcher : public nsRunnable
{
public:
  WifiEventDispatcher(const nsAString& aEvent, const nsACString& aInterface)
    : mEvent(aEvent)
    , mInterface(aInterface)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    gWifiProxyService->DispatchWifiEvent(mEvent, mInterface);
    return NS_OK;
  }

private:
  nsString mEvent;
  nsCString mInterface;
};


class EventRunnable : public nsRunnable
{
public:
  EventRunnable(const nsACString& aInterface)
    : mInterface(aInterface)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    nsAutoString event;
    gWpaSupplicant->WaitForEvent(event, mInterface);
    if (!event.IsEmpty()) {
#ifdef MOZ_TASK_TRACER
      
      
      AutoSourceEvent taskTracerEvent(SourceEventType::WIFI);
      AddLabel("%s %s", mInterface.get(), NS_ConvertUTF16toUTF8(event).get());
#endif
      nsCOMPtr<nsIRunnable> runnable = new WifiEventDispatcher(event, mInterface);
      NS_DispatchToMainThread(runnable);
    }
    return NS_OK;
  }

private:
  nsCString mInterface;
};


class WifiResultDispatcher : public nsRunnable
{
public:
  WifiResultDispatcher(WifiResultOptions& aResult, const nsACString& aInterface)
    : mResult(aResult)
    , mInterface(aInterface)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    gWifiProxyService->DispatchWifiResult(mResult, mInterface);
    return NS_OK;
  }

private:
  WifiResultOptions mResult;
  nsCString mInterface;
};


class ControlRunnable : public nsRunnable
{
public:
  ControlRunnable(CommandOptions aOptions, const nsACString& aInterface)
    : mOptions(aOptions)
    , mInterface(aInterface)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    WifiResultOptions result;
    if (gWpaSupplicant->ExecuteCommand(mOptions, result, mInterface)) {
      nsCOMPtr<nsIRunnable> runnable = new WifiResultDispatcher(result, mInterface);
      NS_DispatchToMainThread(runnable);
    }
    return NS_OK;
  }
private:
   CommandOptions mOptions;
   nsCString mInterface;
};

NS_IMPL_ISUPPORTS(WifiProxyService, nsIWifiProxyService)

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
WifiProxyService::Start(nsIWifiEventListener* aListener,
                        const char ** aInterfaces,
                        uint32_t aNumOfInterfaces)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);

  nsresult rv;

  
  
  
  mEventThreadList.SetLength(aNumOfInterfaces);
  for (uint32_t i = 0; i < aNumOfInterfaces; i++) {
    mEventThreadList[i].mInterface = aInterfaces[i];
    rv = NS_NewThread(getter_AddRefs(mEventThreadList[i].mThread));
    if (NS_FAILED(rv)) {
      NS_WARNING("Can't create wifi event thread");
      Shutdown();
      return NS_ERROR_FAILURE;
    }
  }

  rv = NS_NewThread(getter_AddRefs(mControlThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create wifi control thread");
    Shutdown();
    return NS_ERROR_FAILURE;
  }

  mListener = aListener;

  return NS_OK;
}

NS_IMETHODIMP
WifiProxyService::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  for (size_t i = 0; i < mEventThreadList.Length(); i++) {
    if (mEventThreadList[i].mThread) {
      mEventThreadList[i].mThread->Shutdown();
      mEventThreadList[i].mThread = nullptr;
    }
  }

  mEventThreadList.Clear();

  if (mControlThread) {
    mControlThread->Shutdown();
    mControlThread = nullptr;
  }

  mListener = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
WifiProxyService::SendCommand(JS::Handle<JS::Value> aOptions,
                              const nsACString& aInterface,
                              JSContext* aCx)
{
  MOZ_ASSERT(NS_IsMainThread());
  WifiCommandOptions options;

  if (!options.Init(aCx, aOptions)) {
    NS_WARNING("Bad dictionary passed to WifiProxyService::SendCommand");
    return NS_ERROR_FAILURE;
  }

  
  CommandOptions commandOptions(options);
  nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(commandOptions, aInterface);
  mControlThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
  return NS_OK;
}

NS_IMETHODIMP
WifiProxyService::WaitForEvent(const nsACString& aInterface)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  for (size_t i = 0; i < mEventThreadList.Length(); i++) {
    if (mEventThreadList[i].mInterface.Equals(aInterface)) {
      nsCOMPtr<nsIRunnable> runnable = new EventRunnable(aInterface);
      mEventThreadList[i].mThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

void
WifiProxyService::DispatchWifiResult(const WifiResultOptions& aOptions, const nsACString& aInterface)
{
  MOZ_ASSERT(NS_IsMainThread());

  mozilla::AutoSafeJSContext cx;
  JS::Rooted<JS::Value> val(cx);

  if (!ToJSValue(cx, aOptions, &val)) {
    return;
  }

  
  mListener->OnCommand(val, aInterface);
}

void
WifiProxyService::DispatchWifiEvent(const nsAString& aEvent, const nsACString& aInterface)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsAutoString event;
  if (StringBeginsWith(aEvent, NS_LITERAL_STRING("IFNAME"))) {
    
    event = Substring(aEvent, aEvent.FindChar(' ') + 1);
  }
  else {
    event = aEvent;
  }
  
  mListener->OnWaitEvent(event, aInterface);
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
