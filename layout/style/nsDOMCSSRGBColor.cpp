 





#include "nsDOMCSSRGBColor.h"

#include "nsContentUtils.h"
#include "mozilla/dom/RGBColorBinding.h"
#include "nsROCSSPrimitiveValue.h"

using namespace mozilla;

nsDOMCSSRGBColor::nsDOMCSSRGBColor(nsROCSSPrimitiveValue* aRed,
                                   nsROCSSPrimitiveValue* aGreen,
                                   nsROCSSPrimitiveValue* aBlue,
                                   nsROCSSPrimitiveValue* aAlpha,
                                   bool aHasAlpha)
  : mRed(aRed), mGreen(aGreen), mBlue(aBlue), mAlpha(aAlpha)
  , mHasAlpha(aHasAlpha)
{
  SetIsDOMBinding();
}

nsDOMCSSRGBColor::~nsDOMCSSRGBColor(void)
{
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(nsDOMCSSRGBColor, mAlpha,  mBlue, mGreen, mRed)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsDOMCSSRGBColor, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsDOMCSSRGBColor, Release)

JSObject*
nsDOMCSSRGBColor::WrapObject(JSContext *aCx, JSObject *aScope, bool *aTried)
{
  return dom::RGBColorBinding::Wrap(aCx, aScope, this, aTried);
}

