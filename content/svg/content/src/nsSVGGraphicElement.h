




#ifndef __NS_SVGGRAPHICELEMENT_H__
#define __NS_SVGGRAPHICELEMENT_H__

#include "gfxMatrix.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGTransformable.h"
#include "nsSVGStylableElement.h"
#include "SVGAnimatedTransformList.h"

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

  nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                      PRInt32 aModType) const;

  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const;
  virtual const gfxMatrix* GetAnimateMotionTransform() const;
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
