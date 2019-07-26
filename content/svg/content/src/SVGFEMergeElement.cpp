




#include "mozilla/dom/SVGFEMergeElement.h"
#include "mozilla/dom/SVGFEMergeElementBinding.h"
#include "mozilla/dom/SVGFEMergeNodeElement.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEMerge)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGFEMergeElement::WrapNode(JSContext *aCx)
{
  return SVGFEMergeElementBinding::Wrap(aCx, this);
}

nsSVGElement::StringInfo SVGFEMergeElement::sStringInfo[1] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true }
};

NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEMergeElement)

FilterPrimitiveDescription
SVGFEMergeElement::GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                                           const IntRect& aFilterSubregion,
                                           const nsTArray<bool>& aInputsAreTainted,
                                           nsTArray<RefPtr<SourceSurface>>& aInputImages)
{
  return FilterPrimitiveDescription(PrimitiveType::Merge);
}

void
SVGFEMergeElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  for (nsIContent* child = nsINode::GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsSVG(nsGkAtoms::feMergeNode)) {
      SVGFEMergeNodeElement* node = static_cast<SVGFEMergeNodeElement*>(child);
      aSources.AppendElement(nsSVGStringInfo(node->GetIn1(), node));
    }
  }
}




nsSVGElement::StringAttributesInfo
SVGFEMergeElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
