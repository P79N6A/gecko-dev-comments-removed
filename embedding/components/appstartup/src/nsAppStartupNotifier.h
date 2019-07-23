




































#ifndef nsAppStartupNotifier_h___
#define nsAppStartupNotifier_h___

#include "nsIAppStartupNotifier.h"


#define NS_APPSTARTUPNOTIFIER_CID \
   { 0x1f59b001, 0x2c9, 0x11d5, { 0xae, 0x76, 0xcc, 0x92, 0xf7, 0xdb, 0x9e, 0x3 } }

class nsAppStartupNotifier : public nsIObserver
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_APPSTARTUPNOTIFIER_CID )

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsAppStartupNotifier();
    virtual ~nsAppStartupNotifier();
};

#endif 

