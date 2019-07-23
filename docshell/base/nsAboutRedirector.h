





































#ifndef nsAboutRedirector_h__
#define nsAboutRedirector_h__

#include "nsIAboutModule.h"

class nsAboutRedirector : public nsIAboutModule
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIABOUTMODULE

    nsAboutRedirector() {}
    virtual ~nsAboutRedirector() {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
};

#define NS_ABOUT_REDIRECTOR_MODULE_CID               \
{ /*  f0acde16-1dd1-11b2-9e35-f5786fff5a66*/         \
    0xf0acde16,                                      \
    0x1dd1,                                          \
    0x11b2,                                          \
    {0x9e, 0x35, 0xf5, 0x78, 0x6f, 0xff, 0x5a, 0x66} \
}

#endif
