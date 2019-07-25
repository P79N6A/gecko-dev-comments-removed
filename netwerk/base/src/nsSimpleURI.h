




































#ifndef nsSimpleURI_h__
#define nsSimpleURI_h__

#include "nsIURL.h"
#include "nsAgg.h"
#include "nsISerializable.h"
#include "nsIIPCSerializable.h"
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
                    public nsIIPCSerializable,
                    public nsIClassInfo,
                    public nsIMutable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURI
    NS_DECL_NSISERIALIZABLE
    NS_DECL_NSIIPCSERIALIZABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSIMUTABLE

    

    nsSimpleURI();
    virtual ~nsSimpleURI();

protected:
    
    enum RefHandlingEnum {
        eIgnoreRef,
        eHonorRef
    };

    
    
    virtual nsresult EqualsInternal(nsIURI* other,
                                    RefHandlingEnum refHandlingMode,
                                    PRBool* result);

    virtual nsSimpleURI* StartClone();

    nsCString mScheme;
    nsCString mPath;
    PRBool mMutable;
};

#endif
