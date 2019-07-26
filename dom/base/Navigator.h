





#ifndef mozilla_dom_Navigator_h
#define mozilla_dom_Navigator_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/ErrorResult.h"
#include "nsIDOMNavigator.h"
#include "nsIMozNavigatorNetwork.h"
#include "nsAutoPtr.h"
#include "nsWrapperCache.h"
#include "nsString.h"
#include "nsTArray.h"

class nsPluginArray;
class nsMimeTypeArray;
class nsPIDOMWindow;
class nsIDOMMozConnection;
class nsIDOMMozMobileMessageManager;
class nsIDOMNavigatorSystemMessages;
class nsDOMCameraManager;
class nsDOMDeviceStorage;

namespace mozilla {
namespace dom {
class Geolocation;
class systemMessageCallback;
class MediaStreamConstraints;
class MediaStreamConstraintsInternal;
}
}

#ifdef MOZ_B2G_RIL
class nsIDOMMozIccManager;
#endif 





void NS_GetNavigatorAppName(nsAString& aAppName);

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
#ifdef MOZ_B2G_RIL
class MobileConnectionArray;
#endif
} 

#ifdef MOZ_B2G_BT
namespace bluetooth {
class BluetoothManager;
} 
#endif 

#ifdef MOZ_B2G_RIL
class CellBroadcast;
class IccManager;
class Voicemail;
#endif

class PowerManager;
class Telephony;

namespace time {
class TimeManager;
} 

namespace system {
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
class AudioChannelManager;
#endif
} 

class Navigator : public nsIDOMNavigator
                , public nsIMozNavigatorNetwork
                , public nsWrapperCache
{
public:
  Navigator(nsPIDOMWindow *aInnerWindow);
  virtual ~Navigator();

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

  static bool HasDesktopNotificationSupport();

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
  already_AddRefed<Promise> GetDataStores(const nsAString &aName,
                                          ErrorResult& aRv);
  bool Vibrate(uint32_t aDuration);
  bool Vibrate(const nsTArray<uint32_t>& aDuration);
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
  already_AddRefed<nsIDOMMozWakeLock> RequestWakeLock(const nsAString &aTopic,
                                                      ErrorResult& aRv);
  nsDOMDeviceStorage* GetDeviceStorage(const nsAString& aType,
                                       ErrorResult& aRv);
  void GetDeviceStorages(const nsAString& aType,
                         nsTArray<nsRefPtr<nsDOMDeviceStorage> >& aStores,
                         ErrorResult& aRv);
  DesktopNotificationCenter* GetMozNotification(ErrorResult& aRv);
  bool MozIsLocallyAvailable(const nsAString& aURI, bool aWhenOffline,
                             ErrorResult& aRv);
  nsIDOMMozMobileMessageManager* GetMozMobileMessage();
  Telephony* GetMozTelephony(ErrorResult& aRv);
  nsIDOMMozConnection* GetMozConnection();
  nsDOMCameraManager* GetMozCameras(ErrorResult& aRv);
  void MozSetMessageHandler(const nsAString& aType,
                            systemMessageCallback* aCallback,
                            ErrorResult& aRv);
  bool MozHasPendingMessage(const nsAString& aType, ErrorResult& aRv);
#ifdef MOZ_B2G_RIL
  network::MobileConnectionArray* GetMozMobileConnections(ErrorResult& aRv);
  CellBroadcast* GetMozCellBroadcast(ErrorResult& aRv);
  Voicemail* GetMozVoicemail(ErrorResult& aRv);
  nsIDOMMozIccManager* GetMozIccManager(ErrorResult& aRv);
#endif 
#ifdef MOZ_GAMEPAD
  void GetGamepads(nsTArray<nsRefPtr<Gamepad> >& aGamepads, ErrorResult& aRv);
#endif 
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
#ifdef MOZ_MEDIA_NAVIGATOR
  void MozGetUserMedia(JSContext* aCx,
                       const MediaStreamConstraints& aConstraints,
                       NavigatorUserMediaSuccessCallback& aOnSuccess,
                       NavigatorUserMediaErrorCallback& aOnError,
                       ErrorResult& aRv);
  void MozGetUserMediaDevices(const MediaStreamConstraintsInternal& aConstraints,
                              MozGetUserMediaDevicesSuccessCallback& aOnSuccess,
                              NavigatorUserMediaErrorCallback& aOnError,
                              ErrorResult& aRv);
#endif 
  bool DoNewResolve(JSContext* aCx, JS::Handle<JSObject*> aObject,
                    JS::Handle<jsid> aId, JS::MutableHandle<JS::Value> aValue);
  void GetOwnPropertyNames(JSContext* aCx, nsTArray<nsString>& aNames,
                           ErrorResult& aRv);

  
  static bool HasBatterySupport(JSContext* , JSObject* );
  static bool HasPowerSupport(JSContext* , JSObject* aGlobal);
  static bool HasPhoneNumberSupport(JSContext* , JSObject* aGlobal);
  static bool HasIdleSupport(JSContext* , JSObject* aGlobal);
  static bool HasWakeLockSupport(JSContext* , JSObject* );
  static bool HasDesktopNotificationSupport(JSContext* ,
                                            JSObject* )
  {
    return HasDesktopNotificationSupport();
  }
  static bool HasMobileMessageSupport(JSContext* ,
                                      JSObject* aGlobal);
  static bool HasTelephonySupport(JSContext* ,
                                  JSObject* aGlobal);
  static bool HasCameraSupport(JSContext* ,
                               JSObject* aGlobal);
#ifdef MOZ_B2G_RIL
  static bool HasMobileConnectionSupport(JSContext* ,
                                         JSObject* aGlobal);
  static bool HasCellBroadcastSupport(JSContext* ,
                                      JSObject* aGlobal);
  static bool HasVoicemailSupport(JSContext* ,
                                  JSObject* aGlobal);
  static bool HasIccManagerSupport(JSContext* ,
                                   JSObject* aGlobal);
#endif 
#ifdef MOZ_B2G_BT
  static bool HasBluetoothSupport(JSContext* , JSObject* aGlobal);
#endif 
#ifdef MOZ_B2G_FM
  static bool HasFMRadioSupport(JSContext* , JSObject* aGlobal);
#endif 
#ifdef MOZ_NFC
  static bool HasNfcSupport(JSContext* , JSObject* aGlobal);
#endif 
#ifdef MOZ_TIME_MANAGER
  static bool HasTimeSupport(JSContext* , JSObject* aGlobal);
#endif 
#ifdef MOZ_MEDIA_NAVIGATOR
  static bool HasUserMediaSupport(JSContext* ,
                                  JSObject* );
#endif 

  static bool HasPushNotificationsSupport(JSContext* ,
                                          JSObject* aGlobal);

  static bool HasInputMethodSupport(JSContext* , JSObject* aGlobal);

  nsPIDOMWindow* GetParentObject() const
  {
    return GetWindow();
  }

  virtual JSObject* WrapObject(JSContext* cx,
                               JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

private:
  bool CheckPermission(const char* type);
  static bool CheckPermission(nsPIDOMWindow* aWindow, const char* aType);
  
  
  static already_AddRefed<nsPIDOMWindow> GetWindowFromGlobal(JSObject* aGlobal);

  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsRefPtr<Geolocation> mGeolocation;
  nsRefPtr<DesktopNotificationCenter> mNotification;
  nsRefPtr<battery::BatteryManager> mBatteryManager;
#ifdef MOZ_B2G_FM
  nsRefPtr<FMRadio> mFMRadio;
#endif
  nsRefPtr<PowerManager> mPowerManager;
  nsRefPtr<MobileMessageManager> mMobileMessageManager;
  nsRefPtr<Telephony> mTelephony;
  nsRefPtr<network::Connection> mConnection;
#ifdef MOZ_B2G_RIL
  nsRefPtr<network::MobileConnectionArray> mMobileConnections;
  nsRefPtr<CellBroadcast> mCellBroadcast;
  nsRefPtr<IccManager> mIccManager;
  nsRefPtr<Voicemail> mVoicemail;
#endif
#ifdef MOZ_B2G_BT
  nsCOMPtr<bluetooth::BluetoothManager> mBluetooth;
#endif
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  nsRefPtr<system::AudioChannelManager> mAudioChannelManager;
#endif
  nsRefPtr<nsDOMCameraManager> mCameraManager;
  nsCOMPtr<nsIDOMNavigatorSystemMessages> mMessagesManager;
  nsTArray<nsRefPtr<nsDOMDeviceStorage> > mDeviceStorageStores;
  nsRefPtr<time::TimeManager> mTimeManager;
  nsCOMPtr<nsPIDOMWindow> mWindow;
};

} 
} 

nsresult NS_GetNavigatorUserAgent(nsAString& aUserAgent);
nsresult NS_GetNavigatorPlatform(nsAString& aPlatform);
nsresult NS_GetNavigatorAppVersion(nsAString& aAppVersion);

#endif 
