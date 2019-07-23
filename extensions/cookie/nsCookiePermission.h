




































#ifndef nsCookiePermission_h__
#define nsCookiePermission_h__

#include "nsICookiePermission.h"
#include "nsIPermissionManager.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "prlong.h"

class nsIPrefBranch;

class nsCookiePermission : public nsICookiePermission
                         , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOOKIEPERMISSION
  NS_DECL_NSIOBSERVER

  nsCookiePermission() 
    : mCookiesLifetimeSec(LL_MAXINT)
    , mCookiesLifetimePolicy(0) 
    , mCookiesAlwaysAcceptSession(PR_FALSE)
#ifdef MOZ_MAIL_NEWS
    , mCookiesDisabledForMailNews(PR_TRUE)
#endif
    {}
  virtual ~nsCookiePermission() {}

  nsresult Init();
  void     PrefChanged(nsIPrefBranch *, const char *);

private:
  nsCOMPtr<nsIPermissionManager> mPermMgr;

  PRInt64      mCookiesLifetimeSec;            
  PRUint8      mCookiesLifetimePolicy;         
  PRPackedBool mCookiesAlwaysAcceptSession;    
#ifdef MOZ_MAIL_NEWS
  PRPackedBool mCookiesDisabledForMailNews;
#endif

};


#define NS_COOKIEPERMISSION_CID \
 {0xEF565D0A, 0xAB9A, 0x4A13, {0x91, 0x60, 0x06, 0x44, 0xcd, 0xfd, 0x85, 0x9a }}

#endif
