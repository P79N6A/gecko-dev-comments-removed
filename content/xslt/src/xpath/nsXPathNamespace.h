





































#ifndef nsXPathNamespace_h__
#define nsXPathNamespace_h__

#include "nsIDOMXPathNamespace.h"


#define TRANSFORMIIX_XPATH_NAMESPACE_CID   \
{ 0xd0a75e07, 0xb5e7, 0x11d5, { 0xa7, 0xf2, 0xdf, 0x10, 0x9f, 0xb8, 0xa1, 0xfc } }

#define TRANSFORMIIX_XPATH_NAMESPACE_CONTRACTID \
"@mozilla.org/transformiix/xpath-namespace;1"




class nsXPathNamespace : public nsIDOMXPathNamespace
{
public:
    nsXPathNamespace();
    virtual ~nsXPathNamespace();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIDOMNODE

    
    NS_DECL_NSIDOMXPATHNAMESPACE
};

#endif
