





#ifndef mozilla_dom_Navigator_h
#define mozilla_dom_Navigator_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Nullable.h"
#include "mozilla/ErrorResult.h"
#include "nsIDOMNavigator.h"
#include "nsIMozNavigatorNetwork.h"
#include "nsAutoPtr.h"
#include "nsWrapperCache.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"
#include "nsString.h"
#include "nsTArray.h"
#ifdef MOZ_EME
#include "mozilla/dom/MediaKeySystemAccess.h"
#endif

class nsPluginArray;
class nsMimeTypeArray;
class nsPIDOMWindow;
class nsIDOMNavigatorSystemMessages;
class nsDOMCameraManager;
class nsDOMDeviceStorage;
class nsIPrincipal;
class nsIURI;

namespace mozilla {
namespace dom {
class Geolocation;
class systemMessageCallback;
class MediaDevices;
struct MediaStreamConstraints;
class WakeLock;
class ArrayBufferViewOrBlobOrStringOrFormData;
struct MobileIdOptions;
class ServiceWorkerContainer;
}
}





namespace mozilla {
namespace dom {

namespace battery {
class BatteryManager;
} 

#ifdef MOZ_B2G_FM
class FMRadio;
#endif

class Promise;

class DesktopNotificationCenter;
class MobileMessageManager;
class MozIdleObserver;
#ifdef MOZ_GAMEPAD
class Gamepad;
#endif 
#ifdef MOZ_MEDIA_NAVIGATOR
class NavigatorUserMediaSuccessCallback;
class NavigatorUserMediaErrorCallback;
class MozGetUserMediaDevicesSuccessCallback;
#endif 

namespace network {
class Connection;
} 

#ifdef MOZ_B2G_BT
namespace bluetooth {
class BluetoothManager;
} 
#endif 

#ifdef MOZ_B2G_RIL
class MobileConnectionArray;
#endif

class PowerManager;
class CellBroadcast;
class IccManager;
class Telephony;
class Voicemail;
class TVManager;

namespace time {
class TimeManager;
} 

namespace system {
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
class AudioChannelManager;
#endif
} 

class Navigator final : public nsIDOMNavigator
                          , public nsIMozNavigatorNetwork
                          , public nsWrapperCache
{
public:
  explicit Navigator(nsPIDOMWindow* aInnerWindow);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Navigator,
                                                         nsIDOMNavigator)
  NS_DECL_NSIDOMNAVIGATOR
  NS_DECL_NSIMOZNAVIGATORNETWORK

  static void Init();

  void Invalidate();
  nsPIDOMWindow *GetWindow() const
  {
    return mWindow;
  }

  void RefreshMIMEArray();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  


  void SetWindow(nsPIDOMWindow *aInnerWindow);

  


  void OnNavigation();

  
  nsresult EnsureMessagesManager();

  
  
  bool OnLine();
  void RegisterProtocolHandler(const nsAString& aScheme, const nsAString& aURL,
                               const nsAString& aTitle, ErrorResult& aRv);
  void RegisterContentHandler(const nsAString& aMIMEType, const nsAString& aURL,
                              const nsAString& aTitle, ErrorResult& aRv);
  nsMimeTypeArray* GetMimeTypes(ErrorResult& aRv);
  nsPluginArray* GetPlugins(ErrorResult& aRv);
  
  Geolocation* GetGeolocation(ErrorResult& aRv);
  battery::BatteryManager* GetBattery(ErrorResult& aRv);

  static already_AddRefed<Promise> GetDataStores(nsPIDOMWindow* aWindow,
                                                 const nsAString& aName,
                                                 const nsAString& aOwner,
                                                 ErrorResult& aRv);

  static void AppName(nsAString& aAppName, bool aUsePrefOverriddenValue);

  static nsresult GetPlatform(nsAString& aPlatform,
                              bool aUsePrefOverriddenValue);

  static nsresult GetAppVersion(nsAString& aAppVersion,
                                bool aUsePrefOverriddenValue);

  static nsresult GetUserAgent(nsPIDOMWindow* aWindow,
                               nsIURI* aURI,
                               bool aIsCallerChrome,
                               nsAString& aUserAgent);

  already_AddRefed<Promise> GetDataStores(const nsAString& aName,
                                          const nsAString& aOwner,
                                          ErrorResult& aRv);

  
  already_AddRefed<Promise> GetFeature(const nsAString& aName,
                                       ErrorResult& aRv);

  already_AddRefed<Promise> HasFeature(const nsAString &aName,
                                       ErrorResult& aRv);

  bool Vibrate(uint32_t aDuration);
  bool Vibrate(const nsTArray<uint32_t>& aDuration);
  uint32_t MaxTouchPoints();
  void GetAppCodeName(nsString& aAppCodeName, ErrorResult& aRv)
  {
    aRv = GetAppCodeName(aAppCodeName);
  }
  void GetOscpu(nsString& aOscpu, ErrorResult& aRv)
  {
    aRv = GetOscpu(aOscpu);
  }
  
  
  
  bool CookieEnabled();
  void GetBuildID(nsString& aBuildID, ErrorResult& aRv)
  {
    aRv = GetBuildID(aBuildID);
  }
  PowerManager* GetMozPower(ErrorResult& aRv);
  bool JavaEnabled(ErrorResult& aRv);
  bool TaintEnabled()
  {
    return false;
  }
  void AddIdleObserver(MozIdleObserver& aObserver, ErrorResult& aRv);
  void RemoveIdleObserver(MozIdleObserver& aObserver, ErrorResult& aRv);
  already_AddRefed<WakeLock> RequestWakeLock(const nsAString &aTopic,
                                             ErrorResult& aRv);
  nsDOMDeviceStorage* GetDeviceStorage(const nsAString& aType,
                                       ErrorResult& aRv);
  void GetDeviceStorages(const nsAString& aType,
                         nsTArray<nsRefPtr<nsDOMDeviceStorage> >& aStores,
                         ErrorResult& aRv);
  DesktopNotificationCenter* GetMozNotification(ErrorResult& aRv);
  CellBroadcast* GetMozCellBroadcast(ErrorResult& aRv);
  IccManager* GetMozIccManager(ErrorResult& aRv);
  MobileMessageManager* GetMozMobileMessage();
  Telephony* GetMozTelephony(ErrorResult& aRv);
  Voicemail* GetMozVoicemail(ErrorResult& aRv);
  TVManager* GetTv();
  network::Connection* GetConnection(ErrorResult& aRv);
  nsDOMCameraManager* GetMozCameras(ErrorResult& aRv);
  MediaDevices* GetMediaDevices(ErrorResult& aRv);
  void MozSetMessageHandler(const nsAString& aType,
                            systemMessageCallback* aCallback,
                            ErrorResult& aRv);
  bool MozHasPendingMessage(const nsAString& aType, ErrorResult& aRv);
  void MozSetMessageHandlerPromise(Promise& aPromise, ErrorResult& aRv);

#ifdef MOZ_B2G
  already_AddRefed<Promise> GetMobileIdAssertion(const MobileIdOptions& options,
                                                 ErrorResult& aRv);
#endif
#ifdef MOZ_B2G_RIL
  MobileConnectionArray* GetMozMobileConnections(ErrorResult& aRv);
#endif 
#ifdef MOZ_GAMEPAD
  void GetGamepads(nsTArray<nsRefPtr<Gamepad> >& aGamepads, ErrorResult& aRv);
#endif 
  already_AddRefed<Promise> GetVRDevices(ErrorResult& aRv);
#ifdef MOZ_B2G_FM
  FMRadio* GetMozFMRadio(ErrorResult& aRv);
#endif
#ifdef MOZ_B2G_BT
  bluetooth::BluetoothManager* GetMozBluetooth(ErrorResult& aRv);
#endif 
#ifdef MOZ_TIME_MANAGER
  time::TimeManager* GetMozTime(ErrorResult& aRv);
#endif 
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  system::AudioChannelManager* GetMozAudioChannelManager(ErrorResult& aRv);
#endif 

  bool SendBeacon(const nsAString& aUrl,
                  const Nullable<ArrayBufferViewOrBlobOrStringOrFormData>& aData,
                  ErrorResult& aRv);

#ifdef MOZ_MEDIA_NAVIGATOR
  void MozGetUserMedia(const MediaStreamConstraints& aConstraints,
                       NavigatorUserMediaSuccessCallback& aOnSuccess,
                       NavigatorUserMediaErrorCallback& aOnError,
                       ErrorResult& aRv);
  void MozGetUserMediaDevices(const MediaStreamConstraints& aConstraints,
                              MozGetUserMediaDevicesSuccessCallback& aOnSuccess,
                              NavigatorUserMediaErrorCallback& aOnError,
                              uint64_t aInnerWindowID,
                              ErrorResult& aRv);
#endif 

  already_AddRefed<ServiceWorkerContainer> ServiceWorker();

  bool DoResolve(JSContext* aCx, JS::Handle<JSObject*> aObject,
                 JS::Handle<jsid> aId,
                 JS::MutableHandle<JSPropertyDescriptor> aDesc);
  void GetOwnPropertyNames(JSContext* aCx, nsTArray<nsString>& aNames,
                           ErrorResult& aRv);
  void GetLanguages(nsTArray<nsString>& aLanguages);

  bool MozE10sEnabled();

  static void GetAcceptLanguages(nsTArray<nsString>& aLanguages);

  
  static bool HasWakeLockSupport(JSContext* , JSObject* );
  static bool HasCameraSupport(JSContext* ,
                               JSObject* aGlobal);
  static bool HasWifiManagerSupport(JSContext* ,
                                  JSObject* aGlobal);
#ifdef MOZ_NFC
  static bool HasNFCSupport(JSContext* , JSObject* aGlobal);
#endif 
#ifdef MOZ_MEDIA_NAVIGATOR
  static bool HasUserMediaSupport(JSContext* ,
                                  JSObject* );
#endif 

  static bool HasInputMethodSupport(JSContext* , JSObject* aGlobal);

  static bool HasDataStoreSupport(nsIPrincipal* aPrincipal);

  static bool HasDataStoreSupport(JSContext* cx, JSObject* aGlobal);

#ifdef MOZ_B2G
  static bool HasMobileIdSupport(JSContext* aCx, JSObject* aGlobal);
#endif

  static bool HasTVSupport(JSContext* aCx, JSObject* aGlobal);

  static bool IsE10sEnabled(JSContext* aCx, JSObject* aGlobal);

  nsPIDOMWindow* GetParentObject() const
  {
    return GetWindow();
  }

  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override;

  
  
  static already_AddRefed<nsPIDOMWindow> GetWindowFromGlobal(JSObject* aGlobal);

#ifdef MOZ_EME
  already_AddRefed<Promise>
  RequestMediaKeySystemAccess(const nsAString& aKeySystem,
                              const Optional<Sequence<MediaKeySystemOptions>>& aOptions,
                              ErrorResult& aRv);
#endif

private:
  virtual ~Navigator();

  bool CheckPermission(const char* type);
  static bool CheckPermission(nsPIDOMWindow* aWindow, const char* aType);

  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsRefPtr<Geolocation> mGeolocation;
  nsRefPtr<DesktopNotificationCenter> mNotification;
  nsRefPtr<battery::BatteryManager> mBatteryManager;
#ifdef MOZ_B2G_FM
  nsRefPtr<FMRadio> mFMRadio;
#endif
  nsRefPtr<PowerManager> mPowerManager;
  nsRefPtr<CellBroadcast> mCellBroadcast;
  nsRefPtr<IccManager> mIccManager;
  nsRefPtr<MobileMessageManager> mMobileMessageManager;
  nsRefPtr<Telephony> mTelephony;
  nsRefPtr<Voicemail> mVoicemail;
  nsRefPtr<TVManager> mTVManager;
  nsRefPtr<network::Connection> mConnection;
#ifdef MOZ_B2G_RIL
  nsRefPtr<MobileConnectionArray> mMobileConnections;
#endif
#ifdef MOZ_B2G_BT
  nsRefPtr<bluetooth::BluetoothManager> mBluetooth;
#endif
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  nsRefPtr<system::AudioChannelManager> mAudioChannelManager;
#endif
  nsRefPtr<nsDOMCameraManager> mCameraManager;
  nsRefPtr<MediaDevices> mMediaDevices;
  nsCOMPtr<nsIDOMNavigatorSystemMessages> mMessagesManager;
  nsTArray<nsRefPtr<nsDOMDeviceStorage> > mDeviceStorageStores;
  nsRefPtr<time::TimeManager> mTimeManager;
  nsRefPtr<ServiceWorkerContainer> mServiceWorkerContainer;
  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  
  
  
  
  nsInterfaceHashtable<nsStringHashKey, nsISupports> mCachedResolveResults;
};

} 
} 

#endif 
