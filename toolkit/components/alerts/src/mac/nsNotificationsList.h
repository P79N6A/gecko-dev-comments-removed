




































#ifndef nsNotificationsList_h_
#define nsNotificationsList_h_

#include "nsINotificationsList.h"
#import "mozGrowlDelegate.h"

class nsNotificationsList : public nsINotificationsList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINOTIFICATIONSLIST

  nsNotificationsList();

  void informController(mozGrowlDelegate *aCont);
private:
  virtual ~nsNotificationsList();

  NSMutableArray *mNames;
  NSMutableArray *mEnabled;
};

#endif 
