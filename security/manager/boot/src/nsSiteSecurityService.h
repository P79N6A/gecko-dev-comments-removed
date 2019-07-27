



#ifndef __nsSiteSecurityService_h__
#define __nsSiteSecurityService_h__

#include "mozilla/DataStorage.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsISiteSecurityService.h"
#include "nsString.h"
#include "prtime.h"

class nsIURI;


#define NS_SITE_SECURITY_SERVICE_CID \
  {0x16955eee, 0x6c48, 0x4152, \
    {0x93, 0x09, 0xc4, 0x2a, 0x46, 0x51, 0x38, 0xa1} }










enum SecurityPropertyState {
  SecurityPropertyUnset = 0,
  SecurityPropertySet = 1,
  SecurityPropertyKnockout = 2
};









class SiteSecurityState
{
public:
  explicit SiteSecurityState(nsCString& aStateString);
  SiteSecurityState(PRTime aHSTSExpireTime, SecurityPropertyState aHSTSState,
                    bool aHSTSIncludeSubdomains);

  PRTime mHSTSExpireTime;
  SecurityPropertyState mHSTSState;
  bool mHSTSIncludeSubdomains;

  bool IsExpired(uint32_t aType)
  {
    
    
    if (mHSTSExpireTime == 0) {
      return false;
    }

    PRTime now = PR_Now() / PR_USEC_PER_MSEC;
    if (now > mHSTSExpireTime) {
      return true;
    }

    return false;
  }

  void ToString(nsCString &aString);
};

class nsSTSPreload;

class nsSiteSecurityService : public nsISiteSecurityService
                            , public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSISITESECURITYSERVICE

  nsSiteSecurityService();
  nsresult Init();

protected:
  virtual ~nsSiteSecurityService();

private:
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  nsresult SetState(uint32_t aType, nsIURI* aSourceURI, int64_t maxage,
                    bool includeSubdomains, uint32_t flags);
  nsresult ProcessHeaderMutating(uint32_t aType, nsIURI* aSourceURI,
                                 char* aHeader, uint32_t flags,
                                 uint64_t *aMaxAge, bool *aIncludeSubdomains);
  const nsSTSPreload *GetPreloadListEntry(const char *aHost);

  bool mUsePreloadList;
  int64_t mPreloadListTimeOffset;
  nsRefPtr<mozilla::DataStorage> mSiteStateStorage;
};

#endif 
