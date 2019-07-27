











#include "SVGAttrAnimationRuleProcessor.h"
#include "nsRuleProcessorData.h"
#include "nsSVGElement.h"

using namespace mozilla;
using namespace mozilla::dom;

SVGAttrAnimationRuleProcessor::SVGAttrAnimationRuleProcessor()
{
}

SVGAttrAnimationRuleProcessor::~SVGAttrAnimationRuleProcessor()
{
}

NS_IMPL_ISUPPORTS(SVGAttrAnimationRuleProcessor, nsIStyleRuleProcessor)

 void
SVGAttrAnimationRuleProcessor::RulesMatching(ElementRuleProcessorData* aData)
{
  ElementRulesMatching(aData->mElement, aData->mRuleWalker);
}

void
SVGAttrAnimationRuleProcessor::ElementRulesMatching(Element* aElement,
                                                    nsRuleWalker* aRuleWalker)
{
  if (aElement->IsSVG()) {
    static_cast<nsSVGElement*>(aElement)->
      WalkAnimatedContentStyleRules(aRuleWalker);
  }
}

 nsRestyleHint
SVGAttrAnimationRuleProcessor::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

 nsRestyleHint
SVGAttrAnimationRuleProcessor::HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

 bool
SVGAttrAnimationRuleProcessor::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return false;
}

 nsRestyleHint
SVGAttrAnimationRuleProcessor::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

 bool
SVGAttrAnimationRuleProcessor::MediumFeaturesChanged(nsPresContext* aPresContext)
{
  return false;
}

 void
SVGAttrAnimationRuleProcessor::RulesMatching(PseudoElementRuleProcessorData* aData)
{
}

 void
SVGAttrAnimationRuleProcessor::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
SVGAttrAnimationRuleProcessor::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif

 size_t
SVGAttrAnimationRuleProcessor::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return 0; 
}

 size_t
SVGAttrAnimationRuleProcessor::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return 0; 
}

size_t
SVGAttrAnimationRuleProcessor::DOMSizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);

  return n;
}
