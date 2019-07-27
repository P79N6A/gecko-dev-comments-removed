





#ifndef mozilla_dom_SVGAnimateMotionElement_h
#define mozilla_dom_SVGAnimateMotionElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "SVGMotionSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                       already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateMotionElement final : public SVGAnimationElement
{
protected:
  explicit SVGAnimateMotionElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  SVGMotionSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                     already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  virtual nsSMILAnimationFunction& AnimationFunction() override;
  virtual bool GetTargetAttributeName(int32_t *aNamespaceID,
                                      nsIAtom **aLocalName) const override;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const override;

  
  virtual nsIAtom* GetPathDataAttrName() const override {
    return nsGkAtoms::path;
  }

  
  
  void MpathChanged() { mAnimationFunction.MpathChanged(); }
};

} 
} 

#endif 
