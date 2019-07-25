






































#include "nsInstantiationNode.h"
#include "nsTemplateRule.h"
#include "nsXULTemplateQueryProcessorRDF.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gXULTemplateLog;
#endif

nsInstantiationNode::nsInstantiationNode(nsXULTemplateQueryProcessorRDF* aProcessor,
                                         nsRDFQuery* aQuery)
        : mProcessor(aProcessor),
          mQuery(aQuery)
{
#ifdef PR_LOGGING
    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
           ("nsInstantiationNode[%p] query=%p", this, aQuery));
#endif

    MOZ_COUNT_CTOR(nsInstantiationNode);
}


nsInstantiationNode::~nsInstantiationNode()
{
    MOZ_COUNT_DTOR(nsInstantiationNode);
}

nsresult
nsInstantiationNode::Propagate(InstantiationSet& aInstantiations,
                               PRBool aIsUpdate, PRBool& aTakenInstantiations)
{
    
    
    
    
    
    nsresult rv = NS_OK;

    aTakenInstantiations = PR_FALSE;

    if (aIsUpdate) {
        
        
        
        
        
        nsCOMPtr<nsIDOMNode> querynode;
        mQuery->GetQueryNode(getter_AddRefs(querynode));

        InstantiationSet::ConstIterator last = aInstantiations.Last();
        for (InstantiationSet::ConstIterator inst = aInstantiations.First(); inst != last; ++inst) {
            nsAssignmentSet assignments = inst->mAssignments;

            nsCOMPtr<nsIRDFNode> node;
            assignments.GetAssignmentFor(mQuery->mMemberVariable,
                                         getter_AddRefs(node));
            if (node) {
                nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(node);
                if (resource) {
                    nsRefPtr<nsXULTemplateResultRDF> nextresult =
                        new nsXULTemplateResultRDF(mQuery, *inst, resource);
                    if (! nextresult)
                        return NS_ERROR_OUT_OF_MEMORY;

                    rv = mProcessor->AddMemoryElements(*inst, nextresult);
                    if (NS_FAILED(rv))
                        return rv;

                    mProcessor->GetBuilder()->AddResult(nextresult, querynode);
                }
            }
        }
    }
    else {
        nsresult rv = mQuery->SetCachedResults(mProcessor, aInstantiations);
        if (NS_SUCCEEDED(rv))
            aTakenInstantiations = PR_TRUE;
    }

    return rv;
}
