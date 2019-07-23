



































 
#ifndef profilemigrator___h___
#define profilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsIProfileMigrator.h"
#include "nsCOMPtr.h"

#define NS_FIREFOX_PROFILEMIGRATOR_CID \
{ 0x4ca3c946, 0x5408, 0x49f0, { 0x9e, 0xca, 0x3a, 0x97, 0xd5, 0xc6, 0x77, 0x50 } }

class nsProfileMigrator : public nsIProfileMigrator
{
public:
  NS_DECL_NSIPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsProfileMigrator() { };

protected:
  ~nsProfileMigrator() { };

  nsresult GetDefaultBrowserMigratorKey(nsACString& key,
                                        nsCOMPtr<nsIBrowserProfileMigrator>& bpm);

  



  PRBool ImportRegistryProfiles(const nsACString& aAppName);
};

#endif

