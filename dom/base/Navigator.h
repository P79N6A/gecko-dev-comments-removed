





#ifndef mozilla_dom_Navigator_h
#define mozilla_dom_Navigator_h

#include "mozilla/MemoryReporting.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMMobileMessageManager.h"
#include "nsIMozNavigatorNetwork.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "DeviceStorage.h"
#include "nsWrapperCache.h"

class nsPluginArray;
class nsMimeTypeArray;
class nsPIDOMWindow;
class nsIDOMMozConnection;

namespace mozilla {
namespace dom {
class Geolocation;
class systemMessageCallback;
}
}

#ifdef MOZ_MEDIA_NAVIGATOR
#include "nsIDOMNavigatorUserMedia.h"
#endif

#ifdef MOZ_B2G_RIL
class nsIDOMTelephony;
class nsIDOMMozMobileConnection;
class nsIDOMMozCellBroadcast;
class nsIDOMMozVoicemail;
class nsIDOMMozIccManager;
#endif 

#ifdef MOZ_B2G_BT
class nsIDOMBluetoothManager;
#endif 

#include "nsIDOMNavigatorSystemMessages.h"

#include "DOMCameraManager.h"





void NS_GetNavigatorAppName(nsAString& aAppName);

namespace mozilla {
namespace dom {

namespace battery {
class BatteryManager;
} 

class DesktopNotificationCenter;
class MobileMessageManager;
class MozIdleObserver;
#ifdef MOZ_GAMEPAD
class Gamepad;
#endif 
#ifdef MOZ_MEDIA_NAVIGATOR
class MozDOMGetUserMediaSuccessCallback;
class MozDOMGetUserMediaErrorCallback;
class MozGetUserMediaDevicesSuccessCallback;
#endif 

namespace icc {
#ifdef MOZ_B2G_RIL
class IccManager;
#endif
}

namespace network {
class Connection;
#ifdef MOZ_B2G_RIL
class MobileConnection;
#endif
} 

namespace power {
class PowerManager;
} 

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

  
  void GetAppName(nsString& aAppName)
  {
    NS_GetNavigatorAppName(aAppName);
  }
  void GetAppVersion(nsString& aAppVersion, ErrorResult& aRv)
  {
    aRv = GetAppVersion(aAppVersion);
  }
  void GetPlatform(nsString& aPlatform, ErrorResult& aRv)
  {
    aRv = GetPlatform(aPlatform);
  }
  void GetUserAgent(nsString& aUserAgent, ErrorResult& aRv)
  {
    aRv = GetUserAgent(aUserAgent);
  }
  
  
  bool OnLine();
  void RegisterProtocolHandler(const nsAString& aScheme, const nsAString& aURL,
                               const nsAString& aTitle, ErrorResult& aRv);
  void RegisterContentHandler(const nsAString& aMIMEType, const nsAString& aURL,
                              const nsAString& aTitle, ErrorResult& aRv);
  nsMimeTypeArray* GetMimeTypes(ErrorResult& aRv);
  nsPluginArray* GetPlugins(ErrorResult& aRv);
  
  Geolocation* GetGeolocation(ErrorResult& aRv);
  battery::BatteryManager* GetBattery(ErrorResult& aRv);
  void Vibrate(uint32_t aDuration, ErrorResult& aRv);
  void Vibrate(const nsTArray<uint32_t>& aDuration, ErrorResult& aRv);
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
  nsIDOMMozPowerManager* GetMozPower(ErrorResult& aRv);
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
  nsIDOMMozConnection* GetMozConnection();
  nsDOMCameraManager* GetMozCameras(ErrorResult& aRv);
  void MozSetMessageHandler(const nsAString& aType,
                            systemMessageCallback* aCallback,
                            ErrorResult& aRv);
  bool MozHasPendingMessage(const nsAString& aType, ErrorResult& aRv);
#ifdef MOZ_B2G_RIL
  nsIDOMTelephony* GetMozTelephony(ErrorResult& aRv);
  nsIDOMMozMobileConnection* GetMozMobileConnection(ErrorResult& aRv);
  nsIDOMMozCellBroadcast* GetMozCellBroadcast(ErrorResult& aRv);
  nsIDOMMozVoicemail* GetMozVoicemail(ErrorResult& aRv);
  nsIDOMMozIccManager* GetMozIccManager(ErrorResult& aRv);
#endif 
#ifdef MOZ_GAMEPAD
  void GetGamepads(nsTArray<nsRefPtr<Gamepad> >& aGamepads, ErrorResult& aRv);
#endif 
#ifdef MOZ_B2G_BT
  nsIDOMBluetoothManager* GetMozBluetooth(ErrorResult& aRv);
#endif 
#ifdef MOZ_TIME_MANAGER
  time::TimeManager* GetMozTime(ErrorResult& aRv);
#endif 
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  system::AudioChannelManager* GetMozAudioChannelManager(ErrorResult& aRv);
#endif 
#ifdef MOZ_MEDIA_NAVIGATOR
  void MozGetUserMedia(nsIMediaStreamOptions* aParams,
                       MozDOMGetUserMediaSuccessCallback* aOnSuccess,
                       MozDOMGetUserMediaErrorCallback* aOnError,
                       ErrorResult& aRv);
  void MozGetUserMediaDevices(MozGetUserMediaDevicesSuccessCallback* aOnSuccess,
                              MozDOMGetUserMediaErrorCallback* aOnError,
                              ErrorResult& aRv);
#endif 
  bool DoNewResolve(JSContext* aCx, JS::Handle<JSObject*> aObject,
                    JS::Handle<jsid> aId, JS::MutableHandle<JS::Value> aValue);
  void GetOwnPropertyNames(JSContext* aCx, nsTArray<nsString>& aNames,
                           ErrorResult& aRv);

  
  static bool HasBatterySupport(JSContext* , JSObject* );
  static bool HasPowerSupport(JSContext* , JSObject* aGlobal);
  static bool HasIdleSupport(JSContext* , JSObject* aGlobal);
  static bool HasWakeLockSupport(JSContext* , JSObject* );
  static bool HasDesktopNotificationSupport(JSContext* ,
                                            JSObject* )
  {
    return HasDesktopNotificationSupport();
  }
  static bool HasMobileMessageSupport(JSContext* ,
                                      JSObject* aGlobal);
  static bool HasCameraSupport(JSContext* ,
                               JSObject* aGlobal);
#ifdef MOZ_B2G_RIL
  static bool HasTelephonySupport(JSContext* ,
                                  JSObject* aGlobal);
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
#ifdef MOZ_TIME_MANAGER
  static bool HasTimeSupport(JSContext* , JSObject* aGlobal);
#endif 
#ifdef MOZ_MEDIA_NAVIGATOR
  static bool HasUserMediaSupport(JSContext* ,
                                  JSObject* );
#endif 

  static bool HasPushNotificationsSupport(JSContext* ,
                                          JSObject* aGlobal);

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
  nsRefPtr<power::PowerManager> mPowerManager;
  nsRefPtr<MobileMessageManager> mMobileMessageManager;
#ifdef MOZ_B2G_RIL
  nsCOMPtr<nsIDOMTelephony> mTelephony;
  nsCOMPtr<nsIDOMMozVoicemail> mVoicemail;
#endif
  nsRefPtr<network::Connection> mConnection;
#ifdef MOZ_B2G_RIL
  nsRefPtr<network::MobileConnection> mMobileConnection;
  nsCOMPtr<nsIDOMMozCellBroadcast> mCellBroadcast;
  nsRefPtr<icc::IccManager> mIccManager;
#endif
#ifdef MOZ_B2G_BT
  nsCOMPtr<nsIDOMBluetoothManager> mBluetooth;
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
