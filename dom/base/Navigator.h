









































#ifndef mozilla_dom_Navigator_h
#define mozilla_dom_Navigator_h

#include "nsIDOMNavigator.h"
#include "nsIDOMNavigatorGeolocation.h"
#include "nsIDOMNavigatorDesktopNotification.h"
#include "nsIDOMClientInformation.h"
#include "nsIDOMNavigatorBattery.h"
#include "nsIDOMNavigatorSms.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"

class nsPluginArray;
class nsMimeTypeArray;
class nsGeolocation;
class nsDesktopNotificationCenter;
class nsPIDOMWindow;





namespace mozilla {
namespace dom {

namespace battery {
class BatteryManager;
} 

namespace sms {
class SmsManager;
} 

class Navigator : public nsIDOMNavigator,
                  public nsIDOMClientInformation,
                  public nsIDOMNavigatorGeolocation,
                  public nsIDOMNavigatorDesktopNotification,
                  public nsIDOMMozNavigatorBattery,
                  public nsIDOMMozNavigatorSms
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

  static void Init();

  void Invalidate();
  nsPIDOMWindow *GetWindow();

  void RefreshMIMEArray();

  static bool HasDesktopNotificationSupport();

  PRInt64 SizeOf() const;

private:
  bool IsSmsAllowed() const;
  bool IsSmsSupported() const;

  static bool sDoNotTrackEnabled;

  nsRefPtr<nsMimeTypeArray> mMimeTypes;
  nsRefPtr<nsPluginArray> mPlugins;
  nsRefPtr<nsGeolocation> mGeolocation;
  nsRefPtr<nsDesktopNotificationCenter> mNotification;
  nsRefPtr<battery::BatteryManager> mBatteryManager;
  nsRefPtr<sms::SmsManager> mSmsManager;
  nsWeakPtr mWindow;
};

} 
} 

nsresult NS_GetNavigatorUserAgent(nsAString& aUserAgent);
nsresult NS_GetNavigatorPlatform(nsAString& aPlatform);
nsresult NS_GetNavigatorAppVersion(nsAString& aAppVersion);
nsresult NS_GetNavigatorAppName(nsAString& aAppName);

#endif 
