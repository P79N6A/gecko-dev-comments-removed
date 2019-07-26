




#include "mozilla/dom/SVGAnimatedLength.h"
#include "mozilla/dom/SVGAnimatedLengthBinding.h"
#include "nsContentUtils.h"
#include "nsSVGLength2.h"

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedLength, mSVGElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGAnimatedLength, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGAnimatedLength, Release)

JSObject*
SVGAnimatedLength::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return SVGAnimatedLengthBinding::Wrap(aCx, aScope, this);
}

already_AddRefed<nsIDOMSVGLength>
SVGAnimatedLength::BaseVal()
{
  nsRefPtr<nsIDOMSVGLength> angle;
  mVal->ToDOMBaseVal(getter_AddRefs(angle), mSVGElement);
  return angle.forget();
}

already_AddRefed<nsIDOMSVGLength>
SVGAnimatedLength::AnimVal()
{
  nsRefPtr<nsIDOMSVGLength> angle;
  mVal->ToDOMAnimVal(getter_AddRefs(angle), mSVGElement);
  return angle.forget();
}

} 
} 
