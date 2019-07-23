











































#ifndef nsIStyleRuleProcessor_h___
#define nsIStyleRuleProcessor_h___

#include "nsISupports.h"
#include "nsChangeHint.h"
#include "nsIContent.h"

struct RuleProcessorData;
struct ElementRuleProcessorData;
struct PseudoElementRuleProcessorData;
struct AnonBoxRuleProcessorData;
#ifdef MOZ_XUL
struct XULTreeRuleProcessorData;
#endif
struct StateRuleProcessorData;
struct AttributeRuleProcessorData;
class nsPresContext;



#define NS_ISTYLE_RULE_PROCESSOR_IID     \
{ 0xec92bc0c, 0x9518, 0x48ea, \
 { 0x92, 0x89, 0x74, 0xe6, 0x54, 0x65, 0x9b, 0xe9 } }









class nsIStyleRuleProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_PROCESSOR_IID)

  
  
  typedef PRBool (* EnumFunc)(nsIStyleRuleProcessor*, void*);

  





  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData) = 0;

  



  NS_IMETHOD RulesMatching(PseudoElementRuleProcessorData* aData) = 0;

  


  NS_IMETHOD RulesMatching(AnonBoxRuleProcessorData* aData) = 0;

#ifdef MOZ_XUL
  



  NS_IMETHOD RulesMatching(XULTreeRuleProcessorData* aData) = 0;
#endif

  







  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsReStyleHint* aResult) = 0;

  















  virtual nsReStyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) = 0;

  




  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                   PRBool* aRulesChanged) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRuleProcessor,
                              NS_ISTYLE_RULE_PROCESSOR_IID)

#endif 
