





































#ifndef nsXPathNSResolver_h__
#define nsXPathNSResolver_h__

#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMNode.h"
#include "nsIDOM3Node.h"
#include "nsCOMPtr.h"




class nsXPathNSResolver : public nsIDOMXPathNSResolver
{
public:
    nsXPathNSResolver(nsIDOMNode* aNode);
    virtual ~nsXPathNSResolver();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIDOMXPATHNSRESOLVER

private:
    nsCOMPtr<nsIDOM3Node> mNode;
};

#endif
