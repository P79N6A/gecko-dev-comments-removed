




































#ifndef nsAboutProtocolHandler_h___
#define nsAboutProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsSimpleNestedURI.h"

class nsCString;
class nsIAboutModule;

class nsAboutProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsAboutProtocolHandler() {}
    virtual ~nsAboutProtocolHandler() {}
};

class nsSafeAboutProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsSafeAboutProtocolHandler() {}

private:
    ~nsSafeAboutProtocolHandler() {}
};



class nsNestedAboutURI : public nsSimpleNestedURI {
public:
    NS_DECL_NSIIPCSERIALIZABLE

    nsNestedAboutURI(nsIURI* aInnerURI, nsIURI* aBaseURI)
        : nsSimpleNestedURI(aInnerURI)
        , mBaseURI(aBaseURI)
    {}

    
    nsNestedAboutURI() : nsSimpleNestedURI() {}

    virtual ~nsNestedAboutURI() {}

    
    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

    
    
    
    virtual nsSimpleURI* StartClone(RefHandlingEnum aRefHandlingMode);
    NS_IMETHOD Read(nsIObjectInputStream* aStream);
    NS_IMETHOD Write(nsIObjectOutputStream* aStream);
    NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc);

    nsIURI* GetBaseURI() const {
        return mBaseURI;
    }

protected:
    nsCOMPtr<nsIURI> mBaseURI;
};

#endif 
