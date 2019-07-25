








































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


#define NS_STRICT_TRANSPORT_SECURITY_CID \
  {0x16955eee, 0x6c48, 0x4152, \
    {0x93, 0x09, 0xc4, 0x2a, 0x46, 0x51, 0x38, 0xa1} }





























class nsSTSHostEntry : public PLDHashEntryHdr
{
  public:
    explicit nsSTSHostEntry(const char* aHost);
    explicit nsSTSHostEntry(const nsSTSHostEntry& toCopy);

    nsCString    mHost;
    PRInt64      mExpireTime;
    bool mDeleted;
    bool mIncludeSubdomains;

    
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
      return PL_DHashStringKey(nsnull, aKey);
    }

    
    enum { ALLOW_MEMMOVE = false };
};


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
  nsresult SetStsState(nsIURI* aSourceURI, PRInt64 maxage, bool includeSubdomains);
  nsresult ProcessStsHeaderMutating(nsIURI* aSourceURI, char* aHeader);

  
  nsresult AddPermission(nsIURI     *aURI,
                         const char *aType,
                         PRUint32   aPermission,
                         PRUint32   aExpireType,
                         PRInt64    aExpireTime);
  nsresult RemovePermission(const nsCString  &aHost,
                            const char       *aType);
  nsresult TestPermission(nsIURI     *aURI,
                          const char *aType,
                          PRUint32   *aPermission,
                          bool       testExact);

  
  nsCOMPtr<nsIPermissionManager> mPermMgr;
  nsCOMPtr<nsIObserverService> mObserverService;

  bool mInPrivateMode;
  nsTHashtable<nsSTSHostEntry> mPrivateModeHostTable;
};

#endif 
