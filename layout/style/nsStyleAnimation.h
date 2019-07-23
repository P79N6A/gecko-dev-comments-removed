








































#ifndef nsStyleAnimation_h_
#define nsStyleAnimation_h_

#include "prtypes.h"
#include "nsAString.h"
#include "nsCRTGlue.h"
#include "nsStringBuffer.h"
#include "nsCSSProperty.h"
#include "nsCoord.h"
#include "nsColor.h"

class nsCSSDeclaration;
class nsIContent;
class nsPresContext;
class nsStyleContext;
struct nsCSSValueList;
struct nsCSSValuePair;




class nsStyleAnimation {
public:
  class Value;

  
  
  











  static PRBool Add(nsCSSProperty aProperty, Value& aDest,
                    const Value& aValueToAdd, PRUint32 aCount) {
    return AddWeighted(aProperty, 1.0, aDest, aCount, aValueToAdd, aDest);
  }

  

















  static PRBool ComputeDistance(nsCSSProperty aProperty,
                                const Value& aStartValue,
                                const Value& aEndValue,
                                double& aDistance);

  















  static PRBool Interpolate(nsCSSProperty aProperty,
                            const Value& aStartValue,
                            const Value& aEndValue,
                            double aPortion,
                            Value& aResultValue) {
    NS_ABORT_IF_FALSE(0.0 <= aPortion && aPortion <= 1.0, "out of range");
    return AddWeighted(aProperty, 1.0 - aPortion, aStartValue,
                       aPortion, aEndValue, aResultValue);
  }

  













  static PRBool AddWeighted(nsCSSProperty aProperty,
                            double aCoeff1, const Value& aValue1,
                            double aCoeff2, const Value& aValue2,
                            Value& aResultValue);

  
  
  



















  static PRBool ComputeValue(nsCSSProperty aProperty,
                             nsIContent* aElement,
                             const nsAString& aSpecifiedValue,
                             Value& aComputedValue);

  


















  static PRBool UncomputeValue(nsCSSProperty aProperty,
                               nsPresContext* aPresContext,
                               const Value& aComputedValue,
                               void* aSpecifiedValue);
  static PRBool UncomputeValue(nsCSSProperty aProperty,
                               nsPresContext* aPresContext,
                               const Value& aComputedValue,
                               nsAString& aSpecifiedValue);

  








  static PRBool ExtractComputedValue(nsCSSProperty aProperty,
                                     nsStyleContext* aStyleContext,
                                     Value& aComputedValue);

  


  enum Unit {
    eUnit_Null, 
    eUnit_Normal,
    eUnit_Auto,
    eUnit_None,
    eUnit_Enumerated,
    eUnit_Integer,
    eUnit_Coord,
    eUnit_Percent,
    eUnit_Float,
    eUnit_Color,
    eUnit_CSSValuePair, 
    eUnit_Dasharray, 
    eUnit_Shadow, 
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
      nsCSSValuePair* mCSSValuePair;
      nsCSSValueList* mCSSValueList;
      nsStringBuffer* mString;
    } mValue;
  public:
    Unit GetUnit() const {
      NS_ASSERTION(mUnit != eUnit_Null, "uninitialized");
      return mUnit;
    }

    
    
    PRBool IsNull() const {
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
    nsCSSValuePair* GetCSSValuePairValue() const {
      NS_ASSERTION(IsCSSValuePairUnit(mUnit), "unit mismatch");
      return mValue.mCSSValuePair;
    }
    nsCSSValueList* GetCSSValueListValue() const {
      NS_ASSERTION(IsCSSValueListUnit(mUnit), "unit mismatch");
      return mValue.mCSSValueList;
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

    
    
    void SetAndAdoptCSSValueListValue(nsCSSValueList *aValue, Unit aUnit);
    void SetAndAdoptCSSValuePairValue(nsCSSValuePair *aValue, Unit aUnit);

    Value& operator=(const Value& aOther);

    PRBool operator==(const Value& aOther) const;
    PRBool operator!=(const Value& aOther) const
      { return !(*this == aOther); }

  private:
    void FreeValue();

    static const PRUnichar* GetBufferValue(nsStringBuffer* aBuffer) {
      return static_cast<PRUnichar*>(aBuffer->Data());
    }

    static PRBool IsIntUnit(Unit aUnit) {
      return aUnit == eUnit_Enumerated || aUnit == eUnit_Integer;
    }
    static PRBool IsCSSValuePairUnit(Unit aUnit) {
      return aUnit == eUnit_CSSValuePair;
    }
    static PRBool IsCSSValueListUnit(Unit aUnit) {
      return aUnit == eUnit_Dasharray || aUnit == eUnit_Shadow;
    }
    static PRBool IsStringUnit(Unit aUnit) {
      return aUnit == eUnit_UnparsedString;
    }
  };
};

#endif
