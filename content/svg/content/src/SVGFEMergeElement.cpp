




#include "mozilla/dom/SVGFEMergeElement.h"
#include "mozilla/dom/SVGFEMergeElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEMerge)

namespace mozilla {
namespace dom {

JSObject*
SVGFEMergeElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGFEMergeElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::StringInfo SVGFEMergeElement::sStringInfo[1] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true }
};




NS_IMPL_ISUPPORTS_INHERITED3(SVGFEMergeElement, SVGFEMergeElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)

NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEMergeElement)

nsresult
SVGFEMergeElement::Filter(nsSVGFilterInstance *instance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& rect)
{
  gfxContext ctx(aTarget->mImage);
  ctx.Clip(aTarget->mFilterPrimitiveSubregion);

  for (uint32_t i = 0; i < aSources.Length(); i++) {
    ctx.SetSource(aSources[i]->mImage);
    ctx.Paint();
  }
  return NS_OK;
}

void
SVGFEMergeElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  for (nsIContent* child = nsINode::GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    nsRefPtr<nsSVGFEMergeNodeElement> node;
    CallQueryInterface(child, (nsSVGFEMergeNodeElement**)getter_AddRefs(node));
    if (node) {
      aSources.AppendElement(nsSVGStringInfo(node->In1(), node));
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
