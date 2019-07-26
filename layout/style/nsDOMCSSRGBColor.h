






#ifndef nsDOMCSSRGBColor_h__
#define nsDOMCSSRGBColor_h__

#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "nsIDOMNSRGBAColor.h"
#include "nsWrapperCache.h"

class nsROCSSPrimitiveValue;

class nsDOMCSSRGBColor : public nsIDOMNSRGBAColor,
                         public nsWrapperCache
{
public:
  nsDOMCSSRGBColor(nsROCSSPrimitiveValue* aRed,
                   nsROCSSPrimitiveValue* aGreen,
                   nsROCSSPrimitiveValue* aBlue,
                   nsROCSSPrimitiveValue* aAlpha,
                   bool aHasAlpha);

  virtual ~nsDOMCSSRGBColor(void);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMRGBCOLOR
  NS_DECL_NSIDOMNSRGBACOLOR

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMCSSRGBColor)

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

private:
  nsRefPtr<nsROCSSPrimitiveValue> mRed;
  nsRefPtr<nsROCSSPrimitiveValue> mGreen;
  nsRefPtr<nsROCSSPrimitiveValue> mBlue;
  nsRefPtr<nsROCSSPrimitiveValue> mAlpha;
  bool mHasAlpha;
};

#endif 
