




#include "mozilla/dom/SVGAnimatedLength.h"
#include "mozilla/dom/SVGAnimatedLengthBinding.h"
#include "nsSVGLength2.h"
#include "DOMSVGLength.h"

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedLength, mSVGElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGAnimatedLength, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGAnimatedLength, Release)

JSObject*
SVGAnimatedLength::WrapObject(JSContext* aCx)
{
  return SVGAnimatedLengthBinding::Wrap(aCx, this);
}

already_AddRefed<DOMSVGLength>
SVGAnimatedLength::BaseVal()
{
  nsRefPtr<DOMSVGLength> angle;
  mVal->ToDOMBaseVal(getter_AddRefs(angle), mSVGElement);
  return angle.forget();
}

already_AddRefed<DOMSVGLength>
SVGAnimatedLength::AnimVal()
{
  nsRefPtr<DOMSVGLength> angle;
  mVal->ToDOMAnimVal(getter_AddRefs(angle), mSVGElement);
  return angle.forget();
}

} 
} 
