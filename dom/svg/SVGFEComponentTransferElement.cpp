





#include "mozilla/dom/SVGComponentTransferFunctionElement.h"
#include "mozilla/dom/SVGFEComponentTransferElement.h"
#include "mozilla/dom/SVGFEComponentTransferElementBinding.h"
#include "nsSVGUtils.h"
#include "mozilla/gfx/2D.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEComponentTransfer)

using namespace mozilla::gfx;;

namespace mozilla {
namespace dom {

JSObject*
SVGFEComponentTransferElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGFEComponentTransferElementBinding::Wrap(aCx, this, aGivenProto);
}

nsSVGElement::StringInfo SVGFEComponentTransferElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEComponentTransferElement)

already_AddRefed<SVGAnimatedString>
SVGFEComponentTransferElement::In1()
{
  return mStringAttributes[IN1].ToDOMAnimatedString(this);
}




nsSVGElement::StringAttributesInfo
SVGFEComponentTransferElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



FilterPrimitiveDescription
SVGFEComponentTransferElement::GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                                                       const IntRect& aFilterSubregion,
                                                       const nsTArray<bool>& aInputsAreTainted,
                                                       nsTArray<RefPtr<SourceSurface>>& aInputImages)
{
  nsRefPtr<SVGComponentTransferFunctionElement> childForChannel[4];

  for (nsIContent* childContent = nsINode::GetFirstChild();
       childContent;
       childContent = childContent->GetNextSibling()) {

    nsRefPtr<SVGComponentTransferFunctionElement> child;
    CallQueryInterface(childContent,
            (SVGComponentTransferFunctionElement**)getter_AddRefs(child));
    if (child) {
      childForChannel[child->GetChannel()] = child;
    }
  }

  static const AttributeName attributeNames[4] = {
    eComponentTransferFunctionR,
    eComponentTransferFunctionG,
    eComponentTransferFunctionB,
    eComponentTransferFunctionA
  };

  FilterPrimitiveDescription descr(PrimitiveType::ComponentTransfer);
  for (int32_t i = 0; i < 4; i++) {
    if (childForChannel[i]) {
      descr.Attributes().Set(attributeNames[i], childForChannel[i]->ComputeAttributes());
    } else {
      AttributeMap functionAttributes;
      functionAttributes.Set(eComponentTransferFunctionType,
                             (uint32_t)SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY);
      descr.Attributes().Set(attributeNames[i], functionAttributes);
    }
  }
  return descr;
}

bool
SVGFEComponentTransferElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                         nsIAtom* aAttribute) const
{
  return SVGFEComponentTransferElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          aAttribute == nsGkAtoms::in);
}

void
SVGFEComponentTransferElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

} 
} 
