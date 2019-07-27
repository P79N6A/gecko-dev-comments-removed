





#include "mozilla/dom/SVGAnimatedInteger.h"

#include "mozilla/dom/SVGAnimatedIntegerBinding.h"

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedInteger,
                                               mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedInteger)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedInteger)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimatedInteger)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
SVGAnimatedInteger::WrapObject(JSContext* aCx)
{
  return SVGAnimatedIntegerBinding::Wrap(aCx, this);
}

} 
} 
