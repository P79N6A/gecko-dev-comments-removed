






#include "base/basictypes.h"

#include "Navigator.h"
#include "nsIXULAppInfo.h"
#include "nsPluginArray.h"
#include "nsMimeTypeArray.h"
#include "nsDesktopNotification.h"
#include "nsGeolocation.h"
#include "nsIHttpProtocolHandler.h"
#include "nsICachingChannel.h"
#include "nsIDocShell.h"
#include "nsIWebContentHandlerRegistrar.h"
#include "nsICookiePermission.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSContextStack.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "BatteryManager.h"
#include "PowerManager.h"
#include "nsIDOMWakeLock.h"
#include "nsIPowerManagerService.h"
#include "SmsManager.h"
#include "nsISmsService.h"
#include "mozilla/Hal.h"
#include "nsIWebNavigation.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "Connection.h"
#include "MobileConnection.h"
#include "nsIIdleObserver.h"
#include "nsIPermissionManager.h"

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

#include "nsIDOMGlobalPropertyInitializer.h"

using namespace mozilla::dom::power;
using namespace mozilla::dom::sms;


DOMCI_DATA(Navigator, mozilla::dom::Navigator)

namespace mozilla {
namespace dom {

static const char sJSStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";

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
  : mWindow(do_GetWeakReference(aWindow))
{
  NS_ASSERTION(aWindow->IsInnerWindow(),
               "Navigator must get an inner window!");
}

Navigator::~Navigator()
{
  Invalidate();
}

NS_INTERFACE_MAP_BEGIN(Navigator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMClientInformation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorDeviceStorage)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsINavigatorBattery)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorDesktopNotification)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozNavigatorSms)
#ifdef MOZ_MEDIA_NAVIGATOR
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorUserMedia)
#endif
#ifdef MOZ_B2G_RIL
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorTelephony)
#endif
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozNavigatorNetwork)
#ifdef MOZ_B2G_BT
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorBluetooth)
#endif
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorCamera)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigatorSystemMessages)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Navigator)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(Navigator)
NS_IMPL_RELEASE(Navigator)

void
Navigator::Invalidate()
{
  mWindow = nullptr;

  if (mPlugins) {
    mPlugins->Invalidate();
    mPlugins = nullptr;
  }

  
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

  if (mMobileConnection) {
    mMobileConnection->Shutdown();
    mMobileConnection = nullptr;
  }

#ifdef MOZ_B2G_BT
  if (mBluetooth) {
    mBluetooth = nullptr;
  }
#endif

  mCameraManager = nullptr;

#ifdef MOZ_SYS_MSG
  if (mMessagesManager) {
    mMessagesManager = nullptr;
  }
#endif

  uint32_t len = mDeviceStorageStores.Length();
  for (uint32_t i = 0; i < len; ++i) {
    mDeviceStorageStores[i]->Shutdown();
  }
  mDeviceStorageStores.Clear();

}

nsPIDOMWindow *
Navigator::GetWindow()
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

  return win;
}






NS_IMETHODIMP
Navigator::GetUserAgent(nsAString& aUserAgent)
{
  return NS_GetNavigatorUserAgent(aUserAgent);
}

NS_IMETHODIMP
Navigator::GetAppCodeName(nsAString& aAppCodeName)
{
  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString appName;
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
  if (!nsContentUtils::IsCallerTrustedForRead()) {
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

  nsCAutoString oscpu;
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
Navigator::GetMimeTypes(nsIDOMMimeTypeArray** aMimeTypes)
{
  if (!mMimeTypes) {
    mMimeTypes = new nsMimeTypeArray(this);
  }

  NS_ADDREF(*aMimeTypes = mMimeTypes);

  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetPlugins(nsIDOMPluginArray** aPlugins)
{
  if (!mPlugins) {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

    mPlugins = new nsPluginArray(this, win ? win->GetDocShell() : nullptr);
  }

  NS_ADDREF(*aPlugins = mPlugins);

  return NS_OK;
}



#define COOKIE_BEHAVIOR_REJECT 2

NS_IMETHODIMP
Navigator::GetCookieEnabled(bool* aCookieEnabled)
{
  *aCookieEnabled =
    (Preferences::GetInt("network.cookie.cookieBehavior",
                         COOKIE_BEHAVIOR_REJECT) != COOKIE_BEHAVIOR_REJECT);

  
  
  
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

  if (!win || !win->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(win->GetExtantDocument());
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
  if (!nsContentUtils::IsCallerTrustedForRead()) {
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

  nsCAutoString buildID;
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
    mMimeTypes = new nsMimeTypeArray(this);
  }

  RefreshMIMEArray();

  uint32_t count;
  mMimeTypes->GetLength(&count);
  for (uint32_t i = 0; i < count; i++) {
    nsresult rv;
    nsIDOMMimeType* type = mMimeTypes->GetItemAt(i, &rv);

    if (NS_FAILED(rv) || !type) {
      continue;
    }

    nsAutoString mimeString;
    if (NS_FAILED(type->GetType(mimeString))) {
      continue;
    }

    if (mimeString.EqualsLiteral("application/x-java-vm")) {
      *aReturn = true;
      break;
    }
  }

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
  VibrateWindowListener(nsIDOMWindow *aWindow, nsIDOMDocument *aDocument)
  {
    mWindow = do_GetWeakReference(aWindow);
    mDocument = do_GetWeakReference(aDocument);

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(aDocument);
    NS_NAMED_LITERAL_STRING(visibilitychange, "mozvisibilitychange");
    target->AddSystemEventListener(visibilitychange,
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
  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));
  nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(target);

  bool hidden = true;
  if (doc) {
    doc->GetMozHidden(&hidden);
  }

  if (hidden) {
    
    
    
    
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
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryReferent(mDocument);
  if (!target) {
    return;
  }
  NS_NAMED_LITERAL_STRING(visibilitychange, "mozvisibilitychange");
  target->RemoveSystemEventListener(visibilitychange, this,
                                    true );
}







bool
GetVibrationDurationFromJsval(const jsval& aJSVal, JSContext* cx,
                              int32_t* aOut)
{
  return JS_ValueToInt32(cx, aJSVal, aOut) &&
         *aOut >= 0 && static_cast<uint32_t>(*aOut) <= sMaxVibrateMS;
}

} 

NS_IMETHODIMP
Navigator::AddIdleObserver(nsIIdleObserver* aIdleObserver)
{
  if (!nsContentUtils::IsIdleObserverAPIEnabled()) {
    NS_WARNING("The IdleObserver API has been disabled.");
    return NS_OK;
  }

  NS_ENSURE_ARG_POINTER(aIdleObserver);

  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(win, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDocument> doc = win->GetExtantDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsIPrincipal* principal = doc->NodePrincipal();
  if (!nsContentUtils::IsSystemPrincipal(principal)) {
    uint16_t appStatus = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
    principal->GetAppStatus(&appStatus);
    if (appStatus != nsIPrincipal::APP_STATUS_CERTIFIED) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
  }

  if (NS_FAILED(win->RegisterIdleObserver(aIdleObserver))) {
    NS_WARNING("Failed to add idle observer.");
  }

  return NS_OK;
}

NS_IMETHODIMP
Navigator::RemoveIdleObserver(nsIIdleObserver* aIdleObserver)
{
  if (!nsContentUtils::IsIdleObserverAPIEnabled()) {
    NS_WARNING("The IdleObserver API has been disabled");
    return NS_OK;
  }

  NS_ENSURE_ARG_POINTER(aIdleObserver);

  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(win, NS_ERROR_UNEXPECTED);
  if (NS_FAILED(win->UnregisterIdleObserver(aIdleObserver))) {
    NS_WARNING("Failed to remove idle observer.");
  }
  return NS_OK;
}

NS_IMETHODIMP
Navigator::Vibrate(const jsval& aPattern, JSContext* cx)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(win, NS_OK);

  nsIDOMDocument* domDoc = win->GetExtantDocument();
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  bool hidden = true;
  domDoc->GetMozHidden(&hidden);
  if (hidden) {
    
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
    JSObject *obj = JSVAL_TO_OBJECT(aPattern);
    uint32_t length;
    if (!JS_GetArrayLength(cx, obj, &length) || length > sMaxVibrateListLen) {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
    pattern.SetLength(length);

    for (uint32_t i = 0; i < length; ++i) {
      jsval v;
      int32_t pv;
      if (JS_GetElement(cx, obj, i, &v) &&
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
  gVibrateWindowListener = new VibrateWindowListener(win, domDoc);

  nsCOMPtr<nsIDOMWindow> domWindow =
    do_QueryInterface(static_cast<nsIDOMWindow*>(win));
  hal::Vibrate(pattern, domWindow);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::RegisterContentHandler(const nsAString& aMIMEType,
                                  const nsAString& aURI,
                                  const nsAString& aTitle)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

  if (!win || !win->GetOuterWindow() || !win->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar =
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (!registrar) {
    return NS_OK;
  }

  return registrar->RegisterContentHandler(aMIMEType, aURI, aTitle,
                                           win->GetOuterWindow());
}

NS_IMETHODIMP
Navigator::RegisterProtocolHandler(const nsAString& aProtocol,
                                   const nsAString& aURI,
                                   const nsAString& aTitle)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

  if (!win || !win->GetOuterWindow() || !win->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar =
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (!registrar) {
    return NS_OK;
  }

  return registrar->RegisterProtocolHandler(aProtocol, aURI, aTitle,
                                            win->GetOuterWindow());
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

  
  nsCOMPtr<nsIJSContextStack> stack = do_GetService(sJSStackContractID);
  NS_ENSURE_TRUE(stack, NS_ERROR_FAILURE);

  JSContext* cx = nullptr;
  rv = stack->Peek(&cx);
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

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), uri,
                     nullptr, nullptr, nullptr, loadFlags);
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
  if (!Preferences::GetBool("device.storage.enabled", false)) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

  if (!win || !win->GetOuterWindow() || !win->GetDocShell()) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsDOMDeviceStorage> storage;
  nsDOMDeviceStorage::CreateDeviceStoragesFor(win, aType, getter_AddRefs(storage));

  if (!storage) {
    return NS_OK;
  }

  NS_ADDREF(*_retval = storage.get());
  mDeviceStorageStores.AppendElement(storage);                                                                                                                                                                                              
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

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));

  if (!win || !win->GetOuterWindow() || !win->GetDocShell()) {
    return NS_ERROR_FAILURE;
  }

  mGeolocation = new nsGeolocation();
  if (!mGeolocation) {
    return NS_ERROR_FAILURE;
  }

  if (NS_FAILED(mGeolocation->Init(win->GetOuterWindow()))) {
    mGeolocation = nullptr;
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*_retval = mGeolocation);
  return NS_OK;
}




#ifdef MOZ_MEDIA_NAVIGATOR
NS_IMETHODIMP
Navigator::MozGetUserMedia(nsIMediaStreamOptions* aParams,
                           nsIDOMGetUserMediaSuccessCallback* onSuccess,
                           nsIDOMGetUserMediaErrorCallback* onError)
{
  if (!Preferences::GetBool("media.navigator.enabled", false)) {
    return NS_OK;
  }

  MediaManager *manager = MediaManager::Get();
  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);

  if (!win || !win->GetOuterWindow() ||
      win->GetOuterWindow()->GetCurrentInnerWindow() != win) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return manager->GetUserMedia(win, aParams, onSuccess, onError);
}
#endif





NS_IMETHODIMP Navigator::GetMozNotification(nsIDOMDesktopNotificationCenter** aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  *aRetVal = nullptr;

  if (mNotification) {
    NS_ADDREF(*aRetVal = mNotification);
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));
  NS_ENSURE_TRUE(win && win->GetDocShell(), NS_ERROR_FAILURE);

  mNotification = new nsDesktopNotificationCenter(win);

  NS_ADDREF(*aRetVal = mNotification);
  return NS_OK;
}





NS_IMETHODIMP
Navigator::GetBattery(nsIDOMBatteryManager** aBattery)
{
  if (!mBatteryManager) {
    *aBattery = nullptr;

    nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mWindow));
    NS_ENSURE_TRUE(win && win->GetDocShell(), NS_OK);

    mBatteryManager = new battery::BatteryManager();
    mBatteryManager->Init(win);
  }

  NS_ADDREF(*aBattery = mBatteryManager);

  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetMozPower(nsIDOMMozPowerManager** aPower)
{
  *aPower = nullptr;

  if (!mPowerManager) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window, NS_OK);

    mPowerManager = PowerManager::CheckPermissionAndCreateInstance(window);
    NS_ENSURE_TRUE(mPowerManager, NS_OK);
  }

  nsCOMPtr<nsIDOMMozPowerManager> power(mPowerManager);
  power.forget(aPower);

  return NS_OK;
}

NS_IMETHODIMP
Navigator::RequestWakeLock(const nsAString &aTopic, nsIDOMMozWakeLock **aWakeLock)
{
  *aWakeLock = nullptr;

  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(win, NS_OK);

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(pmService, NS_OK);

  return pmService->NewWakeLock(aTopic, win, aWakeLock);
}





bool
Navigator::IsSmsSupported() const
{
#ifdef MOZ_WEBSMS_BACKEND
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, false);

  bool result = false;
  smsService->HasSupport(&result);

  return result;
#else
  return false;
#endif
}

NS_IMETHODIMP
Navigator::GetMozSms(nsIDOMMozSmsManager** aSmsManager)
{
  *aSmsManager = nullptr;

  if (!mSmsManager) {
    if (!IsSmsSupported()) {
      return NS_OK;
    }

    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window && window->GetDocShell(), NS_OK);

    mSmsManager = SmsManager::CheckPermissionAndCreateInstance(window);
    NS_ENSURE_TRUE(mSmsManager, NS_OK);
  }

  NS_ADDREF(*aSmsManager = mSmsManager);

  return NS_OK;
}

#ifdef MOZ_B2G_RIL





NS_IMETHODIMP
Navigator::GetMozTelephony(nsIDOMTelephony** aTelephony)
{
  nsCOMPtr<nsIDOMTelephony> telephony = mTelephony;

  if (!telephony) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

    nsresult rv = NS_NewTelephony(window, getter_AddRefs(mTelephony));
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
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window && window->GetDocShell(), NS_OK);

    
    
    if (!nsContentUtils::IsCallerChrome()) {
      nsCOMPtr<nsIDocument> doc = do_QueryInterface(window->GetExtantDocument());
      NS_ENSURE_TRUE(doc, NS_OK);

      nsCOMPtr<nsIURI> uri;
      doc->NodePrincipal()->GetURI(getter_AddRefs(uri));

      if (!nsContentUtils::URIIsChromeOrInPref(uri, "dom.voicemail.whitelist")) {
        return NS_OK;
      }
    }

    nsresult rv = NS_NewVoicemail(window, getter_AddRefs(mVoicemail));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aVoicemail = mVoicemail);
  return NS_OK;
}

#endif 





NS_IMETHODIMP
Navigator::GetMozConnection(nsIDOMMozConnection** aConnection)
{
  *aConnection = nullptr;

  if (!mConnection) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window && window->GetDocShell(), NS_OK);

    mConnection = new network::Connection();
    mConnection->Init(window);
  }

  NS_ADDREF(*aConnection = mConnection);
  return NS_OK;
}

NS_IMETHODIMP
Navigator::GetMozMobileConnection(nsIDOMMozMobileConnection** aMobileConnection)
{
  *aMobileConnection = nullptr;

  if (!mMobileConnection) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window, NS_OK);

    nsCOMPtr<nsIDocument> document = do_QueryInterface(window->GetExtantDocument());
    NS_ENSURE_TRUE(document, NS_OK);
    nsCOMPtr<nsIPrincipal> principal = document->NodePrincipal();
    nsCOMPtr<nsIPermissionManager> permMgr =
      do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
    NS_ENSURE_TRUE(permMgr, NS_OK);

    uint32_t permission = nsIPermissionManager::DENY_ACTION;
    permMgr->TestPermissionFromPrincipal(principal, "mobileconnection", &permission);

    if (permission != nsIPermissionManager::ALLOW_ACTION) {
      return NS_OK;
    }

    mMobileConnection = new network::MobileConnection();
    mMobileConnection->Init(window);
  }

  NS_ADDREF(*aMobileConnection = mMobileConnection);
  return NS_OK;
}

#ifdef MOZ_B2G_BT




NS_IMETHODIMP
Navigator::GetMozBluetooth(nsIDOMBluetoothManager** aBluetooth)
{
  nsCOMPtr<nsIDOMBluetoothManager> bluetooth = mBluetooth;

  if (!bluetooth) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

    nsresult rv = NS_NewBluetoothManager(window, getter_AddRefs(mBluetooth));
    NS_ENSURE_SUCCESS(rv, rv);

    bluetooth = mBluetooth;
  }

  bluetooth.forget(aBluetooth);
  return NS_OK;
}
#endif 




#ifdef MOZ_SYS_MSG
nsresult
Navigator::EnsureMessagesManager()
{
  if (mMessagesManager) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

  nsresult rv;
  nsCOMPtr<nsIDOMNavigatorSystemMessages> messageManager =
    do_CreateInstance("@mozilla.org/system-message-manager;1", &rv);
  
  nsCOMPtr<nsIDOMGlobalPropertyInitializer> gpi =
    do_QueryInterface(messageManager);
  NS_ENSURE_TRUE(gpi, NS_ERROR_FAILURE);

  
  jsval prop_val = JSVAL_VOID;
  rv = gpi->Init(window, &prop_val);
  NS_ENSURE_SUCCESS(rv, rv);

  mMessagesManager = messageManager.forget();

  return NS_OK;
}
#endif

NS_IMETHODIMP
Navigator::MozHasPendingMessage(const nsAString& aType, bool *aResult)
{
#ifdef MOZ_SYS_MSG
  *aResult = false;
  nsresult rv = EnsureMessagesManager();
  NS_ENSURE_SUCCESS(rv, rv);

  return mMessagesManager->MozHasPendingMessage(aType, aResult);
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
Navigator::MozSetMessageHandler(const nsAString& aType,
                                nsIDOMSystemMessageCallback *aCallback)
{
#ifdef MOZ_SYS_MSG
  nsresult rv = EnsureMessagesManager();
  NS_ENSURE_SUCCESS(rv, rv);

  return mMessagesManager->MozSetMessageHandler(aType, aCallback);
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}





NS_IMETHODIMP
Navigator::GetMozCameras(nsIDOMCameraManager** aCameraManager)
{
  if (!mCameraManager) {
    nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);
    NS_ENSURE_TRUE(win, NS_ERROR_FAILURE);

    if (!win->GetOuterWindow() || win->GetOuterWindow()->GetCurrentInnerWindow() != win) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    mCameraManager = nsDOMCameraManager::Create(win->WindowID());
  }

  nsRefPtr<nsDOMCameraManager> cameraManager = mCameraManager;
  cameraManager.forget(aCameraManager);

  return NS_OK;
}

size_t
Navigator::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  
  
  
  

  return n;
}

void
Navigator::SetWindow(nsPIDOMWindow *aInnerWindow)
{
  NS_ASSERTION(aInnerWindow->IsInnerWindow(),
               "Navigator must get an inner window!");
  mWindow = do_GetWeakReference(aInnerWindow);
}

void
Navigator::OnNavigation()
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mWindow);
  if (!win) {
    return;
  }

#ifdef MOZ_MEDIA_NAVIGATOR
  
  MediaManager *manager = MediaManager::Get();
  manager->OnNavigation(win->WindowID());
#endif
  if (mCameraManager) {
    mCameraManager->OnNavigation(win->WindowID());
  }
}

} 
} 

nsresult
NS_GetNavigatorUserAgent(nsAString& aUserAgent)
{
  nsresult rv;

  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString ua;
  rv = service->GetUserAgent(ua);
  CopyASCIItoUTF16(ua, aUserAgent);

  return rv;
}

nsresult
NS_GetNavigatorPlatform(nsAString& aPlatform)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
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
  
  
  
  nsCAutoString plat;
  rv = service->GetOscpu(plat);
  CopyASCIItoUTF16(plat, aPlatform);
#endif

  return rv;
}
nsresult
NS_GetNavigatorAppVersion(nsAString& aAppVersion)
{
  if (!nsContentUtils::IsCallerTrustedForRead()) {
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

  nsCAutoString str;
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
  if (!nsContentUtils::IsCallerTrustedForRead()) {
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
