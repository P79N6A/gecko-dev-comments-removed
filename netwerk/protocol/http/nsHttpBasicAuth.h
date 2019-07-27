




#ifndef nsBasicAuth_h__
#define nsBasicAuth_h__

#include "nsIHttpAuthenticator.h"

namespace mozilla { namespace net {






class nsHttpBasicAuth : public nsIHttpAuthenticator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPAUTHENTICATOR

	nsHttpBasicAuth();
private:
	virtual ~nsHttpBasicAuth();
};

} 
} 

#endif 
