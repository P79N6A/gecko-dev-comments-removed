





#include "mozilla/dom/SVGAnimatedEnumeration.h"

#include "mozilla/dom/SVGAnimatedEnumerationBinding.h"

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedEnumeration,
                                               mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedEnumeration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedEnumeration)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimatedEnumeration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
SVGAnimatedEnumeration::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return SVGAnimatedEnumerationBinding::Wrap(aCx, aScope, this);
}

} 
} 
