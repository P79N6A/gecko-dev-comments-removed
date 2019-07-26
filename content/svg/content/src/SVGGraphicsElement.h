




#ifndef mozilla_dom_SVGGraphicsElement_h
#define mozilla_dom_SVGGraphicsElement_h

#include "mozilla/dom/SVGTransformableElement.h"
#include "DOMSVGTests.h"

#define MOZILLA_SVGGRAPHICSELEMENT_IID \
  { 0xe57b8fe5, 0x9088, 0x446e, \
    {0xa1, 0x87, 0xd1, 0xdb, 0xbb, 0x58, 0xce, 0xdc}}

namespace mozilla {
namespace dom {

typedef SVGTransformableElement SVGGraphicsElementBase;

class SVGGraphicsElement : public SVGGraphicsElementBase,
                           public DOMSVGTests
{
protected:
  SVGGraphicsElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_SVGGRAPHICSELEMENT_IID)
  NS_FORWARD_NSIDOMSVGLOCATABLE(SVGLocatableElement::)
  NS_FORWARD_NSIDOMSVGTRANSFORMABLE(SVGTransformableElement::)

protected:
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope, bool *triedToWrap) MOZ_OVERRIDE;

};

NS_DEFINE_STATIC_IID_ACCESSOR(SVGGraphicsElement,
                              MOZILLA_SVGGRAPHICSELEMENT_IID)

} 
} 

#endif 
