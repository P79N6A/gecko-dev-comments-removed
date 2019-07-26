



#ifndef nsMacAlertsService_h_
#define nsMacAlertsService_h_

#include "nsIAlertsService.h"
#include "nsIObserver.h"

struct GrowlDelegateWrapper;
struct NotificationCenterDelegateWrapper;

class nsMacAlertsService : public nsIAlertsService,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_NSIOBSERVER

  nsMacAlertsService();
  nsresult Init();

private:
  virtual ~nsMacAlertsService();
  nsresult InitGrowl();
  nsresult InitNotificationCenter();

  GrowlDelegateWrapper* mGrowlDelegate;
  NotificationCenterDelegateWrapper* mNCDelegate;
};

#endif 
