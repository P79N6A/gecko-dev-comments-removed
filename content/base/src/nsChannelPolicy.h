




#ifndef nsChannelPolicy_h___
#define nsChannelPolicy_h___

#include "nsCOMPtr.h"
#include "nsIChannelPolicy.h"

#define NSCHANNELPOLICY_CONTRACTID "@mozilla.org/nschannelpolicy;1"
#define NSCHANNELPOLICY_CID \
{ 0xd396b3cd, 0xf164, 0x4ce8, \
  { 0x93, 0xa7, 0xe3, 0x85, 0xe1, 0x46, 0x56, 0x3c } }

class nsChannelPolicy : public nsIChannelPolicy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICHANNELPOLICY

    nsChannelPolicy();

protected:
    virtual ~nsChannelPolicy();

    


    unsigned long mLoadType;

    
    nsCOMPtr<nsISupports> mCSP;
};

#endif 
