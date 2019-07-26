




#include "SVGAnimatedBoolean.h"
#include "mozilla/dom/SVGAnimatedBooleanBinding.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedBoolean, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedBoolean)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedBoolean)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimatedBoolean)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
SVGAnimatedBoolean::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return SVGAnimatedBooleanBinding::Wrap(aCx, aScope, this);
}

