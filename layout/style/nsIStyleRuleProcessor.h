











































#ifndef nsIStyleRuleProcessor_h___
#define nsIStyleRuleProcessor_h___

#include "nsISupports.h"
#include "nsChangeHint.h"
#include "nsIContent.h"

struct RuleProcessorData;
struct ElementRuleProcessorData;
struct PseudoElementRuleProcessorData;
struct PseudoRuleProcessorData;
struct StateRuleProcessorData;
struct AttributeRuleProcessorData;
class nsPresContext;


#define NS_ISTYLE_RULE_PROCESSOR_IID     \
{ 0xa4ec760e, 0x6bfb, 0x4b9f, \
 { 0xbd, 0x08, 0x9d, 0x1c, 0x23, 0xb7, 0x00, 0xf6 } }









class nsIStyleRuleProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_PROCESSOR_IID)

  
  
  typedef PRBool (* EnumFunc)(nsIStyleRuleProcessor*, void*);

  





  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData) = 0;

  



  NS_IMETHOD RulesMatching(PseudoElementRuleProcessorData* aData) = 0;

  



  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData) = 0;

  







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
