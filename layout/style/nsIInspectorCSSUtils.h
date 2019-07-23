








































#ifndef nsIInspectorCSSUtils_h___
#define nsIInspectorCSSUtils_h___

#include "nsISupports.h"
#include "nsCSSProps.h"
class nsRuleNode;
class nsIStyleRule;
class nsIFrame;
struct nsRect;
class nsIContent;
class nsIDOMElement;
class nsIArray;


#define NS_IINSPECTORCSSUTILS_IID \
  { 0x35dfc2a6, 0xb069, 0x4014, \
    {0xad, 0x4b, 0x01, 0x92, 0x7e, 0x77, 0xd8, 0x28 } }


#define NS_INSPECTORCSSUTILS_CID \
  { 0x7ef2f07f, 0x6e34, 0x410b, \
    {0x83, 0x36, 0x88, 0xac, 0xd1, 0xcd, 0x16, 0xb7 } }

class nsIInspectorCSSUtils : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IINSPECTORCSSUTILS_IID)

    
    
    
    NS_IMETHOD LookupCSSProperty(const nsAString& aName, nsCSSProperty *aProp) = 0;

    
    
    NS_IMETHOD GetRuleNodeParent(nsRuleNode *aNode, nsRuleNode **aParent) = 0;
    NS_IMETHOD GetRuleNodeRule(nsRuleNode *aNode, nsIStyleRule **aRule) = 0;
    NS_IMETHOD IsRuleNodeRoot(nsRuleNode *aNode, PRBool *aIsRoot) = 0;

    
    NS_IMETHOD GetRuleNodeForContent(nsIContent* aContent,
                                     nsRuleNode** aParent) = 0;

    
    NS_IMETHOD GetBindingURLs(nsIDOMElement *aElement, nsIArray **aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIInspectorCSSUtils, NS_IINSPECTORCSSUTILS_IID)

#endif 
