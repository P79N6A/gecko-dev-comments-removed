














#ifndef nsSimpleNestedURI_h__
#define nsSimpleNestedURI_h__

#include "nsCOMPtr.h"
#include "nsSimpleURI.h"
#include "nsINestedURI.h"

#include "nsIIPCSerializableURI.h"

class nsSimpleNestedURI : public nsSimpleURI,
                          public nsINestedURI
{
protected:
    ~nsSimpleNestedURI() {}

public:
    
    
    nsSimpleNestedURI()
    {
    }

    
    
    explicit nsSimpleNestedURI(nsIURI* innerURI);

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSINESTEDURI

    
  
    
    virtual nsresult EqualsInternal(nsIURI* other,
                                    RefHandlingEnum refHandlingMode,
                                    bool* result) override;
    virtual nsSimpleURI* StartClone(RefHandlingEnum refHandlingMode) override;

    
    NS_IMETHOD Read(nsIObjectInputStream* aStream) override;
    NS_IMETHOD Write(nsIObjectOutputStream* aStream) override;

    
    NS_DECL_NSIIPCSERIALIZABLEURI

    
    
    NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc) override;  

protected:
    nsCOMPtr<nsIURI> mInnerURI;
};

#endif 
