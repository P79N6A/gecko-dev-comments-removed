




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
  explicit SVGGraphicsElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  ~SVGGraphicsElement();

public:
  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif
