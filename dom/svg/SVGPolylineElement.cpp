




#include "mozilla/dom/SVGPolylineElement.h"
#include "mozilla/dom/SVGPolylineElementBinding.h"
#include "mozilla/gfx/2D.h"

using namespace mozilla;
using namespace mozilla::gfx;

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polyline)

namespace mozilla {
namespace dom {

JSObject*
SVGPolylineElement::WrapNode(JSContext *aCx)
{
  return SVGPolylineElementBinding::Wrap(aCx, this);
}




SVGPolylineElement::SVGPolylineElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGPolylineElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPolylineElement)




TemporaryRef<Path>
SVGPolylineElement::BuildPath(PathBuilder* aBuilder)
{
  const SVGPointList &points = mPoints.GetAnimValue();

  if (points.IsEmpty()) {
    return nullptr;
  }

  aBuilder->MoveTo(points[0]);
  for (uint32_t i = 1; i < points.Length(); ++i) {
    aBuilder->LineTo(points[i]);
  }

  return aBuilder->Finish();
}

} 
} 
