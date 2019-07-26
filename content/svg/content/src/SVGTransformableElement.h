




#ifndef SVGTransformableElement_h
#define SVGTransformableElement_h

#include "SVGLocatableElement.h"
#include "nsIDOMSVGTransformable.h"

#define MOZILLA_SVGTRANSFORMABLEELEMENT_IID \
  { 0x77888cba, 0x0b43, 0x4654, \
    {0x96, 0x3c, 0xf5, 0x50, 0xfc, 0xb5, 0x5e, 0x32}}

namespace mozilla {
class DOMSVGAnimatedTransformList;

namespace dom {
class SVGTransformableElement : public SVGLocatableElement,
                                public nsIDOMSVGTransformable
{
public:
  SVGTransformableElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGLocatableElement(aNodeInfo) {}
  virtual ~SVGTransformableElement() {}

  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_SVGTRANSFORMABLEELEMENT_IID)
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTRANSFORMABLE

  
  already_AddRefed<DOMSVGAnimatedTransformList> Transform();
};

NS_DEFINE_STATIC_IID_ACCESSOR(SVGTransformableElement,
                              MOZILLA_SVGTRANSFORMABLEELEMENT_IID)

} 
} 

#endif 

