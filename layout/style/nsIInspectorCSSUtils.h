








































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
  { 0x5cfdb02f, 0x0962, 0x454c, \
    { 0xb0, 0x15, 0xb0, 0x51, 0x23, 0x92, 0x70, 0x21 } }


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
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIInspectorCSSUtils, NS_IINSPECTORCSSUTILS_IID)

#endif 
