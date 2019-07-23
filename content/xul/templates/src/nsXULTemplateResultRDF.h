



































#ifndef nsXULTemplateResultRDF_h__
#define nsXULTemplateResultRDF_h__

#include "nsCOMPtr.h"
#include "nsIRDFResource.h"
#include "nsXULTemplateQueryProcessorRDF.h"
#include "nsRDFQuery.h"
#include "nsRuleNetwork.h"
#include "nsIXULTemplateResult.h"
#include "nsRDFBinding.h"




class nsXULTemplateResultRDF : public nsIXULTemplateResult
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsXULTemplateResultRDF)

    NS_DECL_NSIXULTEMPLATERESULT

    nsXULTemplateResultRDF(nsIRDFResource* aNode);

    nsXULTemplateResultRDF(nsRDFQuery* aQuery,
                           const Instantiation& aInst,
                           nsIRDFResource* aNode);

    ~nsXULTemplateResultRDF();

    nsITemplateRDFQuery* Query() { return mQuery; }

    nsXULTemplateQueryProcessorRDF* GetProcessor()
    {
        return (mQuery ? mQuery->Processor() : nsnull);
    }

    



    void
    GetAssignment(nsIAtom* aVar, nsIRDFNode** aValue);

    



    PRBool
    SyncAssignments(nsIRDFResource* aSubject,
                    nsIRDFResource* aPredicate,
                    nsIRDFNode* aTarget);

    



    PRBool
    HasMemoryElement(const MemoryElement& aMemoryElement);

protected:

    
    nsCOMPtr<nsITemplateRDFQuery> mQuery;

    
    nsCOMPtr<nsIRDFResource> mNode;

    
    Instantiation mInst;

    
    nsBindingValues mBindingValues;
};

#endif 
