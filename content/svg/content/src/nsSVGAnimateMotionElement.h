




































#ifndef NS_SVGANIMATEMOTIONELEMENT_H_
#define NS_SVGANIMATEMOTIONELEMENT_H_

#include "nsSVGAnimationElement.h"
#include "nsIDOMSVGAnimateMotionElement.h"
#include "nsSVGEnum.h"
#include "SVGMotionSMILAnimationFunction.h"

typedef nsSVGAnimationElement nsSVGAnimateMotionElementBase;

class nsSVGAnimateMotionElement : public nsSVGAnimateMotionElementBase,
                                  public nsIDOMSVGAnimateMotionElement
{
protected:
  friend nsresult NS_NewSVGAnimateMotionElement(nsIContent **aResult,
                                                already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGAnimateMotionElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  mozilla::SVGMotionSMILAnimationFunction mAnimationFunction;

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGANIMATEMOTIONELEMENT

  NS_FORWARD_NSIDOMNODE(nsSVGAnimateMotionElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGAnimateMotionElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGAnimateMotionElementBase::)
  NS_FORWARD_NSIDOMSVGANIMATIONELEMENT(nsSVGAnimateMotionElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
  virtual bool GetTargetAttributeName(PRInt32 *aNamespaceID,
                                        nsIAtom **aLocalName) const;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const;

  
  virtual nsIAtom* GetPathDataAttrName() const {
    return nsGkAtoms::path;
  }

  
  
  void MpathChanged() { mAnimationFunction.MpathChanged(); }

  virtual nsXPCClassInfo* GetClassInfo();
};

#endif 
