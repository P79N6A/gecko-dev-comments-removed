




#ifndef mozilla_dom_SVGGraphicsElement_h
#define mozilla_dom_SVGGraphicsElement_h

#include "mozilla/dom/SVGTests.h"
#include "mozilla/dom/SVGTransformableElement.h"

namespace mozilla {
namespace dom {

typedef SVGTransformableElement SVGGraphicsElementBase;

class SVGGraphicsElement : public SVGGraphicsElementBase,
                           public SVGTests
{
protected:
  SVGGraphicsElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif
