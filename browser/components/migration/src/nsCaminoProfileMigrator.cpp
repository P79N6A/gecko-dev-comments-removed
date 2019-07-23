




































#include "nsBrowserProfileMigratorUtils.h"
#include "nsCaminoProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsIProfileMigrator.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"




NS_IMPL_ISUPPORTS1(nsCaminoProfileMigrator, nsIBrowserProfileMigrator)

nsCaminoProfileMigrator::nsCaminoProfileMigrator()
{
  mObserverService = do_GetService("@mozilla.org/observer-service;1");
}

nsCaminoProfileMigrator::~nsCaminoProfileMigrator()
{
}




NS_IMETHODIMP
nsCaminoProfileMigrator::Migrate(PRUint16 aItems, nsIProfileStartup* aStartup, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsCaminoProfileMigrator::GetMigrateData(const PRUnichar* aProfile, 
                                        PRBool aReplace, 
                                        PRUint16* aResult)
{
  *aResult = 0; 
  return NS_OK;
}

NS_IMETHODIMP
nsCaminoProfileMigrator::GetSourceExists(PRBool* aResult)
{
  *aResult = PR_FALSE; 

  return NS_OK;
}

NS_IMETHODIMP
nsCaminoProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsCaminoProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  *aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsCaminoProfileMigrator::GetSourceHomePageURL(nsACString& aResult)
{
  aResult.Truncate();
  return NS_OK;
}



