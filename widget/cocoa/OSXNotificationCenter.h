




#ifndef OSXNotificationCenter_h
#define OSXNotificationCenter_h

#import <Foundation/Foundation.h>
#include "nsIAlertsService.h"
#include "imgINotificationObserver.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "mozilla/RefPtr.h"

@class mozNotificationCenterDelegate;

namespace mozilla {

class OSXNotificationInfo;

class OSXNotificationCenter : public nsIAlertsService,
                              public imgINotificationObserver,
                              public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_IMGINOTIFICATIONOBSERVER
  NS_DECL_NSITIMERCALLBACK

  OSXNotificationCenter();

  nsresult Init();
  void CloseAlertCocoaString(NSString *aAlertName);
  void OnClick(NSString *aAlertName);
  void ShowPendingNotification(OSXNotificationInfo *osxni);

protected:
  virtual ~OSXNotificationCenter();

private:
  mozNotificationCenterDelegate *mDelegate;
  nsTArray<nsRefPtr<OSXNotificationInfo> > mActiveAlerts;
  nsTArray<nsRefPtr<OSXNotificationInfo> > mPendingAlerts;
};

} 

#endif 
