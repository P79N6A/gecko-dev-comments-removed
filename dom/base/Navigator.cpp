






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
#include "nsIWebContentHandlerRegistrar.h"
#include "nsICookiePermission.h"
#include "nsIScriptSecurityManager.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "BatteryManager.h"
#include "mozilla/dom/PowerManager.h"
#include "mozilla/dom/WakeLock.h"
#include "mozilla/dom/power/PowerManagerService.h"
#include "mozilla/dom/MobileMessageManager.h"
#include "mozilla/dom/Telephony.h"
#include "mozilla/Hal.h"
#include "nsISiteSpecificUserAgent.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "Connection.h"
#include "nsDOMEvent.h"
#include "nsGlobalWindow.h"
#ifdef MOZ_B2G_RIL
#include "mozilla/dom/IccManager.h"
#include "mozilla/dom/CellBroadcast.h"
#include "mozilla/dom/MobileConnectionArray.h"
#include "mozilla/dom/Voicemail.h"
#endif
#include "nsIIdleObserver.h"
#include "nsIPermissionManager.h"
#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "TimeManager.h"
#include "DeviceStorage.h"
#include "nsIDOMNavigatorSystemMessages.h"
#include "nsIAppsService.h"
#include "mozIApplication.h"
#include "WidgetUtils.h"

#ifdef MOZ_MEDIA_NAVIGATOR
#include "MediaManager.h"
#endif
#ifdef MOZ_B2G_BT
#include "BluetoothManager.h"
#endif
#include "DOMCameraManager.h"

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
#include "AudioChannelManager.h"
#endif

#ifdef MOZ_B2G_FM
#include "mozilla/dom/FMRadio.h"
#endif

#include "nsIDOMGlobalPropertyInitializer.h"
#include "nsIDataStoreService.h"
#include "nsJSUtils.h"

#include "nsScriptNameSpaceManager.h"

#include "mozilla/dom/NavigatorBinding.h"
#include "mozilla/dom/Promise.h"

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
  MOZ_ASSERT(aWindow->IsInnerWindow(), "Navigator must get an inner window!");
  SetIsDOMBinding();
}

Navigator::~Navigator()
{
  Invalidate();
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Navigator)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigator)
  NS_INTERFACE_MAP_ENTRY(nsIMozNavigatorNetwork)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Navigator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Navigator)

NS_IMPL_CYCLE_COLLECTION_CLASS(Navigator)

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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPowerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileMessageManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTelephony)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mConnection)
#ifdef MOZ_B2G_RIL
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileConnections)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCellBroadcast)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIccManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mVoicemail)
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

#ifdef MOZ_B2G_FM
  if (mFMRadio) {
    mFMRadio->Shutdown();
    mFMRadio = nullptr;
  }
#endif

  if (mPowerManager) {
    mPowerManager->Shutdown();
    mPowerManager = nullptr;
  }

  if (mMobileMessageManager) {
    mMobileMessageManager->Shutdown();
    mMobileMessageManager = nullptr;
  }

  if (mTelephony) {
    mTelephony = nullptr;
  }

  if (mConnection) {
    mConnection->Shutdown();
    mConnection = nullptr;
  }

#ifdef MOZ_B2G_RIL
  if (mMobileConnections) {
    mMobileConnections = nullptr;
  }

  if (mCellBroadcast) {
    mCellBroadcast = nullptr;
  }

  if (mIccManager) {
    mIccManager->Shutdown();
    mIccManager = nullptr;
  }

  if (mVoicemail) {
    mVoicemail = nullptr;
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
  NS_GetNavigatorAppName(aAppName);
  return NS_OK;
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

  
  
  if (aLanguage.Length() > 2 && aLanguage[2] == char16_t('_')) {
    aLanguage.Replace(2, 1, char16_t('-')); 
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

nsMimeTypeArray*
Navigator::GetMimeTypes(ErrorResult& aRv)
{
  if (!mMimeTypes) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mMimeTypes = new nsMimeTypeArray(mWindow);
  }

  return mMimeTypes;
}

nsPluginArray*
Navigator::GetPlugins(ErrorResult& aRv)
{
  if (!mPlugins) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mPlugins = new nsPluginArray(mWindow);
    mPlugins->Init();
  }

  return mPlugins;
}



#define COOKIE_BEHAVIOR_REJECT 2

bool
Navigator::CookieEnabled()
{
  bool cookieEnabled =
    (Preferences::GetInt("network.cookie.cookieBehavior",
                         COOKIE_BEHAVIOR_REJECT) != COOKIE_BEHAVIOR_REJECT);

  
  
  
  if (!mWindow || !mWindow->GetDocShell()) {
    return cookieEnabled;
  }

  nsCOMPtr<nsIDocument> doc = mWindow->GetExtantDoc();
  if (!doc) {
    return cookieEnabled;
  }

  nsCOMPtr<nsIURI> codebaseURI;
  doc->NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));

  if (!codebaseURI) {
    
    
    return cookieEnabled;
  }

  nsCOMPtr<nsICookiePermission> permMgr =
    do_GetService(NS_COOKIEPERMISSION_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, cookieEnabled);

  
  nsCookieAccess access;
  nsresult rv = permMgr->CanAccess(codebaseURI, nullptr, &access);
  NS_ENSURE_SUCCESS(rv, cookieEnabled);

  if (access != nsICookiePermission::ACCESS_DEFAULT) {
    cookieEnabled = access != nsICookiePermission::ACCESS_DENY;
  }

  return cookieEnabled;
}

bool
Navigator::OnLine()
{
  return !NS_IsOffline();
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

bool
Navigator::JavaEnabled(ErrorResult& aRv)
{
  Telemetry::AutoTimer<Telemetry::CHECK_JAVA_ENABLED> telemetryTimer;
  
  
  if (!mMimeTypes) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return false;
    }
    mMimeTypes = new nsMimeTypeArray(mWindow);
  }

  RefreshMIMEArray();

  nsMimeType *mimeType =
    mMimeTypes->NamedItem(NS_LITERAL_STRING("application/x-java-vm"));

  return mimeType && mimeType->GetEnabledPlugin();
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
    gVibrateWindowListener = nullptr;
    
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

} 

void
Navigator::AddIdleObserver(MozIdleObserver& aIdleObserver, ErrorResult& aRv)
{
  if (!mWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
  CallbackObjectHolder<MozIdleObserver, nsIIdleObserver> holder(&aIdleObserver);
  nsCOMPtr<nsIIdleObserver> obs = holder.ToXPCOMCallback();
  if (NS_FAILED(mWindow->RegisterIdleObserver(obs))) {
    NS_WARNING("Failed to add idle observer.");
  }
}

void
Navigator::RemoveIdleObserver(MozIdleObserver& aIdleObserver, ErrorResult& aRv)
{
  if (!mWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
  CallbackObjectHolder<MozIdleObserver, nsIIdleObserver> holder(&aIdleObserver);
  nsCOMPtr<nsIIdleObserver> obs = holder.ToXPCOMCallback();
  if (NS_FAILED(mWindow->UnregisterIdleObserver(obs))) {
    NS_WARNING("Failed to remove idle observer.");
  }
}

bool
Navigator::Vibrate(uint32_t aDuration)
{
  nsAutoTArray<uint32_t, 1> pattern;
  pattern.AppendElement(aDuration);
  return Vibrate(pattern);
}

bool
Navigator::Vibrate(const nsTArray<uint32_t>& aPattern)
{
  if (!mWindow) {
    return false;
  }

  nsCOMPtr<nsIDocument> doc = mWindow->GetExtantDoc();
  if (!doc) {
    return false;
  }

  if (doc->Hidden()) {
    
    return false;
  }

  if (aPattern.Length() > sMaxVibrateListLen) {
    return false;
  }

  for (size_t i = 0; i < aPattern.Length(); ++i) {
    if (aPattern[i] > sMaxVibrateMS) {
      return false;
    }
  }

  
  
  if (aPattern.IsEmpty() || !sVibratorEnabled) {
    return true;
  }

  
  

  if (!gVibrateWindowListener) {
    
    
    
    ClearOnShutdown(&gVibrateWindowListener);
  }
  else {
    gVibrateWindowListener->RemoveListener();
  }
  gVibrateWindowListener = new VibrateWindowListener(mWindow, doc);

  hal::Vibrate(aPattern, mWindow);
  return true;
}





uint32_t
Navigator::MaxTouchPoints()
{
  nsCOMPtr<nsIWidget> widget = widget::WidgetUtils::DOMWindowToWidget(mWindow);

  NS_ENSURE_TRUE(widget, 0);
  return widget->GetMaxTouchPoints();
}





void
Navigator::RegisterContentHandler(const nsAString& aMIMEType,
                                  const nsAString& aURI,
                                  const nsAString& aTitle,
                                  ErrorResult& aRv)
{
  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return;
  }

  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar =
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (!registrar) {
    return;
  }

  aRv = registrar->RegisterContentHandler(aMIMEType, aURI, aTitle,
                                          mWindow->GetOuterWindow());
}

void
Navigator::RegisterProtocolHandler(const nsAString& aProtocol,
                                   const nsAString& aURI,
                                   const nsAString& aTitle,
                                   ErrorResult& aRv)
{
  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    return;
  }

  nsCOMPtr<nsIWebContentHandlerRegistrar> registrar =
    do_GetService(NS_WEBCONTENTHANDLERREGISTRAR_CONTRACTID);
  if (!registrar) {
    return;
  }

  aRv = registrar->RegisterProtocolHandler(aProtocol, aURI, aTitle,
                                           mWindow->GetOuterWindow());
}

bool
Navigator::MozIsLocallyAvailable(const nsAString &aURI,
                                 bool aWhenOffline,
                                 ErrorResult& aRv)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  
  bool match;
  rv = uri->SchemeIs("http", &match);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  if (!match) {
    rv = uri->SchemeIs("https", &match);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return false;
    }
    if (!match) {
      aRv.Throw(NS_ERROR_DOM_BAD_URI);
      return false;
    }
  }

  
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (!cx) {
    aRv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  rv = nsContentUtils::GetSecurityManager()->CheckSameOrigin(cx, uri);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  
  
  
  
  uint32_t loadFlags = nsIChannel::INHIBIT_CACHING |
                       nsICachingChannel::LOAD_NO_NETWORK_IO |
                       nsICachingChannel::LOAD_ONLY_IF_MODIFIED |
                       nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;

  if (aWhenOffline) {
    loadFlags |= nsICachingChannel::LOAD_CHECK_OFFLINE_CACHE |
                 nsICachingChannel::LOAD_ONLY_FROM_CACHE |
                 nsIRequest::LOAD_FROM_CACHE;
  }

  if (!mWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return false;
  }

  nsCOMPtr<nsILoadGroup> loadGroup;
  nsCOMPtr<nsIDocument> doc = mWindow->GetDoc();
  if (doc) {
    loadGroup = doc->GetDocumentLoadGroup();
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), uri,
                     nullptr, loadGroup, nullptr, loadFlags);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  stream->Close();

  nsresult status;
  rv = channel->GetStatus(&status);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  if (NS_FAILED(status)) {
    return false;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
  bool isAvailable;
  rv = httpChannel->GetRequestSucceeded(&isAvailable);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }
  return isAvailable;
}

nsDOMDeviceStorage*
Navigator::GetDeviceStorage(const nsAString& aType, ErrorResult& aRv)
{
  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<nsDOMDeviceStorage> storage;
  nsDOMDeviceStorage::CreateDeviceStorageFor(mWindow, aType,
                                             getter_AddRefs(storage));

  if (!storage) {
    return nullptr;
  }

  mDeviceStorageStores.AppendElement(storage);
  return storage;
}

void
Navigator::GetDeviceStorages(const nsAString& aType,
                             nsTArray<nsRefPtr<nsDOMDeviceStorage> >& aStores,
                             ErrorResult& aRv)
{
  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsDOMDeviceStorage::CreateDeviceStoragesFor(mWindow, aType, aStores);

  mDeviceStorageStores.AppendElements(aStores);
}

Geolocation*
Navigator::GetGeolocation(ErrorResult& aRv)
{
  if (mGeolocation) {
    return mGeolocation;
  }

  if (!mWindow || !mWindow->GetOuterWindow() || !mWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  mGeolocation = new Geolocation();
  if (NS_FAILED(mGeolocation->Init(mWindow->GetOuterWindow()))) {
    mGeolocation = nullptr;
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return mGeolocation;
}

#ifdef MOZ_MEDIA_NAVIGATOR
void
Navigator::MozGetUserMedia(JSContext* aCx,
                           const MediaStreamConstraints& aConstraints,
                           NavigatorUserMediaSuccessCallback& aOnSuccess,
                           NavigatorUserMediaErrorCallback& aOnError,
                           ErrorResult& aRv)
{
  CallbackObjectHolder<NavigatorUserMediaSuccessCallback,
                       nsIDOMGetUserMediaSuccessCallback> holder1(&aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onsuccess =
    holder1.ToXPCOMCallback();

  CallbackObjectHolder<NavigatorUserMediaErrorCallback,
                       nsIDOMGetUserMediaErrorCallback> holder2(&aOnError);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onerror = holder2.ToXPCOMCallback();

  if (!mWindow || !mWindow->GetOuterWindow() ||
      mWindow->GetOuterWindow()->GetCurrentInnerWindow() != mWindow) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);
    return;
  }

  bool privileged = nsContentUtils::IsChromeDoc(mWindow->GetExtantDoc());

  MediaManager* manager = MediaManager::Get();
  aRv = manager->GetUserMedia(aCx, privileged, mWindow, aConstraints,
                              onsuccess, onerror);
}

void
Navigator::MozGetUserMediaDevices(const MediaStreamConstraintsInternal& aConstraints,
                                  MozGetUserMediaDevicesSuccessCallback& aOnSuccess,
                                  NavigatorUserMediaErrorCallback& aOnError,
                                  ErrorResult& aRv)
{
  CallbackObjectHolder<MozGetUserMediaDevicesSuccessCallback,
                       nsIGetUserMediaDevicesSuccessCallback> holder1(&aOnSuccess);
  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> onsuccess =
    holder1.ToXPCOMCallback();

  CallbackObjectHolder<NavigatorUserMediaErrorCallback,
                       nsIDOMGetUserMediaErrorCallback> holder2(&aOnError);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onerror = holder2.ToXPCOMCallback();

  if (!mWindow || !mWindow->GetOuterWindow() ||
      mWindow->GetOuterWindow()->GetCurrentInnerWindow() != mWindow) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);
    return;
  }

  MediaManager* manager = MediaManager::Get();
  aRv = manager->GetUserMediaDevices(mWindow, aConstraints, onsuccess, onerror);
}
#endif

DesktopNotificationCenter*
Navigator::GetMozNotification(ErrorResult& aRv)
{
  if (mNotification) {
    return mNotification;
  }

  if (!mWindow || !mWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  mNotification = new DesktopNotificationCenter(mWindow);
  return mNotification;
}

#ifdef MOZ_B2G_FM

using mozilla::dom::FMRadio;

FMRadio*
Navigator::GetMozFMRadio(ErrorResult& aRv)
{
  if (!mFMRadio) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    NS_ENSURE_TRUE(mWindow->GetDocShell(), nullptr);

    mFMRadio = new FMRadio();
    mFMRadio->Init(mWindow);
  }

  return mFMRadio;
}

#endif  





battery::BatteryManager*
Navigator::GetBattery(ErrorResult& aRv)
{
  if (!mBatteryManager) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    NS_ENSURE_TRUE(mWindow->GetDocShell(), nullptr);

    mBatteryManager = new battery::BatteryManager(mWindow);
    mBatteryManager->Init();
  }

  return mBatteryManager;
}

already_AddRefed<Promise>
Navigator::GetDataStores(const nsAString& aName, ErrorResult& aRv)
{
  if (!mWindow || !mWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIDataStoreService> service =
    do_GetService("@mozilla.org/datastore-service;1");
  if (!service) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsISupports> promise;
  aRv = service->GetDataStores(mWindow, aName, getter_AddRefs(promise));

  nsRefPtr<Promise> p = static_cast<Promise*>(promise.get());
  return p.forget();
}

PowerManager*
Navigator::GetMozPower(ErrorResult& aRv)
{
  if (!mPowerManager) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mPowerManager = PowerManager::CreateInstance(mWindow);
    if (!mPowerManager) {
      
      aRv.Throw(NS_ERROR_UNEXPECTED);
    }
  }

  return mPowerManager;
}

already_AddRefed<WakeLock>
Navigator::RequestWakeLock(const nsAString &aTopic, ErrorResult& aRv)
{
  if (!mWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<power::PowerManagerService> pmService =
    power::PowerManagerService::GetInstance();
  
  
  if (!pmService) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  return pmService->NewWakeLock(aTopic, mWindow, aRv);
}

nsIDOMMozMobileMessageManager*
Navigator::GetMozMobileMessage()
{
  if (!mMobileMessageManager) {
    
    NS_ENSURE_TRUE(mWindow, nullptr);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), nullptr);

    mMobileMessageManager = new MobileMessageManager();
    mMobileMessageManager->Init(mWindow);
  }

  return mMobileMessageManager;
}

Telephony*
Navigator::GetMozTelephony(ErrorResult& aRv)
{
  if (!mTelephony) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mTelephony = Telephony::Create(mWindow, aRv);
  }

  return mTelephony;
}

#ifdef MOZ_B2G_RIL

MobileConnectionArray*
Navigator::GetMozMobileConnections(ErrorResult& aRv)
{
  if (!mMobileConnections) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mMobileConnections = new MobileConnectionArray(mWindow);
  }

  return mMobileConnections;
}

CellBroadcast*
Navigator::GetMozCellBroadcast(ErrorResult& aRv)
{
  if (!mCellBroadcast) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mCellBroadcast = CellBroadcast::Create(mWindow, aRv);
  }

  return mCellBroadcast;
}

Voicemail*
Navigator::GetMozVoicemail(ErrorResult& aRv)
{
  if (!mVoicemail) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    aRv = NS_NewVoicemail(mWindow, getter_AddRefs(mVoicemail));
    if (aRv.Failed()) {
      return nullptr;
    }
  }

  return mVoicemail;
}

nsIDOMMozIccManager*
Navigator::GetMozIccManager(ErrorResult& aRv)
{
  if (!mIccManager) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    NS_ENSURE_TRUE(mWindow->GetDocShell(), nullptr);

    mIccManager = new IccManager(mWindow);
  }

  return mIccManager;
}

#endif 

#ifdef MOZ_GAMEPAD
void
Navigator::GetGamepads(nsTArray<nsRefPtr<Gamepad> >& aGamepads,
                       ErrorResult& aRv)
{
  if (!mWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
  NS_ENSURE_TRUE_VOID(mWindow->GetDocShell());
  nsGlobalWindow* win = static_cast<nsGlobalWindow*>(mWindow.get());
  win->SetHasGamepadEventListener(true);
  win->GetGamepads(aGamepads);
}
#endif





NS_IMETHODIMP
Navigator::GetMozConnection(nsISupports** aConnection)
{
  nsCOMPtr<nsINetworkProperties> properties = GetMozConnection();
  properties.forget(aConnection);
  return NS_OK;
}

network::Connection*
Navigator::GetMozConnection()
{
  if (!mConnection) {
    NS_ENSURE_TRUE(mWindow, nullptr);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), nullptr);

    mConnection = new network::Connection();
    mConnection->Init(mWindow);
  }

  return mConnection;
}

#ifdef MOZ_B2G_BT
bluetooth::BluetoothManager*
Navigator::GetMozBluetooth(ErrorResult& aRv)
{
  if (!mBluetooth) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mBluetooth = bluetooth::BluetoothManager::Create(mWindow);
  }

  return mBluetooth;
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
  rv = gpi->Init(mWindow, &prop_val);
  NS_ENSURE_SUCCESS(rv, rv);

  mMessagesManager = messageManager.forget();

  return NS_OK;
}

bool
Navigator::MozHasPendingMessage(const nsAString& aType, ErrorResult& aRv)
{
  
  nsresult rv = EnsureMessagesManager();
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  bool result = false;
  rv = mMessagesManager->MozHasPendingMessage(aType, &result);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }
  return result;
}

void
Navigator::MozSetMessageHandler(const nsAString& aType,
                                systemMessageCallback* aCallback,
                                ErrorResult& aRv)
{
  
  nsresult rv = EnsureMessagesManager();
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  CallbackObjectHolder<systemMessageCallback, nsIDOMSystemMessageCallback>
    holder(aCallback);
  nsCOMPtr<nsIDOMSystemMessageCallback> callback = holder.ToXPCOMCallback();

  rv = mMessagesManager->MozSetMessageHandler(aType, callback);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
}

#ifdef MOZ_TIME_MANAGER
time::TimeManager*
Navigator::GetMozTime(ErrorResult& aRv)
{
  if (!mWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  if (!mTimeManager) {
    mTimeManager = new time::TimeManager(mWindow);
  }

  return mTimeManager;
}
#endif

nsDOMCameraManager*
Navigator::GetMozCameras(ErrorResult& aRv)
{
  if (!mCameraManager) {
    if (!mWindow ||
        !mWindow->GetOuterWindow() ||
        mWindow->GetOuterWindow()->GetCurrentInnerWindow() != mWindow) {
      aRv.Throw(NS_ERROR_NOT_AVAILABLE);
      return nullptr;
    }

    mCameraManager = nsDOMCameraManager::CreateInstance(mWindow);
  }

  return mCameraManager;
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
  return CheckPermission(mWindow, type);
}


bool
Navigator::CheckPermission(nsPIDOMWindow* aWindow, const char* aType)
{
  if (!aWindow) {
    return false;
  }

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromWindow(aWindow, aType, &permission);
  return permission == nsIPermissionManager::ALLOW_ACTION;
}

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
system::AudioChannelManager*
Navigator::GetMozAudioChannelManager(ErrorResult& aRv)
{
  if (!mAudioChannelManager) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mAudioChannelManager = new system::AudioChannelManager();
    mAudioChannelManager->Init(mWindow);
  }

  return mAudioChannelManager;
}
#endif

bool
Navigator::DoNewResolve(JSContext* aCx, JS::Handle<JSObject*> aObject,
                        JS::Handle<jsid> aId,
                        JS::MutableHandle<JSPropertyDescriptor> aDesc)
{
  if (!JSID_IS_STRING(aId)) {
    return true;
  }

  nsScriptNameSpaceManager* nameSpaceManager = GetNameSpaceManager();
  if (!nameSpaceManager) {
    return Throw(aCx, NS_ERROR_NOT_INITIALIZED);
  }

  nsDependentJSString name(aId);

  const nsGlobalNameStruct* name_struct =
    nameSpaceManager->LookupNavigatorName(name);
  if (!name_struct) {
    return true;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeNewDOMBinding) {
    ConstructNavigatorProperty construct = name_struct->mConstructNavigatorProperty;
    MOZ_ASSERT(construct);

    JS::Rooted<JSObject*> naviObj(aCx,
                                  js::CheckedUnwrap(aObject,
                                                     false));
    if (!naviObj) {
      return Throw(aCx, NS_ERROR_DOM_SECURITY_ERR);
    }

    JS::Rooted<JSObject*> domObject(aCx);
    {
      JSAutoCompartment ac(aCx, naviObj);

      
      
      
      if (name_struct->mConstructorEnabled &&
          !(*name_struct->mConstructorEnabled)(aCx, naviObj)) {
        return true;
      }

      if (name.EqualsLiteral("mozSettings")) {
        bool hasPermission = CheckPermission("settings-read") ||
                             CheckPermission("settings-write");
        if (!hasPermission) {
          FillPropertyDescriptor(aDesc, aObject, JS::NullValue(), false);
          return true;
        }
      }

      if (name.EqualsLiteral("mozDownloadManager")) {
        if (!CheckPermission("downloads")) {
          FillPropertyDescriptor(aDesc, aObject, JS::NullValue(), false);
          return true;
        }
      }

      domObject = construct(aCx, naviObj);
      if (!domObject) {
        return Throw(aCx, NS_ERROR_FAILURE);
      }
    }

    if (!JS_WrapObject(aCx, &domObject)) {
      return false;
    }

    FillPropertyDescriptor(aDesc, aObject, JS::ObjectValue(*domObject), false);
    return true;
  }

  NS_ASSERTION(name_struct->mType == nsGlobalNameStruct::eTypeNavigatorProperty,
               "unexpected type");

  nsresult rv = NS_OK;

  nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
  if (NS_FAILED(rv)) {
    return Throw(aCx, rv);
  }

  JS::Rooted<JS::Value> prop_val(aCx, JS::UndefinedValue()); 

  nsCOMPtr<nsIDOMGlobalPropertyInitializer> gpi(do_QueryInterface(native));

  if (gpi) {
    if (!mWindow) {
      return Throw(aCx, NS_ERROR_UNEXPECTED);
    }

    rv = gpi->Init(mWindow, &prop_val);
    if (NS_FAILED(rv)) {
      return Throw(aCx, rv);
    }
  }

  if (JSVAL_IS_PRIMITIVE(prop_val) && !JSVAL_IS_NULL(prop_val)) {
    rv = nsContentUtils::WrapNative(aCx, aObject, native, &prop_val, true);

    if (NS_FAILED(rv)) {
      return Throw(aCx, rv);
    }
  }

  if (!JS_WrapValue(aCx, &prop_val)) {
    return Throw(aCx, NS_ERROR_UNEXPECTED);
  }

  FillPropertyDescriptor(aDesc, aObject, prop_val, false);
  return true;
}

static PLDHashOperator
SaveNavigatorName(const nsAString& aName, void* aClosure)
{
  nsTArray<nsString>* arr = static_cast<nsTArray<nsString>*>(aClosure);
  arr->AppendElement(aName);
  return PL_DHASH_NEXT;
}

void
Navigator::GetOwnPropertyNames(JSContext* aCx, nsTArray<nsString>& aNames,
                               ErrorResult& aRv)
{
  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  if (!nameSpaceManager) {
    NS_ERROR("Can't get namespace manager.");
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  nameSpaceManager->EnumerateNavigatorNames(SaveNavigatorName, &aNames);
}

JSObject*
Navigator::WrapObject(JSContext* cx, JS::Handle<JSObject*> scope)
{
  return NavigatorBinding::Wrap(cx, scope, this);
}


bool
Navigator::HasBatterySupport(JSContext* , JSObject* )
{
  return battery::BatteryManager::HasSupport();
}


bool
Navigator::HasPowerSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && PowerManager::CheckPermission(win);
}


bool
Navigator::HasPhoneNumberSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return CheckPermission(win, "phonenumberservice");
}


bool
Navigator::HasIdleSupport(JSContext*  , JSObject* aGlobal)
{
  if (!nsContentUtils::IsIdleObserverAPIEnabled()) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return CheckPermission(win, "idle");
}


bool
Navigator::HasWakeLockSupport(JSContext* , JSObject* )
{
  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  
  return !!pmService;
}


bool
Navigator::HasMobileMessageSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);

#ifndef MOZ_WEBSMS_BACKEND
  return false;
#endif

  
  bool enabled = false;
  Preferences::GetBool("dom.sms.enabled", &enabled);
  if (!enabled) {
    return false;
  }

  NS_ENSURE_TRUE(win, false);
  NS_ENSURE_TRUE(win->GetDocShell(), false);

  if (!CheckPermission(win, "sms")) {
    return false;
  }

  return true;
}


bool
Navigator::HasTelephonySupport(JSContext* cx, JSObject* aGlobal)
{
  JS::Rooted<JSObject*> global(cx, aGlobal);

  
  bool enabled = false;
  Preferences::GetBool("dom.telephony.enabled", &enabled);
  if (!enabled) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(global);
  return win && CheckPermission(win, "telephony");
}


bool
Navigator::HasCameraSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && nsDOMCameraManager::CheckPermission(win);
}

#ifdef MOZ_B2G_RIL

bool
Navigator::HasMobileConnectionSupport(JSContext* ,
                                      JSObject* aGlobal)
{
  
  bool enabled = false;
  Preferences::GetBool("dom.mobileconnection.enabled", &enabled);
  NS_ENSURE_TRUE(enabled, false);

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && (CheckPermission(win, "mobileconnection") ||
                 CheckPermission(win, "mobilenetwork"));
}


bool
Navigator::HasCellBroadcastSupport(JSContext* ,
                                   JSObject* aGlobal)
{
  
  bool enabled = false;
  Preferences::GetBool("dom.cellbroadcast.enabled", &enabled);
  NS_ENSURE_TRUE(enabled, false);

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "cellbroadcast");
}


bool
Navigator::HasVoicemailSupport(JSContext* ,
                               JSObject* aGlobal)
{
  
  bool enabled = false;
  Preferences::GetBool("dom.voicemail.enabled", &enabled);
  NS_ENSURE_TRUE(enabled, false);

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "voicemail");
}


bool
Navigator::HasIccManagerSupport(JSContext* ,
                                JSObject* aGlobal)
{
  
  bool enabled = false;
  Preferences::GetBool("dom.icc.enabled", &enabled);
  NS_ENSURE_TRUE(enabled, false);

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "mobileconnection");
}
#endif 


bool
Navigator::HasWifiManagerSupport(JSContext* ,
                                 JSObject* aGlobal)
{
  
  
  

  nsIPrincipal* principal = nsContentUtils::GetObjectPrincipal(aGlobal);

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromPrincipal(principal, "wifi-manage", &permission);
  return nsIPermissionManager::ALLOW_ACTION == permission;
}

#ifdef MOZ_B2G_BT

bool
Navigator::HasBluetoothSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && bluetooth::BluetoothManager::CheckPermission(win);
}
#endif 

#ifdef MOZ_B2G_FM

bool
Navigator::HasFMRadioSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "fmradio");
}
#endif 

#ifdef MOZ_NFC

bool
Navigator::HasNfcSupport(JSContext* , JSObject* aGlobal)
{
  
  nsCOMPtr<nsISupports> contentHelper = do_GetService("@mozilla.org/nfc/content-helper;1");
  if (!contentHelper) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && (CheckPermission(win, "nfc-read") ||
                 CheckPermission(win, "nfc-write"));
}


bool
Navigator::HasNfcPeerSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "nfc-write");
}


bool
Navigator::HasNfcManagerSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "nfc-manager");
}
#endif 

#ifdef MOZ_TIME_MANAGER

bool
Navigator::HasTimeSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && CheckPermission(win, "time");
}
#endif 

#ifdef MOZ_MEDIA_NAVIGATOR

bool
Navigator::HasUserMediaSupport(JSContext* ,
                               JSObject* )
{
  
  return Preferences::GetBool("media.navigator.enabled", false) ||
         Preferences::GetBool("media.peerconnection.enabled", false);
}
#endif 


bool
Navigator::HasPushNotificationsSupport(JSContext* ,
                                       JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return Preferences::GetBool("services.push.enabled", false) &&
         win && CheckPermission(win, "push");
}


bool
Navigator::HasInputMethodSupport(JSContext* ,
                                 JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  if (Preferences::GetBool("dom.mozInputMethod.testing", false)) {
    return true;
  }

  return Preferences::GetBool("dom.mozInputMethod.enabled", false) &&
         win && CheckPermission(win, "input");
}


bool
Navigator::HasDataStoreSupport(JSContext* cx, JSObject* aGlobal)
{
  JS::Rooted<JSObject*> global(cx, aGlobal);

  
  bool enabled = false;
  Preferences::GetBool("dom.datastore.enabled", &enabled);
  if (!enabled) {
    return false;
  }

  
  if (Preferences::GetBool("dom.testing.datastore_enabled_for_hosted_apps", false)) {
    return true;
  }

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(global);
  if (!win) {
    return false;
  }

  nsIDocument* doc = win->GetExtantDoc();
  if (!doc || !doc->NodePrincipal()) {
    return false;
  }

  uint16_t status;
  if (NS_FAILED(doc->NodePrincipal()->GetAppStatus(&status))) {
    return false;
  }

  return status == nsIPrincipal::APP_STATUS_CERTIFIED;
}


bool
Navigator::HasDownloadsSupport(JSContext* aCx, JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);

  return win &&
         CheckPermission(win, "downloads")  &&
         Preferences::GetBool("dom.mozDownloads.enabled");
}


already_AddRefed<nsPIDOMWindow>
Navigator::GetWindowFromGlobal(JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win =
    do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(aGlobal));
  MOZ_ASSERT(!win || win->IsInnerWindow());
  return win.forget();
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
  aAppVersion.Append(char16_t(')'));

  return rv;
}

void
NS_GetNavigatorAppName(nsAString& aAppName)
{
  if (!nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      mozilla::Preferences::GetString("general.appname.override");

    if (override) {
      aAppName = override;
      return;
    }
  }

  aAppName.AssignLiteral("Netscape");
}
