




#ifndef __NS_SVGGRAPHICELEMENT_H__
#define __NS_SVGGRAPHICELEMENT_H__

#include "gfxMatrix.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGTransformable.h"
#include "nsSVGElement.h"
#include "SVGAnimatedTransformList.h"
#include "DOMSVGTests.h"

typedef nsSVGElement nsSVGGraphicElementBase;

class nsSVGGraphicElement : public nsSVGGraphicElementBase,
                            public DOMSVGTests,
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
                                      int32_t aModType) const;


  virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;


  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const;
  virtual const gfxMatrix* GetAnimateMotionTransform() const;
  virtual void SetAnimateMotionTransform(const gfxMatrix* aMatrix);

  virtual mozilla::SVGAnimatedTransformList*
    GetAnimatedTransformList(uint32_t aFlags = 0);
  virtual nsIAtom* GetTransformListAttrName() const {
    return nsGkAtoms::transform;
  }

protected:
  

  nsAutoPtr<mozilla::SVGAnimatedTransformList> mTransforms;

  
  nsAutoPtr<gfxMatrix> mAnimateMotionTransform;
};

#endif 
