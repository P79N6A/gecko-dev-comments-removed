








































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
#include "mozIStorageAsyncStatement.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageConnection.h"
#include "mozIStorageRow.h"
#include "mozIStorageCompletionCallback.h"
#include "mozIStorageStatementCallback.h"

class nsICookiePermission;
class nsIEffectiveTLDService;
class nsIIDNService;
class nsIPrefBranch;
class nsIObserverService;
class nsIURI;
class nsIChannel;
class mozIStorageService;
class mozIThirdPartyUtil;
class ReadCookieDBListener;

struct nsCookieAttributes;
struct nsListIter;
struct nsEnumerationData;

namespace mozilla {
namespace net {
#ifdef MOZ_IPC
class CookieServiceParent;
#endif
}
}


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


struct CookieDomainTuple
{
  nsCString baseDomain;
  nsRefPtr<nsCookie> cookie;
};



struct DBState
{
  DBState() : cookieCount(0), cookieOldestTime(LL_MAXINT)
  {
    hostTable.Init();
  }

  NS_INLINE_DECL_REFCOUNTING(DBState)

  nsTHashtable<nsCookieEntry>     hostTable;
  PRUint32                        cookieCount;
  PRInt64                         cookieOldestTime;
  nsCOMPtr<mozIStorageConnection> dbConn;
  nsCOMPtr<mozIStorageAsyncStatement> stmtInsert;
  nsCOMPtr<mozIStorageAsyncStatement> stmtDelete;
  nsCOMPtr<mozIStorageAsyncStatement> stmtUpdate;

  
  
  nsCOMPtr<mozIStorageConnection>       syncConn;
  nsCOMPtr<mozIStorageStatement>        stmtReadDomain;
  nsCOMPtr<mozIStoragePendingStatement> pendingRead;
  
  
  ReadCookieDBListener*                 readListener;
  
  
  nsTArray<CookieDomainTuple>           hostArray;
  
  
  
  nsTHashtable<nsCStringHashKey>        readSet;

  
  nsCOMPtr<mozIStorageStatementCallback>  insertListener;
  nsCOMPtr<mozIStorageStatementCallback>  updateListener;
  nsCOMPtr<mozIStorageStatementCallback>  removeListener;
  nsCOMPtr<mozIStorageCompletionCallback> closeListener;
};


enum CookieStatus
{
  STATUS_ACCEPTED,
  STATUS_ACCEPT_SESSION,
  STATUS_REJECTED,
  
  
  
  
  STATUS_REJECTED_WITH_ERROR
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
    static nsICookieService*      GetXPCOMSingleton();
    nsresult                      Init();

  protected:
    void                          PrefChanged(nsIPrefBranch *aPrefBranch);
    void                          InitDBStates();
    nsresult                      TryInitDB(PRBool aDeleteExistingDB);
    nsresult                      CreateTable();
    void                          CloseDBStates();
    void                          CloseDefaultDBConnection();
    nsresult                      Read();
    template<class T> nsCookie*   GetCookieFromRow(T &aRow);
    void                          AsyncReadComplete();
    void                          CancelAsyncRead(PRBool aPurgeReadSet);
    mozIStorageConnection*        GetSyncDBConn();
    void                          EnsureReadDomain(const nsCString &aBaseDomain);
    void                          EnsureReadComplete();
    nsresult                      NormalizeHost(nsCString &aHost);
    nsresult                      GetBaseDomain(nsIURI *aHostURI, nsCString &aBaseDomain, PRBool &aRequireHostMatch);
    nsresult                      GetBaseDomainFromHost(const nsACString &aHost, nsCString &aBaseDomain);
    nsresult                      GetCookieStringCommon(nsIURI *aHostURI, nsIChannel *aChannel, bool aHttpBound, char** aCookie);
    void                          GetCookieStringInternal(nsIURI *aHostURI, bool aIsForeign, PRBool aHttpBound, nsCString &aCookie);
    nsresult                      SetCookieStringCommon(nsIURI *aHostURI, const char *aCookieHeader, const char *aServerTime, nsIChannel *aChannel, bool aFromHttp);
    void                          SetCookieStringInternal(nsIURI *aHostURI, bool aIsForeign, const nsCString &aCookieHeader, const nsCString &aServerTime, PRBool aFromHttp);
    PRBool                        SetCookieInternal(nsIURI *aHostURI, const nsCString& aBaseDomain, PRBool aRequireHostMatch, CookieStatus aStatus, nsDependentCString &aCookieHeader, PRInt64 aServerTime, PRBool aFromHttp);
    void                          AddInternal(const nsCString& aBaseDomain, nsCookie *aCookie, PRInt64 aCurrentTimeInUsec, nsIURI *aHostURI, const char *aCookieHeader, PRBool aFromHttp);
    void                          RemoveCookieFromList(const nsListIter &aIter, mozIStorageBindingParamsArray *aParamsArray = NULL);
    void                          AddCookieToList(const nsCString& aBaseDomain, nsCookie *aCookie, DBState *aDBState, mozIStorageBindingParamsArray *aParamsArray, PRBool aWriteToDB = PR_TRUE);
    void                          UpdateCookieInList(nsCookie *aCookie, PRInt64 aLastAccessed, mozIStorageBindingParamsArray *aParamsArray);
    static PRBool                 GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter, nsASingleFragmentCString::const_char_iterator &aEndIter, nsDependentCSubstring &aTokenString, nsDependentCSubstring &aTokenValue, PRBool &aEqualsFound);
    static PRBool                 ParseAttributes(nsDependentCString &aCookieHeader, nsCookieAttributes &aCookie);
    bool                          RequireThirdPartyCheck();
    CookieStatus                  CheckPrefs(nsIURI *aHostURI, bool aIsForeign, const nsCString &aBaseDomain, PRBool aRequireHostMatch, const char *aCookieHeader);
    PRBool                        CheckDomain(nsCookieAttributes &aCookie, nsIURI *aHostURI, const nsCString &aBaseDomain, PRBool aRequireHostMatch);
    static PRBool                 CheckPath(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static PRBool                 GetExpiry(nsCookieAttributes &aCookie, PRInt64 aServerTime, PRInt64 aCurrentTime);
    void                          RemoveAllFromMemory();
    void                          PurgeCookies(PRInt64 aCurrentTimeInUsec);
    PRBool                        FindCookie(const nsCString& aBaseDomain, const nsAFlatCString &aHost, const nsAFlatCString &aName, const nsAFlatCString &aPath, nsListIter &aIter);
    static void                   FindStaleCookie(nsCookieEntry *aEntry, PRInt64 aCurrentTime, nsListIter &aIter);
    void                          NotifyRejected(nsIURI *aHostURI);
    void                          NotifyChanged(nsISupports *aSubject, const PRUnichar *aData);
    void                          NotifyPurged(nsICookie2* aCookie);

  protected:
    
    nsCOMPtr<nsIObserverService>     mObserverService;
    nsCOMPtr<nsICookiePermission>    mPermissionService;
    nsCOMPtr<mozIThirdPartyUtil>     mThirdPartyUtil;
    nsCOMPtr<nsIEffectiveTLDService> mTLDService;
    nsCOMPtr<nsIIDNService>          mIDNService;
    nsCOMPtr<mozIStorageService>     mStorageService;

    
    
    
    
    
    DBState                      *mDBState;
    nsRefPtr<DBState>             mDefaultDBState;
    nsRefPtr<DBState>             mPrivateDBState;

    
    PRUint8                       mCookieBehavior; 
    PRBool                        mThirdPartySession;
    PRUint16                      mMaxNumberOfCookies;
    PRUint16                      mMaxCookiesPerHost;
    PRInt64                       mCookiePurgeAge;

    
    friend PLDHashOperator purgeCookiesCallback(nsCookieEntry *aEntry, void *aArg);
    friend class ReadCookieDBListener;

    static nsCookieService*       GetSingleton();
#ifdef MOZ_IPC
    friend class mozilla::net::CookieServiceParent;
#endif
};

#endif 
