






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






#define CSS_TURN 30U


#define CSS_NUMBER_INT32 31U
#define CSS_NUMBER_UINT32 32U





class nsROCSSPrimitiveValue final : public mozilla::dom::CSSValue,
                                    public nsIDOMCSSPrimitiveValue
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsROCSSPrimitiveValue, mozilla::dom::CSSValue)

  
  NS_DECL_NSIDOMCSSPRIMITIVEVALUE

  
  NS_DECL_NSIDOMCSSVALUE

  
  virtual void GetCssText(nsString& aText, mozilla::ErrorResult& aRv) override final;
  virtual void SetCssText(const nsAString& aText, mozilla::ErrorResult& aRv) override final;
  virtual uint16_t CssValueType() const override final;

  
  uint16_t PrimitiveType()
  {
    
    
    if (mType > CSS_RGBCOLOR) {
      if (mType == CSS_NUMBER_INT32 || mType == CSS_NUMBER_UINT32) {
        return CSS_NUMBER;
      }
      return CSS_UNKNOWN;
    }
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

  void SetNumber(float aValue);
  void SetNumber(int32_t aValue);
  void SetNumber(uint32_t aValue);
  void SetPercent(float aValue);
  void SetDegree(float aValue);
  void SetGrad(float aValue);
  void SetRadian(float aValue);
  void SetTurn(float aValue);
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

  virtual JSObject *WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

private:
  ~nsROCSSPrimitiveValue();

  uint16_t mType;

  union {
    nscoord         mAppUnits;
    float           mFloat;
    int32_t         mInt32;
    uint32_t        mUint32;
    
    nsDOMCSSRGBColor* MOZ_OWNING_REF mColor;
    nsDOMCSSRect* MOZ_OWNING_REF mRect;
    char16_t*      mString;
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

