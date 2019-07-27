




#ifndef mozilla_dom_SVGAnimateMotionElement_h
#define mozilla_dom_SVGAnimateMotionElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "SVGMotionSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                       already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateMotionElement MOZ_FINAL : public SVGAnimationElement
{
protected:
  explicit SVGAnimateMotionElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  SVGMotionSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                     already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

public:
  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  virtual nsSMILAnimationFunction& AnimationFunction() MOZ_OVERRIDE;
  virtual bool GetTargetAttributeName(int32_t *aNamespaceID,
                                      nsIAtom **aLocalName) const MOZ_OVERRIDE;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const MOZ_OVERRIDE;

  
  virtual nsIAtom* GetPathDataAttrName() const MOZ_OVERRIDE {
    return nsGkAtoms::path;
  }

  
  
  void MpathChanged() { mAnimationFunction.MpathChanged(); }
};

} 
} 

#endif 
