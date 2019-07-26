



#include "MobileConnection.h"
#include "nsIDOMDOMRequest.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMEvent.h"
#include "nsIDOMUSSDReceivedEvent.h"
#include "nsIDOMDataErrorEvent.h"
#include "nsIDOMCFStateChangeEvent.h"
#include "GeneratedEvents.h"
#include "mozilla/Preferences.h"
#include "nsIPermissionManager.h"

#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "nsJSON.h"
#include "jsapi.h"
#include "mozilla/Services.h"

#define NS_RILCONTENTHELPER_CONTRACTID "@mozilla.org/ril/content-helper;1"

using namespace mozilla::dom::network;

class MobileConnection::Listener : public nsIMobileConnectionListener
{
  MobileConnection* mMobileConnection;

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_NSIMOBILECONNECTIONLISTENER(mMobileConnection)

  Listener(MobileConnection* aMobileConnection)
    : mMobileConnection(aMobileConnection)
  {
    MOZ_ASSERT(mMobileConnection);
  }

  void Disconnect()
  {
    MOZ_ASSERT(mMobileConnection);
    mMobileConnection = nullptr;
  }
};

NS_IMPL_ISUPPORTS1(MobileConnection::Listener, nsIMobileConnectionListener)

DOMCI_DATA(MozMobileConnection, MobileConnection)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(MobileConnection,
                                                  nsDOMEventTargetHelper)
  
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(MobileConnection,
                                                nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MobileConnection)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozMobileConnection)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozMobileConnection)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(MobileConnection, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MobileConnection, nsDOMEventTargetHelper)

NS_IMPL_EVENT_HANDLER(MobileConnection, voicechange)
NS_IMPL_EVENT_HANDLER(MobileConnection, datachange)
NS_IMPL_EVENT_HANDLER(MobileConnection, ussdreceived)
NS_IMPL_EVENT_HANDLER(MobileConnection, dataerror)
NS_IMPL_EVENT_HANDLER(MobileConnection, cfstatechange)

MobileConnection::MobileConnection()
{
  mProvider = do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  mWindow = nullptr;

  
  
  if (!mProvider) {
    NS_WARNING("Could not acquire nsIMobileConnectionProvider!");
    return;
  }
}

void
MobileConnection::Init(nsPIDOMWindow* aWindow)
{
  BindToOwner(aWindow);

  mWindow = do_GetWeakReference(aWindow);
  mListener = new Listener(this);

  if (!CheckPermission("mobilenetwork") &&
      CheckPermission("mobileconnection")) {
    DebugOnly<nsresult> rv = mProvider->RegisterMobileConnectionMsg(mListener);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Failed registering mobile connection messages with provider");

    printf_stderr("MobileConnection initialized");
  }
}

void
MobileConnection::Shutdown()
{
  if (mProvider && mListener) {
    mListener->Disconnect();
    mProvider->UnregisterMobileConnectionMsg(mListener);
    mProvider = nullptr;
    mListener = nullptr;
  }
}



NS_IMETHODIMP
MobileConnection::GetLastKnownNetwork(nsAString& network)
{
  network.SetIsVoid(true);

  if (!CheckPermission("mobilenetwork")) {
    return NS_OK;
  }

  network = mozilla::Preferences::GetString("ril.lastKnownNetwork");
  return NS_OK;
}

NS_IMETHODIMP
MobileConnection::GetLastKnownHomeNetwork(nsAString& network)
{
  network.SetIsVoid(true);

  if (!CheckPermission("mobilenetwork")) {
    return NS_OK;
  }

  network = mozilla::Preferences::GetString("ril.lastKnownHomeNetwork");
  return NS_OK;
}



bool
MobileConnection::CheckPermission(const char* type)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(window, false);

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromWindow(window, type, &permission);
  return permission == nsIPermissionManager::ALLOW_ACTION;
}

NS_IMETHODIMP
MobileConnection::GetRetryCount(int32_t* retryCount)
{
  *retryCount = 0;

  if (!mProvider || !CheckPermission("mobileconnection")) {
    return NS_OK;
  }
  return mProvider->GetRetryCount(retryCount);
}

NS_IMETHODIMP
MobileConnection::GetVoice(nsIDOMMozMobileConnectionInfo** voice)
{
  *voice = nullptr;

  if (!mProvider || !CheckPermission("mobileconnection")) {
    return NS_OK;
  }
  return mProvider->GetVoiceConnectionInfo(voice);
}

NS_IMETHODIMP
MobileConnection::GetData(nsIDOMMozMobileConnectionInfo** data)
{
  *data = nullptr;

  if (!mProvider || !CheckPermission("mobileconnection")) {
    return NS_OK;
  }
  return mProvider->GetDataConnectionInfo(data);
}

NS_IMETHODIMP
MobileConnection::GetNetworkSelectionMode(nsAString& networkSelectionMode)
{
  networkSelectionMode.SetIsVoid(true);

  if (!mProvider || !CheckPermission("mobileconnection")) {
     return NS_OK;
  }
  return mProvider->GetNetworkSelectionMode(networkSelectionMode);
}

NS_IMETHODIMP
MobileConnection::GetNetworks(nsIDOMDOMRequest** request)
{
  *request = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetNetworks(GetOwner(), request);
}

NS_IMETHODIMP
MobileConnection::SelectNetwork(nsIDOMMozMobileNetworkInfo* network, nsIDOMDOMRequest** request)
{
  *request = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SelectNetwork(GetOwner(), network, request);
}

NS_IMETHODIMP
MobileConnection::SelectNetworkAutomatically(nsIDOMDOMRequest** request)
{
  *request = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SelectNetworkAutomatically(GetOwner(), request);
}

NS_IMETHODIMP
MobileConnection::SendMMI(const nsAString& aMMIString,
                          nsIDOMDOMRequest** aRequest)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SendMMI(GetOwner(), aMMIString, aRequest);
}

NS_IMETHODIMP
MobileConnection::CancelMMI(nsIDOMDOMRequest** aRequest)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->CancelMMI(GetOwner(), aRequest);
}

NS_IMETHODIMP
MobileConnection::GetCallForwardingOption(uint16_t aReason,
                                          nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetCallForwardingOption(GetOwner(), aReason, aRequest);
}

NS_IMETHODIMP
MobileConnection::SetCallForwardingOption(nsIDOMMozMobileCFInfo* aCFInfo,
                                          nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetCallForwardingOption(GetOwner(), aCFInfo, aRequest);
}

NS_IMETHODIMP
MobileConnection::GetCallBarringOption(const JS::Value& aOption,
                                       nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetCallBarringOption(GetOwner(), aOption, aRequest);
}

NS_IMETHODIMP
MobileConnection::SetCallBarringOption(const JS::Value& aOption,
                                       nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetCallBarringOption(GetOwner(), aOption, aRequest);
}

NS_IMETHODIMP
MobileConnection::GetCallWaitingOption(nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetCallWaitingOption(GetOwner(), aRequest);
}

NS_IMETHODIMP
MobileConnection::SetCallWaitingOption(bool aEnabled,
                                       nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetCallWaitingOption(GetOwner(), aEnabled, aRequest);
}

NS_IMETHODIMP
MobileConnection::GetCallingLineIdRestriction(nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetCallingLineIdRestriction(GetOwner(), aRequest);
}

NS_IMETHODIMP
MobileConnection::SetCallingLineIdRestriction(unsigned short aClirMode,
                                              nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetCallingLineIdRestriction(GetOwner(), aClirMode, aRequest);
}



NS_IMETHODIMP
MobileConnection::NotifyVoiceChanged()
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  return DispatchTrustedEvent(NS_LITERAL_STRING("voicechange"));
}

NS_IMETHODIMP
MobileConnection::NotifyDataChanged()
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  return DispatchTrustedEvent(NS_LITERAL_STRING("datachange"));
}

NS_IMETHODIMP
MobileConnection::NotifyUssdReceived(const nsAString& aMessage,
                                     bool aSessionEnded)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMUSSDReceivedEvent(getter_AddRefs(event), this, nullptr, nullptr);

  nsCOMPtr<nsIDOMUSSDReceivedEvent> ce = do_QueryInterface(event);
  nsresult rv = ce->InitUSSDReceivedEvent(NS_LITERAL_STRING("ussdreceived"),
                                          false, false,
                                          aMessage, aSessionEnded);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(ce);
}

NS_IMETHODIMP
MobileConnection::NotifyDataError(const nsAString& aMessage)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMDataErrorEvent(getter_AddRefs(event), this, nullptr, nullptr);

  nsCOMPtr<nsIDOMDataErrorEvent> ce = do_QueryInterface(event);
  nsresult rv = ce->InitDataErrorEvent(NS_LITERAL_STRING("dataerror"),
                                       false, false, aMessage);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(ce);
}

NS_IMETHODIMP
MobileConnection::NotifyCFStateChange(bool aSuccess,
                                      unsigned short aAction,
                                      unsigned short aReason,
                                      const nsAString& aNumber,
                                      unsigned short aSeconds,
                                      unsigned short aServiceClass)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMCFStateChangeEvent(getter_AddRefs(event), this, nullptr, nullptr);

  nsCOMPtr<nsIDOMCFStateChangeEvent> ce = do_QueryInterface(event);
  nsresult rv = ce->InitCFStateChangeEvent(NS_LITERAL_STRING("cfstatechange"),
                                           false, false,
                                           aSuccess, aAction, aReason, aNumber,
                                           aSeconds, aServiceClass);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(ce);
}
