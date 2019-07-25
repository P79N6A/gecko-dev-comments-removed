







































#ifndef ieprofilemigrator___h___
#define ieprofilemigrator___h___

#include <time.h>
#include <windows.h>
#include <pstore.h>
#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsTArray.h"
#include "nsINavHistoryService.h"

class nsIFile;
class nsICookieManager2;
class nsIRDFResource;
class nsINavBookmarksService;
class nsIPrefBranch;

struct SignonData {
  PRUnichar* user;
  PRUnichar* pass;
  char*      realm;
};

class nsIEProfileMigrator : public nsIBrowserProfileMigrator,
                            public nsINavHistoryBatchCallback {
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_NSINAVHISTORYBATCHCALLBACK
  NS_DECL_ISUPPORTS

  nsIEProfileMigrator();
  virtual ~nsIEProfileMigrator();

protected:
  nsresult CopyPreferences(bool aReplace);
  nsresult CopyStyleSheet(bool aReplace);
  nsresult CopyCookies(bool aReplace);
  nsresult CopyProxyPreferences(nsIPrefBranch* aPrefs);
  nsresult CopySecurityPrefs(nsIPrefBranch* aPrefs);
  







  nsresult CopyHistory(bool aReplace);
  nsresult CopyHistoryBatched(bool aReplace);

  bool     KeyIsURI(const nsAString& aKey, char** aRealm);

  nsresult CopyPasswords(bool aReplace);
  nsresult MigrateSiteAuthSignons(IPStore* aPStore);
  nsresult GetSignonsListFromPStore(IPStore* aPStore, nsTArray<SignonData>* aSignonsFound);
  nsresult ResolveAndMigrateSignons(IPStore* aPStore, nsTArray<SignonData>* aSignonsFound);
  void     EnumerateUsernames(const nsAString& aKey, PRUnichar* aData, unsigned long aCount, nsTArray<SignonData>* aSignonsFound);
  void     GetUserNameAndPass(unsigned char* data, unsigned long len, unsigned char** username, unsigned char** pass);

  nsresult CopyFormData(bool aReplace);
  nsresult AddDataToFormHistory(const nsAString& aKey, PRUnichar* data, unsigned long len);
  








  nsresult CopyFavorites(bool aReplace);
  nsresult CopyFavoritesBatched(bool aReplace);
  void     ResolveShortcut(const nsString &aFileName, char** aOutURL);
  nsresult ParseFavoritesFolder(nsIFile* aDirectory, 
                                PRInt64 aParentFolder,
                                nsINavBookmarksService* aBookmarksService,
                                const nsAString& aPersonalToolbarFolderName,
                                bool aIsAtRootLevel);
  nsresult CopySmartKeywords(nsINavBookmarksService* aBMS,
                             PRInt64 aParentFolder);

  nsresult CopyCookiesFromBuffer(char *aBuffer, PRUint32 aBufferLength,
                                 nsICookieManager2 *aCookieManager);
  void     DelimitField(char **aBuffer, const char *aBufferEnd, char **aField);
  time_t   FileTimeToTimeT(const char *aLowDateIntString,
                           const char *aHighDateIntString);
  void     GetUserStyleSheetFile(nsIFile **aUserFile);
  bool     TestForIE7();

private:
  nsCOMPtr<nsIObserverService> mObserverService;
};

#endif

