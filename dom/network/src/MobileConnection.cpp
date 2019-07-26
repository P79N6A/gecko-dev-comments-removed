



#include "mozilla/dom/network/MobileConnection.h"

#include "GeneratedEvents.h"
#include "mozilla/Preferences.h"
#include "nsDOMEvent.h"
#include "nsIDOMCFStateChangeEvent.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMDOMRequest.h"
#include "nsIDOMDataErrorEvent.h"
#include "nsIDOMMozEmergencyCbModeEvent.h"
#include "nsIDOMMozOtaStatusEvent.h"
#include "nsIDOMUSSDReceivedEvent.h"
#include "nsIPermissionManager.h"

#include "nsJSUtils.h"
#include "nsJSON.h"
#include "mozilla/Services.h"

#define NS_RILCONTENTHELPER_CONTRACTID "@mozilla.org/ril/content-helper;1"

using namespace mozilla::dom::network;

class MobileConnection::Listener MOZ_FINAL : public nsIMobileConnectionListener
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

NS_IMPL_CYCLE_COLLECTION_CLASS(MobileConnection)

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
NS_IMPL_EVENT_HANDLER(MobileConnection, emergencycbmodechange)
NS_IMPL_EVENT_HANDLER(MobileConnection, otastatuschange)
NS_IMPL_EVENT_HANDLER(MobileConnection, iccchange)
NS_IMPL_EVENT_HANDLER(MobileConnection, radiostatechange)

MobileConnection::MobileConnection(uint32_t aClientId)
: mClientId(aClientId)
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

  if (CheckPermission("mobileconnection")) {
    DebugOnly<nsresult> rv = mProvider->RegisterMobileConnectionMsg(mClientId, mListener);
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
    mProvider->UnregisterMobileConnectionMsg(mClientId, mListener);
    mProvider = nullptr;
    mListener = nullptr;
  }
}



NS_IMETHODIMP
MobileConnection::GetLastKnownNetwork(nsAString& aNetwork)
{
  aNetwork.SetIsVoid(true);

  if (!CheckPermission("mobilenetwork")) {
    return NS_OK;
  }

  aNetwork = mozilla::Preferences::GetString("ril.lastKnownNetwork");
  return NS_OK;
}

NS_IMETHODIMP
MobileConnection::GetLastKnownHomeNetwork(nsAString& aNetwork)
{
  aNetwork.SetIsVoid(true);

  if (!CheckPermission("mobilenetwork")) {
    return NS_OK;
  }

  aNetwork = mozilla::Preferences::GetString("ril.lastKnownHomeNetwork");
  return NS_OK;
}



bool
MobileConnection::CheckPermission(const char* aType)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(window, false);

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromWindow(window, aType, &permission);
  return permission == nsIPermissionManager::ALLOW_ACTION;
}

NS_IMETHODIMP
MobileConnection::GetVoice(nsIDOMMozMobileConnectionInfo** aVoice)
{
  *aVoice = nullptr;

  if (!mProvider || !CheckPermission("mobileconnection")) {
    return NS_OK;
  }
  return mProvider->GetVoiceConnectionInfo(mClientId, aVoice);
}

NS_IMETHODIMP
MobileConnection::GetData(nsIDOMMozMobileConnectionInfo** aData)
{
  *aData = nullptr;

  if (!mProvider || !CheckPermission("mobileconnection")) {
    return NS_OK;
  }
  return mProvider->GetDataConnectionInfo(mClientId, aData);
}

NS_IMETHODIMP
MobileConnection::GetIccId(nsAString& aIccId)
{
  aIccId.SetIsVoid(true);

  if (!mProvider || !CheckPermission("mobileconnection")) {
     return NS_OK;
  }
  return mProvider->GetIccId(mClientId, aIccId);
}

NS_IMETHODIMP
MobileConnection::GetNetworkSelectionMode(nsAString& aNetworkSelectionMode)
{
  aNetworkSelectionMode.SetIsVoid(true);

  if (!mProvider || !CheckPermission("mobileconnection")) {
     return NS_OK;
  }
  return mProvider->GetNetworkSelectionMode(mClientId, aNetworkSelectionMode);
}

NS_IMETHODIMP
MobileConnection::GetRadioState(nsAString& aRadioState)
{
  aRadioState.SetIsVoid(true);

  if (!mProvider || !CheckPermission("mobileconnection")) {
     return NS_OK;
  }
  return mProvider->GetRadioState(mClientId, aRadioState);
}

NS_IMETHODIMP
MobileConnection::GetNetworks(nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetNetworks(mClientId, GetOwner(), aRequest);
}

NS_IMETHODIMP
MobileConnection::SelectNetwork(nsIDOMMozMobileNetworkInfo* aNetwork, nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SelectNetwork(mClientId, GetOwner(), aNetwork, aRequest);
}

NS_IMETHODIMP
MobileConnection::SelectNetworkAutomatically(nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SelectNetworkAutomatically(mClientId, GetOwner(), aRequest);
}

NS_IMETHODIMP
MobileConnection::SetPreferredNetworkType(const nsAString& aType,
                                          nsIDOMDOMRequest** aDomRequest)
{
  *aDomRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetPreferredNetworkType(mClientId, GetOwner(), aType, aDomRequest);
}

NS_IMETHODIMP
MobileConnection::GetPreferredNetworkType(nsIDOMDOMRequest** aDomRequest)
{
  *aDomRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetPreferredNetworkType(mClientId, GetOwner(), aDomRequest);
}

NS_IMETHODIMP
MobileConnection::SetRoamingPreference(const nsAString& aMode, nsIDOMDOMRequest** aDomRequest)
{
  *aDomRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetRoamingPreference(mClientId, GetOwner(), aMode, aDomRequest);
}

NS_IMETHODIMP
MobileConnection::GetRoamingPreference(nsIDOMDOMRequest** aDomRequest)
{
  *aDomRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetRoamingPreference(mClientId, GetOwner(), aDomRequest);
}

NS_IMETHODIMP
MobileConnection::SetVoicePrivacyMode(bool aEnabled, nsIDOMDOMRequest** aDomRequest)
{
  *aDomRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetVoicePrivacyMode(mClientId, GetOwner(), aEnabled, aDomRequest);
}

NS_IMETHODIMP
MobileConnection::GetVoicePrivacyMode(nsIDOMDOMRequest** aDomRequest)
{
  *aDomRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->GetVoicePrivacyMode(mClientId, GetOwner(), aDomRequest);
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

  return mProvider->SendMMI(mClientId, GetOwner(), aMMIString, aRequest);
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

  return mProvider->CancelMMI(mClientId, GetOwner(),aRequest);
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

  return mProvider->GetCallForwardingOption(mClientId, GetOwner(), aReason, aRequest);
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

  return mProvider->SetCallForwardingOption(mClientId, GetOwner(), aCFInfo, aRequest);
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

  return mProvider->GetCallBarringOption(mClientId, GetOwner(), aOption, aRequest);
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

  return mProvider->SetCallBarringOption(mClientId, GetOwner(), aOption, aRequest);
}

NS_IMETHODIMP
MobileConnection::ChangeCallBarringPassword(const JS::Value& aInfo,
                                            nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->ChangeCallBarringPassword(mClientId, GetOwner(), aInfo, aRequest);
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

  return mProvider->GetCallWaitingOption(mClientId, GetOwner(), aRequest);
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

  return mProvider->SetCallWaitingOption(mClientId, GetOwner(), aEnabled, aRequest);
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

  return mProvider->GetCallingLineIdRestriction(mClientId, GetOwner(), aRequest);
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

  return mProvider->SetCallingLineIdRestriction(mClientId, GetOwner(), aClirMode, aRequest);
}

NS_IMETHODIMP
MobileConnection::ExitEmergencyCbMode(nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->ExitEmergencyCbMode(mClientId, GetOwner(), aRequest);
}

NS_IMETHODIMP
MobileConnection::SetRadioEnabled(bool aEnabled,
                                  nsIDOMDOMRequest** aRequest)
{
  *aRequest = nullptr;

  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  return mProvider->SetRadioEnabled(mClientId, GetOwner(), aEnabled, aRequest);
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

NS_IMETHODIMP
MobileConnection::NotifyEmergencyCbModeChanged(bool aActive,
                                               uint32_t aTimeoutMs)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMMozEmergencyCbModeEvent(getter_AddRefs(event), this, nullptr,
                                   nullptr);
  MOZ_ASSERT(event);

  nsCOMPtr<nsIDOMMozEmergencyCbModeEvent> ce = do_QueryInterface(event);
  nsresult rv = ce->InitMozEmergencyCbModeEvent(
      NS_LITERAL_STRING("emergencycbmodechange"), false, false,
      aActive, aTimeoutMs);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(ce);
}

NS_IMETHODIMP
MobileConnection::NotifyOtaStatusChanged(const nsAString& aStatus)
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMMozOtaStatusEvent(getter_AddRefs(event), this, nullptr, nullptr);
  MOZ_ASSERT(event);

  nsCOMPtr<nsIDOMMozOtaStatusEvent> ce = do_QueryInterface(event);
  nsresult rv = ce->InitMozOtaStatusEvent(NS_LITERAL_STRING("otastatuschange"),
                                          false, false, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(ce);
}

NS_IMETHODIMP
MobileConnection::NotifyIccChanged()
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  return DispatchTrustedEvent(NS_LITERAL_STRING("iccchange"));
}

NS_IMETHODIMP
MobileConnection::NotifyRadioStateChanged()
{
  if (!CheckPermission("mobileconnection")) {
    return NS_OK;
  }

  return DispatchTrustedEvent(NS_LITERAL_STRING("radiostatechange"));
}
