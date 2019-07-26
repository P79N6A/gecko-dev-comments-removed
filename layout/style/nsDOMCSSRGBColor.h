






#ifndef nsDOMCSSRGBColor_h__
#define nsDOMCSSRGBColor_h__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsWrapperCache.h"

class nsROCSSPrimitiveValue;

class nsDOMCSSRGBColor : public nsWrapperCache
{
public:
  nsDOMCSSRGBColor(nsROCSSPrimitiveValue* aRed,
                   nsROCSSPrimitiveValue* aGreen,
                   nsROCSSPrimitiveValue* aBlue,
                   nsROCSSPrimitiveValue* aAlpha,
                   bool aHasAlpha);

  virtual ~nsDOMCSSRGBColor(void);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsDOMCSSRGBColor)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsDOMCSSRGBColor)

  bool HasAlpha() const { return mHasAlpha; }

  
  nsROCSSPrimitiveValue* Red() const
  {
    return mRed;
  }
  nsROCSSPrimitiveValue* Green() const
  {
    return mGreen;
  }
  nsROCSSPrimitiveValue* Blue() const
  {
    return mBlue;
  }
  nsROCSSPrimitiveValue* Alpha() const
  {
    return mAlpha;
  }

  nsISupports* GetParentObject() const
  {
    return nullptr;
  }

  virtual JSObject *WrapObject(JSContext *cx, JSObject *aScope)
    MOZ_OVERRIDE MOZ_FINAL;

private:
  nsRefPtr<nsROCSSPrimitiveValue> mRed;
  nsRefPtr<nsROCSSPrimitiveValue> mGreen;
  nsRefPtr<nsROCSSPrimitiveValue> mBlue;
  nsRefPtr<nsROCSSPrimitiveValue> mAlpha;
  bool mHasAlpha;
};

#endif 
