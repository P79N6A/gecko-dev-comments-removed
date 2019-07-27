




#ifndef nsXULTemplateResultRDF_h__
#define nsXULTemplateResultRDF_h__

#include "nsCOMPtr.h"
#include "nsIRDFResource.h"
#include "nsXULTemplateQueryProcessorRDF.h"
#include "nsRDFQuery.h"
#include "nsRuleNetwork.h"
#include "nsIXULTemplateResult.h"
#include "nsRDFBinding.h"
#include "mozilla/Attributes.h"




class nsXULTemplateResultRDF MOZ_FINAL : public nsIXULTemplateResult
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsXULTemplateResultRDF)

    NS_DECL_NSIXULTEMPLATERESULT

    explicit nsXULTemplateResultRDF(nsIRDFResource* aNode);

    nsXULTemplateResultRDF(nsRDFQuery* aQuery,
                           const Instantiation& aInst,
                           nsIRDFResource* aNode);

    nsITemplateRDFQuery* Query() { return mQuery; }

    nsXULTemplateQueryProcessorRDF* GetProcessor()
    {
        return (mQuery ? mQuery->Processor() : nullptr);
    }

    



    void
    GetAssignment(nsIAtom* aVar, nsIRDFNode** aValue);

    



    bool
    SyncAssignments(nsIRDFResource* aSubject,
                    nsIRDFResource* aPredicate,
                    nsIRDFNode* aTarget);

    



    bool
    HasMemoryElement(const MemoryElement& aMemoryElement);

protected:
    ~nsXULTemplateResultRDF();

    
    nsCOMPtr<nsITemplateRDFQuery> mQuery;

    
    nsCOMPtr<nsIRDFResource> mNode;

    
    Instantiation mInst;

    
    nsBindingValues mBindingValues;
};

#endif 
