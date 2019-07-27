






#ifndef nsSystemPrincipal_h__
#define nsSystemPrincipal_h__

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"

#define NS_SYSTEMPRINCIPAL_CID \
{ 0x4a6212db, 0xaccb, 0x11d3, \
{ 0xb7, 0x65, 0x0, 0x60, 0xb0, 0xb6, 0xce, 0xcb }}
#define NS_SYSTEMPRINCIPAL_CONTRACTID "@mozilla.org/systemprincipal;1"


class nsSystemPrincipal final : public nsJSPrincipals
{
public:
    
    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPRINCIPAL
    NS_DECL_NSISERIALIZABLE

    nsSystemPrincipal();

    virtual void GetScriptLocation(nsACString &aStr) override;

#ifdef DEBUG
    virtual void dumpImpl() override;
#endif 

protected:
    virtual ~nsSystemPrincipal(void);

    
    NS_DECL_OWNINGTHREAD
};

#endif
