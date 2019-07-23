




































#ifndef nsSimpleURI_h__
#define nsSimpleURI_h__

#include "nsIURL.h"
#include "nsAgg.h"
#include "nsISerializable.h"
#include "nsString.h"
#include "nsIClassInfo.h"
#include "nsIMutable.h"

#define NS_THIS_SIMPLEURI_IMPLEMENTATION_CID         \
{ /* 22b8f64a-2f7b-11d3-8cd0-0060b0fc14a3 */         \
    0x22b8f64a,                                      \
    0x2f7b,                                          \
    0x11d3,                                          \
    {0x8c, 0xd0, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsSimpleURI : public nsIURI,
                    public nsISerializable,
                    public nsIClassInfo,
                    public nsIMutable
{
public:
    NS_DECL_AGGREGATED
    NS_DECL_NSIURI
    NS_DECL_NSISERIALIZABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSIMUTABLE

    

    nsSimpleURI(nsISupports* outer);
    virtual ~nsSimpleURI();

protected:
    virtual nsSimpleURI* StartClone();

    nsCString mScheme;
    nsCString mPath;
    PRBool mMutable;
};

#endif 
