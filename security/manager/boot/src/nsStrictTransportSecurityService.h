







#ifndef __nsStrictTransportSecurityService_h__
#define __nsStrictTransportSecurityService_h__

#include "nsIStrictTransportSecurityService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "prtime.h"


#define NS_STRICT_TRANSPORT_SECURITY_CID \
  {0x16955eee, 0x6c48, 0x4152, \
    {0x93, 0x09, 0xc4, 0x2a, 0x46, 0x51, 0x38, 0xa1} }
































class nsSTSHostEntry : public PLDHashEntryHdr
{
  public:
    explicit nsSTSHostEntry(const char* aHost);
    explicit nsSTSHostEntry(const nsSTSHostEntry& toCopy);

    nsCString    mHost;
    PRTime       mExpireTime;
    uint32_t     mStsPermission;
    bool         mExpired;
    bool         mIncludeSubdomains;

    
    typedef const char* KeyType;
    typedef const char* KeyTypePointer;

    KeyType GetKey() const
    {
      return mHost.get();
    }

    bool KeyEquals(KeyTypePointer aKey) const
    {
      return !strcmp(mHost.get(), aKey);
    }

    static KeyTypePointer KeyToPointer(KeyType aKey)
    {
      return aKey;
    }

    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      return PL_DHashStringKey(nullptr, aKey);
    }

    void SetExpireTime(PRTime aExpireTime)
    {
      mExpireTime = aExpireTime;
      mExpired = false;
    }

    bool IsExpired()
    {
      
      
      
      if (mExpired || mExpireTime == 0) {
        return mExpired;
      }

      PRTime now = PR_Now() / PR_USEC_PER_MSEC;
      if (now > mExpireTime) {
        mExpired = true;
      }

      return mExpired;
    }

    
    enum { ALLOW_MEMMOVE = false };
};


class nsSTSPreload;

class nsStrictTransportSecurityService : public nsIStrictTransportSecurityService
                                       , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSISTRICTTRANSPORTSECURITYSERVICE

  nsStrictTransportSecurityService();
  nsresult Init();
  virtual ~nsStrictTransportSecurityService();

private:
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  nsresult GetPrincipalForURI(nsIURI *aURI, nsIPrincipal **aPrincipal);
  nsresult SetStsState(nsIURI* aSourceURI, int64_t maxage, bool includeSubdomains, uint32_t flags);
  nsresult ProcessStsHeaderMutating(nsIURI* aSourceURI, char* aHeader, uint32_t flags,
                                    uint64_t *aMaxAge, bool *aIncludeSubdomains);
  const nsSTSPreload *GetPreloadListEntry(const char *aHost);

  
  nsresult AddPermission(nsIURI     *aURI,
                         const char *aType,
                         uint32_t   aPermission,
                         uint32_t   aExpireType,
                         int64_t    aExpireTime,
                         bool       aIsPrivate);
  nsresult RemovePermission(const nsCString  &aHost,
                            const char       *aType,
                            bool              aIsPrivate);

  
  nsCOMPtr<nsIPermissionManager> mPermMgr;
  nsCOMPtr<nsIObserverService> mObserverService;

  nsTHashtable<nsSTSHostEntry> mPrivateModeHostTable;
  bool mUsePreloadList;
};

#endif 
