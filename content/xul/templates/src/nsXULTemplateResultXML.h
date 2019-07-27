




#ifndef nsXULTemplateResultXML_h__
#define nsXULTemplateResultXML_h__

#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIRDFResource.h"
#include "nsXULTemplateQueryProcessorXML.h"
#include "nsIXULTemplateResult.h"
#include "mozilla/Attributes.h"




class nsXULTemplateResultXML MOZ_FINAL : public nsIXULTemplateResult
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIXULTEMPLATERESULT

    nsXULTemplateResultXML(nsXMLQuery* aQuery,
                           nsIDOMNode* aNode,
                           nsXMLBindingSet* aBindings);

    void GetNode(nsIDOMNode** aNode);

protected:

    ~nsXULTemplateResultXML() {}

    
    
    
    
    nsAutoString mId;

    
    nsCOMPtr<nsXMLQuery> mQuery;

    
    nsCOMPtr<nsIDOMNode> mNode;

    
    nsXMLBindingValues mRequiredValues;

    
    nsXMLBindingValues mOptionalValues;
};

#endif 
