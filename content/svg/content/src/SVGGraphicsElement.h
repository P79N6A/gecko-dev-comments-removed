




#ifndef mozilla_dom_SVGGraphicsElement_h
#define mozilla_dom_SVGGraphicsElement_h

#include "mozilla/dom/SVGTransformableElement.h"
#include "DOMSVGTests.h"

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
  NS_FORWARD_NSIDOMSVGLOCATABLE(SVGLocatableElement::)
  NS_FORWARD_NSIDOMSVGTRANSFORMABLE(SVGTransformableElement::)
};

} 
} 

#endif
