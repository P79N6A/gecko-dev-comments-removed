




#ifndef nsAboutBloat_h__
#define nsAboutBloat_h__

#include "nsIAboutModule.h"

class nsAboutBloat : public nsIAboutModule 
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIABOUTMODULE

    nsAboutBloat() {}

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    virtual ~nsAboutBloat() {}
};

#define NS_ABOUT_BLOAT_MODULE_CID                    \
{ /* f9666720-801f-11d3-9399-00104ba0fd40 */         \
    0xf9666720,                                      \
    0x801f,                                          \
    0x11d3,                                          \
    {0x93, 0x99, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

#endif 
