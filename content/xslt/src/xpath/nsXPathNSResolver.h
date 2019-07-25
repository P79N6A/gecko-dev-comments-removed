





































#ifndef nsXPathNSResolver_h__
#define nsXPathNSResolver_h__

#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMNode.h"
#include "nsIDOM3Node.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"




class nsXPathNSResolver : public nsIDOMXPathNSResolver
{
public:
    nsXPathNSResolver(nsIDOMNode* aNode);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsXPathNSResolver)

    
    NS_DECL_NSIDOMXPATHNSRESOLVER

private:
    nsCOMPtr<nsIDOM3Node> mNode;
};

#endif
