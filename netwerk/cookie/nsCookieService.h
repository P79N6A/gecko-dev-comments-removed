




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
#include "nsIMemoryReporter.h"
#include "nsTHashtable.h"
#include "mozIStorageStatement.h"
#include "mozIStorageAsyncStatement.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageConnection.h"
#include "mozIStorageRow.h"
#include "mozIStorageCompletionCallback.h"
#include "mozIStorageStatementCallback.h"

#include "mozilla/MemoryReporting.h"

class nsICookiePermission;
class nsIEffectiveTLDService;
class nsIIDNService;
class nsIPrefBranch;
class nsIObserverService;
class nsIURI;
class nsIChannel;
class nsIArray;
class mozIStorageService;
class mozIThirdPartyUtil;
class ReadCookieDBListener;

struct nsCookieAttributes;
struct nsListIter;

namespace mozilla {
namespace net {
class CookieServiceParent;
}
}


class nsCookieKey : public PLDHashEntryHdr
{
public:
  typedef const nsCookieKey& KeyType;
  typedef const nsCookieKey* KeyTypePointer;

  nsCookieKey()
  {}

  nsCookieKey(const nsCString &baseDomain, uint32_t appId, bool inBrowser)
    : mBaseDomain(baseDomain)
    , mAppId(appId)
    , mInBrowserElement(inBrowser)
  {}

  explicit nsCookieKey(KeyTypePointer other)
    : mBaseDomain(other->mBaseDomain)
    , mAppId(other->mAppId)
    , mInBrowserElement(other->mInBrowserElement)
  {}

  nsCookieKey(KeyType other)
    : mBaseDomain(other.mBaseDomain)
    , mAppId(other.mAppId)
    , mInBrowserElement(other.mInBrowserElement)
  {}

  ~nsCookieKey()
  {}

  bool KeyEquals(KeyTypePointer other) const
  {
    return mBaseDomain == other->mBaseDomain &&
           mAppId == other->mAppId &&
           mInBrowserElement == other->mInBrowserElement;
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return &aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    nsAutoCString temp(aKey->mBaseDomain);
    temp.Append('#');
    temp.Append(aKey->mAppId);
    temp.Append('#');
    temp.Append(aKey->mInBrowserElement ? 1 : 0);
    return mozilla::HashString(temp);
  }

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  enum { ALLOW_MEMMOVE = true };

  nsCString   mBaseDomain;
  uint32_t    mAppId;
  bool        mInBrowserElement;
};



class nsCookieEntry : public nsCookieKey
{
  public:
    
    typedef nsTArray< nsRefPtr<nsCookie> > ArrayType;
    typedef ArrayType::index_type IndexType;

    explicit nsCookieEntry(KeyTypePointer aKey)
     : nsCookieKey(aKey)
    {}

    nsCookieEntry(const nsCookieEntry& toCopy)
    {
      
      
      NS_NOTREACHED("nsCookieEntry copy constructor is forbidden!");
    }

    ~nsCookieEntry()
    {}

    inline ArrayType& GetCookies() { return mCookies; }

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  private:
    ArrayType mCookies;
};


struct CookieDomainTuple
{
  nsCookieKey key;
  nsRefPtr<nsCookie> cookie;

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
};



struct DBState final
{
  DBState() : cookieCount(0), cookieOldestTime(INT64_MAX), corruptFlag(OK)
  {
  }

private:
  
  ~DBState()
  {
  }

public:
  NS_INLINE_DECL_REFCOUNTING(DBState)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  enum CorruptFlag {
    OK,                   
    CLOSING_FOR_REBUILD,  
    REBUILDING            
  };

  nsTHashtable<nsCookieEntry>     hostTable;
  uint32_t                        cookieCount;
  int64_t                         cookieOldestTime;
  nsCOMPtr<nsIFile>               cookieFile;
  nsCOMPtr<mozIStorageConnection> dbConn;
  nsCOMPtr<mozIStorageAsyncStatement> stmtInsert;
  nsCOMPtr<mozIStorageAsyncStatement> stmtDelete;
  nsCOMPtr<mozIStorageAsyncStatement> stmtUpdate;
  CorruptFlag                     corruptFlag;

  
  
  nsCOMPtr<mozIStorageConnection>       syncConn;
  nsCOMPtr<mozIStorageStatement>        stmtReadDomain;
  nsCOMPtr<mozIStoragePendingStatement> pendingRead;
  
  
  ReadCookieDBListener*                 readListener;
  
  
  nsTArray<CookieDomainTuple>           hostArray;
  
  
  
  nsTHashtable<nsCookieKey>        readSet;

  
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


enum OpenDBResult
{
  RESULT_OK,
  RESULT_RETRY,
  RESULT_FAILURE
};






class nsCookieService final : public nsICookieService
                            , public nsICookieManager2
                            , public nsIObserver
                            , public nsSupportsWeakReference
                            , public nsIMemoryReporter
{
  private:
    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSICOOKIESERVICE
    NS_DECL_NSICOOKIEMANAGER
    NS_DECL_NSICOOKIEMANAGER2
    NS_DECL_NSIMEMORYREPORTER

    nsCookieService();
    static nsICookieService*      GetXPCOMSingleton();
    nsresult                      Init();

  





  static void AppClearDataObserverInit();

  protected:
    virtual ~nsCookieService();

    void                          PrefChanged(nsIPrefBranch *aPrefBranch);
    void                          InitDBStates();
    OpenDBResult                  TryInitDB(bool aDeleteExistingDB);
    nsresult                      CreateTable();
    void                          CloseDBStates();
    void                          CleanupCachedStatements();
    void                          CleanupDefaultDBConnection();
    void                          HandleDBClosed(DBState* aDBState);
    void                          HandleCorruptDB(DBState* aDBState);
    void                          RebuildCorruptDB(DBState* aDBState);
    OpenDBResult                  Read();
    template<class T> nsCookie*   GetCookieFromRow(T &aRow);
    void                          AsyncReadComplete();
    void                          CancelAsyncRead(bool aPurgeReadSet);
    void                          EnsureReadDomain(const nsCookieKey &aKey);
    void                          EnsureReadComplete();
    nsresult                      NormalizeHost(nsCString &aHost);
    nsresult                      GetBaseDomain(nsIURI *aHostURI, nsCString &aBaseDomain, bool &aRequireHostMatch);
    nsresult                      GetBaseDomainFromHost(const nsACString &aHost, nsCString &aBaseDomain);
    nsresult                      GetCookieStringCommon(nsIURI *aHostURI, nsIChannel *aChannel, bool aHttpBound, char** aCookie);
  void                            GetCookieStringInternal(nsIURI *aHostURI, bool aIsForeign, bool aHttpBound, uint32_t aAppId, bool aInBrowserElement, bool aIsPrivate, nsCString &aCookie);
    nsresult                      SetCookieStringCommon(nsIURI *aHostURI, const char *aCookieHeader, const char *aServerTime, nsIChannel *aChannel, bool aFromHttp);
  void                            SetCookieStringInternal(nsIURI *aHostURI, bool aIsForeign, nsDependentCString &aCookieHeader, const nsCString &aServerTime, bool aFromHttp, uint32_t aAppId, bool aInBrowserElement, bool aIsPrivate, nsIChannel* aChannel);
    bool                          SetCookieInternal(nsIURI *aHostURI, const nsCookieKey& aKey, bool aRequireHostMatch, CookieStatus aStatus, nsDependentCString &aCookieHeader, int64_t aServerTime, bool aFromHttp, nsIChannel* aChannel);
    void                          AddInternal(const nsCookieKey& aKey, nsCookie *aCookie, int64_t aCurrentTimeInUsec, nsIURI *aHostURI, const char *aCookieHeader, bool aFromHttp);
    void                          RemoveCookieFromList(const nsListIter &aIter, mozIStorageBindingParamsArray *aParamsArray = nullptr);
    void                          AddCookieToList(const nsCookieKey& aKey, nsCookie *aCookie, DBState *aDBState, mozIStorageBindingParamsArray *aParamsArray, bool aWriteToDB = true);
    void                          UpdateCookieInList(nsCookie *aCookie, int64_t aLastAccessed, mozIStorageBindingParamsArray *aParamsArray);
    static bool                   GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter, nsASingleFragmentCString::const_char_iterator &aEndIter, nsDependentCSubstring &aTokenString, nsDependentCSubstring &aTokenValue, bool &aEqualsFound);
    static bool                   ParseAttributes(nsDependentCString &aCookieHeader, nsCookieAttributes &aCookie);
    bool                          RequireThirdPartyCheck();
    CookieStatus                  CheckPrefs(nsIURI *aHostURI, bool aIsForeign, bool aRequireHostMatch, const char *aCookieHeader);
    bool                          CheckDomain(nsCookieAttributes &aCookie, nsIURI *aHostURI, const nsCString &aBaseDomain, bool aRequireHostMatch);
    static bool                   CheckPath(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static bool                   GetExpiry(nsCookieAttributes &aCookie, int64_t aServerTime, int64_t aCurrentTime);
    void                          RemoveAllFromMemory();
    already_AddRefed<nsIArray>    PurgeCookies(int64_t aCurrentTimeInUsec);
    bool                          FindCookie(const nsCookieKey& aKey, const nsAFlatCString &aHost, const nsAFlatCString &aName, const nsAFlatCString &aPath, nsListIter &aIter);
    static void                   FindStaleCookie(nsCookieEntry *aEntry, int64_t aCurrentTime, nsListIter &aIter);
    void                          NotifyRejected(nsIURI *aHostURI);
    void                          NotifyThirdParty(nsIURI *aHostURI, bool aAccepted, nsIChannel *aChannel);
    void                          NotifyChanged(nsISupports *aSubject, const char16_t *aData);
    void                          NotifyPurged(nsICookie2* aCookie);
    already_AddRefed<nsIArray>    CreatePurgeList(nsICookie2* aCookie);

    



    static PLDHashOperator GetCookiesForApp(nsCookieEntry* entry, void* arg);

    




    nsresult Remove(const nsACString& aHost, uint32_t aAppId,
                    bool aInBrowserElement, const nsACString& aName,
                    const nsACString& aPath, bool aBlocked);

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

    
    uint8_t                       mCookieBehavior; 
    bool                          mThirdPartySession;
    uint16_t                      mMaxNumberOfCookies;
    uint16_t                      mMaxCookiesPerHost;
    int64_t                       mCookiePurgeAge;

    
    friend PLDHashOperator purgeCookiesCallback(nsCookieEntry *aEntry, void *aArg);
    friend class DBListenerErrorHandler;
    friend class ReadCookieDBListener;
    friend class CloseCookieDBListener;

    static nsCookieService*       GetSingleton();
    friend class mozilla::net::CookieServiceParent;
};

#endif 
