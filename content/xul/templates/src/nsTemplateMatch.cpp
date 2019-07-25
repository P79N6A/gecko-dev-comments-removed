





































#include "nsTemplateMatch.h"
#include "nsTemplateRule.h"

#ifdef NEED_CPP_UNUSED_IMPLEMENTATIONS
nsTemplateMatch::nsTemplateMatch(const nsTemplateMatch& aMatch) {}
void nsTemplateMatch::operator=(const nsTemplateMatch& aMatch) {}
#endif


void
nsTemplateMatch::Destroy(nsFixedSizeAllocator& aPool,
                         nsTemplateMatch*& aMatch,
                         PRBool aRemoveResult) {
    if (aRemoveResult && aMatch->mResult)
        aMatch->mResult->HasBeenRemoved();
    aMatch->~nsTemplateMatch();
    aPool.Free(aMatch, sizeof(*aMatch));
    aMatch = nsnull;
}

nsresult
nsTemplateMatch::RuleMatched(nsTemplateQuerySet* aQuerySet,
                             nsTemplateRule* aRule,
                             PRInt16 aRuleIndex,
                             nsIXULTemplateResult* aResult)
{
    
    
    mRuleIndex = aRuleIndex;

    nsCOMPtr<nsIDOMNode> rulenode;
    aRule->GetRuleNode(getter_AddRefs(rulenode));
    if (rulenode)
        return aResult->RuleMatched(aQuerySet->mCompiledQuery, rulenode);

    return NS_OK;
}
