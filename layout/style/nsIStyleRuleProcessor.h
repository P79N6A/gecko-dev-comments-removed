











































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
{ 0x566a7bea, 0xfdc5, 0x40a5, \
 { 0xbf, 0x8a, 0x87, 0xb5, 0xa2, 0x31, 0xd7, 0x9e } }









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

  







  virtual nsReStyleHint
    HasStateDependentStyle(StateRuleProcessorData* aData) = 0;

  















  virtual nsReStyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) = 0;

  




  NS_IMETHOD MediumFeaturesChanged(nsPresContext* aPresContext,
                                   PRBool* aRulesChanged) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRuleProcessor,
                              NS_ISTYLE_RULE_PROCESSOR_IID)

#endif 
