











































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
{ 0x32612c0e, 0x3d34, 0x4a6f, \
  {0x89, 0xd9, 0x46, 0x4f, 0x68, 0x11, 0xac, 0x13} }










class nsIStyleRuleProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_PROCESSOR_IID)

  
  
  typedef bool (* EnumFunc)(nsIStyleRuleProcessor*, void*);

  





  virtual void RulesMatching(ElementRuleProcessorData* aData) = 0;

  



  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) = 0;

  


  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) = 0;

#ifdef MOZ_XUL
  



  virtual void RulesMatching(XULTreeRuleProcessorData* aData) = 0;
#endif

  




  virtual bool
    HasDocumentStateDependentStyle(StateRuleProcessorData* aData) = 0;

  







  virtual nsRestyleHint
    HasStateDependentStyle(StateRuleProcessorData* aData) = 0;

  















  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) = 0;

  




  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) = 0;

  



  virtual PRInt64 SizeOf() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRuleProcessor,
                              NS_ISTYLE_RULE_PROCESSOR_IID)

#endif 
