




#ifndef mozilla_dom_SVGAnimateMotionElement_h
#define mozilla_dom_SVGAnimateMotionElement_h

#include "mozilla/dom/SVGAnimationElement.h"
#include "SVGMotionSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateMotionElement MOZ_FINAL : public SVGAnimationElement,
                                          public nsIDOMSVGElement
{
protected:
  SVGAnimateMotionElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  SVGMotionSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                     already_AddRefed<nsINodeInfo> aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGAnimationElement::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
  virtual bool GetTargetAttributeName(int32_t *aNamespaceID,
                                        nsIAtom **aLocalName) const;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const;

  
  virtual nsIAtom* GetPathDataAttrName() const {
    return nsGkAtoms::path;
  }

  
  
  void MpathChanged() { mAnimationFunction.MpathChanged(); }

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
