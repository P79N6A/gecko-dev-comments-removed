




































#ifndef icabprofilemigrator___h___
#define icabprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"

class nsICabProfileMigrator : public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsICabProfileMigrator();
  virtual ~nsICabProfileMigrator();

protected:

private:
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
