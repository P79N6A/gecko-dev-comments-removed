





#include "mozilla/dom/SVGAnimatedNumber.h"

#include "mozilla/dom/SVGAnimatedNumberBinding.h"

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedNumber,
                                               mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedNumber)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedNumber)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimatedNumber)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
SVGAnimatedNumber::WrapObject(JSContext* aCx)
{
  return SVGAnimatedNumberBinding::Wrap(aCx, this);
}

} 
} 
