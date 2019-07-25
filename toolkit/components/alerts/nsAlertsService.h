





































#ifndef nsAlertsService_h__
#define nsAlertsService_h__

#include "nsIAlertsService.h"
#include "nsCOMPtr.h"

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
};

#endif
