






#ifndef mozilla_StyleAnimationValue_h_
#define mozilla_StyleAnimationValue_h_

#include "nsStringFwd.h"
#include "nsStringBuffer.h"
#include "nsCSSProperty.h"
#include "nsCoord.h"
#include "nsColor.h"
#include "nsCSSValue.h"

class nsStyleContext;
class gfx3DMatrix;

namespace mozilla {

namespace dom {
class Element;
} 




class StyleAnimationValue {
public:
  
  
  











  static bool Add(nsCSSProperty aProperty, StyleAnimationValue& aDest,
                  const StyleAnimationValue& aValueToAdd, uint32_t aCount) {
    return AddWeighted(aProperty, 1.0, aDest, aCount, aValueToAdd, aDest);
  }

  

















  static bool ComputeDistance(nsCSSProperty aProperty,
                              const StyleAnimationValue& aStartValue,
                              const StyleAnimationValue& aEndValue,
                              double& aDistance);

  















  static bool Interpolate(nsCSSProperty aProperty,
                          const StyleAnimationValue& aStartValue,
                          const StyleAnimationValue& aEndValue,
                          double aPortion,
                          StyleAnimationValue& aResultValue) {
    return AddWeighted(aProperty, 1.0 - aPortion, aStartValue,
                       aPortion, aEndValue, aResultValue);
  }

  













  static bool AddWeighted(nsCSSProperty aProperty,
                          double aCoeff1, const StyleAnimationValue& aValue1,
                          double aCoeff2, const StyleAnimationValue& aValue2,
                          StyleAnimationValue& aResultValue);

  
  
  























  static bool ComputeValue(nsCSSProperty aProperty,
                             mozilla::dom::Element* aTargetElement,
                             const nsAString& aSpecifiedValue,
                             bool aUseSVGMode,
                             StyleAnimationValue& aComputedValue,
                             bool* aIsContextSensitive = nullptr);

  












  static bool UncomputeValue(nsCSSProperty aProperty,
                             const StyleAnimationValue& aComputedValue,
                             nsCSSValue& aSpecifiedValue);
  static bool UncomputeValue(nsCSSProperty aProperty,
                             const StyleAnimationValue& aComputedValue,
                             nsAString& aSpecifiedValue);

  








  static bool ExtractComputedValue(nsCSSProperty aProperty,
                                   nsStyleContext* aStyleContext,
                                   StyleAnimationValue& aComputedValue);

  






  static gfx3DMatrix InterpolateTransformMatrix(const gfx3DMatrix &aMatrix1,
                                                const gfx3DMatrix &aMatrix2,
                                                double aProgress);

  static already_AddRefed<nsCSSValue::Array>
    AppendTransformFunction(nsCSSKeyword aTransformFunction,
                            nsCSSValueList**& aListTail);

  


  enum Unit {
    eUnit_Null, 
    eUnit_Normal,
    eUnit_Auto,
    eUnit_None,
    eUnit_Enumerated,
    eUnit_Visibility, 
                      
    eUnit_Integer,
    eUnit_Coord,
    eUnit_Percent,
    eUnit_Float,
    eUnit_Color,
    eUnit_Calc, 
                
    eUnit_ObjectPosition, 
                          
    eUnit_CSSValuePair, 
    eUnit_CSSValueTriplet, 
    eUnit_CSSRect, 
    eUnit_Dasharray, 
    eUnit_Filter, 
    eUnit_Shadow, 
    eUnit_Transform, 
    eUnit_BackgroundPosition, 
    eUnit_CSSValuePairList, 
    eUnit_UnparsedString 
  };

private:
  Unit mUnit;
  union {
    int32_t mInt;
    nscoord mCoord;
    float mFloat;
    nscolor mColor;
    nsCSSValue* mCSSValue;
    nsCSSValuePair* mCSSValuePair;
    nsCSSValueTriplet* mCSSValueTriplet;
    nsCSSRect* mCSSRect;
    nsCSSValueList* mCSSValueList;
    nsCSSValueSharedList* mCSSValueSharedList;
    nsCSSValuePairList* mCSSValuePairList;
    nsStringBuffer* mString;
  } mValue;

public:
  Unit GetUnit() const {
    NS_ASSERTION(mUnit != eUnit_Null, "uninitialized");
    return mUnit;
  }

  
  
  bool IsNull() const {
    return mUnit == eUnit_Null;
  }

  int32_t GetIntValue() const {
    NS_ASSERTION(IsIntUnit(mUnit), "unit mismatch");
    return mValue.mInt;
  }
  nscoord GetCoordValue() const {
    NS_ASSERTION(mUnit == eUnit_Coord, "unit mismatch");
    return mValue.mCoord;
  }
  float GetPercentValue() const {
    NS_ASSERTION(mUnit == eUnit_Percent, "unit mismatch");
    return mValue.mFloat;
  }
  float GetFloatValue() const {
    NS_ASSERTION(mUnit == eUnit_Float, "unit mismatch");
    return mValue.mFloat;
  }
  nscolor GetColorValue() const {
    NS_ASSERTION(mUnit == eUnit_Color, "unit mismatch");
    return mValue.mColor;
  }
  nsCSSValue* GetCSSValueValue() const {
    NS_ASSERTION(IsCSSValueUnit(mUnit), "unit mismatch");
    return mValue.mCSSValue;
  }
  nsCSSValuePair* GetCSSValuePairValue() const {
    NS_ASSERTION(IsCSSValuePairUnit(mUnit), "unit mismatch");
    return mValue.mCSSValuePair;
  }
  nsCSSValueTriplet* GetCSSValueTripletValue() const {
    NS_ASSERTION(IsCSSValueTripletUnit(mUnit), "unit mismatch");
    return mValue.mCSSValueTriplet;
  }
  nsCSSRect* GetCSSRectValue() const {
    NS_ASSERTION(IsCSSRectUnit(mUnit), "unit mismatch");
    return mValue.mCSSRect;
  }
  nsCSSValueList* GetCSSValueListValue() const {
    NS_ASSERTION(IsCSSValueListUnit(mUnit), "unit mismatch");
    return mValue.mCSSValueList;
  }
  nsCSSValueSharedList* GetCSSValueSharedListValue() const {
    NS_ASSERTION(IsCSSValueSharedListValue(mUnit), "unit mismatch");
    return mValue.mCSSValueSharedList;
  }
  nsCSSValuePairList* GetCSSValuePairListValue() const {
    NS_ASSERTION(IsCSSValuePairListUnit(mUnit), "unit mismatch");
    return mValue.mCSSValuePairList;
  }
  const char16_t* GetStringBufferValue() const {
    NS_ASSERTION(IsStringUnit(mUnit), "unit mismatch");
    return GetBufferValue(mValue.mString);
  }

  void GetStringValue(nsAString& aBuffer) const {
    NS_ASSERTION(IsStringUnit(mUnit), "unit mismatch");
    aBuffer.Truncate();
    uint32_t len = NS_strlen(GetBufferValue(mValue.mString));
    mValue.mString->ToString(len, aBuffer);
  }

  explicit StyleAnimationValue(Unit aUnit = eUnit_Null) : mUnit(aUnit) {
    NS_ASSERTION(aUnit == eUnit_Null || aUnit == eUnit_Normal ||
                 aUnit == eUnit_Auto || aUnit == eUnit_None,
                 "must be valueless unit");
  }
  StyleAnimationValue(const StyleAnimationValue& aOther)
    : mUnit(eUnit_Null) { *this = aOther; }
  enum IntegerConstructorType { IntegerConstructor };
  StyleAnimationValue(int32_t aInt, Unit aUnit, IntegerConstructorType);
  enum CoordConstructorType { CoordConstructor };
  StyleAnimationValue(nscoord aLength, CoordConstructorType);
  enum PercentConstructorType { PercentConstructor };
  StyleAnimationValue(float aPercent, PercentConstructorType);
  enum FloatConstructorType { FloatConstructor };
  StyleAnimationValue(float aFloat, FloatConstructorType);
  enum ColorConstructorType { ColorConstructor };
  StyleAnimationValue(nscolor aColor, ColorConstructorType);

  ~StyleAnimationValue() { FreeValue(); }

  void SetNormalValue();
  void SetAutoValue();
  void SetNoneValue();
  void SetIntValue(int32_t aInt, Unit aUnit);
  void SetCoordValue(nscoord aCoord);
  void SetPercentValue(float aPercent);
  void SetFloatValue(float aFloat);
  void SetColorValue(nscolor aColor);
  void SetUnparsedStringValue(const nsString& aString);

  
  
  void SetAndAdoptCSSValueValue(nsCSSValue *aValue, Unit aUnit);
  void SetAndAdoptCSSValuePairValue(nsCSSValuePair *aValue, Unit aUnit);
  void SetAndAdoptCSSValueTripletValue(nsCSSValueTriplet *aValue, Unit aUnit);
  void SetAndAdoptCSSRectValue(nsCSSRect *aValue, Unit aUnit);
  void SetAndAdoptCSSValueListValue(nsCSSValueList *aValue, Unit aUnit);
  void SetAndAdoptCSSValuePairListValue(nsCSSValuePairList *aValue);

  void SetTransformValue(nsCSSValueSharedList* aList);

  StyleAnimationValue& operator=(const StyleAnimationValue& aOther);

  bool operator==(const StyleAnimationValue& aOther) const;
  bool operator!=(const StyleAnimationValue& aOther) const
    { return !(*this == aOther); }

private:
  void FreeValue();

  static const char16_t* GetBufferValue(nsStringBuffer* aBuffer) {
    return static_cast<char16_t*>(aBuffer->Data());
  }

  static bool IsIntUnit(Unit aUnit) {
    return aUnit == eUnit_Enumerated || aUnit == eUnit_Visibility ||
           aUnit == eUnit_Integer;
  }
  static bool IsCSSValueUnit(Unit aUnit) {
    return aUnit == eUnit_Calc ||
           aUnit == eUnit_ObjectPosition;
  }
  static bool IsCSSValuePairUnit(Unit aUnit) {
    return aUnit == eUnit_CSSValuePair;
  }
  static bool IsCSSValueTripletUnit(Unit aUnit) {
    return aUnit == eUnit_CSSValueTriplet;
  }
  static bool IsCSSRectUnit(Unit aUnit) {
    return aUnit == eUnit_CSSRect;
  }
  static bool IsCSSValueListUnit(Unit aUnit) {
    return aUnit == eUnit_Dasharray || aUnit == eUnit_Filter ||
           aUnit == eUnit_Shadow ||
           aUnit == eUnit_BackgroundPosition;
  }
  static bool IsCSSValueSharedListValue(Unit aUnit) {
    return aUnit == eUnit_Transform;
  }
  static bool IsCSSValuePairListUnit(Unit aUnit) {
    return aUnit == eUnit_CSSValuePairList;
  }
  static bool IsStringUnit(Unit aUnit) {
    return aUnit == eUnit_UnparsedString;
  }
};

} 

#endif
