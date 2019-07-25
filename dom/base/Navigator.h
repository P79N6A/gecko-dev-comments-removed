





#ifndef mozilla_dom_Navigator_h
#define mozilla_dom_Navigator_h

#include "nsIDOMNavigator.h"
#include "nsIDOMNavigatorGeolocation.h"
#include "nsIDOMNavigatorDeviceStorage.h"
#include "nsIDOMNavigatorDesktopNotification.h"
#include "nsIDOMClientInformation.h"
#include "nsINavigatorBattery.h"
#include "nsIDOMNavigatorSms.h"
#include "nsIDOMNavigatorNetwork.h"
#include "nsIDOMNavigatorTime.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "DeviceStorage.h"

class nsPluginArray;
class nsMimeTypeArray;
class nsGeolocation;
class nsDesktopNotificationCenter;
class nsPIDOMWindow;
class nsIDOMMozConnection;

#ifdef MOZ_MEDIA_NAVIGATOR
#include "nsIDOMNavigatorUserMedia.h"
#endif

#ifdef MOZ_B2G_RIL
#include "nsIDOMNavigatorTelephony.h"
class nsIDOMTelephony;
class nsIDOMMozVoicemail;
#endif

#ifdef MOZ_B2G_BT
#include "nsIDOMNavigatorBluetooth.h"
#endif

#include "nsIDOMNavigatorSystemMessages.h"

#include "nsIDOMNavigatorCamera.h"
#include "DOMCameraManager.h"





namespace mozilla {
namespace dom {

namespace battery {
class BatteryManager;
} 

namespace sms {
class SmsManager;
} 

namespace network {
class Connection;
class MobileConnection;
} 

namespace power {
class PowerManager;
} 

namespace time {
class TimeManager;
} 

class Navigator : public nsIDOMNavigator
                , public nsIDOMClientInformation
                , public nsIDOMNavigatorDeviceStorage
                , public nsIDOMNavigatorGeolocation
                , public nsIDOMNavigatorDesktopNotification
                , public nsINavigatorBattery
                , public nsIDOMMozNavigatorSms
#ifdef MOZ_MEDIA_NAVIGATOR
                , public nsIDOMNavigatorUserMedia
#endif
#ifdef MOZ_B2G_RIL
                , public nsIDOMNavigatorTelephony
#endif
                , public nsIDOMMozNavigatorNetwork
#ifdef MOZ_B2G_BT
                , public nsIDOMNavigatorBluetooth
#endif
                , public nsIDOMNavigatorCamera
                , public nsIDOMNavigatorSystemMessages
                , public nsIDOMMozNavigatorTime
{
public:
  Navigator(nsPIDOMWindow *aInnerWindow);
  virtual ~Navigator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNAVIGATOR
  NS_DECL_NSIDOMCLIENTINFORMATION
  NS_DECL_NSIDOMNAVIGATORDEVICESTORAGE
  NS_DECL_NSIDOMNAVIGATORGEOLOCATION
  NS_DECL_NSIDOMNAVIGATORDESKTOPNOTIFICATION
  NS_DECL_NSINAVIGATORBATTERY
  NS_DECL_NSIDOMMOZNAVIGATORSMS
#ifdef MOZ_MEDIA_NAVIGATOR
  NS_DECL_NSIDOMNAVIGATORUSERMEDIA
#endif
#ifdef MOZ_B2G_RIL
  NS_DECL_NSIDOMNAVIGATORTELEPHONY
#endif
  NS_DECL_NSIDOMMOZNAVIGATORNETWORK

#ifdef MOZ_B2G_BT
  NS_DECL_NSIDOMNAVIGATORBLUETOOTH
#endif
  NS_DECL_NSIDOMNAVIGATORSYSTEMMESSAGES
  NS_DECL_NSIDOMMOZNAVIGATORTIME

  static void Init();

  void Invalidate();
  nsPIDOMWindow *GetWindow();

  void RefreshMIMEArray();

  static bool HasDesktopNotificationSupport();

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  


  void SetWindow(nsPIDOMWindow *aInnerWindow);

  


  void OnNavigation();

#ifdef MOZ_SYS_MSG
  
  nsresult EnsureMessagesManager();
#endif
  NS_DECL_NSIDOMNAVIGATORCAMERA

private:
  bool IsSmsAllowed() const;
  bool IsSmsSupported() const;
  bool CheckPermission(const char* aPref);

  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsRefPtr<nsGeolocation> mGeolocation;
  nsRefPtr<nsDesktopNotificationCenter> mNotification;
  nsRefPtr<battery::BatteryManager> mBatteryManager;
  nsRefPtr<power::PowerManager> mPowerManager;
  nsRefPtr<sms::SmsManager> mSmsManager;
#ifdef MOZ_B2G_RIL
  nsCOMPtr<nsIDOMTelephony> mTelephony;
  nsCOMPtr<nsIDOMMozVoicemail> mVoicemail;
#endif
  nsRefPtr<network::Connection> mConnection;
  nsRefPtr<network::MobileConnection> mMobileConnection;
#ifdef MOZ_B2G_BT
  nsCOMPtr<nsIDOMBluetoothManager> mBluetooth;
#endif
  nsRefPtr<nsDOMCameraManager> mCameraManager;
  nsCOMPtr<nsIDOMNavigatorSystemMessages> mMessagesManager;
  nsTArray<nsRefPtr<nsDOMDeviceStorage> > mDeviceStorageStores;
  nsRefPtr<time::TimeManager> mTimeManager;
  nsWeakPtr mWindow;
};

} 
} 

nsresult NS_GetNavigatorUserAgent(nsAString& aUserAgent);
nsresult NS_GetNavigatorPlatform(nsAString& aPlatform);
nsresult NS_GetNavigatorAppVersion(nsAString& aAppVersion);
nsresult NS_GetNavigatorAppName(nsAString& aAppName);

#endif 
