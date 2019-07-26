




#ifndef __NS_SVGGRAPHICELEMENT_H__
#define __NS_SVGGRAPHICELEMENT_H__

#include "gfxMatrix.h"
#include "SVGTransformableElement.h"
#include "SVGAnimatedTransformList.h"
#include "DOMSVGTests.h"

typedef mozilla::dom::SVGTransformableElement nsSVGGraphicElementBase;

#define MOZILLA_SVGGRAPHICSELEMENT_IID \
  { 0xe57b8fe5, 0x9088, 0x446e, \
    {0xa1, 0x87, 0xd1, 0xdb, 0xbb, 0x58, 0xce, 0xdc}}

class nsSVGGraphicElement : public nsSVGGraphicElementBase,
                            public DOMSVGTests
{
protected:
  nsSVGGraphicElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_SVGGRAPHICSELEMENT_IID)
  NS_FORWARD_NSIDOMSVGLOCATABLE(mozilla::dom::SVGLocatableElement::)
  NS_FORWARD_NSIDOMSVGTRANSFORMABLE(mozilla::dom::SVGTransformableElement::)

  
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
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope, bool *triedToWrap) MOZ_OVERRIDE;

  

  nsAutoPtr<mozilla::SVGAnimatedTransformList> mTransforms;

  
  nsAutoPtr<gfxMatrix> mAnimateMotionTransform;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsSVGGraphicElement,
                              MOZILLA_SVGGRAPHICSELEMENT_IID)

#endif 
