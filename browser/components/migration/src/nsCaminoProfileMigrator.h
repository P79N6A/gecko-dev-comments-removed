




































#ifndef caminoprofilemigrator___h___
#define caminoprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"

class nsCaminoProfileMigrator : public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsCaminoProfileMigrator();
  virtual ~nsCaminoProfileMigrator();

protected:

private:
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
