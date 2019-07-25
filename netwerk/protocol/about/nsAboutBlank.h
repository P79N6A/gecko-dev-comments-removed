




































#ifndef nsAboutBlank_h__
#define nsAboutBlank_h__

#include "nsIAboutModule.h"

class nsAboutBlank : public nsIAboutModule 
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIABOUTMODULE

    nsAboutBlank() {}
    virtual ~nsAboutBlank() {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
};

#define NS_ABOUT_BLANK_MODULE_CID                    \
{ /* 3decd6c8-30ef-11d3-8cd0-0060b0fc14a3 */         \
    0x3decd6c8,                                      \
    0x30ef,                                          \
    0x11d3,                                          \
    {0x8c, 0xd0, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

#endif
