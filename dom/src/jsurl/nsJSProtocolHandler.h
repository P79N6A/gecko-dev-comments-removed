




































#ifndef nsJSProtocolHandler_h___
#define nsJSProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsITextToSubURI.h"
#include "nsIURI.h"
#include "nsIMutable.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"

#define NS_JSPROTOCOLHANDLER_CID                     \
{ /* bfc310d2-38a0-11d3-8cd3-0060b0fc14a3 */         \
    0xbfc310d2,                                      \
    0x38a0,                                          \
    0x11d3,                                          \
    {0x8c, 0xd3, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

#define NS_JSURI_CID                                 \
{ /* 58f089ee-512a-42d2-a935-d0c874128930 */         \
    0x58f089ee,                                      \
    0x512a,                                          \
    0x42d2,                                          \
    {0xa9, 0x35, 0xd0, 0xc8, 0x74, 0x12, 0x89, 0x30} \
}

#define NS_JSPROTOCOLHANDLER_CONTRACTID \
    NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "javascript"


class nsJSProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsJSProtocolHandler();
    virtual ~nsJSProtocolHandler();

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsresult Init();

protected:

    nsresult EnsureUTF8Spec(const nsAFlatCString &aSpec, const char *aCharset, 
                            nsACString &aUTF8Spec);

    nsCOMPtr<nsITextToSubURI>  mTextToSubURI;
};



class nsJSURI_base : public nsIURI,
                     public nsIMutable
{
public:
    nsJSURI_base(nsIURI* aSimpleURI) :
        mSimpleURI(aSimpleURI)
    {
        mMutable = do_QueryInterface(mSimpleURI);
        NS_ASSERTION(aSimpleURI && mMutable, "This isn't going to work out");
    }
    virtual ~nsJSURI_base() {}

    
    nsJSURI_base() {}
    
    NS_FORWARD_NSIURI(mSimpleURI->)
    NS_FORWARD_NSIMUTABLE(mMutable->)

protected:
    nsCOMPtr<nsIURI> mSimpleURI;
    nsCOMPtr<nsIMutable> mMutable;
};

class nsJSURI : public nsJSURI_base,
                public nsISerializable,
                public nsIClassInfo
{
public:
    nsJSURI(nsIURI* aBaseURI, nsIURI* aSimpleURI) :
        nsJSURI_base(aSimpleURI), mBaseURI(aBaseURI)
    {}
    virtual ~nsJSURI() {}

    
    nsJSURI() : nsJSURI_base() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSISERIALIZABLE
    NS_DECL_NSICLASSINFO

    
    NS_IMETHOD Clone(nsIURI** aClone);
    NS_IMETHOD Equals(nsIURI* aOther, PRBool *aResult);

    nsIURI* GetBaseURI() const {
        return mBaseURI;
    }

private:
    nsCOMPtr<nsIURI> mBaseURI;
};
    
#endif 
