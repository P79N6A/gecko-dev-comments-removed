




#ifndef nsXPathNSResolver_h__
#define nsXPathNSResolver_h__

#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"




class nsXPathNSResolver MOZ_FINAL : public nsIDOMXPathNSResolver
{
    ~nsXPathNSResolver() {}

public:
    nsXPathNSResolver(nsIDOMNode* aNode);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsXPathNSResolver)

    
    NS_DECL_NSIDOMXPATHNSRESOLVER

private:
    nsCOMPtr<nsIDOMNode> mNode;
};

#endif
