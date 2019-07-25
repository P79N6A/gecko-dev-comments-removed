











































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
{ 0xb8e44bbe, 0xaaac, 0x4125, \
 { 0x8a, 0xb2, 0x0f, 0x42, 0x80, 0x2e, 0x14, 0xad } }










class nsIStyleRuleProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_PROCESSOR_IID)

  
  
  typedef PRBool (* EnumFunc)(nsIStyleRuleProcessor*, void*);

  





  virtual void RulesMatching(ElementRuleProcessorData* aData) = 0;

  



  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) = 0;

  


  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) = 0;

#ifdef MOZ_XUL
  



  virtual void RulesMatching(XULTreeRuleProcessorData* aData) = 0;
#endif

  




  virtual PRBool
    HasDocumentStateDependentStyle(StateRuleProcessorData* aData) = 0;

  







  virtual nsRestyleHint
    HasStateDependentStyle(StateRuleProcessorData* aData) = 0;

  















  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) = 0;

  




  virtual PRBool MediumFeaturesChanged(nsPresContext* aPresContext) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRuleProcessor,
                              NS_ISTYLE_RULE_PROCESSOR_IID)

#endif 
