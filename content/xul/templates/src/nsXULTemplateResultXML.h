




#ifndef nsXULTemplateResultXML_h__
#define nsXULTemplateResultXML_h__

#include "nsCOMPtr.h"
#include "nsIContent.h"
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
                           nsIContent* aNode,
                           nsXMLBindingSet* aBindings);

    nsIContent* Node()
    {
        return mNode;
    }

protected:

    ~nsXULTemplateResultXML() {}

    
    
    
    
    nsAutoString mId;

    
    nsCOMPtr<nsXMLQuery> mQuery;

    
    nsCOMPtr<nsIContent> mNode;

    
    nsXMLBindingValues mRequiredValues;

    
    nsXMLBindingValues mOptionalValues;
};

#endif 
