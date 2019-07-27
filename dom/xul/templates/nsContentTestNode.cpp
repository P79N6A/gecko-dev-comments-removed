




#include "nsContentTestNode.h"
#include "nsIRDFResource.h"
#include "nsIAtom.h"
#include "nsIDOMElement.h"
#include "nsXULContentUtils.h"
#include "nsIXULTemplateResult.h"
#include "nsIXULTemplateBuilder.h"
#include "nsXULTemplateQueryProcessorRDF.h"

#include "mozilla/Logging.h"
extern PRLogModuleInfo* gXULTemplateLog;

nsContentTestNode::nsContentTestNode(nsXULTemplateQueryProcessorRDF* aProcessor,
                                     nsIAtom* aRefVariable)
    : TestNode(nullptr),
      mProcessor(aProcessor),
      mDocument(nullptr),
      mRefVariable(aRefVariable),
      mTag(nullptr)
{
    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsAutoString tag(NS_LITERAL_STRING("(none)"));
        if (mTag)
            mTag->ToString(tag);

        nsAutoString refvar(NS_LITERAL_STRING("(none)"));
        if (aRefVariable)
            aRefVariable->ToString(refvar);

        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsContentTestNode[%p]: ref-var=%s tag=%s",
                this, NS_ConvertUTF16toUTF8(refvar).get(),
                NS_ConvertUTF16toUTF8(tag).get()));
    }
}

nsresult
nsContentTestNode::FilterInstantiations(InstantiationSet& aInstantiations,
                                        bool* aCantHandleYet) const

{
    if (aCantHandleYet)
        *aCantHandleYet = false;
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
        bool hasRefBinding = inst->mAssignments.GetAssignmentFor(mRefVariable,
                                                                   getter_AddRefs(refValue));
        if (hasRefBinding) {
            nsCOMPtr<nsIRDFResource> refResource = do_QueryInterface(refValue);
            if (refResource) {
                bool generated;
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
