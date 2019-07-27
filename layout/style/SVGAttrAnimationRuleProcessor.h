










#ifndef mozilla_SVGAttrAnimationRuleProcessor_h_
#define mozilla_SVGAttrAnimationRuleProcessor_h_

#include "nsIStyleRuleProcessor.h"

class nsRuleWalker;

namespace mozilla {

namespace dom {
class Element;
}

class SVGAttrAnimationRuleProcessor final : public nsIStyleRuleProcessor
{
public:
  SVGAttrAnimationRuleProcessor();

private:
  ~SVGAttrAnimationRuleProcessor();

public:
  NS_DECL_ISUPPORTS

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) override;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) override;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) override;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) override;
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) override;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) override;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) override;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;

  size_t DOMSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  void ElementRulesMatching(mozilla::dom::Element* aElement,
                            nsRuleWalker* aRuleWalker);

private:
  SVGAttrAnimationRuleProcessor(const SVGAttrAnimationRuleProcessor& aCopy) = delete;
  SVGAttrAnimationRuleProcessor& operator=(const SVGAttrAnimationRuleProcessor& aCopy) = delete;
};

} 

#endif 
