




#include "SVGAnimatedAngle.h"
#include "nsSVGAngle.h"
#include "mozilla/dom/SVGAnimatedAngleBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedAngle, mSVGElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGAnimatedAngle, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGAnimatedAngle, Release)

JSObject*
SVGAnimatedAngle::WrapObject(JSContext* aCx)
{
  return SVGAnimatedAngleBinding::Wrap(aCx, this);
}

already_AddRefed<SVGAngle>
SVGAnimatedAngle::BaseVal()
{
  return mVal->ToDOMBaseVal(mSVGElement);
}

already_AddRefed<SVGAngle>
SVGAnimatedAngle::AnimVal()
{
  return mVal->ToDOMAnimVal(mSVGElement);
}

