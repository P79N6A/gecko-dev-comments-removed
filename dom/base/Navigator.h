









































#ifndef mozilla_dom_Navigator_h
#define mozilla_dom_Navigator_h

#include "nsIDOMNavigator.h"
#include "nsIDOMNavigatorGeolocation.h"
#include "nsIDOMNavigatorDesktopNotification.h"
#include "nsIDOMClientInformation.h"
#include "nsIDOMNavigatorBattery.h"
#include "nsIDOMNavigatorSms.h"
#include "nsIDOMNavigatorNetwork.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"

class nsPluginArray;
class nsMimeTypeArray;
class nsGeolocation;
class nsDesktopNotificationCenter;
class nsPIDOMWindow;
class nsIDOMMozConnection;

#ifdef MOZ_B2G_RIL
#include "nsIDOMNavigatorTelephony.h"
class nsIDOMTelephony;
#endif

#ifdef MOZ_B2G_BT
#include "nsIDOMNavigatorBluetooth.h"
#endif

#ifdef ANDROID
#include "nsIDOMNavigatorUserMedia.h"
#endif

class nsIDOMAdapter;




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
} 

namespace power {
class PowerManager;
} 

class Navigator : public nsIDOMNavigator
                , public nsIDOMClientInformation
                , public nsIDOMNavigatorGeolocation
                , public nsIDOMNavigatorDesktopNotification
                , public nsIDOMMozNavigatorBattery
                , public nsIDOMMozNavigatorSms
#ifdef MOZ_B2G_RIL
                , public nsIDOMNavigatorTelephony
#endif
                , public nsIDOMMozNavigatorNetwork
#ifdef MOZ_B2G_BT
                , public nsIDOMNavigatorBluetooth
#endif
#ifdef ANDROID
                , public nsIDOMNavigatorUserMedia
#endif
{
public:
  Navigator(nsPIDOMWindow *aInnerWindow);
  virtual ~Navigator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNAVIGATOR
  NS_DECL_NSIDOMCLIENTINFORMATION
  NS_DECL_NSIDOMNAVIGATORGEOLOCATION
  NS_DECL_NSIDOMNAVIGATORDESKTOPNOTIFICATION
  NS_DECL_NSIDOMMOZNAVIGATORBATTERY
  NS_DECL_NSIDOMMOZNAVIGATORSMS
#ifdef MOZ_B2G_RIL
  NS_DECL_NSIDOMNAVIGATORTELEPHONY
#endif
  NS_DECL_NSIDOMMOZNAVIGATORNETWORK

#ifdef MOZ_B2G_BT
  NS_DECL_NSIDOMNAVIGATORBLUETOOTH
#endif

#ifdef ANDROID
  NS_DECL_NSIDOMNAVIGATORUSERMEDIA
#endif

  static void Init();

  void Invalidate();
  nsPIDOMWindow *GetWindow();

  void RefreshMIMEArray();

  static bool HasDesktopNotificationSupport();

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  


  void SetWindow(nsPIDOMWindow *aInnerWindow);

private:
  bool IsSmsAllowed() const;
  bool IsSmsSupported() const;

  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsRefPtr<nsGeolocation> mGeolocation;
  nsRefPtr<nsDesktopNotificationCenter> mNotification;
  nsRefPtr<battery::BatteryManager> mBatteryManager;
  nsRefPtr<power::PowerManager> mPowerManager;
  nsRefPtr<sms::SmsManager> mSmsManager;
#ifdef MOZ_B2G_RIL
  nsCOMPtr<nsIDOMTelephony> mTelephony;
#endif
  nsRefPtr<network::Connection> mConnection;
#ifdef MOZ_B2G_BT
  nsCOMPtr<nsIDOMBluetoothAdapter> mBluetooth;
#endif
  nsWeakPtr mWindow;
};

} 
} 

nsresult NS_GetNavigatorUserAgent(nsAString& aUserAgent);
nsresult NS_GetNavigatorPlatform(nsAString& aPlatform);
nsresult NS_GetNavigatorAppVersion(nsAString& aAppVersion);
nsresult NS_GetNavigatorAppName(nsAString& aAppName);

#endif 
