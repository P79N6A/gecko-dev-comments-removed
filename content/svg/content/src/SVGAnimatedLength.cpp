




#include "mozilla/dom/SVGAnimatedLength.h"
#include "mozilla/dom/SVGAnimatedLengthBinding.h"
#include "nsContentUtils.h"

DOMCI_DATA(SVGAnimatedLength, mozilla::dom::SVGAnimatedLength)

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedLength, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedLength)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedLength)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimatedLength)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedLength)
NS_INTERFACE_MAP_END

JSObject*
SVGAnimatedLength::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return SVGAnimatedLengthBinding::Wrap(aCx, aScope, this, aTriedToWrap);
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
