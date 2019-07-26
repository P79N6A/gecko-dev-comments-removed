




#ifndef nsCSPContext_h___
#define nsCSPContext_h___

#include "nsCSPUtils.h"
#include "nsIChannel.h"
#include "nsIClassInfo.h"
#include "nsIContentSecurityPolicy.h"
#include "nsISerializable.h"
#include "nsXPCOM.h"

#define NS_CSPCONTEXT_CONTRACTID "@mozilla.org/cspcontext;1"
 
#define NS_CSPCONTEXT_CID \
{ 0x09d9ed1a, 0xe5d4, 0x4004, \
  { 0xbf, 0xe0, 0x27, 0xce, 0xb9, 0x23, 0xd9, 0xac } }

class nsCSPContext : public nsIContentSecurityPolicy
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTSECURITYPOLICY
    NS_DECL_NSISERIALIZABLE

    nsCSPContext();
    virtual ~nsCSPContext();

  private:
    NS_IMETHODIMP getAllowsInternal(nsContentPolicyType aContentType,
                                    enum CSPKeyword aKeyword,
                                    const nsAString& aNonceOrContent,
                                    bool* outShouldReportViolations,
                                    bool* outIsAllowed) const;

    nsTArray<nsCSPPolicy*> mPolicies;
    nsCOMPtr<nsIURI>       mSelfURI;
};

#endif 
