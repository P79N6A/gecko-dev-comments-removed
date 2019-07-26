




#ifndef SVGTransformableElement_h
#define SVGTransformableElement_h

#include "mozilla/Attributes.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGElement.h"
#include "mozilla/gfx/Matrix.h"

namespace mozilla {
namespace dom {

class SVGAnimatedTransformList;
class SVGGraphicsElement;
class SVGMatrix;
class SVGIRect;

class SVGTransformableElement : public nsSVGElement
{
public:
  SVGTransformableElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGElement(aNodeInfo) {}
  virtual ~SVGTransformableElement() {}

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE = 0;

  
  already_AddRefed<SVGAnimatedTransformList> Transform();
  nsSVGElement* GetNearestViewportElement();
  nsSVGElement* GetFarthestViewportElement();
  already_AddRefed<SVGIRect> GetBBox(ErrorResult& rv);
  already_AddRefed<SVGMatrix> GetCTM();
  already_AddRefed<SVGMatrix> GetScreenCTM();
  already_AddRefed<SVGMatrix> GetTransformToElement(SVGGraphicsElement& aElement,
                                                    ErrorResult& rv);

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                      int32_t aModType) const MOZ_OVERRIDE;


  virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;


  virtual gfx::Matrix PrependLocalTransformsTo(const gfx::Matrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const MOZ_OVERRIDE;
  virtual const gfx::Matrix* GetAnimateMotionTransform() const MOZ_OVERRIDE;
  virtual void SetAnimateMotionTransform(const gfx::Matrix* aMatrix) MOZ_OVERRIDE;

  virtual nsSVGAnimatedTransformList*
    GetAnimatedTransformList(uint32_t aFlags = 0) MOZ_OVERRIDE;
  virtual nsIAtom* GetTransformListAttrName() const MOZ_OVERRIDE {
    return nsGkAtoms::transform;
  }

  virtual bool IsTransformable() MOZ_OVERRIDE { return true; }

protected:
  

  nsAutoPtr<nsSVGAnimatedTransformList> mTransforms;

  
  nsAutoPtr<gfx::Matrix> mAnimateMotionTransform;
};

} 
} 

#endif 

