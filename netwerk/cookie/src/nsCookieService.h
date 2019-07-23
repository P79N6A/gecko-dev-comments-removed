






































#ifndef nsCookieService_h__
#define nsCookieService_h__

#include "nsICookieServiceInternal.h"
#include "nsICookieManager.h"
#include "nsICookieManager2.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "nsCookie.h"
#include "nsString.h"
#include "nsTHashtable.h"

#include "nsInt64.h"

struct nsCookieAttributes;
struct nsListIter;
struct nsEnumerationData;

class nsAutoVoidArray;

class nsIPrefBranch;
class nsICookieConsent;
class nsICookiePermission;
class nsIPrefBranch;
class nsIObserverService;
class nsIURI;
class nsIChannel;
class nsITimer;
class nsIFile;


class nsCookieEntry : public PLDHashEntryHdr
{
  public:
    
    typedef const char* KeyType;
    typedef const char* KeyTypePointer;

    
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

    KeyTypePointer GetKeyPointer() const
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






class nsCookieService : public nsICookieServiceInternal
                      , public nsICookieManager2
                      , public nsIObserver
                      , public nsSupportsWeakReference
{
  public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSICOOKIESERVICE
    NS_DECL_NSICOOKIESERVICEINTERNAL
    NS_DECL_NSICOOKIEMANAGER
    NS_DECL_NSICOOKIEMANAGER2

    nsCookieService();
    virtual ~nsCookieService();
    static nsCookieService*       GetSingleton();
    nsresult                      Init();

  protected:
    void                          PrefChanged(nsIPrefBranch *aPrefBranch);
    nsresult                      Read();
    nsresult                      Write();
    void                          GetCookieList(nsIURI *aHostURI, nsIURI *aFirstURI, nsIChannel *aChannel, const nsACString *aName, PRBool isHttpBound, nsAutoVoidArray &aResult);
    char*                         CookieStringFromArray(const nsAutoVoidArray& aCookieList, nsIURI *aHostURI);
    PRBool                        SetCookieInternal(nsIURI *aHostURI, nsIChannel *aChannel, nsDependentCString &aCookieHeader, nsInt64 aServerTime, nsCookieStatus aStatus, nsCookiePolicy aPolicy);
    void                          CheckAndAdd(nsIURI *aHostURI, nsIChannel *aChannel, nsCookieAttributes &aAttributes, nsCookieStatus aStatus, nsCookiePolicy aPolicy, const nsAFlatCString &aCookieHeader);
    void                          AddInternal(nsCookie *aCookie, nsInt64 aCurrentTime, nsIURI *aHostURI, const char *aCookieHeader);
    void                          RemoveCookieFromList(nsListIter &aIter);
    PRBool                        AddCookieToList(nsCookie *aCookie);
    static PRBool                 GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter, nsASingleFragmentCString::const_char_iterator &aEndIter, nsDependentCSubstring &aTokenString, nsDependentCSubstring &aTokenValue, PRBool &aEqualsFound);
    static PRBool                 ParseAttributes(nsDependentCString &aCookieHeader, nsCookieAttributes &aCookie);
    static PRBool                 IsIPAddress(const nsAFlatCString &aHost);
    static PRBool                 IsInDomain(const nsACString &aDomain, const nsACString &aHost, PRBool aIsDomain = PR_TRUE);
    static PRBool                 IsForeign(nsIURI *aHostURI, nsIURI *aFirstURI);
    nsCookieStatus                CheckPrefs(nsIURI *aHostURI, nsIURI *aFirstURI, nsIChannel *aChannel, const char *aCookieHeader, nsCookiePolicy &aPolicy);
    static PRBool                 CheckDomain(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static PRBool                 CheckPath(nsCookieAttributes &aCookie, nsIURI *aHostURI);
    static PRBool                 GetExpiry(nsCookieAttributes &aCookie, nsInt64 aServerTime, nsInt64 aCurrentTime, nsCookieStatus aStatus);
    void                          RemoveAllFromMemory();
    void                          RemoveExpiredCookies(nsInt64 aCurrentTime);
    PRBool                        FindCookie(const nsAFlatCString &aHost, const nsAFlatCString &aName, const nsAFlatCString &aPath, nsListIter &aIter);
    void                          FindOldestCookie(nsEnumerationData &aData);
    PRUint32                      CountCookiesFromHost(nsCookie *aCookie, nsEnumerationData &aData);
    void                          NotifyRejected(nsIURI *aHostURI);
    void                          NotifyChanged(nsICookie2 *aCookie, const PRUnichar *aData);

    
    
    void                          LazyWrite();
    static void                   DoLazyWrite(nsITimer *aTimer, void *aClosure);

  protected:
    
    nsCOMPtr<nsIFile>             mCookieFile;
    nsCOMPtr<nsIObserverService>  mObserverService;
    nsCOMPtr<nsICookieConsent>    mP3PService;
    nsCOMPtr<nsICookiePermission> mPermissionService;

    
    nsCOMPtr<nsITimer>            mWriteTimer;
    nsTHashtable<nsCookieEntry>   mHostTable;
    PRUint32                      mCookieCount;
    PRPackedBool                  mCookieChanged;
    PRPackedBool                  mCookieIconVisible;

    
    PRUint8                       mCookiesPermissions;   
    PRUint16                      mMaxNumberOfCookies;
    PRUint16                      mMaxCookiesPerHost;

    
    
    static nsCookieService        *gCookieService;

    
    friend PLDHashOperator PR_CALLBACK removeExpiredCallback(nsCookieEntry *aEntry, void *aArg);
};

#endif 
