




#include "SVGAnimatedBoolean.h"
#include "mozilla/dom/SVGAnimatedBooleanBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedBoolean, mSVGElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGAnimatedBoolean, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGAnimatedBoolean, Release)

JSObject*
SVGAnimatedBoolean::WrapObject(JSContext* aCx)
{
  return SVGAnimatedBooleanBinding::Wrap(aCx, this);
}

