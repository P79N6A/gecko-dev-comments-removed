






































#ifndef nsROCSSPrimitiveValue_h___
#define nsROCSSPrimitiveValue_h___

#include "nsIDOMCSSPrimitiveValue.h"
#include "nsString.h"
#include "nsCoord.h"
#include "nsReadableUtils.h"
#include "nsIURI.h"
#include "nsIAtom.h"
#include "nsCSSKeywords.h"

#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsDOMCSSRect.h"
#include "nsDOMCSSRGBColor.h"

class nsROCSSPrimitiveValue : public nsIDOMCSSPrimitiveValue
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMCSSPRIMITIVEVALUE

  
  NS_DECL_NSIDOMCSSVALUE

  
  nsROCSSPrimitiveValue(PRInt32 aAppUnitsPerInch);
  virtual ~nsROCSSPrimitiveValue();

  

  void SetNumber(float aValue)
  {
    Reset();
    mValue.mFloat = aValue;
    mType = CSS_NUMBER;
  }

  void SetNumber(PRInt32 aValue)
  {
    Reset();
    mValue.mFloat = float(aValue);
    mType = CSS_NUMBER;
  }

  void SetNumber(PRUint32 aValue)
  {
    Reset();
    mValue.mFloat = float(aValue);
    mType = CSS_NUMBER;
  }

  void SetPercent(float aValue)
  {
    Reset();
    mValue.mFloat = aValue;
    mType = CSS_PERCENTAGE;
  }

  void SetAppUnits(nscoord aValue)
  {
    Reset();
    mValue.mAppUnits = aValue;
    mType = CSS_PX;
  }

  void SetAppUnits(float aValue)
  {
    SetAppUnits(NSToCoordRound(aValue));
  }

  void SetIdent(nsCSSKeyword aKeyword)
  {
    NS_PRECONDITION(aKeyword != eCSSKeyword_UNKNOWN &&
                    0 <= aKeyword && aKeyword < eCSSKeyword_COUNT,
                    "bad keyword");
    Reset();
    mValue.mKeyword = aKeyword;
    mType = CSS_IDENT;
  }

  
  void SetString(const nsACString& aString, PRUint16 aType = CSS_STRING)
  {
    Reset();
    mValue.mString = ToNewUnicode(aString);
    if (mValue.mString) {
      mType = aType;
    } else {
      
      mType = CSS_UNKNOWN;
    }
  }

  
  void SetString(const nsAString& aString, PRUint16 aType = CSS_STRING)
  {
    Reset();
    mValue.mString = ToNewUnicode(aString);
    if (mValue.mString) {
      mType = aType;
    } else {
      
      mType = CSS_UNKNOWN;
    }
  }

  void SetURI(nsIURI *aURI)
  {
    Reset();
    mValue.mURI = aURI;
    NS_IF_ADDREF(mValue.mURI);
    mType = CSS_URI;
  }

  void SetColor(nsDOMCSSRGBColor* aColor)
  {
    NS_PRECONDITION(aColor, "Null RGBColor being set!");
    Reset();
    mValue.mColor = aColor;
    if (mValue.mColor) {
      NS_ADDREF(mValue.mColor);
      mType = CSS_RGBCOLOR;
    }
    else {
      mType = CSS_UNKNOWN;
    }
  }

  void SetRect(nsIDOMRect* aRect)
  {
    NS_PRECONDITION(aRect, "Null rect being set!");
    Reset();
    mValue.mRect = aRect;
    if (mValue.mRect) {
      NS_ADDREF(mValue.mRect);
      mType = CSS_RECT;
    }
    else {
      mType = CSS_UNKNOWN;
    }
  }

  void SetTime(float aValue)
  {
    Reset();
    mValue.mFloat = aValue;
    mType = CSS_S;
  }

  void Reset(void)
  {
    switch (mType) {
      case CSS_IDENT:
        break;
      case CSS_STRING:
      case CSS_ATTR:
      case CSS_COUNTER: 
        NS_ASSERTION(mValue.mString, "Null string should never happen");
        nsMemory::Free(mValue.mString);
        mValue.mString = nsnull;
        break;
      case CSS_URI:
        NS_IF_RELEASE(mValue.mURI);
        break;
      case CSS_RECT:
        NS_ASSERTION(mValue.mRect, "Null Rect should never happen");
        NS_RELEASE(mValue.mRect);
        break;
      case CSS_RGBCOLOR:
        NS_ASSERTION(mValue.mColor, "Null RGBColor should never happen");
        NS_RELEASE(mValue.mColor);
        break;
    }
  }

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
  
  PRInt32 mAppUnitsPerInch;
};

#endif 

