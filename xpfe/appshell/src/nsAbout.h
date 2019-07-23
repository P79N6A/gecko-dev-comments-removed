




































#ifndef nsAbout_h_
#define nsAbout_h_

#include "nsIAboutModule.h"

#define NS_ABOUT_CID                                                           \
{ /* {1f1ce501-663a-11d3-b7a0-be426e4e69bc} */                                 \
0x1f1ce501, 0x663a, 0x11d3, { 0xb7, 0xa0, 0xbe, 0x42, 0x6e, 0x4e, 0x69, 0xbc } \
}

class nsAbout : public nsIAboutModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIABOUTMODULE

    nsAbout() {}
    virtual ~nsAbout() {}
};

#endif 
