








































#ifndef nsInspectorCSSUtils_h___
#define nsInspectorCSSUtils_h___

#include "nsIInspectorCSSUtils.h"
#include "nsStyleContext.h"
#include "nsAutoPtr.h"

class nsIPresShell;

class nsInspectorCSSUtils : public nsIInspectorCSSUtils {

public:

    nsInspectorCSSUtils();
    virtual ~nsInspectorCSSUtils();

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD LookupCSSProperty(const nsAString& aName, nsCSSProperty *aProp);
    NS_IMETHOD GetRuleNodeParent(nsRuleNode *aNode, nsRuleNode **aParent);
    NS_IMETHOD GetRuleNodeRule(nsRuleNode *aNode, nsIStyleRule **aRule);
    NS_IMETHOD IsRuleNodeRoot(nsRuleNode *aNode, PRBool *aIsRoot);
};

#endif 
