




#ifndef nsHttpNegotiateAuth_h__
#define nsHttpNegotiateAuth_h__

#include "nsIHttpAuthenticator.h"
#include "nsIURI.h"
#include "nsSubstring.h"




class nsHttpNegotiateAuth : public nsIHttpAuthenticator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPAUTHENTICATOR

private:
    
    bool TestBoolPref(const char *pref);

    
    bool TestNonFqdn(nsIURI *uri);

    
    bool TestPref(nsIURI *, const char *pref);

    bool MatchesBaseURI(const nsCSubstring &scheme,
                          const nsCSubstring &host,
                          PRInt32             port,
                          const char         *baseStart,
                          const char         *baseEnd);
};
#endif 
