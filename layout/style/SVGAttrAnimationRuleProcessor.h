










#ifndef mozilla_SVGAttrAnimationRuleProcessor_h_
#define mozilla_SVGAttrAnimationRuleProcessor_h_

#include "nsIStyleRuleProcessor.h"

class nsIDocument;

namespace mozilla {

class SVGAttrAnimationRuleProcessor MOZ_FINAL : public nsIStyleRuleProcessor
{
public:
  SVGAttrAnimationRuleProcessor();

private:
  ~SVGAttrAnimationRuleProcessor();

public:
  NS_DECL_ISUPPORTS

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) MOZ_OVERRIDE;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  size_t DOMSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  SVGAttrAnimationRuleProcessor(const SVGAttrAnimationRuleProcessor& aCopy) MOZ_DELETE;
  SVGAttrAnimationRuleProcessor& operator=(const SVGAttrAnimationRuleProcessor& aCopy) MOZ_DELETE;
};

} 

#endif 
