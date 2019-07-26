




#ifndef nsAlertsService_h__
#define nsAlertsService_h__

#include "nsIAlertsService.h"
#include "nsCOMPtr.h"
#include "nsXULAlerts.h"

#ifdef XP_WIN
typedef enum tagMOZ_QUERY_USER_NOTIFICATION_STATE {
    QUNS_NOT_PRESENT = 1,
    QUNS_BUSY = 2,
    QUNS_RUNNING_D3D_FULL_SCREEN = 3,
    QUNS_PRESENTATION_MODE = 4,
    QUNS_ACCEPTS_NOTIFICATIONS = 5,
    QUNS_QUIET_TIME = 6,
    QUNS_IMMERSIVE = 7
} MOZ_QUERY_USER_NOTIFICATION_STATE;

extern "C" {

typedef HRESULT (__stdcall *SHQueryUserNotificationStatePtr)(MOZ_QUERY_USER_NOTIFICATION_STATE *pquns);
}
#endif 

class nsAlertsService : public nsIAlertsService,
                        public nsIAlertsProgressListener
{
public:
  NS_DECL_NSIALERTSPROGRESSLISTENER
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_ISUPPORTS

  nsAlertsService();
  virtual ~nsAlertsService();

protected:
  bool ShouldShowAlert();
  nsXULAlerts mXULAlerts;
};

#endif 
