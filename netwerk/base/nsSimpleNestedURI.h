














#ifndef nsSimpleNestedURI_h__
#define nsSimpleNestedURI_h__

#include "nsCOMPtr.h"
#include "nsSimpleURI.h"
#include "nsINestedURI.h"

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
                                    bool* result) MOZ_OVERRIDE;
    virtual nsSimpleURI* StartClone(RefHandlingEnum refHandlingMode) MOZ_OVERRIDE;

    
    NS_IMETHOD Read(nsIObjectInputStream* aStream) MOZ_OVERRIDE;
    NS_IMETHOD Write(nsIObjectOutputStream* aStream) MOZ_OVERRIDE;

    
    
    NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc) MOZ_OVERRIDE;  

protected:
    nsCOMPtr<nsIURI> mInnerURI;
};

#endif 
