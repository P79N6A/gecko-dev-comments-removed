






































#ifndef nsSystemPrincipal_h__
#define nsSystemPrincipal_h__

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"

#define NS_SYSTEMPRINCIPAL_CLASSNAME "systemprincipal"
#define NS_SYSTEMPRINCIPAL_CID \
{ 0x4a6212db, 0xaccb, 0x11d3, \
{ 0xb7, 0x65, 0x0, 0x60, 0xb0, 0xb6, 0xce, 0xcb }}
#define NS_SYSTEMPRINCIPAL_CONTRACTID "@mozilla.org/systemprincipal;1"


class nsSystemPrincipal : public nsIPrincipal
{
public:
    
    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPRINCIPAL
    NS_DECL_NSISERIALIZABLE

    nsresult Init();

    nsSystemPrincipal();

protected:
    virtual ~nsSystemPrincipal(void);

    nsJSPrincipals mJSPrincipals;
    
    NS_DECL_OWNINGTHREAD
};

#endif
