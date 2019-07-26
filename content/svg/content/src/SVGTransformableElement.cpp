




#include "mozilla/dom/SVGTransformableElement.h"
#include "DOMSVGAnimatedTransformList.h"
#include "nsIDOMMutationEvent.h"
#include "nsIFrame.h"
#include "nsSVGUtils.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGTransformableElement, SVGLocatableElement)
NS_IMPL_RELEASE_INHERITED(SVGTransformableElement, SVGLocatableElement)

NS_INTERFACE_MAP_BEGIN(SVGTransformableElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformable)
NS_INTERFACE_MAP_END_INHERITING(SVGLocatableElement)






NS_IMETHODIMP
SVGTransformableElement::GetTransform(nsISupports **aTransform)
{
  *aTransform = Transform().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedTransformList>
SVGTransformableElement::Transform()
{
  
  
  return DOMSVGAnimatedTransformList::GetDOMWrapper(
           GetAnimatedTransformList(DO_ALLOCATE), this).get();

}




NS_IMETHODIMP_(bool)
SVGTransformableElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sGraphicsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGLocatableElement::IsAttributeMapped(name);
}

nsChangeHint
SVGTransformableElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                                int32_t aModType) const
{
  nsChangeHint retval =
    SVGLocatableElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::transform ||
      aAttribute == nsGkAtoms::mozAnimateMotionDummyAttr) {
    
    
    nsIFrame* frame =
      const_cast<SVGTransformableElement*>(this)->GetPrimaryFrame();
    if (!frame || (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
      return retval; 
    }
    if (aModType == nsIDOMMutationEvent::ADDITION ||
        aModType == nsIDOMMutationEvent::REMOVAL) {
      
      NS_UpdateHint(retval, nsChangeHint_ReconstructFrame);
    } else {
      NS_ABORT_IF_FALSE(aModType == nsIDOMMutationEvent::MODIFICATION,
                        "Unknown modification type.");
      
      NS_UpdateHint(retval, NS_CombineHint(nsChangeHint_UpdateOverflow,
                                           nsChangeHint_UpdateTransformLayer));
    }
  }
  return retval;
}

bool
SVGTransformableElement::IsEventAttributeName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}




gfxMatrix
SVGTransformableElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                                  TransformTypes aWhich) const
{
  NS_ABORT_IF_FALSE(aWhich != eChildToUserSpace || aMatrix.IsIdentity(),
                    "Skipping eUserSpaceToParent transforms makes no sense");

  gfxMatrix result(aMatrix);

  if (aWhich == eChildToUserSpace) {
    
    
    
    
    return result;
  }

  NS_ABORT_IF_FALSE(aWhich == eAllTransforms || aWhich == eUserSpaceToParent,
                    "Unknown TransformTypes");

  
  
  
  if (mAnimateMotionTransform) {
    result.PreMultiply(*mAnimateMotionTransform);
  }

  if (mTransforms) {
    result.PreMultiply(mTransforms->GetAnimValue().GetConsolidationMatrix());
  }

  return result;
}

const gfxMatrix*
SVGTransformableElement::GetAnimateMotionTransform() const
{
  return mAnimateMotionTransform.get();
}

void
SVGTransformableElement::SetAnimateMotionTransform(const gfxMatrix* aMatrix)
{
  if ((!aMatrix && !mAnimateMotionTransform) ||
      (aMatrix && mAnimateMotionTransform && *aMatrix == *mAnimateMotionTransform)) {
    return;
  }
  mAnimateMotionTransform = aMatrix ? new gfxMatrix(*aMatrix) : nullptr;
  DidAnimateTransformList();
}

SVGAnimatedTransformList*
SVGTransformableElement::GetAnimatedTransformList(uint32_t aFlags)
{
  if (!mTransforms && (aFlags & DO_ALLOCATE)) {
    mTransforms = new SVGAnimatedTransformList();
  }
  return mTransforms;
}

} 
} 

