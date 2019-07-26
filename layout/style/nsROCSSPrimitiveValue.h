






#ifndef nsROCSSPrimitiveValue_h___
#define nsROCSSPrimitiveValue_h___

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsCSSKeywords.h"
#include "CSSValue.h"
#include "nsCOMPtr.h"
#include "nsCoord.h"

class nsIURI;
class nsDOMCSSRect;
class nsDOMCSSRGBColor;





class nsROCSSPrimitiveValue MOZ_FINAL : public mozilla::dom::CSSValue,
  public nsIDOMCSSPrimitiveValue
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsROCSSPrimitiveValue, mozilla::dom::CSSValue)

  
  NS_DECL_NSIDOMCSSPRIMITIVEVALUE

  
  NS_DECL_NSIDOMCSSVALUE

  
  virtual void GetCssText(nsString& aText, mozilla::ErrorResult& aRv) MOZ_OVERRIDE MOZ_FINAL;
  virtual void SetCssText(const nsAString& aText, mozilla::ErrorResult& aRv) MOZ_OVERRIDE MOZ_FINAL;
  virtual uint16_t CssValueType() const MOZ_OVERRIDE MOZ_FINAL;

  
  uint16_t PrimitiveType()
  {
    return mType;
  }
  void SetFloatValue(uint16_t aUnitType, float aValue,
                     mozilla::ErrorResult& aRv);
  float GetFloatValue(uint16_t aUnitType, mozilla::ErrorResult& aRv);
  void GetStringValue(nsString& aString, mozilla::ErrorResult& aRv);
  void SetStringValue(uint16_t aUnitType, const nsAString& aString,
                      mozilla::ErrorResult& aRv);
  already_AddRefed<nsIDOMCounter> GetCounterValue(mozilla::ErrorResult& aRv);
  nsDOMCSSRect* GetRectValue(mozilla::ErrorResult& aRv);
  nsDOMCSSRGBColor *GetRGBColorValue(mozilla::ErrorResult& aRv);

  
  nsROCSSPrimitiveValue();
  ~nsROCSSPrimitiveValue();

  void SetNumber(float aValue);
  void SetNumber(int32_t aValue);
  void SetNumber(uint32_t aValue);
  void SetPercent(float aValue);
  void SetAppUnits(nscoord aValue);
  void SetAppUnits(float aValue);
  void SetIdent(nsCSSKeyword aKeyword);
  
  void SetString(const nsACString& aString, uint16_t aType = CSS_STRING);
  
  void SetString(const nsAString& aString, uint16_t aType = CSS_STRING);
  void SetURI(nsIURI *aURI);
  void SetColor(nsDOMCSSRGBColor* aColor);
  void SetRect(nsDOMCSSRect* aRect);
  void SetTime(float aValue);
  void Reset();

  nsISupports* GetParentObject() const
  {
    return nullptr;
  }

  virtual JSObject *WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

private:
  uint16_t mType;

  union {
    nscoord         mAppUnits;
    float           mFloat;
    nsDOMCSSRGBColor* mColor;
    nsDOMCSSRect*     mRect;
    PRUnichar*      mString;
    nsIURI*         mURI;
    nsCSSKeyword    mKeyword;
  } mValue;
};

inline nsROCSSPrimitiveValue *mozilla::dom::CSSValue::AsPrimitiveValue()
{
  return CssValueType() == nsIDOMCSSValue::CSS_PRIMITIVE_VALUE ?
    static_cast<nsROCSSPrimitiveValue*>(this) : nullptr;
}

#endif 

