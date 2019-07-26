




#ifndef mozilla_dom_SVGAnimateMotionElement_h
#define mozilla_dom_SVGAnimateMotionElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "SVGMotionSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateMotionElement MOZ_FINAL : public SVGAnimationElement
{
protected:
  SVGAnimateMotionElement(already_AddRefed<nsINodeInfo>& aNodeInfo);

  SVGMotionSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                     already_AddRefed<nsINodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
  virtual bool GetTargetAttributeName(int32_t *aNamespaceID,
                                      nsIAtom **aLocalName) const;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const;

  
  virtual nsIAtom* GetPathDataAttrName() const MOZ_OVERRIDE {
    return nsGkAtoms::path;
  }

  
  
  void MpathChanged() { mAnimationFunction.MpathChanged(); }
};

} 
} 

#endif 
