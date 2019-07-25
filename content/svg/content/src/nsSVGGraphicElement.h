





































#ifndef __NS_SVGGRAPHICELEMENT_H__
#define __NS_SVGGRAPHICELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGTransformable.h"
#include "SVGAnimatedTransformList.h"
#include "gfxMatrix.h"

typedef nsSVGStylableElement nsSVGGraphicElementBase;

class nsSVGGraphicElement : public nsSVGGraphicElementBase,
                            public nsIDOMSVGTransformable 
{
protected:
  nsSVGGraphicElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGLOCATABLE
  NS_DECL_NSIDOMSVGTRANSFORMABLE

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual gfxMatrix PrependLocalTransformTo(const gfxMatrix &aMatrix) const;
  virtual void SetAnimateMotionTransform(const gfxMatrix* aMatrix);

  virtual mozilla::SVGAnimatedTransformList* GetAnimatedTransformList();
  virtual nsIAtom* GetTransformListAttrName() const {
    return nsGkAtoms::transform;
  }

protected:
  
  virtual bool IsEventName(nsIAtom* aName);

  nsAutoPtr<mozilla::SVGAnimatedTransformList> mTransforms;

  
  nsAutoPtr<gfxMatrix> mAnimateMotionTransform;
};

#endif 
