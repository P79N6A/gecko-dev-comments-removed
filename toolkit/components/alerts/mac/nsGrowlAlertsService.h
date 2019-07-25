



#ifndef nsGrowlAlertsService_h_
#define nsGrowlAlertsService_h_

#include "nsIAlertsService.h"
#include "nsIObserver.h"

struct GrowlDelegateWrapper;

class nsGrowlAlertsService : public nsIAlertsService,
                             public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_NSIOBSERVER

  nsGrowlAlertsService();
  nsresult Init();
private:
  GrowlDelegateWrapper* mDelegate;
  virtual ~nsGrowlAlertsService();
};

#endif 
