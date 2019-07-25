








































#ifndef __nsStrictTransportSecurityService_h__
#define __nsStrictTransportSecurityService_h__

#include "nsIStrictTransportSecurityService.h"
#include "nsIPermissionManager.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsString.h"


#define NS_STRICT_TRANSPORT_SECURITY_CID \
  {0x16955eee, 0x6c48, 0x4152, \
    {0x93, 0x09, 0xc4, 0x2a, 0x46, 0x51, 0x38, 0xa1} }

class nsStrictTransportSecurityService : public nsIStrictTransportSecurityService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRICTTRANSPORTSECURITYSERVICE

  nsStrictTransportSecurityService();
  nsresult Init();
  virtual ~nsStrictTransportSecurityService();

private:
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  nsresult SetStsState(nsIURI* aSourceURI, PRInt64 maxage, PRBool includeSubdomains);
  nsresult ProcessStsHeaderMutating(nsIURI* aSourceURI, char* aHeader);
  nsCOMPtr<nsIPermissionManager> mPermMgr;
};

#endif 
