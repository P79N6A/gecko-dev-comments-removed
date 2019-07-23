








































#ifndef nsStyleAnimation_h_
#define nsStyleAnimation_h_

#include "prtypes.h"
#include "nsAString.h"
#include "nsCSSProperty.h"
#include "nsCoord.h"
#include "nsColor.h"

class nsCSSDeclaration;
class nsIContent;
class nsPresContext;
class nsStyleContext;




class nsStyleAnimation {
public:
  class Value;

  
  
  











  static PRBool Add(Value& aDest, const Value& aValueToAdd,
                    PRUint32 aCount);

  












  static PRBool ComputeDistance(const Value& aStartValue,
                                const Value& aEndValue,
                                double& aDistance);

  















  static PRBool Interpolate(const Value& aStartValue,
                            const Value& aEndValue,
                            double aPortion,
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
    eUnit_Coord,
    eUnit_Percent,
    eUnit_Float,
    eUnit_Color
  };

  class Value {
  private:
    Unit mUnit;
    union {
      nscoord mCoord;
      float mFloat;
      nscolor mColor;
    } mValue;
  public:
    Unit GetUnit() const {
      NS_ASSERTION(mUnit != eUnit_Null, "uninitialized");
      return mUnit;
    }

    
    
    PRBool IsNull() const {
      return mUnit == eUnit_Null;
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

    explicit Value(Unit aUnit = eUnit_Null) : mUnit(aUnit) {
      NS_ASSERTION(aUnit == eUnit_Null || aUnit == eUnit_Normal ||
                   aUnit == eUnit_Auto || aUnit == eUnit_None,
                   "must be valueless unit");
    }
    Value(const Value& aOther) : mUnit(eUnit_Null) { *this = aOther; }
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
    void SetCoordValue(nscoord aCoord);
    void SetPercentValue(float aPercent);
    void SetFloatValue(float aFloat);
    void SetColorValue(nscolor aColor);

    Value& operator=(const Value& aOther);

    PRBool operator==(const Value& aOther) const;
    PRBool operator!=(const Value& aOther) const
      { return !(*this == aOther); }

  private:
    void FreeValue();
  };
};

#endif
