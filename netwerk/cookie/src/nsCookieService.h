








































#ifndef nsCookieService_h__
#define nsCookieService_h__

#include "nsICookieService.h"
#include "nsICookieManager.h"
#include "nsICookieManager2.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "nsCookie.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"
#include "mozIStorageStatement.h"
#include "mozIStorageConnection.h"

struct nsCookieAttributes;
struct nsListIter;
struct nsEnumerationData;

class nsICookiePermission;
class nsIEffectiveTLDService;
class nsIIDNService;
class nsIPrefBranch;
class nsIObserverService;
class nsIURI;
class nsIChannel;


class nsCookieEntry : public PLDHashEntryHdr
{
  public:
    
    typedef const nsCString& KeyType;
    typedef const nsCString* KeyTypePointer;
    typedef nsTArray< nsRefPtr<nsCookie> > ArrayType;
    typedef ArrayType::index_type IndexType;

    explicit
    nsCookieEntry(KeyTypePointer aBaseDomain)
     : mBaseDomain(*aBaseDomain)
    {
    }

    nsCookieEntry(const nsCookieEntry& toCopy)
    {
      
      
      NS_NOTREACHED("nsCookieEntry copy constructor is forbidden!");
    }

    ~nsCookieEntry()
    {
    }

    KeyType GetKey() const
    {
      return mBaseDomain;
    }

    PRBool KeyEquals(KeyTypePointer aKey) const
    {
      return mBaseDomain == *aKey;
    }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
      return &aKey;
    }

    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      return HashString(*aKey);
    }

    enum { ALLOW_MEMMOVE = PR_TRUE };

    inline ArrayType& GetCookies() { return mCookies; }

  private:
    nsCString mBaseDomain;
    ArrayType mCookies;
};



struct DBState
{
  DBState() : cookieCount(0), cookieOldestTime(LL_MAXINT) { }

  nsTHashtable<nsCookieEntry>     hostTable;
  PRUint32                        cookieCount;
  PRInt64                         cookieOldestTime;
  nsCOMPtr<mozIStorageConnection> dbConn;
  nsCOMPtr<mozIStorageStatement>  stmtInsert;
  nsCOMPtr<mozIStorageStatement>  stmtDelete;
  nsCOMPtr<mozIStorageStatement>  stmtUpdate;
};






class nsCookieService : public nsICookieService
                      , public nsICookieManager2
                      , public nsIObserver
                      , public nsSupportsWeakReference
{
  public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSICOOKIESERVICE
    NS_DECL_NSICOOKIEMANAGER
    NS_DECL_NSICOOKIEMANAGER2

    nsCookieService();
    virtual ~nsCookieService();
    static nsCookieService*       GetSingleton();
    nsresult                      Init();

  protected:
    void                          PrefChanged(nsIPrefBranch *aPrefBranch);
    nsresult                      InitDB();
    nsresult                      TryInitDB(PRBool aDeleteExistingDB);
    nsresult                      CreateTable();
    void                          CloseDB();
    nsresult                      Read();
    nsresult                      NormalizeHost(nsCString &aHost);
    nsresult                      GetBaseDomain(nsIURI *aHostURI, nsCString &aBaseDomain, PRBool &aRequireHostMatch);
    nsresult                      GetBaseDomainFromHost(const nsACString &aHost, nsCString &aBaseDomain);
    void                          GetCookieInternal(nsIURI *aHostURI, nsIChannel *aChannel, PRBool aHttpBound, char **aCookie);
    nsresult                      SetCookieStringInternal(nsIURI *aHostURI, nsIPrompt *aPrompt, const char *aCookieHeader, const char *aServerTime, nsIChannel *aChannel, PRBool aFromHttp);
    PRBool                        SetCookieInternal(nsIURI *aHostURI, nsIChannel *aChannel, const nsCString& aBaseDomain, PRBool aRequireHostMatch, nsDependentCString &aCookieHeader, PRInt64 aServerTime, PRBool aFromHttp);
    void                          AddInternal(const nsCString& aBaseDomain, nsCookie *aCookie, PRInt64 aCurrentTimeInUsec, nsIURI *aHostURI, const char *aCookieHeader, PRBool aFromHttp);
    void                          RemoveCookieFromList(const nsListIter &aIter);
    PRBool                        AddCookieToList(const nsCString& aBaseDomain, nsCookie *aCookie, PRBool aWriteToDB = PR_TRUE);
    void                          UpdateCookieInList(nsCookie *aCookie, PRInt64 aLastAccessed);
    static PRBool                 GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter, nsASingleFragmentCString::const_char_iterator &aEndIter, nsDependentCSubstring &aTokenString, nsDependentCSubstring &aTokenValue, PRBool &aEqualsFound);
    static PRBool                 ParseAttributes(nsDependentCString &aCookieHeader, nsCookieAttributes &aCookie);
    PRBool                        IsForeign(const nsCString &aBaseDomain, PRBool aRequireHostMatch, nsIURI *aFirstURI);
    PRUint32                      CheckPrefs(nsIURI *aHostURI, nsIChannel *aChannel, const nsCString &aBaseDomain, PRBool aRequireHostMatch, const char *aCookieHeader);
    PRBool                        CheckDomain(nsCookieAttributes &aCookie, nsIURI *aHostURI, const nsCString &aBaseDomain, PRBool aRequireHostMatch);
    static PRBool                 CheckPath(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static PRBool                 GetExpiry(nsCookieAttributes &aCookie, PRInt64 aServerTime, PRInt64 aCurrentTime);
    void                          RemoveAllFromMemory();
    void                          PurgeCookies(PRInt64 aCurrentTimeInUsec);
    PRBool                        FindCookie(const nsCString& aBaseDomain, const nsAFlatCString &aHost, const nsAFlatCString &aName, const nsAFlatCString &aPath, nsListIter &aIter, PRInt64 aCurrentTime);
    PRUint32                      CountCookiesFromHostInternal(const nsCString &aBaseDomain, nsEnumerationData &aData);
    void                          NotifyRejected(nsIURI *aHostURI);
    void                          NotifyChanged(nsISupports *aSubject, const PRUnichar *aData);

  protected:
    
    nsCOMPtr<nsIObserverService>     mObserverService;
    nsCOMPtr<nsICookiePermission>    mPermissionService;
    nsCOMPtr<nsIEffectiveTLDService> mTLDService;
    nsCOMPtr<nsIIDNService>          mIDNService;

    
    
    
    
    
    DBState                      *mDBState;
    DBState                       mDefaultDBState;
    DBState                       mPrivateDBState;

    
    PRUint8                       mCookiesPermissions;   
    PRUint16                      mMaxNumberOfCookies;
    PRUint16                      mMaxCookiesPerHost;
    PRInt64                       mCookiePurgeAge;

    
    
    static nsCookieService        *gCookieService;

    
    friend PLDHashOperator purgeCookiesCallback(nsCookieEntry *aEntry, void *aArg);
};

#endif 
