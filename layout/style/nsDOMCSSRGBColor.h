







































#ifndef nsDOMCSSRGBColor_h__
#define nsDOMCSSRGBColor_h__

#include "nsISupports.h"
#include "nsIDOMNSRGBAColor.h"
#include "nsCOMPtr.h"

class nsIDOMCSSPrimitiveValue;

class nsDOMCSSRGBColor : public nsIDOMNSRGBAColor {
public:
  nsDOMCSSRGBColor(nsIDOMCSSPrimitiveValue* aRed,
                   nsIDOMCSSPrimitiveValue* aGreen,
                   nsIDOMCSSPrimitiveValue* aBlue,
                   nsIDOMCSSPrimitiveValue* aAlpha,
                   PRBool aHasAlpha);

  virtual ~nsDOMCSSRGBColor(void);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMRGBCOLOR
  NS_DECL_NSIDOMNSRGBACOLOR

  PRBool HasAlpha() const { return mHasAlpha; }

private:
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mRed;
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mGreen;
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mBlue;
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mAlpha;
  PRBool mHasAlpha;
};

#endif 
