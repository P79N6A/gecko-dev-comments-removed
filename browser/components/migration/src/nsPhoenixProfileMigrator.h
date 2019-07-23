




































#ifndef phoenixprofilemigrator___h___
#define phoenixprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsNetscapeProfileMigratorBase.h"
#include "nsStringAPI.h"

class nsIFile;
class nsIPrefBranch;
class nsIPrefService;
class nsVoidArray;

class nsPhoenixProfileMigrator : public nsNetscapeProfileMigratorBase, 
                                 public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsPhoenixProfileMigrator();
  virtual ~nsPhoenixProfileMigrator();

public:
  static nsresult SetImage(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetCookie(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDownloadManager(void* aTransform, nsIPrefBranch* aBranch);

protected:
  nsresult FillProfileDataFromPhoenixRegistry();
  nsresult GetSourceProfile(const PRUnichar* aProfile);

  nsresult CopyPreferences(PRBool aReplace);
  nsresult CopyUserStyleSheets();

  nsresult CopyCookies(PRBool aReplace);
  nsresult CopyHistory(PRBool aReplace);
  nsresult CopyPasswords(PRBool aReplace);
  nsresult CopyBookmarks(PRBool aReplace);
  nsresult CopyOtherData(PRBool aReplace);

private:
  nsCOMPtr<nsISupportsArray> mProfileNames;
  nsCOMPtr<nsISupportsArray> mProfileLocations;
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif

