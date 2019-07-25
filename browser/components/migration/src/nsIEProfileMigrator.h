







































#ifndef ieprofilemigrator___h___
#define ieprofilemigrator___h___

#include <time.h>
#include <windows.h>
#include <ole2.h>
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



class IEnumPStoreItems : public IUnknown {
public:
  virtual HRESULT STDMETHODCALLTYPE Next(DWORD celt, LPWSTR* rgelt,
                                         DWORD* pceltFetched) = 0;
  virtual HRESULT STDMETHODCALLTYPE Skip(DWORD celt) = 0;
  virtual HRESULT STDMETHODCALLTYPE Reset() = 0;
  virtual HRESULT STDMETHODCALLTYPE Clone(IEnumPStoreItems** ppenum) = 0;
};

class IEnumPStoreTypes; 
struct PST_PROVIDERINFO; 
struct PST_TYPEINFO; 
struct PST_PROMPTINFO; 
struct PST_ACCESSRULESET; 
typedef DWORD PST_KEY;
typedef DWORD PST_ACCESSMODE;

class IPStore : public IUnknown {
public:
  virtual HRESULT STDMETHODCALLTYPE GetInfo(PST_PROVIDERINFO** ppProperties) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProvParam(DWORD dwParam, DWORD* pcbData,
                                                 BYTE** ppbData, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetProvParam(DWORD dwParam, DWORD cbData,
                                                 BYTE* pbData, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE CreateType(PST_KEY Key, const GUID* pType,
                                               PST_TYPEINFO* pInfo, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(PST_KEY Key, const GUID* pType,
                                                PST_TYPEINFO** ppInfo, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE DeleteType(PST_KEY Key, const GUID* pType,
                                               DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE CreateSubtype(PST_KEY Key, const GUID* pType,
                                                  const GUID* pSubtype, PST_TYPEINFO* pInfo,
                                                  PST_ACCESSRULESET* pRules, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetSubtypeInfo(PST_KEY Key, const GUID* pType,
                                                   const GUID* pSubtype, PST_TYPEINFO** ppInfo,
                                                   DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE DeleteSubtype(PST_KEY Key, const GUID* pType,
                                                  const GUID* pSubtype, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE ReadAccessRuleset(PST_KEY Key, const GUID* pType,
                                                      const GUID* pSubtype, PST_ACCESSRULESET** ppRules,
                                                      DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE WriteAccessRuleset(PST_KEY Key, const GUID* pType,
                                                       const GUID* pSubtype, PST_ACCESSRULESET* pRules,
                                                       DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE EnumTypes(PST_KEY Key, DWORD dwFlags, IEnumPStoreTypes** ppenum) = 0;
  virtual HRESULT STDMETHODCALLTYPE EnumSubtypes(PST_KEY Key, const GUID* pType,
                                                 DWORD dwFlags, IEnumPStoreTypes** ppenum) = 0;
  virtual HRESULT STDMETHODCALLTYPE DeleteItem(PST_KEY Key, const GUID* pItemType,
                                               const GUID* pItemSubtype, LPCWSTR szItemName,
                                               PST_PROMPTINFO* pPromptInfo, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE ReadItem(PST_KEY Key, const GUID* pItemType,
                                             const GUID* pItemSubtype, LPCWSTR szItemName,
                                             DWORD* pcbData, BYTE** ppbData,
                                             PST_PROMPTINFO* pPromptInfo, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE WriteItem(PST_KEY Key, const GUID* pItemType,
                                              const GUID* pItemSubtype, LPCWSTR szItemName,
                                              DWORD cbData, BYTE* pbData,
                                              PST_PROMPTINFO* pPromptInfo, DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE OpenItem(PST_KEY Key, const GUID* pItemType,
                                             const GUID* pItemSubtype, LPCWSTR szItemName,
                                             PST_ACCESSMODE ModeFlags, PST_PROMPTINFO* pPromptInfo,
                                             DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE CloseItem(PST_KEY Key, const GUID* pItemType,
                                              const GUID* pItemSubtype, LPCWSTR szItemName,
                                              DWORD dwFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE EnumItems(PST_KEY Key, const GUID* pItemType,
                                              const GUID* pItemSubtype, DWORD dwFlags,
                                              IEnumPStoreItems** ppenum) = 0;
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

