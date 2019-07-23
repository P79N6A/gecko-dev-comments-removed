





































#ifndef nsAlertsService_h__
#define nsAlertsService_h__

#include "nsIAlertsService.h"
#include "nsCOMPtr.h"

#define NS_ALERTSSERVICE_CID { 0xa0ccaaf8, 0x9da, 0x44d8, { 0xb2, 0x50, 0x9a, 0xc3, 0xe9, 0x3c, 0x81, 0x17 } }

class nsAlertsService : public nsIAlertsService
{
public:
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_ISUPPORTS

  nsAlertsService();
  virtual ~nsAlertsService();

protected:
};

#endif
