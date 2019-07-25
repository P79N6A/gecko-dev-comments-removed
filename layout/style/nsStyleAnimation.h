








































#ifndef nsStyleAnimation_h_
#define nsStyleAnimation_h_

#include "prtypes.h"
#include "nsAString.h"
#include "nsCRTGlue.h"
#include "nsStringBuffer.h"
#include "nsCSSProperty.h"
#include "nsCoord.h"
#include "nsColor.h"

class nsPresContext;
class nsStyleContext;
class nsCSSValue;
struct nsCSSValueList;
struct nsCSSValuePair;
struct nsCSSValueTriplet;
struct nsCSSValuePairList;
struct nsCSSRect;
class gfx3DMatrix;

namespace mozilla {
namespace dom {
class Element;
} 
} 




class nsStyleAnimation {
public:
  class Value;

  
  
  











  static bool Add(nsCSSProperty aProperty, Value& aDest,
                    const Value& aValueToAdd, PRUint32 aCount) {
    return AddWeighted(aProperty, 1.0, aDest, aCount, aValueToAdd, aDest);
  }

  

















  static bool ComputeDistance(nsCSSProperty aProperty,
                                const Value& aStartValue,
                                const Value& aEndValue,
                                double& aDistance);

  















  static bool Interpolate(nsCSSProperty aProperty,
                            const Value& aStartValue,
                            const Value& aEndValue,
                            double aPortion,
                            Value& aResultValue) {
    return AddWeighted(aProperty, 1.0 - aPortion, aStartValue,
                       aPortion, aEndValue, aResultValue);
  }

  













  static bool AddWeighted(nsCSSProperty aProperty,
                            double aCoeff1, const Value& aValue1,
                            double aCoeff2, const Value& aValue2,
                            Value& aResultValue);

  
  
  























  static bool ComputeValue(nsCSSProperty aProperty,
                             mozilla::dom::Element* aTargetElement,
                             const nsAString& aSpecifiedValue,
                             bool aUseSVGMode,
                             Value& aComputedValue,
                             bool* aIsContextSensitive = nsnull);

  














  static bool UncomputeValue(nsCSSProperty aProperty,
                               nsPresContext* aPresContext,
                               const Value& aComputedValue,
                               nsCSSValue& aSpecifiedValue);
  static bool UncomputeValue(nsCSSProperty aProperty,
                               nsPresContext* aPresContext,
                               const Value& aComputedValue,
                               nsAString& aSpecifiedValue);

  








  static bool ExtractComputedValue(nsCSSProperty aProperty,
                                     nsStyleContext* aStyleContext,
                                     Value& aComputedValue);

   






   static gfx3DMatrix InterpolateTransformMatrix(const gfx3DMatrix &aMatrix1,
                                                 const gfx3DMatrix &aMatrix2, 
                                                 double aProgress);

  


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
                
    eUnit_CSSValuePair, 
    eUnit_CSSValueTriplet, 
    eUnit_CSSRect, 
    eUnit_Dasharray, 
    eUnit_Shadow, 
    eUnit_Transform, 
    eUnit_CSSValuePairList, 
    eUnit_UnparsedString 
  };

  class Value {
  private:
    Unit mUnit;
    union {
      PRInt32 mInt;
      nscoord mCoord;
      float mFloat;
      nscolor mColor;
      nsCSSValue* mCSSValue;
      nsCSSValuePair* mCSSValuePair;
      nsCSSValueTriplet* mCSSValueTriplet;
      nsCSSRect* mCSSRect;
      nsCSSValueList* mCSSValueList;
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

    PRInt32 GetIntValue() const {
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
    nsCSSValuePairList* GetCSSValuePairListValue() const {
      NS_ASSERTION(IsCSSValuePairListUnit(mUnit), "unit mismatch");
      return mValue.mCSSValuePairList;
    }
    const PRUnichar* GetStringBufferValue() const {
      NS_ASSERTION(IsStringUnit(mUnit), "unit mismatch");
      return GetBufferValue(mValue.mString);
    }

    void GetStringValue(nsAString& aBuffer) const {
      NS_ASSERTION(IsStringUnit(mUnit), "unit mismatch");
      aBuffer.Truncate();
      PRUint32 len = NS_strlen(GetBufferValue(mValue.mString));
      mValue.mString->ToString(len, aBuffer);
    }

    explicit Value(Unit aUnit = eUnit_Null) : mUnit(aUnit) {
      NS_ASSERTION(aUnit == eUnit_Null || aUnit == eUnit_Normal ||
                   aUnit == eUnit_Auto || aUnit == eUnit_None,
                   "must be valueless unit");
    }
    Value(const Value& aOther) : mUnit(eUnit_Null) { *this = aOther; }
    enum IntegerConstructorType { IntegerConstructor };
    Value(PRInt32 aInt, Unit aUnit, IntegerConstructorType);
    enum CoordConstructorType { CoordConstructor };
    Value(nscoord aLength, CoordConstructorType);
    enum PercentConstructorType { PercentConstructor };
    Value(float aPercent, PercentConstructorType);
    enum FloatConstructorType { FloatConstructor };
    Value(float aFloat, FloatConstructorType);
    enum ColorConstructorType { ColorConstructor };
    Value(nscolor aColor, ColorConstructorType);

    ~Value() { FreeValue(); }

    void SetNormalValue();
    void SetAutoValue();
    void SetNoneValue();
    void SetIntValue(PRInt32 aInt, Unit aUnit);
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

    Value& operator=(const Value& aOther);

    bool operator==(const Value& aOther) const;
    bool operator!=(const Value& aOther) const
      { return !(*this == aOther); }

  private:
    void FreeValue();

    static const PRUnichar* GetBufferValue(nsStringBuffer* aBuffer) {
      return static_cast<PRUnichar*>(aBuffer->Data());
    }

    static bool IsIntUnit(Unit aUnit) {
      return aUnit == eUnit_Enumerated || aUnit == eUnit_Visibility ||
             aUnit == eUnit_Integer;
    }
    static bool IsCSSValueUnit(Unit aUnit) {
      return aUnit == eUnit_Calc;
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
      return aUnit == eUnit_Dasharray || aUnit == eUnit_Shadow ||
             aUnit == eUnit_Transform;
    }
    static bool IsCSSValuePairListUnit(Unit aUnit) {
      return aUnit == eUnit_CSSValuePairList;
    }
    static bool IsStringUnit(Unit aUnit) {
      return aUnit == eUnit_UnparsedString;
    }
  };
};

#endif
