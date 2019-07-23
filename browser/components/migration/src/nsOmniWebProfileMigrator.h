




































#ifndef omniwebprofilemigrator___h___
#define omniwebprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"

class nsOmniWebProfileMigrator : public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsOmniWebProfileMigrator();
  virtual ~nsOmniWebProfileMigrator();

protected:

private:
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
