 





#include "nsDOMCSSRGBColor.h"

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
}

nsDOMCSSRGBColor::~nsDOMCSSRGBColor(void)
{
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(nsDOMCSSRGBColor, mAlpha,  mBlue, mGreen, mRed)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsDOMCSSRGBColor, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsDOMCSSRGBColor, Release)

JSObject*
nsDOMCSSRGBColor::WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return dom::RGBColorBinding::Wrap(aCx, this, aGivenProto);
}

