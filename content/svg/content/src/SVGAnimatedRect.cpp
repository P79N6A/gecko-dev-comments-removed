




#include "SVGAnimatedRect.h"
#include "mozilla/dom/SVGAnimatedRectBinding.h"
#include "nsSVGElement.h"
#include "nsSVGViewBox.h"
#include "SVGIRect.h"

namespace mozilla {
namespace dom {

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedRect, mSVGElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGAnimatedRect, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGAnimatedRect, Release)

SVGAnimatedRect::SVGAnimatedRect(nsSVGViewBox* aVal, nsSVGElement* aSVGElement)
  : mVal(aVal)
  , mSVGElement(aSVGElement)
{
  SetIsDOMBinding();
}

SVGAnimatedRect::~SVGAnimatedRect()
{
  nsSVGViewBox::sSVGAnimatedRectTearoffTable.RemoveTearoff(mVal);
}

already_AddRefed<SVGIRect>
SVGAnimatedRect::GetBaseVal()
{
  return mVal->ToDOMBaseVal(mSVGElement);
}

already_AddRefed<SVGIRect>
SVGAnimatedRect::GetAnimVal()
{
  return mVal->ToDOMAnimVal(mSVGElement);
}

JSObject*
SVGAnimatedRect::WrapObject(JSContext* aCx)
{
  return SVGAnimatedRectBinding::Wrap(aCx, this);
}

} 
} 
