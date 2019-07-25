



































#ifndef nsAlertsService_h_
#define nsAlertsService_h_

#include "nsIAlertsService.h"
#include "nsIObserver.h"

struct GrowlDelegateWrapper;

class nsAlertsService : public nsIAlertsService,
                        public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_NSIOBSERVER

  nsAlertsService();
  nsresult Init();
private:
  GrowlDelegateWrapper* mDelegate;
  virtual ~nsAlertsService();
};

#endif 
