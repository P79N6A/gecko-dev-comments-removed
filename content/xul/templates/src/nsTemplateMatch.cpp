





































#include "nsTemplateMatch.h"
#include "nsTemplateRule.h"

#ifdef NEED_CPP_UNUSED_IMPLEMENTATIONS
nsTemplateMatch::nsTemplateMatch(const nsTemplateMatch& aMatch) {}
void nsTemplateMatch::operator=(const nsTemplateMatch& aMatch) {}
#endif

nsresult
nsTemplateMatch::RuleMatched(nsTemplateQuerySet* aQuerySet,
                             nsTemplateRule* aRule,
                             PRInt16 aRuleIndex,
                             nsIXULTemplateResult* aResult)
{
    
    
    mRuleIndex = aRuleIndex;

    nsCOMPtr<nsIDOMNode> rulenode;
    aRule->GetRuleNode(getter_AddRefs(rulenode));
    if (rulenode) {
        nsCOMPtr<nsIDOMNode> querynode = do_QueryInterface(aQuerySet->mQueryNode);
        return aResult->RuleMatched(querynode, rulenode);
    }

    return NS_OK;
}
