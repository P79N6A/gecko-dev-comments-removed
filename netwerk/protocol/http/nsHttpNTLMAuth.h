



#ifndef nsHttpNTLMAuth_h__
#define nsHttpNTLMAuth_h__

#include "nsIHttpAuthenticator.h"

namespace mozilla { namespace net {

class nsHttpNTLMAuth : public nsIHttpAuthenticator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPAUTHENTICATOR

    nsHttpNTLMAuth() {}

private:
    virtual ~nsHttpNTLMAuth() {}

    
    
    bool  mUseNative;
};

} 
} 

#endif 
