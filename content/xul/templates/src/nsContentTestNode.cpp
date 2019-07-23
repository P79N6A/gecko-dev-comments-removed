





































#include "nsContentTestNode.h"
#include "nsISupportsArray.h"
#include "nsIRDFResource.h"
#include "nsIAtom.h"
#include "nsIDOMElement.h"
#include "nsXULContentUtils.h"
#include "nsPrintfCString.h"
#include "nsIXULTemplateResult.h"
#include "nsIXULTemplateBuilder.h"
#include "nsXULTemplateQueryProcessorRDF.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gXULTemplateLog;
#endif

nsContentTestNode::nsContentTestNode(nsXULTemplateQueryProcessorRDF* aProcessor,
                                     nsIAtom* aRefVariable)
    : TestNode(nsnull),
      mProcessor(aProcessor),
      mDocument(nsnull),
      mRefVariable(aRefVariable),
      mTag(nsnull)
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsAutoString tag(NS_LITERAL_STRING("(none)"));
        if (mTag)
            mTag->ToString(tag);

        nsAutoString refvar(NS_LITERAL_STRING("(none)"));
        if (aRefVariable)
            aRefVariable->ToString(refvar);

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsContentTestNode[%p]: ref-var=%s tag=%s",
                this, NS_ConvertUTF16toUTF8(refvar).get(),
                NS_ConvertUTF16toUTF8(tag).get()));
    }
#endif
}

nsresult
nsContentTestNode::FilterInstantiations(InstantiationSet& aInstantiations,
                                        PRBool* aCantHandleYet) const

{
    if (aCantHandleYet)
        *aCantHandleYet = PR_FALSE;
    return NS_OK;
}

nsresult
nsContentTestNode::Constrain(InstantiationSet& aInstantiations)
{
    

    nsIXULTemplateBuilder* builder = mProcessor->GetBuilder();
    if (!builder) {
        aInstantiations.Clear();
        return NS_OK;
    }

    nsresult rv;

    InstantiationSet::Iterator last = aInstantiations.Last();
    for (InstantiationSet::Iterator inst = aInstantiations.First(); inst != last; ++inst) {

        nsCOMPtr<nsIRDFNode> refValue;
        PRBool hasRefBinding = inst->mAssignments.GetAssignmentFor(mRefVariable,
                                                                   getter_AddRefs(refValue));
        if (hasRefBinding) {
            nsCOMPtr<nsIRDFResource> refResource = do_QueryInterface(refValue);
            if (refResource) {
                PRBool generated;
                rv = builder->HasGeneratedContent(refResource, mTag, &generated);
                if (NS_FAILED(rv)) return rv;

                if (generated)
                    continue;
            }
        }

        aInstantiations.Erase(inst--);
    }

    return NS_OK;
}
