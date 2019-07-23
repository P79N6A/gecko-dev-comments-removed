



































#ifndef nsXULTemplateResultXML_h__
#define nsXULTemplateResultXML_h__

#include "nsCOMPtr.h"
#include "nsIRDFResource.h"
#include "nsXULTemplateQueryProcessorXML.h"
#include "nsIXULTemplateResult.h"




class nsXULTemplateResultXML : public nsIXULTemplateResult
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIXULTEMPLATERESULT

    nsXULTemplateResultXML(nsXMLQuery* aQuery,
                           nsIDOMNode* aNode,
                           nsXMLBindingSet* aBindings);

    ~nsXULTemplateResultXML() {}

    void GetNode(nsIDOMNode** aNode);

protected:

    
    PRUint32 mId;

    
    nsCOMPtr<nsXMLQuery> mQuery;

    
    nsCOMPtr<nsIDOMNode> mNode;

    
    nsXMLBindingValues mRequiredValues;

    
    nsXMLBindingValues mOptionalValues;
};

#endif 
