




#ifndef mozilla_dom_SVGForeignObjectElement_h
#define mozilla_dom_SVGForeignObjectElement_h

#include "mozilla/dom/SVGGraphicsElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGForeignObjectElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);

class nsSVGForeignObjectFrame;

namespace mozilla {
namespace dom {

class SVGForeignObjectElement MOZ_FINAL : public SVGGraphicsElement
{
  friend class ::nsSVGForeignObjectFrame;

protected:
  friend nsresult (::NS_NewSVGForeignObjectElement(nsIContent **aResult,
                                                   already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGForeignObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

public:

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const;
  virtual bool HasValidDimensions() const;

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Height();

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

} 
} 

#endif 

