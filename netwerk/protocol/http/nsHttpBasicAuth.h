






































#ifndef nsBasicAuth_h__
#define nsBasicAuth_h__

#include "nsIHttpAuthenticator.h"

class nsIURI;






class nsHttpBasicAuth : public nsIHttpAuthenticator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPAUTHENTICATOR

	nsHttpBasicAuth();
	virtual ~nsHttpBasicAuth();
};

#endif 
