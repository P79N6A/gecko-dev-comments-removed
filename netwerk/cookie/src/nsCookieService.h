








































#ifndef nsCookieService_h__
#define nsCookieService_h__

#include "nsICookieService.h"
#include "nsICookieManager.h"
#include "nsICookieManager2.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "nsCookie.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "mozIStorageStatement.h"
#include "mozIStorageConnection.h"

struct nsCookieAttributes;
struct nsListIter;
struct nsEnumerationData;

class nsICookiePermission;
class nsIEffectiveTLDService;
class nsIPrefBranch;
class nsIObserverService;
class nsIURI;
class nsIChannel;


class nsCookieEntry : public PLDHashEntryHdr
{
  public:
    
    typedef const char* KeyType;
    typedef const char* KeyTypePointer;

    
    explicit
    nsCookieEntry(KeyTypePointer aHost)
     : mHead(nsnull)
    {
    }

    nsCookieEntry(const nsCookieEntry& toCopy)
    {
      
      
      NS_NOTREACHED("nsCookieEntry copy constructor is forbidden!");
    }

    ~nsCookieEntry()
    {
      
      
      
      
      
      nsCookie *current = mHead, *next;
      do {
        next = current->Next();
        NS_RELEASE(current);
      } while ((current = next));
    }

    KeyType GetKey() const
    {
      return HostPtr();
    }

    PRBool KeyEquals(KeyTypePointer aKey) const
    {
      return !strcmp(HostPtr(), aKey);
    }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
      return aKey;
    }

    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      
      
      return PL_DHashStringKey(nsnull, aKey);
    }

    enum { ALLOW_MEMMOVE = PR_TRUE };

    
    inline const nsDependentCString Host() const { return mHead->Host(); }

    
    inline nsCookie*& Head() { return mHead; }

    inline KeyTypePointer HostPtr() const
    {
      return mHead->Host().get();
    }

  private:
    nsCookie *mHead;
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
    void                          GetCookieInternal(nsIURI *aHostURI, nsIChannel *aChannel, PRBool aHttpBound, char **aCookie);
    nsresult                      SetCookieStringInternal(nsIURI *aHostURI, nsIPrompt *aPrompt, const char *aCookieHeader, const char *aServerTime, nsIChannel *aChannel, PRBool aFromHttp);
    PRBool                        SetCookieInternal(nsIURI *aHostURI, nsIChannel *aChannel, nsDependentCString &aCookieHeader, PRInt64 aServerTime, PRBool aFromHttp);
    void                          AddInternal(nsCookie *aCookie, PRInt64 aCurrentTimeInUsec, nsIURI *aHostURI, const char *aCookieHeader, PRBool aFromHttp);
    void                          RemoveCookieFromList(nsListIter &aIter);
    PRBool                        AddCookieToList(nsCookie *aCookie, PRBool aWriteToDB = PR_TRUE);
    void                          UpdateCookieInList(nsCookie *aCookie, PRInt64 aLastAccessed);
    static PRBool                 GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter, nsASingleFragmentCString::const_char_iterator &aEndIter, nsDependentCSubstring &aTokenString, nsDependentCSubstring &aTokenValue, PRBool &aEqualsFound);
    static PRBool                 ParseAttributes(nsDependentCString &aCookieHeader, nsCookieAttributes &aCookie);
    PRBool                        IsForeign(nsIURI *aHostURI, nsIURI *aFirstURI);
    PRUint32                      CheckPrefs(nsIURI *aHostURI, nsIChannel *aChannel, const char *aCookieHeader);
    PRBool                        CheckDomain(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static PRBool                 CheckPath(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static PRBool                 GetExpiry(nsCookieAttributes &aCookie, PRInt64 aServerTime, PRInt64 aCurrentTime);
    void                          RemoveAllFromMemory();
    void                          PurgeCookies(PRInt64 aCurrentTimeInUsec);
    PRBool                        FindCookie(const nsAFlatCString &aHost, const nsAFlatCString &aName, const nsAFlatCString &aPath, nsListIter &aIter, PRInt64 aCurrentTime);
    PRUint32                      CountCookiesFromHostInternal(const nsACString &aHost, nsEnumerationData &aData);
    void                          NotifyRejected(nsIURI *aHostURI);
    void                          NotifyChanged(nsISupports *aSubject, const PRUnichar *aData);

  protected:
    
    nsCOMPtr<nsIObserverService>     mObserverService;
    nsCOMPtr<nsICookiePermission>    mPermissionService;
    nsCOMPtr<nsIEffectiveTLDService> mTLDService;

    
    
    
    
    
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
