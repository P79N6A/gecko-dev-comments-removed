








































#include "nsInspectorCSSUtils.h"
#include "nsIStyleRule.h"
#include "nsRuleNode.h"
#include "nsString.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsIFrame.h"
#include "nsStyleSet.h"

nsInspectorCSSUtils::nsInspectorCSSUtils()
{
    nsCSSProps::AddRefTable();
}

nsInspectorCSSUtils::~nsInspectorCSSUtils()
{
    nsCSSProps::ReleaseTable();
}

NS_IMPL_ISUPPORTS1(nsInspectorCSSUtils, nsIInspectorCSSUtils)

NS_IMETHODIMP
nsInspectorCSSUtils::LookupCSSProperty(const nsAString& aName, nsCSSProperty *aProp)
{
    *aProp = nsCSSProps::LookupProperty(aName);
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::GetRuleNodeParent(nsRuleNode *aNode, nsRuleNode **aParent)
{
    *aParent = aNode->GetParent();
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::GetRuleNodeRule(nsRuleNode *aNode, nsIStyleRule **aRule)
{
    *aRule = aNode->GetRule();
    NS_IF_ADDREF(*aRule);
    return NS_OK;
}

NS_IMETHODIMP
nsInspectorCSSUtils::IsRuleNodeRoot(nsRuleNode *aNode, PRBool *aIsRoot)
{
    *aIsRoot = aNode->IsRoot();
    return NS_OK;
}
