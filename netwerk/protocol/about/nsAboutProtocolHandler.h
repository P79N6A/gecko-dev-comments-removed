




#ifndef nsAboutProtocolHandler_h___
#define nsAboutProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsSimpleNestedURI.h"
#include "mozilla/Attributes.h"

class nsIURI;

class nsAboutProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsAboutProtocolHandler() {}

private:
    virtual ~nsAboutProtocolHandler() {}
};

class nsSafeAboutProtocolHandler final : public nsIProtocolHandler
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
