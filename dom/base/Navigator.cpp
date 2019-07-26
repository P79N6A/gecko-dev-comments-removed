






#include "base/basictypes.h"

#include "Navigator.h"
#include "nsIXULAppInfo.h"
#include "nsPluginArray.h"
#include "nsMimeTypeArray.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/DesktopNotification.h"
#include "nsGeolocation.h"
#include "nsIHttpProtocolHandler.h"
#include "nsICachingChannel.h"
#include "nsIDocShell.h"
#include "nsIWebContentHandlerRegistrar.h"
#include "nsICookiePermission.h"
#include "nsIScriptSecurityManager.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsVariant.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "BatteryManager.h"
#include "PowerManager.h"
#include "nsIDOMWakeLock.h"
#include "nsIPowerManagerService.h"
#include "mozilla/dom/SmsManager.h"
#include "mozilla/dom/MobileMessageManager.h"
#include "nsISmsService.h"
#include "mozilla/Hal.h"
#include "nsIWebNavigation.h"
#include "nsISiteSpecificUserAgent.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "Connection.h"
#include "nsDOMClassInfo.h"
#include "nsDOMEvent.h"
#include "nsGlobalWindow.h"
#ifdef MOZ_B2G_RIL
#include "IccManager.h"
#include "MobileConnection.h"
#include "mozilla/dom/CellBroadcast.h"
#include "mozilla/dom/Voicemail.h"
#endif
#include "nsIIdleObserver.h"
#include "nsIPermissionManager.h"
#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "TimeManager.h"

#ifdef MOZ_MEDIA_NAVIGATOR
#include "MediaManager.h"
#endif
#ifdef MOZ_B2G_RIL
#include "TelephonyFactory.h"
#endif
#ifdef MOZ_B2G_BT
#include "nsIDOMBluetoothManager.h"
#include "BluetoothManager.h"
#endif
#include "nsIDOMCameraManager.h"
#include "DOMCameraManager.h"

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
#include "AudioChannelManager.h"
#endif

#include "nsIDOMGlobalPropertyInitializer.h"

using namespace mozilla::dom::power;


DOMCI_DATA(Navigator, mozilla::dom::Navigator)

namespace mozilla {
namespace dom {

static bool sDoNotTrackEnabled = false;
static bool sVibratorEnabled   = false;
static uint32_t sMaxVibrateMS  = 0;
static uint32_t sMaxVibrateListLen = 0;


void
Navigator::Init()
{
  Preferences::AddBoolVarCache(&sDoNotTrackEnabled,
                               "privacy.donottrackheader.enabled",
                               false);
  Preferences::AddBoolVarCache(&sVibratorEnabled,
                               "dom.vibrator.enabled", true);
  Preferences::AddUintVarCache(&sMaxVibrateMS,
                               "dom.vibrator.max_vibrate_ms", 10000);
  Preferences::AddUintVarCache(&sMaxVibrateListLen,
                               "dom.vibrator.max_vibrate_list_len", 128);
}

Navigator::Navigator(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
  NS_ASSERTION(aWindow->IsInnerWindow(),
               "Navigator must get an inner window!");
}

Navigator::~Navigator()
{
  Invalidate();
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Navigator)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMClientInformation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorDeviceStorage)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsINavigatorBattery)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorDesktopNotification)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozNavigatorSms)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozNavigatorMobileMessage)
#ifdef MOZ_MEDIA_NAVIGATOR
  NS_INTERFACE_MAP_ENTRY(nsINavigatorUserMedia)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorUserMedia)
#endif
#ifdef MOZ_B2G_RIL
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorTelephony)
#endif
#ifdef MOZ_GAMEPAD
  NS_INTERFACE_MAP_ENTRY(nsINavigatorGamepads)
#endif
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozNavigatorNetwork)
#ifdef MOZ_B2G_RIL
  NS_INTERFACE_MAP_ENTRY(nsIMozNavigatorMobileConnection)
  NS_INTERFACE_MAP_ENTRY(nsIMozNavigatorCellBroadcast)
  NS_INTERFACE_MAP_ENTRY(nsIMozNavigatorVoicemail)
  NS_INTERFACE_MAP_ENTRY(nsIMozNavigatorIccManager)
#endif
#ifdef MOZ_B2G_BT
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorBluetooth)
#endif
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorCamera)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorSystemMessages)
#ifdef MOZ_TIME_MANAGER
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozNavigatorTime)
#endif
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  NS_INTERFACE_MAP_ENTRY(nsIMozNavigatorAudioChannelManager)
#endif
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Navigator)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Navigator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Navigator)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Navigator)
  tmp->Invalidate();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Navigator)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPlugins)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMimeTypes)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGeolocation)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNotification)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBatteryManager)
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSmsManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileMessageManager)
#ifdef MOZ_B2G_RIL
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTelephony)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mVoicemail)
#endif
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mConnection)
#ifdef MOZ_B2G_RIL
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileConnection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCellBroadcast)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIccManager)
#endif
#ifdef MOZ_B2G_BT
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBluetooth)
#endif
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAudioChannelManager)
#endif
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCameraManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMessagesManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDeviceStorageStores)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTimeManager)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(Navigator)

void
Navigator::Invalidate()
{
  
  

  if (mPlugins) {
    mPlugins->Invalidate();
    mPlugins = nullptr;
  }

  mMimeTypes = nullptr;

  
  if (mGeolocation) {
    mGeolocation->Shutdown();
    mGeolocation = nullptr;
  }

  if (mNotification) {
    mNotification->Shutdown();
    mNotification = nullptr;
  }

  if (mBatteryManager) {
    mBatteryManager->Shutdown();
    mBatteryManager = nullptr;
  }

  if (mPowerManager) {
    mPowerManager->Shutdown();
    mPowerManager = nullptr;
  }

  if (mSmsManager) {
    mSmsManager->Shutdown();
    mSmsManager = nullptr;
  }

  if (mMobileMessageManager) {
    mMobileMessageManager->Shutdown();
    mMobileMessageManager = nullptr;
  }

#ifdef MOZ_B2G_RIL
  if (mTelephony) {
    mTelephony = nullptr;
  }

  if (mVoicemail) {
    mVoicemail = nullptr;
  }
#endif

  if (mConnection) {
    mConnection->Shutdown();
    mConnection = nullptr;
  }

#ifdef MOZ_B2G_RIL
  if (mMobileConnection) {
    mMobileConnection->Shutdown();
    mMobileConnection = nullptr;
  }

  if (mCellBroadcast) {
    mCellBroadcast = nullptr;
  }

  if (mIccManager) {
    mIccManager->Shutdown();
    mIccManager = nullptr;
  }
#endif

#ifdef MOZ_B2G_BT
  if (mBluetooth) {
    mBluetooth = nullptr;
  }
#endif

  mCameraManager = nullptr;

  if (mMessagesManager) {
    mMessagesManager = nullptr;
  }

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  if (mAudioChannelManager) {
    mAudioChannelManager = nullptr;
  }
#endif

  uint32_t len = mDeviceStorageStores.Length();
  for (uint32_t i = 0; i < len; ++i) {
    mDeviceStorageStores[i]->Shutdown();
  }
  mDeviceStorageStores.Clear();

  if (mTimeManager) {
    mTimeManager = nullptr;
  }
}





NS_IMETHODIMP
Navigator::GetUserAgent(nsAString& aUserAgent)
{
  nsresult rv = NS_GetNavigatorUserAgent(aUserAgent);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mWindow || !mWindow->GetDocShell()) {
    return NS_OK;
  }

  nsIDocument* doc = mWindow->GetExtantDoc();
  if (!doc) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> codebaseURI;
  doc->NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));
  if (!codebaseURI) {
    return NS_OK;
  }

  nsCOMPtr<nsISiteSpecificUserAgent> siteSpecificUA =
    do_GetService("@mozilla.org/dom/site-specific-user-agent;1");
  NS_ENSURE_TRUE(siteSpecificUA, NS_OK);

  return siteSpecificUA->GetUserAgentForURIAndWindow(codebaseURI, mWindow,
                                                     aUserAgent);
}

NS_IMETHODIMP
Navigator::GetAppCodeName(nsAString& aAppCodeName)
{
  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString appName;
  rv = service->GetAppName(appName);
  CopyASCIItoUTF16(appName, aAppCodeName);

  return rv;
}

NS_IMETHODIMP
Navigator::GetAppVersion(nsAString& aAppVersion)
{
  return NS_GetNavigatorAppVersion(aAppVersion);
}

NS_IMETHODIMP
Navigator::GetAppName(nsAString& aAppName)
{
  return NS_GetNavigatorAppName(aAppName);
}













NS_IMETHODIMP
Navigator::GetLanguage(nsAString& aLanguage)
{
  
  const nsAdoptingString& acceptLang =
    Preferences::GetLocalizedString("intl.accept_languages");

  
  nsCharSeparatedTokenizer langTokenizer(acceptLang, ',');
  const nsSubstring &firstLangPart = langTokenizer.nextToken();
  nsCharSeparatedTokenizer qTokenizer(firstLangPart, ';');
  aLanguage.Assign(qTokenizer.nextToken());

  
  
  if (aLanguage.Length() > 2 && aLanguage[2] == PRUnichar('_')) {
    aLanguage.Replace(2, 1, PRUnichar('-')); 
  }

  
  
  if (aLanguage.Length() <= 2) {
    return NS_OK;
  }

  nsCharSeparatedTokenizer localeTokenizer(aLanguage, '-');
  int32_t pos = 0;
  bool first = true;
  while (localeTokenizer.hasMoreTokens()) {
    const nsSubstring& code = localeTokenizer.nextToken();

    if (code.Length() == 2 && !first) {
      nsAutoString upper(code);
      ToUpperCase(upper);
      aLanguage.Replace(pos, code.Length(), upper);
    }

    pos += code.Length() + 1; 
    first = false;
  }

  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetPlatform(nsAString& aPlatform)
{
  return NS_GetNavigatorPlatform(aPlatform);
}

NS_IMETHODIMP
Navigator::GetOscpu(nsAString& aOSCPU)
{
  if (!nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      Preferences::GetString("general.oscpu.override");

    if (override) {
      aOSCPU = override;
      return NS_OK;
    }
  }

  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString oscpu;
  rv = service->GetOscpu(oscpu);
  CopyASCIItoUTF16(oscpu, aOSCPU);

  return rv;
}

NS_IMETHODIMP
Navigator::GetVendor(nsAString& aVendor)
{
  aVendor.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetVendorSub(nsAString& aVendorSub)
{
  aVendorSub.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetProduct(nsAString& aProduct)
{
  aProduct.AssignLiteral("Gecko");
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetProductSub(nsAString& aProductSub)
{
  
  aProductSub.AssignLiteral("20100101");
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetMimeTypes(nsISupports** aMimeTypes)
{
  if (!mMimeTypes) {
    NS_ENSURE_STATE(mWindow);
    nsWeakPtr win = do_GetWeakReference(mWindow);
    mMimeTypes = new nsMimeTypeArray(win);
  }

  NS_ADDREF(*aMimeTypes = mMimeTypes);

  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetPlugins(nsISupports** aPlugins)
{
  if (!mPlugins) {
    NS_ENSURE_STATE(mWindow);
    nsWeakPtr win = do_GetWeakReference(mWindow);
    mPlugins = new nsPluginArray(win);
    mPlugins->Init();
  }

  NS_ADDREF(*aPlugins = static_cast<nsIObserver*>(mPlugins.get()));

  return NS_OK;
}



#define COOKIE_BEHAVIOR_REJECT 2

NS_IMETHODIMP
Navigator::GetCookieEnabled(bool* aCookieEnabled)
{
  *aCookieEnabled =
    (Preferences::GetInt("network.cookie.cookieBehavior",
                         COOKIE_BEHAVIOR_REJECT) != COOKIE_BEHAVIOR_REJECT);

  
  
  
  if (!mWindow || !mWindow->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc = mWindow->GetExtantDoc();
  if (!doc) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> codebaseURI;
  doc->NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));

  if (!codebaseURI) {
    
    
    return NS_OK;
  }

  nsCOMPtr<nsICookiePermission> permMgr =
    do_GetService(NS_COOKIEPERMISSION_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, NS_OK);

  
  nsCookieAccess access;
  nsresult rv = permMgr->CanAccess(codebaseURI, nullptr, &access);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  if (access != nsICookiePermission::ACCESS_DEFAULT) {
    *aCookieEnabled = access != nsICookiePermission::ACCESS_DENY;
  }

  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetOnLine(bool* aOnline)
{
  NS_PRECONDITION(aOnline, "Null out param");

  *aOnline = !NS_IsOffline();
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetBuildID(nsAString& aBuildID)
{
  if (!nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      Preferences::GetString("general.buildID.override");

    if (override) {
      aBuildID = override;
      return NS_OK;
    }
  }

  nsCOMPtr<nsIXULAppInfo> appInfo =
    do_GetService("@mozilla.org/xre/app-info;1");
  if (!appInfo) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsAutoCString buildID;
  nsresult rv = appInfo->GetAppBuildID(buildID);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aBuildID.Truncate();
  AppendASCIItoUTF16(buildID, aBuildID);
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetDoNotTrack(nsAString &aResult)
{
  if (sDoNotTrackEnabled) {
    aResult.AssignLiteral("yes");
  } else {
    aResult.AssignLiteral("unspecified");
  }

  return NS_OK;
}

NS_IMETHODIMP
Navigator::JavaEnabled(bool* aReturn)
{
  Telemetry::AutoTimer<Telemetry::CHECK_JAVA_ENABLED> telemetryTimer;
  
  
  *aReturn = false;

  if (!mMimeTypes) {
    NS_ENSURE_STATE(mWindow);
    nsWeakPtr win = do_GetWeakReference(mWindow);
    mMimeTypes = new nsMimeTypeArray(win);
  }

  RefreshMIMEArray();

  nsMimeType *mimeType =
    mMimeTypes->NamedItem(NS_LITERAL_STRING("application/x-java-vm"));

  *aReturn = mimeType && mimeType->GetEnabledPlugin();

  return NS_OK;
}

NS_IMETHODIMP
Navigator::TaintEnabled(bool *aReturn)
{
  *aReturn = false;
  return NS_OK;
}

void
Navigator::RefreshMIMEArray()
{
  if (mMimeTypes) {
    mMimeTypes->Refresh();
  }
}

bool
Navigator::HasDesktopNotificationSupport()
{
  return Preferences::GetBool("notification.feature.enabled", false);
}

namespace {

class VibrateWindowListener : public nsIDOMEventListener
{
public:
  VibrateWindowListener(nsIDOMWindow* aWindow, nsIDocument* aDocument)
  {
    mWindow = do_GetWeakReference(aWindow);
    mDocument = do_GetWeakReference(aDocument);

    NS_NAMED_LITERAL_STRING(visibilitychange, "visibilitychange");
    aDocument->AddSystemEventListener(visibilitychange,
                                      this, 
                                      true, 
                                      false );
  }

  virtual ~VibrateWindowListener()
  {
  }

  void RemoveListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

private:
  nsWeakPtr mWindow;
  nsWeakPtr mDocument;
};

NS_IMPL_ISUPPORTS1(VibrateWindowListener, nsIDOMEventListener)

StaticRefPtr<VibrateWindowListener> gVibrateWindowListener;

NS_IMETHODIMP
VibrateWindowListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDocument> doc =
    do_QueryInterface(aEvent->InternalDOMEvent()->GetTarget());

  if (!doc || doc->Hidden()) {
    
    
    
    
    nsCOMPtr<nsIDOMWindow> window = do_QueryReferent(mWindow);
    hal::CancelVibrate(window);
    RemoveListener();
    gVibrateWindowListener = NULL;
    
  }

  return NS_OK;
}

void
VibrateWindowListener::RemoveListener()
{
  nsCOMPtr<EventTarget> target = do_QueryReferent(mDocument);
  if (!target) {
    return;
  }
  NS_NAMED_LITERAL_STRING(visibilitychange, "visibilitychange");
  target->RemoveSystemEventListener(visibilitychange, this,
                                    true );
}







bool
GetVibrationDurationFromJsval(const JS::Value& aJSVal, JSContext* cx,
                              int32_t* aOut)
{
  return JS_ValueToInt32(cx, aJSVal, aOut) &&
         *aOut >= 0 && static_cast<uint32_t>(*aOut) <= sMaxVibrateMS;
}

} 

NS_IMETHODIMP
Navigator::AddIdleObserver(nsIIdleObserver* aIdleObserver)
{
  NS_ENSURE_STATE(mWindow);

  if (!nsContentUtils::IsIdleObserverAPIEnabled()) {
    NS_WARNING("The IdleObserver API has been disabled.");
    return NS_OK;
  }

  NS_ENSURE_ARG_POINTER(aIdleObserver);

  if (!CheckPermission("idle")) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (NS_FAILED(mWindow->RegisterIdleObserver(aIdleObserver))) {
    NS_WARNING("Failed to add idle observer.");
  }

  return NS_OK;
}

NS_IMETHODIMP
Navigator::RemoveIdleObserver(nsIIdleObserver* aIdleObserver)
{
  NS_ENSURE_STATE(mWindow);

  if (!nsContentUtils::IsIdleObserverAPIEnabled()) {
    NS_WARNING("The IdleObserver API has been disabled");
    return NS_OK;
  }

  NS_ENSURE_ARG_POINTER(aIdleObserver);

  if (NS_FAILED(mWindow->UnregisterIdleObserver(aIdleObserver))) {
    NS_WARNING("Failed to remove idle observer.");
  }
  return NS_OK;
}

NS_IMETHODIMP
Navigator::Vibrate(const JS::Value& aPattern, JSContext* cx)
{
  NS_ENSURE_STATE(mWindow);

  nsCOMPtr<nsIDocument> doc = mWindow->GetExtantDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
  if (doc->Hidden()) {
    
    return NS_OK;
  }

  nsAutoTArray<uint32_t, 8> pattern;

  
  if (JSVAL_IS_NULL(aPattern) || JSVAL_IS_VOID(aPattern)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (JSVAL_IS_PRIMITIVE(aPattern)) {
    int32_t p;
    if (GetVibrationDurationFromJsval(aPattern, cx, &p)) {
      pattern.AppendElement(p);
    }
    else {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
  }
  else {
    JS::Rooted<JSObject*> obj(cx, aPattern.toObjectOrNull());
    uint32_t length;
    if (!JS_GetArrayLength(cx, obj, &length) || length > sMaxVibrateListLen) {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
    pattern.SetLength(length);

    for (uint32_t i = 0; i < length; ++i) {
      JS::Rooted<JS::Value> v(cx);
      int32_t pv;
      if (JS_GetElement(cx, obj, i, v.address()) &&
          GetVibrationDurationFromJsval(v, cx, &pv)) {
        pattern[i] = pv;
      }
      else {
        return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
      }
    }
  }

  
  
  if (!sVibratorEnabled) {
    return NS_OK;
  }

  
  

  if (!gVibrateWindowListener) {
    
    
    
    ClearOnShutdown(&gVibrateWindowListener);
  }
  else {
    gVibrateWindowListener->RemoveListener();
  }
  gVibrateWindowListener = new VibrateWindowListener(mWindow, doc);

  hal::Vibrate(pattern, mWindow);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::RegisterContentHandler(const nsAString& aMIMEType,
                                  const nsAString& aURI,
                                  const nsAString& aTitle)
{
  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar =
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (!registrar) {
    return NS_OK;
  }

  return registrar->RegisterContentHandler(aMIMEType, aURI, aTitle,
                                           mWindow->GetOuterWindow());
}

NS_IMETHODIMP
Navigator::RegisterProtocolHandler(const nsAString& aProtocol,
                                   const nsAString& aURI,
                                   const nsAString& aTitle)
{
  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar =
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (!registrar) {
    return NS_OK;
  }

  return registrar->RegisterProtocolHandler(aProtocol, aURI, aTitle,
                                            mWindow->GetOuterWindow());
}

NS_IMETHODIMP
Navigator::MozIsLocallyAvailable(const nsAString &aURI,
                                 bool aWhenOffline,
                                 bool* aIsAvailable)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool match;
  rv = uri->SchemeIs("http", &match);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!match) {
    rv = uri->SchemeIs("https", &match);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!match) {
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);

  rv = nsContentUtils::GetSecurityManager()->CheckSameOrigin(cx, uri);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  uint32_t loadFlags = nsIChannel::INHIBIT_CACHING |
                       nsICachingChannel::LOAD_NO_NETWORK_IO |
                       nsICachingChannel::LOAD_ONLY_IF_MODIFIED |
                       nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;

  if (aWhenOffline) {
    loadFlags |= nsICachingChannel::LOAD_CHECK_OFFLINE_CACHE |
                 nsICachingChannel::LOAD_ONLY_FROM_CACHE |
                 nsIRequest::LOAD_FROM_CACHE;
  }

  NS_ENSURE_STATE(mWindow);

  nsCOMPtr<nsILoadGroup> loadGroup;
  nsCOMPtr<nsIDocument> doc = mWindow->GetDoc();
  if (doc) {
    loadGroup = doc->GetDocumentLoadGroup();
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), uri,
                     nullptr, loadGroup, nullptr, loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  stream->Close();

  nsresult status;
  rv = channel->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if (NS_FAILED(status)) {
    *aIsAvailable = false;
    return NS_OK;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
  return httpChannel->GetRequestSucceeded(aIsAvailable);
}





NS_IMETHODIMP Navigator::GetDeviceStorage(const nsAString &aType, nsIDOMDeviceStorage** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

  if (!Preferences::GetBool("device.storage.enabled", false)) {
    return NS_OK;
  }

  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsDOMDeviceStorage> storage;
  nsDOMDeviceStorage::CreateDeviceStorageFor(mWindow, aType,
                                             getter_AddRefs(storage));

  if (!storage) {
    return NS_OK;
  }

  NS_ADDREF(*_retval = storage.get());
  mDeviceStorageStores.AppendElement(storage);
  return NS_OK;
}

NS_IMETHODIMP Navigator::GetDeviceStorages(const nsAString &aType, nsIVariant** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

  if (!Preferences::GetBool("device.storage.enabled", false)) {
    return NS_OK;
  }

  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return NS_ERROR_FAILURE;
  }

  nsTArray<nsRefPtr<nsDOMDeviceStorage> > stores;
  nsDOMDeviceStorage::CreateDeviceStoragesFor(mWindow, aType, stores, false);

  nsCOMPtr<nsIWritableVariant> result = do_CreateInstance("@mozilla.org/variant;1");
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  if (stores.Length() == 0) {
    result->SetAsEmptyArray();
  } else {
    result->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                       &NS_GET_IID(nsIDOMDeviceStorage),
                       stores.Length(),
                       const_cast<void*>(static_cast<const void*>(stores.Elements())));
  }
  result.forget(_retval);

  mDeviceStorageStores.AppendElements(stores);
  return NS_OK;
}





NS_IMETHODIMP Navigator::GetGeolocation(nsIDOMGeoGeolocation** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

  if (!Preferences::GetBool("geo.enabled", true)) {
    return NS_OK;
  }

  if (mGeolocation) {
    NS_ADDREF(*_retval = mGeolocation);
    return NS_OK;
  }

  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return NS_ERROR_FAILURE;
  }

  mGeolocation = new Geolocation();
  if (!mGeolocation) {
    return NS_ERROR_FAILURE;
  }

  if (NS_FAILED(mGeolocation->Init(mWindow->GetOuterWindow()))) {
    mGeolocation = nullptr;
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*_retval = mGeolocation);
  return NS_OK;
}




#ifdef MOZ_MEDIA_NAVIGATOR
NS_IMETHODIMP
Navigator::MozGetUserMedia(nsIMediaStreamOptions* aParams,
                           nsIDOMGetUserMediaSuccessCallback* aOnSuccess,
                           nsIDOMGetUserMediaErrorCallback* aOnError)
{
  
  if (!(Preferences::GetBool("media.navigator.enabled", false) ||
        Preferences::GetBool("media.peerconnection.enabled", false))) {
    return NS_OK;
  }

  if (!mWindow || !mWindow->GetOuterWindow() ||
      mWindow->GetOuterWindow()->GetCurrentInnerWindow() != mWindow) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  bool privileged = nsContentUtils::IsChromeDoc(mWindow->GetExtantDoc());

  MediaManager* manager = MediaManager::Get();
  return manager->GetUserMedia(privileged, mWindow, aParams, aOnSuccess,
                               aOnError);
}




NS_IMETHODIMP
Navigator::MozGetUserMediaDevices(nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
                                  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  if (!mWindow || !mWindow->GetOuterWindow() ||
      mWindow->GetOuterWindow()->GetCurrentInnerWindow() != mWindow) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_FAILURE;
  }

  MediaManager* manager = MediaManager::Get();
  return manager->GetUserMediaDevices(mWindow, aOnSuccess, aOnError);
}
#endif





NS_IMETHODIMP Navigator::GetMozNotification(nsISupports** aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  *aRetVal = nullptr;

  if (mNotification) {
    NS_ADDREF(*aRetVal = mNotification);
    return NS_OK;
  }

  NS_ENSURE_TRUE(mWindow && mWindow->GetDocShell(), NS_ERROR_FAILURE);

  mNotification = new DesktopNotificationCenter(mWindow);

  NS_ADDREF(*aRetVal = mNotification);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::GetBattery(nsISupports** aBattery)
{
  if (!mBatteryManager) {
    *aBattery = nullptr;

    NS_ENSURE_STATE(mWindow);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), NS_OK);

    mBatteryManager = new battery::BatteryManager();
    mBatteryManager->Init(mWindow);
  }

  NS_ADDREF(*aBattery = mBatteryManager);

  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetMozPower(nsIDOMMozPowerManager** aPower)
{
  *aPower = nullptr;

  if (!mPowerManager) {
    NS_ENSURE_STATE(mWindow);
    mPowerManager = PowerManager::CheckPermissionAndCreateInstance(mWindow);
    NS_ENSURE_TRUE(mPowerManager, NS_OK);
  }

  nsCOMPtr<nsIDOMMozPowerManager> power(mPowerManager);
  power.forget(aPower);

  return NS_OK;
}

NS_IMETHODIMP
Navigator::RequestWakeLock(const nsAString &aTopic, nsIDOMMozWakeLock **aWakeLock)
{
  NS_ENSURE_STATE(mWindow);

  *aWakeLock = nullptr;

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(pmService, NS_OK);

  return pmService->NewWakeLock(aTopic, mWindow, aWakeLock);
}





NS_IMETHODIMP
Navigator::GetMozSms(nsIDOMMozSmsManager** aSmsManager)
{
  *aSmsManager = nullptr;

  if (!mSmsManager) {
    NS_ENSURE_STATE(mWindow);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), NS_OK);

    mSmsManager = SmsManager::CreateInstanceIfAllowed(mWindow);
    NS_ENSURE_TRUE(mSmsManager, NS_OK);
  }

  NS_ADDREF(*aSmsManager = mSmsManager);

  return NS_OK;
}





NS_IMETHODIMP
Navigator::GetMozMobileMessage(nsIDOMMozMobileMessageManager** aMobileMessageManager)
{
  *aMobileMessageManager = nullptr;

#ifndef MOZ_WEBSMS_BACKEND
  return NS_OK;
#endif

  
  bool enabled = false;
  Preferences::GetBool("dom.sms.enabled", &enabled);
  NS_ENSURE_TRUE(enabled, NS_OK);

  if (!mMobileMessageManager) {
    NS_ENSURE_STATE(mWindow);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), NS_OK);

    if (!CheckPermission("sms")) {
      return NS_OK;
    }

    mMobileMessageManager = new MobileMessageManager();
    mMobileMessageManager->Init(mWindow);
  }

  NS_ADDREF(*aMobileMessageManager = mMobileMessageManager);

  return NS_OK;
}

#ifdef MOZ_B2G_RIL





NS_IMETHODIMP
Navigator::GetMozCellBroadcast(nsIDOMMozCellBroadcast** aCellBroadcast)
{
  *aCellBroadcast = nullptr;

  if (!mCellBroadcast) {
    NS_ENSURE_STATE(mWindow);

    if (!CheckPermission("cellbroadcast")) {
      return NS_OK;
    }

    nsresult rv = NS_NewCellBroadcast(mWindow, getter_AddRefs(mCellBroadcast));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aCellBroadcast = mCellBroadcast);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::GetMozTelephony(nsIDOMTelephony** aTelephony)
{
  nsCOMPtr<nsIDOMTelephony> telephony = mTelephony;

  if (!telephony) {
    NS_ENSURE_STATE(mWindow);
    nsresult rv = NS_NewTelephony(mWindow, getter_AddRefs(mTelephony));
    NS_ENSURE_SUCCESS(rv, rv);

    
    telephony = mTelephony;
  }

  telephony.forget(aTelephony);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::GetMozVoicemail(nsIDOMMozVoicemail** aVoicemail)
{
  *aVoicemail = nullptr;

  if (!mVoicemail) {
    NS_ENSURE_STATE(mWindow);
    if (!CheckPermission("voicemail")) {
      return NS_OK;
    }

    nsresult rv = NS_NewVoicemail(mWindow, getter_AddRefs(mVoicemail));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aVoicemail = mVoicemail);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::GetMozIccManager(nsIDOMMozIccManager** aIccManager)
{
  *aIccManager = nullptr;

  if (!mIccManager) {
    NS_ENSURE_STATE(mWindow);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), NS_OK);

    if (!CheckPermission("mobileconnection")) {
      return NS_OK;
    }

    mIccManager = new icc::IccManager();
    mIccManager->Init(mWindow);
  }

  NS_ADDREF(*aIccManager = mIccManager);
  return NS_OK;
}

#endif 

#ifdef MOZ_GAMEPAD




NS_IMETHODIMP
Navigator::GetGamepads(nsIVariant** aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  *aRetVal = nullptr;

  NS_ENSURE_STATE(mWindow);
  NS_ENSURE_TRUE(mWindow->GetDocShell(), NS_OK);
  nsGlobalWindow* win = static_cast<nsGlobalWindow*>(mWindow.get());

  nsAutoTArray<nsRefPtr<Gamepad>, 2> gamepads;
  win->GetGamepads(gamepads);

  nsRefPtr<nsVariant> out = new nsVariant();
  NS_ENSURE_STATE(out);

  if (gamepads.Length() == 0) {
    nsresult rv = out->SetAsEmptyArray();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    out->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                    &NS_GET_IID(nsISupports),
                    gamepads.Length(),
                    const_cast<void*>(static_cast<const void*>(gamepads.Elements())));
  }
  out.forget(aRetVal);

  return NS_OK;
}
#endif





NS_IMETHODIMP
Navigator::GetMozConnection(nsIDOMMozConnection** aConnection)
{
  *aConnection = nullptr;

  if (!mConnection) {
    NS_ENSURE_STATE(mWindow);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), NS_OK);

    mConnection = new network::Connection();
    mConnection->Init(mWindow);
  }

  NS_ADDREF(*aConnection = mConnection);
  return NS_OK;
}

#ifdef MOZ_B2G_RIL



NS_IMETHODIMP
Navigator::GetMozMobileConnection(nsIDOMMozMobileConnection** aMobileConnection)
{
  *aMobileConnection = nullptr;

  if (!mMobileConnection) {
    NS_ENSURE_STATE(mWindow);
    if (!CheckPermission("mobileconnection") &&
        !CheckPermission("mobilenetwork")) {
      return NS_OK;
    }

    mMobileConnection = new network::MobileConnection();
    mMobileConnection->Init(mWindow);
  }

  NS_ADDREF(*aMobileConnection = mMobileConnection);
  return NS_OK;
}
#endif 

#ifdef MOZ_B2G_BT




NS_IMETHODIMP
Navigator::GetMozBluetooth(nsIDOMBluetoothManager** aBluetooth)
{
  nsCOMPtr<nsIDOMBluetoothManager> bluetooth = mBluetooth;

  if (!bluetooth) {
    NS_ENSURE_STATE(mWindow);
    nsresult rv = NS_NewBluetoothManager(mWindow, getter_AddRefs(mBluetooth));
    NS_ENSURE_SUCCESS(rv, rv);

    bluetooth = mBluetooth;
  }

  bluetooth.forget(aBluetooth);
  return NS_OK;
}
#endif 




nsresult
Navigator::EnsureMessagesManager()
{
  if (mMessagesManager) {
    return NS_OK;
  }

  NS_ENSURE_STATE(mWindow);

  nsresult rv;
  nsCOMPtr<nsIDOMNavigatorSystemMessages> messageManager =
    do_CreateInstance("@mozilla.org/system-message-manager;1", &rv);
  
  nsCOMPtr<nsIDOMGlobalPropertyInitializer> gpi =
    do_QueryInterface(messageManager);
  NS_ENSURE_TRUE(gpi, NS_ERROR_FAILURE);

  
  AutoJSContext cx;
  JS::Rooted<JS::Value> prop_val(cx);
  rv = gpi->Init(mWindow, prop_val.address());
  NS_ENSURE_SUCCESS(rv, rv);

  mMessagesManager = messageManager.forget();

  return NS_OK;
}

NS_IMETHODIMP
Navigator::MozHasPendingMessage(const nsAString& aType, bool *aResult)
{
  if (!Preferences::GetBool("dom.sysmsg.enabled", false)) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  *aResult = false;
  nsresult rv = EnsureMessagesManager();
  NS_ENSURE_SUCCESS(rv, rv);

  return mMessagesManager->MozHasPendingMessage(aType, aResult);
}

NS_IMETHODIMP
Navigator::MozSetMessageHandler(const nsAString& aType,
                                nsIDOMSystemMessageCallback *aCallback)
{
  if (!Preferences::GetBool("dom.sysmsg.enabled", false)) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsresult rv = EnsureMessagesManager();
  NS_ENSURE_SUCCESS(rv, rv);

  return mMessagesManager->MozSetMessageHandler(aType, aCallback);
}




#ifdef MOZ_TIME_MANAGER
NS_IMETHODIMP
Navigator::GetMozTime(nsISupports** aTime)
{
  *aTime = nullptr;

  NS_ENSURE_STATE(mWindow);
  if (!CheckPermission("time")) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (!mTimeManager) {
    mTimeManager = new time::TimeManager(mWindow);
  }

  NS_ADDREF(*aTime = mTimeManager);
  return NS_OK;
}
#endif





NS_IMETHODIMP
Navigator::GetMozCameras(nsISupports** aCameraManager)
{
  if (!mCameraManager) {
    if (!mWindow ||
        !mWindow->GetOuterWindow() ||
        mWindow->GetOuterWindow()->GetCurrentInnerWindow() != mWindow) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    mCameraManager =
      nsDOMCameraManager::CheckPermissionAndCreateInstance(mWindow);
    NS_ENSURE_TRUE(mCameraManager, NS_OK);
  }

  nsCOMPtr<nsIObserver> cameraManager = mCameraManager.get();
  cameraManager.forget(aCameraManager);

  return NS_OK;
}

size_t
Navigator::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  
  
  
  

  return n;
}

void
Navigator::SetWindow(nsPIDOMWindow *aInnerWindow)
{
  NS_ASSERTION(aInnerWindow->IsInnerWindow(),
               "Navigator must get an inner window!");
  mWindow = aInnerWindow;
}

void
Navigator::OnNavigation()
{
  if (!mWindow) {
    return;
  }

#ifdef MOZ_MEDIA_NAVIGATOR
  
  MediaManager *manager = MediaManager::Get();
  if (manager) {
    manager->OnNavigation(mWindow->WindowID());
  }
#endif
  if (mCameraManager) {
    mCameraManager->OnNavigation(mWindow->WindowID());
  }
}

bool
Navigator::CheckPermission(const char* type)
{
  if (!mWindow) {
    return false;
  }

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromWindow(mWindow, type, &permission);
  return permission == nsIPermissionManager::ALLOW_ACTION;
}




#ifdef MOZ_AUDIO_CHANNEL_MANAGER
NS_IMETHODIMP
Navigator::GetMozAudioChannelManager(nsISupports** aAudioChannelManager)
{
  *aAudioChannelManager = nullptr;

  if (!mAudioChannelManager) {
    NS_ENSURE_STATE(mWindow);
    mAudioChannelManager = new system::AudioChannelManager();
    mAudioChannelManager->Init(mWindow);
  }

  NS_ADDREF(*aAudioChannelManager = mAudioChannelManager);
  return NS_OK;
}
#endif

} 
} 

nsresult
NS_GetNavigatorUserAgent(nsAString& aUserAgent)
{
  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString ua;
  rv = service->GetUserAgent(ua);
  CopyASCIItoUTF16(ua, aUserAgent);

  return rv;
}

nsresult
NS_GetNavigatorPlatform(nsAString& aPlatform)
{
  if (!nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      mozilla::Preferences::GetString("general.platform.override");

    if (override) {
      aPlatform = override;
      return NS_OK;
    }
  }

  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
#if defined(_WIN64)
  aPlatform.AssignLiteral("Win64");
#elif defined(WIN32)
  aPlatform.AssignLiteral("Win32");
#elif defined(XP_MACOSX) && defined(__ppc__)
  aPlatform.AssignLiteral("MacPPC");
#elif defined(XP_MACOSX) && defined(__i386__)
  aPlatform.AssignLiteral("MacIntel");
#elif defined(XP_MACOSX) && defined(__x86_64__)
  aPlatform.AssignLiteral("MacIntel");
#elif defined(XP_OS2)
  aPlatform.AssignLiteral("OS/2");
#else
  
  
  
  nsAutoCString plat;
  rv = service->GetOscpu(plat);
  CopyASCIItoUTF16(plat, aPlatform);
#endif

  return rv;
}
nsresult
NS_GetNavigatorAppVersion(nsAString& aAppVersion)
{
  if (!nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      mozilla::Preferences::GetString("general.appversion.override");

    if (override) {
      aAppVersion = override;
      return NS_OK;
    }
  }

  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString str;
  rv = service->GetAppVersion(str);
  CopyASCIItoUTF16(str, aAppVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  aAppVersion.AppendLiteral(" (");

  rv = service->GetPlatform(str);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendASCIItoUTF16(str, aAppVersion);
  aAppVersion.Append(PRUnichar(')'));

  return rv;
}

nsresult
NS_GetNavigatorAppName(nsAString& aAppName)
{
  if (!nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      mozilla::Preferences::GetString("general.appname.override");

    if (override) {
      aAppName = override;
      return NS_OK;
    }
  }

  aAppName.AssignLiteral("Netscape");
  return NS_OK;
}
