






#include "base/basictypes.h"

#include "Navigator.h"
#include "nsIXULAppInfo.h"
#include "nsPluginArray.h"
#include "nsMimeTypeArray.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/DesktopNotification.h"
#include "nsGeolocation.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsISupportsPriority.h"
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
#include "mozilla/dom/CellBroadcast.h"
#include "mozilla/dom/MobileMessageManager.h"
#include "mozilla/dom/ServiceWorkerContainer.h"
#include "mozilla/dom/Telephony.h"
#include "mozilla/dom/Voicemail.h"
#include "mozilla/Hal.h"
#include "nsISiteSpecificUserAgent.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "Connection.h"
#include "mozilla/dom/Event.h" 
#include "nsGlobalWindow.h"
#ifdef MOZ_B2G
#include "nsIMobileIdentityService.h"
#endif
#ifdef MOZ_B2G_RIL
#include "mozilla/dom/IccManager.h"
#include "mozilla/dom/MobileConnectionArray.h"
#endif
#include "nsIIdleObserver.h"
#include "nsIPermissionManager.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "TimeManager.h"
#include "DeviceStorage.h"
#include "nsIDOMNavigatorSystemMessages.h"
#include "nsStreamUtils.h"
#include "nsIAppsService.h"
#include "mozIApplication.h"
#include "WidgetUtils.h"
#include "mozIThirdPartyUtil.h"
#include "nsChannelPolicy.h"

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
#include "mozilla/dom/DataStoreService.h"
#include "nsJSUtils.h"

#include "nsScriptNameSpaceManager.h"

#include "mozilla/dom/NavigatorBinding.h"
#include "mozilla/dom/Promise.h"

#include "nsIUploadChannel2.h"
#include "nsFormData.h"
#include "nsIPrivateBrowsingChannel.h"
#include "nsIDocShell.h"

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"

#if defined(XP_LINUX)
#include "mozilla/Hal.h"
#endif
#include "mozilla/dom/ContentChild.h"

#include "mozilla/dom/FeatureList.h"

namespace mozilla {
namespace dom {

static bool sDoNotTrackEnabled = false;
static uint32_t sDoNotTrackValue = 1;
static bool sVibratorEnabled   = false;
static uint32_t sMaxVibrateMS  = 0;
static uint32_t sMaxVibrateListLen = 0;


void
Navigator::Init()
{
  Preferences::AddBoolVarCache(&sDoNotTrackEnabled,
                               "privacy.donottrackheader.enabled",
                               false);
  Preferences::AddUintVarCache(&sDoNotTrackValue,
                               "privacy.donottrackheader.value",
                               1);
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
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCachedResolveResults)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Navigator)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPlugins)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMimeTypes)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGeolocation)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNotification)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBatteryManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPowerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCellBroadcast)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileMessageManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTelephony)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mVoicemail)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mConnection)
#ifdef MOZ_B2G_RIL
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileConnections)
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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mServiceWorkerContainer)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCachedResolveResults)
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

  if (mCellBroadcast) {
    mCellBroadcast = nullptr;
  }

  if (mMobileMessageManager) {
    mMobileMessageManager->Shutdown();
    mMobileMessageManager = nullptr;
  }

  if (mTelephony) {
    mTelephony = nullptr;
  }

  if (mVoicemail) {
    mVoicemail->Shutdown();
    mVoicemail = nullptr;
  }

  if (mConnection) {
    mConnection->Shutdown();
    mConnection = nullptr;
  }

#ifdef MOZ_B2G_RIL
  if (mMobileConnections) {
    mMobileConnections = nullptr;
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

  mServiceWorkerContainer = nullptr;
}





NS_IMETHODIMP
Navigator::GetUserAgent(nsAString& aUserAgent)
{
  nsCOMPtr<nsIURI> codebaseURI;
  nsCOMPtr<nsPIDOMWindow> window;

  if (mWindow && mWindow->GetDocShell()) {
    window = mWindow;
    nsIDocument* doc = mWindow->GetExtantDoc();
    if (doc) {
      doc->NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));
    }
  }

  return GetUserAgent(window, codebaseURI, nsContentUtils::IsCallerChrome(),
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
  return GetAppVersion(aAppVersion,  true);
}

NS_IMETHODIMP
Navigator::GetAppName(nsAString& aAppName)
{
  AppName(aAppName,  true);
  return NS_OK;
}










 void
Navigator::GetAcceptLanguages(nsTArray<nsString>& aLanguages)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  const nsAdoptingString& acceptLang =
    Preferences::GetLocalizedString("intl.accept_languages");

  
  nsCharSeparatedTokenizer langTokenizer(acceptLang, ',');
  while (langTokenizer.hasMoreTokens()) {
    nsDependentSubstring lang = langTokenizer.nextToken();

    
    
    if (lang.Length() > 2 && lang[2] == char16_t('_')) {
      lang.Replace(2, 1, char16_t('-'));
    }

    
    
    
    if (lang.Length() > 2) {
      nsCharSeparatedTokenizer localeTokenizer(lang, '-');
      int32_t pos = 0;
      bool first = true;
      while (localeTokenizer.hasMoreTokens()) {
        const nsSubstring& code = localeTokenizer.nextToken();

        if (code.Length() == 2 && !first) {
          nsAutoString upper(code);
          ToUpperCase(upper);
          lang.Replace(pos, code.Length(), upper);
        }

        pos += code.Length() + 1; 
        first = false;
      }
    }

    aLanguages.AppendElement(lang);
  }
}








NS_IMETHODIMP
Navigator::GetLanguage(nsAString& aLanguage)
{
  nsTArray<nsString> languages;
  GetLanguages(languages);
  if (languages.Length() >= 1) {
    aLanguage.Assign(languages[0]);
  } else {
    aLanguage.Truncate();
  }

  return NS_OK;
}

void
Navigator::GetLanguages(nsTArray<nsString>& aLanguages)
{
  GetAcceptLanguages(aLanguages);

  
  
  
  
}

NS_IMETHODIMP
Navigator::GetPlatform(nsAString& aPlatform)
{
  return GetPlatform(aPlatform,  true);
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
  if (mWindow && mWindow->GetDoc()) {
    return !NS_IsAppOffline(mWindow->GetDoc()->NodePrincipal());
  }

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
    if (sDoNotTrackValue == 0) {
      aResult.AssignLiteral("0");
    } else {
      aResult.AssignLiteral("1");
    }
  } else {
    aResult.AssignLiteral("unspecified");
  }

  return NS_OK;
}

bool
Navigator::JavaEnabled(ErrorResult& aRv)
{
  Telemetry::AutoTimer<Telemetry::CHECK_JAVA_ENABLED> telemetryTimer;

  
  nsAdoptingString javaMIME = Preferences::GetString("plugin.java.mime");
  NS_ENSURE_TRUE(!javaMIME.IsEmpty(), false);

  if (!mMimeTypes) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return false;
    }
    mMimeTypes = new nsMimeTypeArray(mWindow);
  }

  RefreshMIMEArray();

  nsMimeType *mimeType = mMimeTypes->NamedItem(javaMIME);

  return mimeType && mimeType->GetEnabledPlugin();
}

void
Navigator::RefreshMIMEArray()
{
  if (mMimeTypes) {
    mMimeTypes->Refresh();
  }
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

  void RemoveListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

private:
  virtual ~VibrateWindowListener()
  {
  }

  nsWeakPtr mWindow;
  nsWeakPtr mDocument;
};

NS_IMPL_ISUPPORTS(VibrateWindowListener, nsIDOMEventListener)

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

  nsTArray<uint32_t> pattern(aPattern);

  if (pattern.Length() > sMaxVibrateListLen) {
    pattern.SetLength(sMaxVibrateListLen);
  }

  for (size_t i = 0; i < pattern.Length(); ++i) {
    if (pattern[i] > sMaxVibrateMS) {
      pattern[i] = sMaxVibrateMS;
    }
  }

  
  
  if (!sVibratorEnabled) {
    return true;
  }

  
  

  if (!gVibrateWindowListener) {
    
    
    
    ClearOnShutdown(&gVibrateWindowListener);
  }
  else {
    gVibrateWindowListener->RemoveListener();
  }
  gVibrateWindowListener = new VibrateWindowListener(mWindow, doc);

  hal::Vibrate(pattern, mWindow);
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

class BeaconStreamListener MOZ_FINAL : public nsIStreamListener
{
    ~BeaconStreamListener() {}

  public:
    BeaconStreamListener() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER
};

NS_IMPL_ISUPPORTS(BeaconStreamListener,
                  nsIStreamListener,
                  nsIRequestObserver)


NS_IMETHODIMP
BeaconStreamListener::OnStartRequest(nsIRequest *aRequest,
                                     nsISupports *aContext)
{
  aRequest->Cancel(NS_ERROR_NET_INTERRUPT);
  return NS_BINDING_ABORTED;
}

NS_IMETHODIMP
BeaconStreamListener::OnStopRequest(nsIRequest *aRequest,
                                    nsISupports *aContext,
                                    nsresult aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
BeaconStreamListener::OnDataAvailable(nsIRequest *aRequest,
                                      nsISupports *ctxt,
                                      nsIInputStream *inStr,
                                      uint64_t sourceOffset,
                                      uint32_t count)
{
  MOZ_ASSERT(false);
  return NS_OK;
}

bool
Navigator::SendBeacon(const nsAString& aUrl,
                      const Nullable<ArrayBufferViewOrBlobOrStringOrFormData>& aData,
                      ErrorResult& aRv)
{
  if (!mWindow) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return false;
  }

  nsCOMPtr<nsIDocument> doc = mWindow->GetDoc();
  if (!doc) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return false;
  }

  nsIURI* documentURI = doc->GetDocumentURI();
  if (!documentURI) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return false;
  }

  nsCOMPtr<nsIURI> uri;
  nsresult rv = nsContentUtils::NewURIWithDocumentCharset(
                  getter_AddRefs(uri),
                  aUrl,
                  doc,
                  doc->GetDocBaseURI());
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_URL_MISMATCH_ERR);
    return false;
  }

  
  
  nsCOMPtr<nsIPrincipal> principal = doc->NodePrincipal();
  nsCOMPtr<nsIScriptSecurityManager> secMan = nsContentUtils::GetSecurityManager();
  uint32_t flags = nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL
                   & nsIScriptSecurityManager::DISALLOW_SCRIPT;
  rv = secMan->CheckLoadURIWithPrincipal(principal,
                                         uri,
                                         flags);
  if (NS_FAILED(rv)) {
    
    aRv.Throw(rv);
    return false;
  }

  
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_BEACON,
                                 uri,
                                 principal,
                                 doc,
                                 EmptyCString(), 
                                 nullptr,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    
    aRv.Throw(NS_ERROR_CONTENT_BLOCKED);
    return false;
  }

  nsCOMPtr<nsIChannel> channel;
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = principal->GetCsp(getter_AddRefs(csp));
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  if (csp) {
    channelPolicy = do_CreateInstance(NSCHANNELPOLICY_CONTRACTID);
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_BEACON);
  }

  rv = NS_NewChannel(getter_AddRefs(channel),
                     uri,
                     doc,
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_BEACON,
                     channelPolicy);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(channel);
  if (pbChannel) {
    nsIDocShell* docShell = mWindow->GetDocShell();
    nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);
    if (loadContext) {
      rv = pbChannel->SetPrivate(loadContext->UsePrivateBrowsing());
      if (NS_FAILED(rv)) {
        NS_WARNING("Setting the privacy status on the beacon channel failed");
      }
    }
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
  if (!httpChannel) {
    
    aRv.Throw(NS_ERROR_DOM_BAD_URI);
    return false;
  }
  httpChannel->SetReferrer(documentURI);

  
  
  
  
  
  
  nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal(do_QueryInterface(channel));
  nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil = do_GetService(THIRDPARTYUTIL_CONTRACTID);
  if (!httpChannelInternal) {
    aRv.Throw(NS_ERROR_DOM_BAD_URI);
    return false;
  }
  bool isForeign = true;
  thirdPartyUtil->IsThirdPartyWindow(mWindow, uri, &isForeign);
  httpChannelInternal->SetForceAllowThirdPartyCookie(!isForeign);

  nsCString mimeType;
  if (!aData.IsNull()) {
    nsCOMPtr<nsIInputStream> in;

    if (aData.Value().IsString()) {
      nsCString stringData = NS_ConvertUTF16toUTF8(aData.Value().GetAsString());
      nsCOMPtr<nsIStringInputStream> strStream = do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
      if (NS_FAILED(rv)) {
        aRv.Throw(NS_ERROR_FAILURE);
        return false;
      }
      rv = strStream->SetData(stringData.BeginReading(), stringData.Length());
      if (NS_FAILED(rv)) {
        aRv.Throw(NS_ERROR_FAILURE);
        return false;
      }
      mimeType.AssignLiteral("text/plain;charset=UTF-8");
      in = strStream;

    } else if (aData.Value().IsArrayBufferView()) {

      nsCOMPtr<nsIStringInputStream> strStream = do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
      if (NS_FAILED(rv)) {
        aRv.Throw(NS_ERROR_FAILURE);
        return false;
      }

      const ArrayBufferView& view = aData.Value().GetAsArrayBufferView();
      view.ComputeLengthAndData();
      rv = strStream->SetData(reinterpret_cast<char*>(view.Data()),
                              view.Length());

      if (NS_FAILED(rv)) {
        aRv.Throw(NS_ERROR_FAILURE);
        return false;
      }
      mimeType.AssignLiteral("application/octet-stream");
      in = strStream;

    } else if (aData.Value().IsBlob()) {
      nsCOMPtr<nsIDOMBlob> blob = aData.Value().GetAsBlob();
      rv = blob->GetInternalStream(getter_AddRefs(in));
      if (NS_FAILED(rv)) {
        aRv.Throw(NS_ERROR_FAILURE);
        return false;
      }
      nsAutoString type;
      rv = blob->GetType(type);
      if (NS_FAILED(rv)) {
        aRv.Throw(NS_ERROR_FAILURE);
        return false;
      }
      mimeType = NS_ConvertUTF16toUTF8(type);

    } else if (aData.Value().IsFormData()) {
      nsFormData& form = aData.Value().GetAsFormData();
      uint64_t len;
      nsAutoCString charset;
      form.GetSendInfo(getter_AddRefs(in),
                       &len,
                       mimeType,
                       charset);
    } else {
      MOZ_ASSERT(false, "switch statements not in sync");
      aRv.Throw(NS_ERROR_FAILURE);
      return false;
    }

    nsCOMPtr<nsIUploadChannel2> uploadChannel = do_QueryInterface(channel);
    if (!uploadChannel) {
      aRv.Throw(NS_ERROR_FAILURE);
      return false;
    }
    uploadChannel->ExplicitSetUploadStream(in, mimeType, -1,
                                           NS_LITERAL_CSTRING("POST"),
                                           false);
  } else {
    httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("POST"));
  }

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(channel);
  if (p) {
    p->SetPriority(nsISupportsPriority::PRIORITY_LOWEST);
  }

  nsRefPtr<nsCORSListenerProxy> cors = new nsCORSListenerProxy(new BeaconStreamListener(),
                                                               principal,
                                                               true);

  
  rv = secMan->CheckSameOriginURI(documentURI, uri, false);
  bool crossOrigin = NS_FAILED(rv);
  nsAutoCString contentType, parsedCharset;
  rv = NS_ParseContentType(mimeType, contentType, parsedCharset);
  if (crossOrigin &&
      contentType.Length() > 0 &&
      !contentType.Equals(APPLICATION_WWW_FORM_URLENCODED) &&
      !contentType.Equals(MULTIPART_FORM_DATA) &&
      !contentType.Equals(TEXT_PLAIN)) {
    nsCOMPtr<nsIChannel> preflightChannel;
    nsTArray<nsCString> unsafeHeaders;
    unsafeHeaders.AppendElement(NS_LITERAL_CSTRING("Content-Type"));
    rv = NS_StartCORSPreflight(channel,
                               cors,
                               principal,
                               true,
                               unsafeHeaders,
                               getter_AddRefs(preflightChannel));
  } else {
    rv = channel->AsyncOpen(cors, nullptr);
  }
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }
  return true;
}

#ifdef MOZ_MEDIA_NAVIGATOR
void
Navigator::MozGetUserMedia(const MediaStreamConstraints& aConstraints,
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

  MediaManager* manager = MediaManager::Get();
  aRv = manager->GetUserMedia(mWindow, aConstraints, onsuccess, onerror);
}

void
Navigator::MozGetUserMediaDevices(const MediaStreamConstraints& aConstraints,
                                  MozGetUserMediaDevicesSuccessCallback& aOnSuccess,
                                  NavigatorUserMediaErrorCallback& aOnError,
                                  uint64_t aInnerWindowID,
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
  aRv = manager->GetUserMediaDevices(mWindow, aConstraints, onsuccess, onerror,
                                     aInnerWindowID);
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
Navigator::GetDataStores(nsPIDOMWindow* aWindow,
                         const nsAString& aName,
                         const nsAString& aOwner,
                         ErrorResult& aRv)
{
  if (!aWindow || !aWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<DataStoreService> service = DataStoreService::GetOrCreate();
  if (!service) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsISupports> promise;
  aRv = service->GetDataStores(aWindow, aName, aOwner, getter_AddRefs(promise));

  nsRefPtr<Promise> p = static_cast<Promise*>(promise.get());
  return p.forget();
}

already_AddRefed<Promise>
Navigator::GetDataStores(const nsAString& aName,
                         const nsAString& aOwner,
                         ErrorResult& aRv)
{
  return GetDataStores(mWindow, aName, aOwner, aRv);
}

already_AddRefed<Promise>
Navigator::GetFeature(const nsAString& aName, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(mWindow);
  nsRefPtr<Promise> p = Promise::Create(go, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

#if defined(XP_LINUX)
  if (aName.EqualsLiteral("hardware.memory")) {
    
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
      uint32_t memLevel = mozilla::hal::GetTotalSystemMemoryLevel();
      if (memLevel == 0) {
        p->MaybeReject(NS_ERROR_NOT_AVAILABLE);
        return p.forget();
      }
      p->MaybeResolve((int)memLevel);
    } else {
      mozilla::dom::ContentChild* cc =
        mozilla::dom::ContentChild::GetSingleton();
      nsRefPtr<Promise> ipcRef(p);
      cc->SendGetSystemMemory(reinterpret_cast<uint64_t>(ipcRef.forget().take()));
    }
    return p.forget();
  } 
#endif

  
  const char manifestFeatures[][64] = {
    "manifest.origin"
  , "manifest.redirects"
#ifdef MOZ_B2G
  , "manifest.chrome.navigation"
  , "manifest.precompile"
#endif
  };

  nsAutoCString feature = NS_ConvertUTF16toUTF8(aName);
  for (uint32_t i = 0; i < MOZ_ARRAY_LENGTH(manifestFeatures); i++) {
    if (feature.Equals(manifestFeatures[i])) {
      p->MaybeResolve(true);
      return p.forget();
    }
  }

  p->MaybeResolve(JS::UndefinedHandleValue);
  return p.forget();
}

already_AddRefed<Promise>
Navigator::HasFeature(const nsAString& aName, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(mWindow);
  nsRefPtr<Promise> p = Promise::Create(go, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  NS_NAMED_LITERAL_STRING(apiWindowPrefix, "api.window.");
  if (StringBeginsWith(aName, apiWindowPrefix)) {
    const nsAString& featureName = Substring(aName, apiWindowPrefix.Length());

    
    if (featureName.EqualsLiteral("Navigator.mozTCPSocket")) {
      p->MaybeResolve(Preferences::GetBool("dom.mozTCPSocket.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.mozMobileConnections") ||
        featureName.EqualsLiteral("MozMobileNetworkInfo")) {
      p->MaybeResolve(Preferences::GetBool("dom.mobileconnection.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.mozInputMethod")) {
      p->MaybeResolve(Preferences::GetBool("dom.mozInputMethod.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.mozContacts")) {
      p->MaybeResolve(true);
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.getDeviceStorage")) {
      p->MaybeResolve(Preferences::GetBool("device.storage.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.mozNetworkStats")) {
      p->MaybeResolve(Preferences::GetBool("dom.mozNetworkStats.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.push")) {
      p->MaybeResolve(Preferences::GetBool("services.push.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.mozAlarms")) {
      p->MaybeResolve(Preferences::GetBool("dom.mozAlarms.enabled"));
      return p.forget();
    }

    if (featureName.EqualsLiteral("Navigator.mozCameras")) {
      p->MaybeResolve(true);
      return p.forget();
    }

#ifdef MOZ_B2G
    if (featureName.EqualsLiteral("Navigator.getMobileIdAssertion")) {
      p->MaybeResolve(true);
      return p.forget();
    }
#endif

    if (featureName.EqualsLiteral("XMLHttpRequest.mozSystem")) {
      p->MaybeResolve(true);
      return p.forget();
    }

    if (IsFeatureDetectible(featureName)) {
      p->MaybeResolve(true);
    } else {
      p->MaybeResolve(JS::UndefinedHandleValue);
    }
    return p.forget();
  }

  
  p->MaybeResolve(JS::UndefinedHandleValue);

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

MobileMessageManager*
Navigator::GetMozMobileMessage()
{
  if (!mMobileMessageManager) {
    
    NS_ENSURE_TRUE(mWindow, nullptr);
    NS_ENSURE_TRUE(mWindow->GetDocShell(), nullptr);

    mMobileMessageManager = new MobileMessageManager(mWindow);
    mMobileMessageManager->Init();
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

#ifdef MOZ_B2G
already_AddRefed<Promise>
Navigator::GetMobileIdAssertion(const MobileIdOptions& aOptions,
                                ErrorResult& aRv)
{
  if (!mWindow || !mWindow->GetDocShell()) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIMobileIdentityService> service =
    do_GetService("@mozilla.org/mobileidentity-service;1");
  if (!service) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (!cx) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  JS::Rooted<JS::Value> optionsValue(cx);
  if (!ToJSValue(cx, aOptions, &optionsValue)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsISupports> promise;
  aRv = service->GetMobileIdAssertion(mWindow,
                                      optionsValue,
                                      getter_AddRefs(promise));

  nsRefPtr<Promise> p = static_cast<Promise*>(promise.get());
  return p.forget();
}
#endif 

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

#endif 

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

    mVoicemail = Voicemail::Create(mWindow, aRv);
  }

  return mVoicemail;
}

#ifdef MOZ_B2G_RIL

IccManager*
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
Navigator::GetProperties(nsINetworkProperties** aProperties)
{
  ErrorResult rv;
  NS_IF_ADDREF(*aProperties = GetConnection(rv));
  return NS_OK;
}

network::Connection*
Navigator::GetConnection(ErrorResult& aRv)
{
  if (!mConnection) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mConnection = new network::Connection(mWindow);
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

already_AddRefed<ServiceWorkerContainer>
Navigator::ServiceWorker()
{
  MOZ_ASSERT(mWindow);

  if (!mServiceWorkerContainer) {
    mServiceWorkerContainer = new ServiceWorkerContainer(mWindow);
  }

  nsRefPtr<ServiceWorkerContainer> ref = mServiceWorkerContainer;
  return ref.forget();
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
    services::GetPermissionManager();
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

  nsAutoJSString name;
  if (!name.init(aCx, JSID_TO_STRING(aId))) {
    return false;
  }

  const nsGlobalNameStruct* name_struct =
    nameSpaceManager->LookupNavigatorName(name);
  if (!name_struct) {
    return true;
  }

  JS::Rooted<JSObject*> naviObj(aCx,
                                js::CheckedUnwrap(aObject,
                                                   false));
  if (!naviObj) {
    return Throw(aCx, NS_ERROR_DOM_SECURITY_ERR);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeNewDOMBinding) {
    ConstructNavigatorProperty construct = name_struct->mConstructNavigatorProperty;
    MOZ_ASSERT(construct);

    JS::Rooted<JSObject*> domObject(aCx);
    {
      
      
      JSAutoCompartment ac(aCx, naviObj);

      
      
      
      if (name_struct->mConstructorEnabled &&
          !(*name_struct->mConstructorEnabled)(aCx, naviObj)) {
        return true;
      }

      if (name.EqualsLiteral("mozSettings")) {
        bool hasPermission = CheckPermission("settings-api-read") ||
          CheckPermission("settings-api-write");
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

      nsISupports* existingObject = mCachedResolveResults.GetWeak(name);
      if (existingObject) {
        
        
        
        JS::Rooted<JS::Value> wrapped(aCx);
        if (!dom::WrapObject(aCx, existingObject, &wrapped)) {
          return false;
        }
        domObject = &wrapped.toObject();
      } else {
        domObject = construct(aCx, naviObj);
        if (!domObject) {
          return Throw(aCx, NS_ERROR_FAILURE);
        }

        
        nsISupports* native = UnwrapDOMObjectToISupports(domObject);
        MOZ_ASSERT(native);
        mCachedResolveResults.Put(name, native);
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

  nsCOMPtr<nsISupports> native;
  bool hadCachedNative = mCachedResolveResults.Get(name, getter_AddRefs(native));
  bool okToUseNative;
  JS::Rooted<JS::Value> prop_val(aCx);
  if (hadCachedNative) {
    okToUseNative = true;
  } else {
    native = do_CreateInstance(name_struct->mCID, &rv);
    if (NS_FAILED(rv)) {
      return Throw(aCx, rv);
    }

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

    okToUseNative = !prop_val.isObjectOrNull();
  }

  if (okToUseNative) {
    
    
    JSAutoCompartment ac(aCx, naviObj);

    rv = nsContentUtils::WrapNative(aCx, native, &prop_val);

    if (NS_FAILED(rv)) {
      return Throw(aCx, rv);
    }

    
    
    if (!hadCachedNative) {
      mCachedResolveResults.Put(name, native);
    }
  }

  if (!JS_WrapValue(aCx, &prop_val)) {
    return Throw(aCx, NS_ERROR_UNEXPECTED);
  }

  FillPropertyDescriptor(aDesc, aObject, prop_val, false);
  return true;
}

struct NavigatorNameEnumeratorClosure
{
  NavigatorNameEnumeratorClosure(JSContext* aCx, JSObject* aWrapper,
                                 nsTArray<nsString>& aNames)
    : mCx(aCx),
      mWrapper(aCx, aWrapper),
      mNames(aNames)
  {
  }

  JSContext* mCx;
  JS::Rooted<JSObject*> mWrapper;
  nsTArray<nsString>& mNames;
};

static PLDHashOperator
SaveNavigatorName(const nsAString& aName,
                  const nsGlobalNameStruct& aNameStruct,
                  void* aClosure)
{
  NavigatorNameEnumeratorClosure* closure =
    static_cast<NavigatorNameEnumeratorClosure*>(aClosure);
  if (!aNameStruct.mConstructorEnabled ||
      aNameStruct.mConstructorEnabled(closure->mCx, closure->mWrapper)) {
    closure->mNames.AppendElement(aName);
  }
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

  NavigatorNameEnumeratorClosure closure(aCx, GetWrapper(), aNames);
  nameSpaceManager->EnumerateNavigatorNames(SaveNavigatorName, &closure);
}

JSObject*
Navigator::WrapObject(JSContext* cx)
{
  return NavigatorBinding::Wrap(cx, this);
}


bool
Navigator::HasWakeLockSupport(JSContext* , JSObject* )
{
  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  
  return !!pmService;
}


bool
Navigator::HasCameraSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  return win && nsDOMCameraManager::CheckPermission(win);
}


bool
Navigator::HasWifiManagerSupport(JSContext* ,
                                 JSObject* aGlobal)
{
  
  
  

  nsIPrincipal* principal = nsContentUtils::ObjectPrincipal(aGlobal);

  nsCOMPtr<nsIPermissionManager> permMgr =
    services::GetPermissionManager();
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromPrincipal(principal, "wifi-manage", &permission);
  return nsIPermissionManager::ALLOW_ACTION == permission;
}

#ifdef MOZ_NFC

bool
Navigator::HasNFCSupport(JSContext* , JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);

  
  nsCOMPtr<nsISupports> contentHelper = do_GetService("@mozilla.org/nfc/content-helper;1");
  return !!contentHelper;
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
Navigator::HasInputMethodSupport(JSContext* ,
                                 JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  if (!win || !Preferences::GetBool("dom.mozInputMethod.enabled", false)) {
    return false;
  }

  if (Preferences::GetBool("dom.mozInputMethod.testing", false)) {
    return true;
  }

  return CheckPermission(win, "input") ||
         CheckPermission(win, "input-manage");
}


bool
Navigator::HasDataStoreSupport(nsIPrincipal* aPrincipal)
{
  workers::AssertIsOnMainThread();

  return DataStoreService::CheckPermission(aPrincipal);
}



class HasDataStoreSupportRunnable MOZ_FINAL
  : public workers::WorkerMainThreadRunnable
{
public:
  bool mResult;

  explicit HasDataStoreSupportRunnable(workers::WorkerPrivate* aWorkerPrivate)
    : workers::WorkerMainThreadRunnable(aWorkerPrivate)
    , mResult(false)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    workers::AssertIsOnMainThread();

    mResult = Navigator::HasDataStoreSupport(mWorkerPrivate->GetPrincipal());

    return true;
  }
};


bool
Navigator::HasDataStoreSupport(JSContext* aCx, JSObject* aGlobal)
{
  
  if (!NS_IsMainThread()) {
    workers::WorkerPrivate* workerPrivate =
      workers::GetWorkerPrivateFromContext(aCx);
    workerPrivate->AssertIsOnWorkerThread();

    nsRefPtr<HasDataStoreSupportRunnable> runnable =
      new HasDataStoreSupportRunnable(workerPrivate);
    runnable->Dispatch(aCx);

    return runnable->mResult;
  }

  workers::AssertIsOnMainThread();

  JS::Rooted<JSObject*> global(aCx, aGlobal);

  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(global);
  if (!win) {
    return false;
  }

  nsIDocument* doc = win->GetExtantDoc();
  if (!doc || !doc->NodePrincipal()) {
    return false;
  }

  return HasDataStoreSupport(doc->NodePrincipal());
}

#ifdef MOZ_B2G

bool
Navigator::HasMobileIdSupport(JSContext* aCx, JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win = GetWindowFromGlobal(aGlobal);
  if (!win) {
    return false;
  }

  nsIDocument* doc = win->GetExtantDoc();
  if (!doc) {
    return false;
  }

  nsIPrincipal* principal = doc->NodePrincipal();

  nsCOMPtr<nsIPermissionManager> permMgr = services::GetPermissionManager();
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::UNKNOWN_ACTION;
  permMgr->TestPermissionFromPrincipal(principal, "mobileid", &permission);
  return permission == nsIPermissionManager::PROMPT_ACTION ||
         permission == nsIPermissionManager::ALLOW_ACTION;
}
#endif


already_AddRefed<nsPIDOMWindow>
Navigator::GetWindowFromGlobal(JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win =
    do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(aGlobal));
  MOZ_ASSERT(!win || win->IsInnerWindow());
  return win.forget();
}

nsresult
Navigator::GetPlatform(nsAString& aPlatform, bool aUsePrefOverriddenValue)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aUsePrefOverriddenValue && !nsContentUtils::IsCallerChrome()) {
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
Navigator::GetAppVersion(nsAString& aAppVersion, bool aUsePrefOverriddenValue)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aUsePrefOverriddenValue && !nsContentUtils::IsCallerChrome()) {
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
Navigator::AppName(nsAString& aAppName, bool aUsePrefOverriddenValue)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aUsePrefOverriddenValue && !nsContentUtils::IsCallerChrome()) {
    const nsAdoptingString& override =
      mozilla::Preferences::GetString("general.appname.override");

    if (override) {
      aAppName = override;
      return;
    }
  }

  aAppName.AssignLiteral("Netscape");
}

nsresult
Navigator::GetUserAgent(nsPIDOMWindow* aWindow, nsIURI* aURI,
                        bool aIsCallerChrome,
                        nsAString& aUserAgent)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aIsCallerChrome) {
    const nsAdoptingString& override =
      mozilla::Preferences::GetString("general.useragent.override");

    if (override) {
      aUserAgent = override;
      return NS_OK;
    }
  }

  nsresult rv;
  nsCOMPtr<nsIHttpProtocolHandler>
    service(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &rv));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoCString ua;
  rv = service->GetUserAgent(ua);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  CopyASCIItoUTF16(ua, aUserAgent);

  if (!aWindow || !aURI) {
    return NS_OK;
  }

  MOZ_ASSERT(aWindow->GetDocShell());

  nsCOMPtr<nsISiteSpecificUserAgent> siteSpecificUA =
    do_GetService("@mozilla.org/dom/site-specific-user-agent;1");
  if (!siteSpecificUA) {
    return NS_OK;
  }

  return siteSpecificUA->GetUserAgentForURIAndWindow(aURI, aWindow, aUserAgent);
}

} 
} 
