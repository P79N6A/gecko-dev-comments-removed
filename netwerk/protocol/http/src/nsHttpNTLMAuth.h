




































#ifndef nsHttpNTLMAuth_h__
#define nsHttpNTLMAuth_h__

#include "nsIHttpAuthenticator.h"

class nsHttpNTLMAuth : public nsIHttpAuthenticator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPAUTHENTICATOR

    nsHttpNTLMAuth() {}
    virtual ~nsHttpNTLMAuth() {}
};

#endif 
