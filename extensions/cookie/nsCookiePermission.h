



#ifndef nsCookiePermission_h__
#define nsCookiePermission_h__

#include "nsICookiePermission.h"
#include "nsIPermissionManager.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "prlong.h"
#include "mozIThirdPartyUtil.h"

class nsIPrefBranch;

class nsCookiePermission : public nsICookiePermission
                         , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOOKIEPERMISSION
  NS_DECL_NSIOBSERVER

  nsCookiePermission()
    : mCookiesLifetimeSec(INT64_MAX)
    , mCookiesLifetimePolicy(0) 
    , mCookiesAlwaysAcceptSession(false)
    {}
  virtual ~nsCookiePermission() {}

  bool Init();
  void PrefChanged(nsIPrefBranch *, const char *);

private:
   bool EnsureInitialized() { return (mPermMgr != nullptr && mThirdPartyUtil != nullptr) || Init(); };

  nsCOMPtr<nsIPermissionManager> mPermMgr;
  nsCOMPtr<mozIThirdPartyUtil> mThirdPartyUtil;

  int64_t      mCookiesLifetimeSec;            
  uint8_t      mCookiesLifetimePolicy;         
  bool mCookiesAlwaysAcceptSession;    
};


#define NS_COOKIEPERMISSION_CID \
 {0xEF565D0A, 0xAB9A, 0x4A13, {0x91, 0x60, 0x06, 0x44, 0xcd, 0xfd, 0x85, 0x9a }}

#endif
