















































#ifndef nsSimpleNestedURI_h__
#define nsSimpleNestedURI_h__

#include "nsCOMPtr.h"
#include "nsSimpleURI.h"
#include "nsINestedURI.h"

class nsSimpleNestedURI : public nsSimpleURI,
                          public nsINestedURI
{
public:
    
    
    nsSimpleNestedURI()
    {
    }

    
    
    nsSimpleNestedURI(nsIURI* innerURI);

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSINESTEDURI
    NS_DECL_NSIIPCSERIALIZABLE

    
  
    
    virtual nsresult EqualsInternal(nsIURI* other,
                                    RefHandlingEnum refHandlingMode,
                                    PRBool* result);
    virtual nsSimpleURI* StartClone();

    
    NS_IMETHOD Read(nsIObjectInputStream* aStream);
    NS_IMETHOD Write(nsIObjectOutputStream* aStream);

    
    
    NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc);  

protected:
    nsCOMPtr<nsIURI> mInnerURI;
};

#endif 
