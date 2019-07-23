







































#ifndef ieprofilemigrator___h___
#define ieprofilemigrator___h___

#include <time.h>
#include <windows.h>
#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsVoidArray.h"
#include "nsINavHistoryService.h"

class nsIFile;
class nsICookieManager2;
class nsIRDFResource;
class nsINavBookmarksService;
class nsIPrefBranch;

#import PSTOREC_DLL raw_interfaces_only
using namespace PSTORECLib;

class nsIEProfileMigrator : public nsIBrowserProfileMigrator,
                            public nsINavHistoryBatchCallback {
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_NSINAVHISTORYBATCHCALLBACK
  NS_DECL_ISUPPORTS

  nsIEProfileMigrator();
  virtual ~nsIEProfileMigrator();

protected:
  nsresult CopyPreferences(PRBool aReplace);
  nsresult CopyStyleSheet(PRBool aReplace);
  nsresult CopyCookies(PRBool aReplace);
  nsresult CopyProxyPreferences(nsIPrefBranch* aPrefs);
  nsresult CopySecurityPrefs(nsIPrefBranch* aPrefs);
  nsresult CopyHistory(PRBool aReplace);

  PRBool   KeyIsURI(const nsAString& aKey, char** aRealm);

  nsresult CopyPasswords(PRBool aReplace);
  nsresult MigrateSiteAuthSignons(IPStore* aPStore);
  nsresult GetSignonsListFromPStore(IPStore* aPStore, nsVoidArray* aSignonsFound);
  nsresult ResolveAndMigrateSignons(IPStore* aPStore, nsVoidArray* aSignonsFound);
  void     EnumerateUsernames(const nsAString& aKey, PRUnichar* aData, unsigned long aCount, nsVoidArray* aSignonsFound);
  void     GetUserNameAndPass(unsigned char* data, unsigned long len, unsigned char** username, unsigned char** pass);

  nsresult CopyFormData(PRBool aReplace);
  nsresult AddDataToFormHistory(const nsAString& aKey, PRUnichar* data, unsigned long len);

  nsresult CopyFavorites(PRBool aReplace);
  void     ResolveShortcut(const nsString &aFileName, char** aOutURL);
  nsresult ParseFavoritesFolder(nsIFile* aDirectory, 
                                PRInt64 aParentFolder,
                                nsINavBookmarksService* aBookmarksService,
                                const nsAString& aPersonalToolbarFolderName,
                                PRBool aIsAtRootLevel);
  nsresult CopySmartKeywords(PRInt64 aParentFolder);

  nsresult CopyCookiesFromBuffer(char *aBuffer, PRUint32 aBufferLength,
                                 nsICookieManager2 *aCookieManager);
  void     DelimitField(char **aBuffer, const char *aBufferEnd, char **aField);
  time_t   FileTimeToTimeT(const char *aLowDateIntString,
                           const char *aHighDateIntString);
  void     GetUserStyleSheetFile(nsIFile **aUserFile);

private:
  nsCOMPtr<nsIObserverService> mObserverService;
};

#endif

