




#ifndef nsSystemAlertsService_h__
#define nsSystemAlertsService_h__

#include "nsIAlertsService.h"
#include "nsCOMPtr.h"

class nsSystemAlertsService : public nsIAlertsService
{
public:
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_ISUPPORTS

  nsSystemAlertsService();

  nsresult Init();

protected:
  virtual ~nsSystemAlertsService();

};

#endif 
