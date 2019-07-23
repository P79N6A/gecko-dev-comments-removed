





































#include "nsBrowserProfileMigratorUtils.h"
#include "nsOmniWebProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsIProfileMigrator.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"




NS_IMPL_ISUPPORTS1(nsOmniWebProfileMigrator, nsIBrowserProfileMigrator)

nsOmniWebProfileMigrator::nsOmniWebProfileMigrator()
{
  mObserverService = do_GetService("@mozilla.org/observer-service;1");
}

nsOmniWebProfileMigrator::~nsOmniWebProfileMigrator()
{
}




NS_IMETHODIMP
nsOmniWebProfileMigrator::Migrate(PRUint16 aItems, nsIProfileStartup* aStartup, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsOmniWebProfileMigrator::GetMigrateData(const PRUnichar* aProfile, 
                                         PRBool aReplace,
                                         PRUint16* aResult)
{
  *aResult = 0; 
  return NS_OK;
}

NS_IMETHODIMP
nsOmniWebProfileMigrator::GetSourceExists(PRBool* aResult)
{
  *aResult = PR_FALSE; 

  return NS_OK;
}

NS_IMETHODIMP
nsOmniWebProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsOmniWebProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  *aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsOmniWebProfileMigrator::GetSourceHomePageURL(nsACString& aResult)
{
  aResult.Truncate();
  return NS_OK;
}



