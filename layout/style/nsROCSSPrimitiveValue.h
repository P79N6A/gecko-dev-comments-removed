






































#ifndef nsROCSSPrimitiveValue_h___
#define nsROCSSPrimitiveValue_h___

#include "nsIDOMCSSPrimitiveValue.h"
#include "nsCoord.h"
#include "nsCSSKeywords.h"

class nsIURI;
class nsDOMCSSRGBColor;

class nsROCSSPrimitiveValue : public nsIDOMCSSPrimitiveValue
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMCSSPRIMITIVEVALUE

  
  NS_DECL_NSIDOMCSSVALUE

  
  nsROCSSPrimitiveValue();
  virtual ~nsROCSSPrimitiveValue();

  void SetNumber(float aValue);
  void SetNumber(PRInt32 aValue);
  void SetNumber(PRUint32 aValue);
  void SetPercent(float aValue);
  void SetAppUnits(nscoord aValue);
  void SetAppUnits(float aValue);
  void SetIdent(nsCSSKeyword aKeyword);
  
  void SetString(const nsACString& aString, PRUint16 aType = CSS_STRING);
  
  void SetString(const nsAString& aString, PRUint16 aType = CSS_STRING);
  void SetURI(nsIURI *aURI);
  void SetColor(nsDOMCSSRGBColor* aColor);
  void SetRect(nsIDOMRect* aRect);
  void SetTime(float aValue);
  void Reset();

private:
  PRUint16 mType;

  union {
    nscoord         mAppUnits;
    float           mFloat;
    nsDOMCSSRGBColor* mColor;
    nsIDOMRect*     mRect;
    PRUnichar*      mString;
    nsIURI*         mURI;
    nsCSSKeyword    mKeyword;
  } mValue;
};

#endif 

