










#ifndef nsIStyleRuleProcessor_h___
#define nsIStyleRuleProcessor_h___

#include "mozilla/MemoryReporting.h"
#include "nsISupports.h"
#include "nsChangeHint.h"

struct RuleProcessorData;
struct ElementRuleProcessorData;
struct PseudoElementRuleProcessorData;
struct AnonBoxRuleProcessorData;
#ifdef MOZ_XUL
struct XULTreeRuleProcessorData;
#endif
struct StateRuleProcessorData;
struct PseudoElementStateRuleProcessorData;
struct AttributeRuleProcessorData;
class nsPresContext;



#define NS_ISTYLE_RULE_PROCESSOR_IID     \
{ 0xc1d6001e, 0x4fcb, 0x4c40, \
  {0xbc, 0xe1, 0x5e, 0xba, 0x80, 0xbf, 0xd8, 0xf3} }










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
    HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) = 0;

  















  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) = 0;

  




  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) = 0;

  



  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const = 0;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRuleProcessor,
                              NS_ISTYLE_RULE_PROCESSOR_IID)

#endif 
