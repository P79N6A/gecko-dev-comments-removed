








































#include "nsStyleAnimation.h"
#include "nsCOMArray.h"
#include "nsIStyleRule.h"
#include "mozilla/css/StyleRule.h"
#include "nsString.h"
#include "nsStyleContext.h"
#include "nsStyleSet.h"
#include "nsComputedDOMStyle.h"
#include "nsCSSParser.h"
#include "mozilla/css/Declaration.h"
#include "mozilla/dom/Element.h"
#include "prlog.h"
#include <math.h>

namespace css = mozilla::css;
namespace dom = mozilla::dom;

















static
nsStyleAnimation::Unit
GetCommonUnit(nsCSSProperty aProperty,
              nsStyleAnimation::Unit aFirstUnit,
              nsStyleAnimation::Unit aSecondUnit)
{
  if (aFirstUnit != aSecondUnit) {
    if (nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_STORES_CALC) &&
        (aFirstUnit == nsStyleAnimation::eUnit_Coord ||
         aFirstUnit == nsStyleAnimation::eUnit_Percent ||
         aFirstUnit == nsStyleAnimation::eUnit_Calc) &&
        (aSecondUnit == nsStyleAnimation::eUnit_Coord ||
         aSecondUnit == nsStyleAnimation::eUnit_Percent ||
         aSecondUnit == nsStyleAnimation::eUnit_Calc)) {
      
      return nsStyleAnimation::eUnit_Calc;
    }
    return nsStyleAnimation::eUnit_Null;
  }
  return aFirstUnit;
}

static
nsCSSUnit
GetCommonUnit(nsCSSProperty aProperty,
              nsCSSUnit aFirstUnit,
              nsCSSUnit aSecondUnit)
{
  if (aFirstUnit != aSecondUnit) {
    if (nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_STORES_CALC) &&
        (aFirstUnit == eCSSUnit_Pixel ||
         aFirstUnit == eCSSUnit_Percent ||
         aFirstUnit == eCSSUnit_Calc) &&
        (aSecondUnit == eCSSUnit_Pixel ||
         aSecondUnit == eCSSUnit_Percent ||
         aSecondUnit == eCSSUnit_Calc)) {
      
      return eCSSUnit_Calc;
    }
    return eCSSUnit_Null;
  }
  return aFirstUnit;
}



static PRUint32
gcd(PRUint32 a, PRUint32 b)
{
  
  
  NS_ABORT_IF_FALSE(a > 0 && b > 0, "positive numbers expected");

  while (a != b) {
    if (a > b) {
      a = a - b;
    } else {
      b = b - a;
    }
  }

  return a;
}


static PRUint32
lcm(PRUint32 a, PRUint32 b)
{
  
  return (a / gcd(a, b)) * b;
}

inline void
nscoordToCSSValue(nscoord aCoord, nsCSSValue& aCSSValue)
{
  aCSSValue.SetFloatValue(nsPresContext::AppUnitsToFloatCSSPixels(aCoord),
                          eCSSUnit_Pixel);
}


struct CalcValue {
  float mLength, mPercent;
  PRBool mHasPercent;
};


static CalcValue
ExtractCalcValueInternal(const nsCSSValue& aValue)
{
  NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Calc, "unexpected unit");
  nsCSSValue::Array *arr = aValue.GetArrayValue();
  NS_ABORT_IF_FALSE(arr->Count() == 1, "unexpected length");

  const nsCSSValue &topval = arr->Item(0);
  CalcValue result;
  if (topval.GetUnit() == eCSSUnit_Pixel) {
    result.mLength = topval.GetFloatValue();
    result.mPercent = 0.0f;
    result.mHasPercent = PR_FALSE;
  } else {
    NS_ABORT_IF_FALSE(topval.GetUnit() == eCSSUnit_Calc_Plus,
                      "unexpected unit");
    nsCSSValue::Array *arr2 = topval.GetArrayValue();
    const nsCSSValue &len = arr2->Item(0);
    const nsCSSValue &pct = arr2->Item(1);
    NS_ABORT_IF_FALSE(len.GetUnit() == eCSSUnit_Pixel, "unexpected unit");
    NS_ABORT_IF_FALSE(pct.GetUnit() == eCSSUnit_Percent, "unexpected unit");
    result.mLength = len.GetFloatValue();
    result.mPercent = pct.GetPercentValue();
    result.mHasPercent = PR_TRUE;
  }

  return result;
}


static CalcValue
ExtractCalcValue(const nsStyleAnimation::Value& aValue)
{
  CalcValue result;
  if (aValue.GetUnit() == nsStyleAnimation::eUnit_Coord) {
    result.mLength =
      nsPresContext::AppUnitsToFloatCSSPixels(aValue.GetCoordValue());
    result.mPercent = 0.0f;
    result.mHasPercent = PR_FALSE;
    return result;
  }
  if (aValue.GetUnit() == nsStyleAnimation::eUnit_Percent) {
    result.mLength = 0.0f;
    result.mPercent = aValue.GetPercentValue();
    result.mHasPercent = PR_TRUE;
    return result;
  }
  NS_ABORT_IF_FALSE(aValue.GetUnit() == nsStyleAnimation::eUnit_Calc,
                    "unexpected unit");
  nsCSSValue *val = aValue.GetCSSValueValue();
  return ExtractCalcValueInternal(*val);
}

static CalcValue
ExtractCalcValue(const nsCSSValue& aValue)
{
  CalcValue result;
  if (aValue.GetUnit() == eCSSUnit_Pixel) {
    result.mLength = aValue.GetFloatValue();
    result.mPercent = 0.0f;
    result.mHasPercent = PR_FALSE;
    return result;
  }
  if (aValue.GetUnit() == eCSSUnit_Percent) {
    result.mLength = 0.0f;
    result.mPercent = aValue.GetPercentValue();
    result.mHasPercent = PR_TRUE;
    return result;
  }
  return ExtractCalcValueInternal(aValue);
}

static void
SetCalcValue(const nsStyleCoord::Calc* aCalc, nsCSSValue& aValue)
{
  nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(1);
  if (!aCalc->mHasPercent) {
    nscoordToCSSValue(aCalc->mLength, arr->Item(0));
  } else {
    nsCSSValue::Array *arr2 = nsCSSValue::Array::Create(2);
    arr->Item(0).SetArrayValue(arr2, eCSSUnit_Calc_Plus);
    nscoordToCSSValue(aCalc->mLength, arr2->Item(0));
    arr2->Item(1).SetPercentValue(aCalc->mPercent);
  }

  aValue.SetArrayValue(arr, eCSSUnit_Calc);
}

static already_AddRefed<nsStringBuffer>
GetURIAsUtf16StringBuffer(nsIURI* aUri)
{
  nsCAutoString utf8String;
  nsresult rv = aUri->GetSpec(utf8String);
  NS_ENSURE_SUCCESS(rv, nsnull);

  return nsCSSValue::BufferFromString(NS_ConvertUTF8toUTF16(utf8String));
}




PRBool
nsStyleAnimation::ComputeDistance(nsCSSProperty aProperty,
                                  const Value& aStartValue,
                                  const Value& aEndValue,
                                  double& aDistance)
{
  Unit commonUnit =
    GetCommonUnit(aProperty, aStartValue.GetUnit(), aEndValue.GetUnit());

  switch (commonUnit) {
    case eUnit_Null:
    case eUnit_Auto:
    case eUnit_None:
    case eUnit_Normal:
    case eUnit_UnparsedString:
      return PR_FALSE;

    case eUnit_Enumerated:
      switch (aProperty) {
        case eCSSProperty_font_stretch: {
          
          PRInt32 startInt = aStartValue.GetIntValue();
          PRInt32 endInt = aEndValue.GetIntValue();
          aDistance = PR_ABS(endInt - startInt);
          return PR_TRUE;
        }
        default:
          return PR_FALSE;
      }
   case eUnit_Visibility: {
      PRInt32 startVal =
        aStartValue.GetIntValue() == NS_STYLE_VISIBILITY_VISIBLE;
      PRInt32 endVal =
        aEndValue.GetIntValue() == NS_STYLE_VISIBILITY_VISIBLE;
      aDistance = PR_ABS(startVal - endVal);
      return PR_TRUE;
    }
    case eUnit_Integer: {
      PRInt32 startInt = aStartValue.GetIntValue();
      PRInt32 endInt = aEndValue.GetIntValue();
      aDistance = PR_ABS(endInt - startInt);
      return PR_TRUE;
    }
    case eUnit_Coord: {
      nscoord startCoord = aStartValue.GetCoordValue();
      nscoord endCoord = aEndValue.GetCoordValue();
      aDistance = fabs(double(endCoord - startCoord));
      return PR_TRUE;
    }
    case eUnit_Percent: {
      float startPct = aStartValue.GetPercentValue();
      float endPct = aEndValue.GetPercentValue();
      aDistance = fabs(double(endPct - startPct));
      return PR_TRUE;
    }
    case eUnit_Float: {
      float startFloat = aStartValue.GetFloatValue();
      float endFloat = aEndValue.GetFloatValue();
      aDistance = fabs(double(endFloat - startFloat));
      return PR_TRUE;
    }
    case eUnit_Color: {
      
      
      
      
      
      
      
      
      
      nscolor startColor = aStartValue.GetColorValue();
      nscolor endColor = aEndValue.GetColorValue();

      
      
      #define GET_COMPONENT(component_, color_) \
        (NS_GET_##component_(color_) * (1.0 / 255.0))

      double startA = GET_COMPONENT(A, startColor);
      double startR = GET_COMPONENT(R, startColor) * startA;
      double startG = GET_COMPONENT(G, startColor) * startA;
      double startB = GET_COMPONENT(B, startColor) * startA;
      double endA = GET_COMPONENT(A, endColor);
      double endR = GET_COMPONENT(R, endColor) * endA;
      double endG = GET_COMPONENT(G, endColor) * endA;
      double endB = GET_COMPONENT(B, endColor) * endA;

      #undef GET_COMPONENT

      double diffA = startA - endA;
      double diffR = startR - endR;
      double diffG = startG - endG;
      double diffB = startB - endB;
      aDistance = sqrt(diffA * diffA + diffR * diffR +
                       diffG * diffG + diffB * diffB);
      return PR_TRUE;
    }
    case eUnit_Calc: {
      CalcValue v1 = ExtractCalcValue(aStartValue);
      CalcValue v2 = ExtractCalcValue(aEndValue);
      float difflen = v2.mLength - v1.mLength;
      float diffpct = v2.mPercent - v1.mPercent;
      aDistance = sqrt(difflen * difflen + diffpct * diffpct);
      return PR_TRUE;
    }
    case eUnit_CSSValuePair: {
      const nsCSSValuePair *pair1 = aStartValue.GetCSSValuePairValue();
      const nsCSSValuePair *pair2 = aEndValue.GetCSSValuePairValue();
      nsCSSUnit unit[2];
      unit[0] = GetCommonUnit(aProperty, pair1->mXValue.GetUnit(),
                              pair2->mXValue.GetUnit());
      unit[1] = GetCommonUnit(aProperty, pair1->mYValue.GetUnit(),
                              pair2->mYValue.GetUnit());
      if (unit[0] == eCSSUnit_Null || unit[1] == eCSSUnit_Null ||
          unit[0] == eCSSUnit_URL) {
        return PR_FALSE;
      }

      double squareDistance = 0.0;
      static nsCSSValue nsCSSValuePair::* const pairValues[2] = {
        &nsCSSValuePair::mXValue, &nsCSSValuePair::mYValue
      };
      for (PRUint32 i = 0; i < 2; ++i) {
        nsCSSValue nsCSSValuePair::*member = pairValues[i];
        double diffsquared;
        switch (unit[i]) {
          case eCSSUnit_Pixel: {
            float diff = (pair1->*member).GetFloatValue() -
                         (pair2->*member).GetFloatValue();
            diffsquared = diff * diff;
            break;
          }
          case eCSSUnit_Percent: {
            float diff = (pair1->*member).GetPercentValue() -
                         (pair2->*member).GetPercentValue();
            diffsquared = diff * diff;
            break;
          }
          case eCSSUnit_Calc: {
            CalcValue v1 = ExtractCalcValue(pair1->*member);
            CalcValue v2 = ExtractCalcValue(pair2->*member);
            float difflen = v2.mLength - v1.mLength;
            float diffpct = v2.mPercent - v1.mPercent;
            diffsquared = difflen * difflen + diffpct * diffpct;
            break;
          }
          default:
            NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
            return PR_FALSE;
        }
        squareDistance += diffsquared;
      }

      aDistance = sqrt(squareDistance);
      return PR_TRUE;
    }
    case eUnit_CSSRect: {
      const nsCSSRect *rect1 = aStartValue.GetCSSRectValue();
      const nsCSSRect *rect2 = aEndValue.GetCSSRectValue();
      if (rect1->mTop.GetUnit() != rect2->mTop.GetUnit() ||
          rect1->mRight.GetUnit() != rect2->mRight.GetUnit() ||
          rect1->mBottom.GetUnit() != rect2->mBottom.GetUnit() ||
          rect1->mLeft.GetUnit() != rect2->mLeft.GetUnit()) {
        
        return PR_FALSE;
      }

      double squareDistance = 0.0;
      for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(nsCSSRect::sides); ++i) {
        nsCSSValue nsCSSRect::*member = nsCSSRect::sides[i];
        NS_ABORT_IF_FALSE((rect1->*member).GetUnit() ==
                            (rect2->*member).GetUnit(),
                          "should have returned above");
        double diff;
        switch ((rect1->*member).GetUnit()) {
          case eCSSUnit_Pixel:
            diff = (rect1->*member).GetFloatValue() -
                   (rect2->*member).GetFloatValue();
            break;
          case eCSSUnit_Auto:
            diff = 0;
            break;
          default:
            NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
            return PR_FALSE;
        }
        squareDistance += diff * diff;
      }

      aDistance = sqrt(squareDistance);
      return PR_TRUE;
    }
    case eUnit_Dasharray: {
      
      
      
      

      
      Value normValue1, normValue2;
      if (!AddWeighted(aProperty, 1.0, aStartValue, 0.0, aEndValue,
                       normValue1) ||
          !AddWeighted(aProperty, 0.0, aStartValue, 1.0, aEndValue,
                       normValue2)) {
        return PR_FALSE;
      }

      double squareDistance = 0.0;
      const nsCSSValueList *list1 = normValue1.GetCSSValueListValue();
      const nsCSSValueList *list2 = normValue2.GetCSSValueListValue();

      NS_ABORT_IF_FALSE(!list1 == !list2, "lists should be same length");
      while (list1) {
        const nsCSSValue &val1 = list1->mValue;
        const nsCSSValue &val2 = list2->mValue;

        NS_ABORT_IF_FALSE(val1.GetUnit() == val2.GetUnit(),
                          "unit match should be assured by AddWeighted");
        double diff;
        switch (val1.GetUnit()) {
          case eCSSUnit_Percent:
            diff = val1.GetPercentValue() - val2.GetPercentValue();
            break;
          case eCSSUnit_Number:
            diff = val1.GetFloatValue() - val2.GetFloatValue();
            break;
          default:
            NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
            return PR_FALSE;
        }
        squareDistance += diff * diff;

        list1 = list1->mNext;
        list2 = list2->mNext;
        NS_ABORT_IF_FALSE(!list1 == !list2, "lists should be same length");
      }

      aDistance = sqrt(squareDistance);
      return PR_TRUE;
    }
    case eUnit_Shadow: {
      
      Value normValue1, normValue2;
      if (!AddWeighted(aProperty, 1.0, aStartValue, 0.0, aEndValue,
                       normValue1) ||
          !AddWeighted(aProperty, 0.0, aStartValue, 1.0, aEndValue,
                       normValue2)) {
        return PR_FALSE;
      }

      const nsCSSValueList *shadow1 = normValue1.GetCSSValueListValue();
      const nsCSSValueList *shadow2 = normValue2.GetCSSValueListValue();

      double squareDistance = 0.0;
      NS_ABORT_IF_FALSE(!shadow1 == !shadow2, "lists should be same length");
      while (shadow1) {
        nsCSSValue::Array *array1 = shadow1->mValue.GetArrayValue();
        nsCSSValue::Array *array2 = shadow2->mValue.GetArrayValue();
        for (size_t i = 0; i < 4; ++i) {
          NS_ABORT_IF_FALSE(array1->Item(i).GetUnit() == eCSSUnit_Pixel,
                            "unexpected unit");
          NS_ABORT_IF_FALSE(array2->Item(i).GetUnit() == eCSSUnit_Pixel,
                            "unexpected unit");
          double diff = array1->Item(i).GetFloatValue() -
                        array2->Item(i).GetFloatValue();
          squareDistance += diff * diff;
        }

        const nsCSSValue &color1 = array1->Item(4);
        const nsCSSValue &color2 = array2->Item(4);
#ifdef DEBUG
        {
          const nsCSSValue &inset1 = array1->Item(5);
          const nsCSSValue &inset2 = array2->Item(5);
          
          
          
          
          NS_ABORT_IF_FALSE(color1.GetUnit() == color2.GetUnit() &&
                            inset1 == inset2,
                            "AddWeighted should have failed");
        }
#endif

        if (color1.GetUnit() != eCSSUnit_Null) {
          nsStyleAnimation::Value color1Value
            (color1.GetColorValue(), nsStyleAnimation::Value::ColorConstructor);
          nsStyleAnimation::Value color2Value
            (color2.GetColorValue(), nsStyleAnimation::Value::ColorConstructor);
          double colorDistance;

        #ifdef DEBUG
          PRBool ok =
        #endif
            nsStyleAnimation::ComputeDistance(eCSSProperty_color,
                                              color1Value, color2Value,
                                              colorDistance);
          NS_ABORT_IF_FALSE(ok, "should not fail");
          squareDistance += colorDistance * colorDistance;
        }

        shadow1 = shadow1->mNext;
        shadow2 = shadow2->mNext;
        NS_ABORT_IF_FALSE(!shadow1 == !shadow2, "lists should be same length");
      }
      aDistance = sqrt(squareDistance);
      return PR_TRUE;
    }
    case eUnit_Transform: {
      
      
      
      Value normValue1, normValue2;
      if (!AddWeighted(aProperty, 1.0, aStartValue, 0.0, aEndValue,
                       normValue1) ||
          !AddWeighted(aProperty, 0.0, aStartValue, 1.0, aEndValue,
                       normValue2)) {
        return PR_FALSE;
      }
      const nsCSSValueList *list1 = normValue1.GetCSSValueListValue();
      const nsCSSValueList *list2 = normValue2.GetCSSValueListValue();

      NS_ABORT_IF_FALSE((list1->mValue.GetUnit() == eCSSUnit_None) ==
                        (list2->mValue.GetUnit() == eCSSUnit_None),
                        "none-ness should match after AddWeighted");
      if (list1->mValue.GetUnit() == eCSSUnit_None) {
        aDistance = 0;
        return PR_TRUE;
      }

      double squareDistance = 0.0;
      for (; list1 && list2; list1 = list1->mNext, list2 = list2->mNext) {
        NS_ABORT_IF_FALSE(list1->mValue.GetUnit() == eCSSUnit_Function &&
                          list2->mValue.GetUnit() == eCSSUnit_Function,
                          "unexpected unit");
        const nsCSSValue::Array *a1 = list1->mValue.GetArrayValue(),
                                *a2 = list2->mValue.GetArrayValue();
        NS_ABORT_IF_FALSE(a1->Item(0).GetUnit() == eCSSUnit_Ident &&
                          a2->Item(0).GetUnit() == eCSSUnit_Ident,
                          "unexpected unit");
        NS_ABORT_IF_FALSE(a1->Item(0) == a2->Item(0),
                          "unexpected function mismatch");
        NS_ABORT_IF_FALSE(a1->Count() == a2->Count(),
                          "unexpected count mismatch");
        for (size_t i = 1, iEnd = NS_MIN(a1->Count(), a2->Count());
             i < iEnd; ++i) {
          const nsCSSValue &v1 = a1->Item(i), &v2 = a2->Item(i);
          NS_ABORT_IF_FALSE(v1.GetUnit() == eCSSUnit_Pixel ||
                            v1.GetUnit() == eCSSUnit_Percent ||
                            v1.GetUnit() == eCSSUnit_Calc ||
                            v1.GetUnit() == eCSSUnit_Radian ||
                            v1.GetUnit() == eCSSUnit_Number,
                            "unexpected unit");
          NS_ABORT_IF_FALSE(v2.GetUnit() == eCSSUnit_Pixel ||
                            v2.GetUnit() == eCSSUnit_Percent ||
                            v2.GetUnit() == eCSSUnit_Calc ||
                            v2.GetUnit() == eCSSUnit_Radian ||
                            v2.GetUnit() == eCSSUnit_Number,
                            "unexpected unit");
          if (v1.GetUnit() == eCSSUnit_Pixel ||
              v1.GetUnit() == eCSSUnit_Percent ||
              v1.GetUnit() == eCSSUnit_Calc) {
            NS_ABORT_IF_FALSE(v2.GetUnit() == eCSSUnit_Pixel ||
                              v2.GetUnit() == eCSSUnit_Percent ||
                              v2.GetUnit() == eCSSUnit_Calc,
                              "unit mismatch");
            CalcValue c1 = ExtractCalcValue(v1),
                      c2 = ExtractCalcValue(v2);
            double diff = c1.mLength - c2.mLength;
            squareDistance += diff * diff;
            diff = c1.mPercent - c2.mPercent;
            squareDistance += diff * diff;
          } else {
            NS_ABORT_IF_FALSE(v1.GetUnit() == v2.GetUnit(), "unit mismatch");
            double diff = v2.GetFloatValue() - v1.GetFloatValue();
            squareDistance += diff * diff;
          }
        }
      }
      NS_ABORT_IF_FALSE(!list1 && !list2,
                        "list lengths should match after AddWeighted");

      aDistance = sqrt(squareDistance);
      return PR_TRUE;
    }
    case eUnit_CSSValuePairList: {
      const nsCSSValuePairList *list1 = aStartValue.GetCSSValuePairListValue();
      const nsCSSValuePairList *list2 = aEndValue.GetCSSValuePairListValue();
      double squareDistance = 0.0;
      do {
        static nsCSSValue nsCSSValuePairList::* const pairListValues[] = {
          &nsCSSValuePairList::mXValue,
          &nsCSSValuePairList::mYValue,
        };
        for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(pairListValues); ++i) {
          const nsCSSValue &v1 = list1->*(pairListValues[i]);
          const nsCSSValue &v2 = list2->*(pairListValues[i]);
          nsCSSUnit unit =
            GetCommonUnit(aProperty, v1.GetUnit(), v2.GetUnit());
          if (unit == eCSSUnit_Null) {
            return PR_FALSE;
          }
          double diffsquared = 0.0;
          switch (unit) {
            case eCSSUnit_Pixel: {
              float diff = v1.GetFloatValue() - v2.GetFloatValue();
              diffsquared = diff * diff;
              break;
            }
            case eCSSUnit_Percent: {
              float diff = v1.GetPercentValue() - v2.GetPercentValue();
              diffsquared = diff * diff;
              break;
            }
            case eCSSUnit_Calc: {
              CalcValue val1 = ExtractCalcValue(v1);
              CalcValue val2 = ExtractCalcValue(v2);
              float difflen = val2.mLength - val1.mLength;
              float diffpct = val2.mPercent - val1.mPercent;
              diffsquared = difflen * difflen + diffpct * diffpct;
              break;
            }
            default:
              if (v1 != v2) {
                return PR_FALSE;
              }
              break;
          }
          squareDistance += diffsquared;
        }
        list1 = list1->mNext;
        list2 = list2->mNext;
      } while (list1 && list2);
      if (list1 || list2) {
        
        return PR_FALSE;
      }
      aDistance = sqrt(squareDistance);
      return PR_TRUE;
    }
  }

  NS_ABORT_IF_FALSE(false, "Can't compute distance using the given common unit");
  return PR_FALSE;
}

#define MAX_PACKED_COLOR_COMPONENT 255

inline PRUint8 ClampColor(double aColor)
{
  if (aColor >= MAX_PACKED_COLOR_COMPONENT)
    return MAX_PACKED_COLOR_COMPONENT;
  if (aColor <= 0.0)
    return 0;
  return NSToIntRound(aColor);
}

template <typename T>
T
RestrictValue(PRUint32 aRestrictions, T aValue)
{
  T result = aValue;
  switch (aRestrictions) {
    case 0:
      break;
    case CSS_PROPERTY_VALUE_NONNEGATIVE:
      if (result < 0) {
        result = 0;
      }
      break;
    case CSS_PROPERTY_VALUE_AT_LEAST_ONE:
      if (result < 1) {
        result = 1;
      }
      break;
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "bad value restriction");
      break;
  }
  return result;
}

template <typename T>
T
RestrictValue(nsCSSProperty aProperty, T aValue)
{
  return RestrictValue(nsCSSProps::ValueRestrictions(aProperty), aValue);
}

static inline void
AddCSSValuePixel(double aCoeff1, const nsCSSValue &aValue1,
                 double aCoeff2, const nsCSSValue &aValue2,
                 nsCSSValue &aResult, PRUint32 aValueRestrictions = 0)
{
  NS_ABORT_IF_FALSE(aValue1.GetUnit() == eCSSUnit_Pixel, "unexpected unit");
  NS_ABORT_IF_FALSE(aValue2.GetUnit() == eCSSUnit_Pixel, "unexpected unit");
  aResult.SetFloatValue(RestrictValue(aValueRestrictions,
                                      aCoeff1 * aValue1.GetFloatValue() +
                                      aCoeff2 * aValue2.GetFloatValue()),
                        eCSSUnit_Pixel);
}

static inline void
AddCSSValueNumber(double aCoeff1, const nsCSSValue &aValue1,
                  double aCoeff2, const nsCSSValue &aValue2,
                  nsCSSValue &aResult, PRUint32 aValueRestrictions = 0)
{
  NS_ABORT_IF_FALSE(aValue1.GetUnit() == eCSSUnit_Number, "unexpected unit");
  NS_ABORT_IF_FALSE(aValue2.GetUnit() == eCSSUnit_Number, "unexpected unit");
  aResult.SetFloatValue(RestrictValue(aValueRestrictions,
                                      aCoeff1 * aValue1.GetFloatValue() +
                                      aCoeff2 * aValue2.GetFloatValue()),
                        eCSSUnit_Number);
}

static inline void
AddCSSValuePercent(double aCoeff1, const nsCSSValue &aValue1,
                   double aCoeff2, const nsCSSValue &aValue2,
                   nsCSSValue &aResult, PRUint32 aValueRestrictions = 0)
{
  NS_ABORT_IF_FALSE(aValue1.GetUnit() == eCSSUnit_Percent, "unexpected unit");
  NS_ABORT_IF_FALSE(aValue2.GetUnit() == eCSSUnit_Percent, "unexpected unit");
  aResult.SetPercentValue(RestrictValue(aValueRestrictions,
                                        aCoeff1 * aValue1.GetPercentValue() +
                                        aCoeff2 * aValue2.GetPercentValue()));
}



static void
AddCSSValueCanonicalCalc(double aCoeff1, const nsCSSValue &aValue1,
                         double aCoeff2, const nsCSSValue &aValue2,
                         nsCSSValue &aResult)
{
  CalcValue v1 = ExtractCalcValue(aValue1);
  CalcValue v2 = ExtractCalcValue(aValue2);
  NS_ABORT_IF_FALSE(v1.mHasPercent || v2.mHasPercent,
                    "only used on properties that always have percent in calc");
  nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2),
                              acalc = nsCSSValue::Array::Create(1);
  a->Item(0).SetFloatValue(aCoeff1 * v1.mLength + aCoeff2 * v2.mLength,
                           eCSSUnit_Pixel);
  a->Item(1).SetPercentValue(aCoeff1 * v1.mPercent + aCoeff2 * v2.mPercent);
  acalc->Item(0).SetArrayValue(a, eCSSUnit_Calc_Plus);
  aResult.SetArrayValue(acalc, eCSSUnit_Calc);
}

static void
AddCSSValueAngle(const nsCSSValue &aValue1, double aCoeff1,
                 const nsCSSValue &aValue2, double aCoeff2,
                 nsCSSValue &aResult)
{
  aResult.SetFloatValue(aCoeff1 * aValue1.GetAngleValueInRadians() +
                        aCoeff2 * aValue2.GetAngleValueInRadians(),
                        eCSSUnit_Radian);
}

static PRBool
AddShadowItems(double aCoeff1, const nsCSSValue &aValue1,
               double aCoeff2, const nsCSSValue &aValue2,
               nsCSSValueList **&aResultTail)
{
  
  NS_ABORT_IF_FALSE(aValue1.GetUnit() == eCSSUnit_Array,
                    "wrong unit");
  NS_ABORT_IF_FALSE(aValue2.GetUnit() == eCSSUnit_Array,
                    "wrong unit");
  nsCSSValue::Array *array1 = aValue1.GetArrayValue();
  nsCSSValue::Array *array2 = aValue2.GetArrayValue();
  nsRefPtr<nsCSSValue::Array> resultArray = nsCSSValue::Array::Create(6);

  for (size_t i = 0; i < 4; ++i) {
    AddCSSValuePixel(aCoeff1, array1->Item(i), aCoeff2, array2->Item(i),
                     resultArray->Item(i),
                     
                     (i == 2) ? CSS_PROPERTY_VALUE_NONNEGATIVE : 0);
  }

  const nsCSSValue& color1 = array1->Item(4);
  const nsCSSValue& color2 = array2->Item(4);
  const nsCSSValue& inset1 = array1->Item(5);
  const nsCSSValue& inset2 = array2->Item(5);
  if (color1.GetUnit() != color2.GetUnit() ||
      inset1.GetUnit() != inset2.GetUnit()) {
    
    
    return PR_FALSE;
  }

  if (color1.GetUnit() != eCSSUnit_Null) {
    nsStyleAnimation::Value color1Value
      (color1.GetColorValue(), nsStyleAnimation::Value::ColorConstructor);
    nsStyleAnimation::Value color2Value
      (color2.GetColorValue(), nsStyleAnimation::Value::ColorConstructor);
    nsStyleAnimation::Value resultColorValue;
  #ifdef DEBUG
    PRBool ok =
  #endif
      nsStyleAnimation::AddWeighted(eCSSProperty_color, aCoeff1, color1Value,
                                    aCoeff2, color2Value, resultColorValue);
    NS_ABORT_IF_FALSE(ok, "should not fail");
    resultArray->Item(4).SetColorValue(resultColorValue.GetColorValue());
  }

  NS_ABORT_IF_FALSE(inset1 == inset2, "should match");
  resultArray->Item(5) = inset1;

  nsCSSValueList *resultItem = new nsCSSValueList;
  if (!resultItem) {
    return PR_FALSE;
  }
  resultItem->mValue.SetArrayValue(resultArray, eCSSUnit_Array);
  *aResultTail = resultItem;
  aResultTail = &resultItem->mNext;
  return PR_TRUE;
}

static void
AddTransformTranslate(const nsCSSValue &aValue1, double aCoeff1,
                      const nsCSSValue &aValue2, double aCoeff2,
                      nsCSSValue &aResult)
{
  NS_ABORT_IF_FALSE(aValue1.GetUnit() == eCSSUnit_Percent ||
                    aValue1.GetUnit() == eCSSUnit_Pixel ||
                    aValue1.IsCalcUnit(),
                    "unexpected unit");
  NS_ABORT_IF_FALSE(aValue2.GetUnit() == eCSSUnit_Percent ||
                    aValue2.GetUnit() == eCSSUnit_Pixel ||
                    aValue2.IsCalcUnit(),
                    "unexpected unit");

  if (aValue1.GetUnit() != aValue2.GetUnit() || aValue1.IsCalcUnit()) {
    
    AddCSSValueCanonicalCalc(aCoeff1, aValue1, aCoeff2, aValue2, aResult);
  } else if (aValue1.GetUnit() == eCSSUnit_Percent) {
    
    AddCSSValuePercent(aCoeff1, aValue1, aCoeff2, aValue2, aResult);
  } else {
    
    AddCSSValuePixel(aCoeff1, aValue1, aCoeff2, aValue2, aResult);
  }
}

static void
AddTransformScale(const nsCSSValue &aValue1, double aCoeff1,
                  const nsCSSValue &aValue2, double aCoeff2,
                  nsCSSValue &aResult)
{
  
  
  
  
  NS_ABORT_IF_FALSE(aValue1.GetUnit() == eCSSUnit_Number, "unexpected unit");
  NS_ABORT_IF_FALSE(aValue2.GetUnit() == eCSSUnit_Number, "unexpected unit");

  float v1 = aValue1.GetFloatValue() - 1.0f,
        v2 = aValue2.GetFloatValue() - 1.0f;
  float result = v1 * aCoeff1 + v2 * aCoeff2;
  aResult.SetFloatValue(result + 1.0f, eCSSUnit_Number);
}

static already_AddRefed<nsCSSValue::Array>
AppendTransformFunction(nsCSSKeyword aTransformFunction,
                        nsCSSValueList**& aListTail)
{
  PRUint32 nargs;
  if (aTransformFunction == eCSSKeyword_matrix) {
    nargs = 6;
  } else if (aTransformFunction == eCSSKeyword_translate ||
             aTransformFunction == eCSSKeyword_skew ||
             aTransformFunction == eCSSKeyword_scale) {
    nargs = 2;
  } else {
    NS_ABORT_IF_FALSE(aTransformFunction == eCSSKeyword_translatex ||
                      aTransformFunction == eCSSKeyword_translatey ||
                      aTransformFunction == eCSSKeyword_scalex ||
                      aTransformFunction == eCSSKeyword_scaley ||
                      aTransformFunction == eCSSKeyword_skewx ||
                      aTransformFunction == eCSSKeyword_skewy ||
                      aTransformFunction == eCSSKeyword_rotate,
                      "must be a transform function");
    nargs = 1;
  }

  nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(nargs + 1);
  arr->Item(0).SetStringValue(
    NS_ConvertUTF8toUTF16(nsCSSKeywords::GetStringValue(aTransformFunction)),
    eCSSUnit_Ident);

  nsCSSValueList *item = new nsCSSValueList;
  item->mValue.SetArrayValue(arr, eCSSUnit_Function);

  *aListTail = item;
  aListTail = &item->mNext;

  return arr.forget();
}






























































































































static PRBool
DecomposeMatrix(const nsStyleTransformMatrix &aMatrix,
                float &aRotate, float &aXYShear, float &aScaleX, float &aScaleY)
{
  float A = aMatrix.GetMainMatrixEntry(0),
        B = aMatrix.GetMainMatrixEntry(1),
        C = aMatrix.GetMainMatrixEntry(2),
        D = aMatrix.GetMainMatrixEntry(3);
  if (A * D == B * C) {
    
    return PR_FALSE;
  }

  float scaleX = sqrt(A * A + B * B);
  A /= scaleX;
  B /= scaleX;

  float XYshear = A * C + B * D;
  C -= A * XYshear;
  D -= B * XYshear;

  float scaleY = sqrt(C * C + D * D);
  C /= scaleY;
  D /= scaleY;
  XYshear /= scaleY;

 
  NS_ASSERTION(0.99 < PR_ABS(A*D - B*C) && PR_ABS(A*D - B*C) < 1.01,
               "determinant should now be 1 or -1");
  if (A * D < B * C) {
    A = -A;
    B = -B;
    C = -C;
    D = -D;
    XYshear = -XYshear;
    scaleX = -scaleX;
  }

  float rotation = atan2f(B, A);

  aRotate = rotation;
  aXYShear = XYshear;
  aScaleX = scaleX;
  aScaleY = scaleY;

  return PR_TRUE;
}

static nsCSSValueList*
AddTransformMatrix(const nsStyleTransformMatrix &aMatrix1, double aCoeff1,
                   const nsStyleTransformMatrix &aMatrix2, double aCoeff2)
{

  nsAutoPtr<nsCSSValueList> result;
  nsCSSValueList **resultTail = getter_Transfers(result);
  nsRefPtr<nsCSSValue::Array> arr;

  
  

  
  arr = AppendTransformFunction(eCSSKeyword_rotate, resultTail);
  arr->Item(1).SetFloatValue(float(M_PI_2), eCSSUnit_Radian);

  
  
  float rtranslateXPercent =
    aMatrix1.GetWidthRelativeYTranslation() * aCoeff1 +
    aMatrix2.GetWidthRelativeYTranslation() * aCoeff2;
  float rtranslateYPercent =
    - (aMatrix1.GetHeightRelativeXTranslation() * aCoeff1 +
       aMatrix2.GetHeightRelativeXTranslation() * aCoeff2);
  arr = AppendTransformFunction(eCSSKeyword_translate, resultTail);
  arr->Item(1).SetPercentValue(rtranslateXPercent);
  arr->Item(2).SetPercentValue(rtranslateYPercent);

  
  arr = AppendTransformFunction(eCSSKeyword_rotate, resultTail);
  arr->Item(1).SetFloatValue(-float(M_PI_2), eCSSUnit_Radian);

  nscoord translateXCoord = NSToCoordRound(
                              aMatrix1.GetCoordXTranslation() * aCoeff1 +
                              aMatrix2.GetCoordXTranslation() * aCoeff2);
  nscoord translateYCoord = NSToCoordRound(
                              aMatrix1.GetCoordYTranslation() * aCoeff1 +
                              aMatrix2.GetCoordYTranslation() * aCoeff2);
  float translateXPercent = aMatrix1.GetWidthRelativeXTranslation() * aCoeff1 +
                            aMatrix2.GetWidthRelativeXTranslation() * aCoeff2;
  float translateYPercent = aMatrix1.GetHeightRelativeYTranslation() * aCoeff1 +
                            aMatrix2.GetHeightRelativeYTranslation() * aCoeff2;

  float rotate1, XYshear1, scaleX1, scaleY1;
  DecomposeMatrix(aMatrix1, rotate1, XYshear1, scaleX1, scaleY1);
  float rotate2, XYshear2, scaleX2, scaleY2;
  DecomposeMatrix(aMatrix2, rotate2, XYshear2, scaleX2, scaleY2);

  float rotate = rotate1 * aCoeff1 + rotate2 * aCoeff2;

  float skewX = atanf(XYshear1) * aCoeff1 + atanf(XYshear2) * aCoeff2;

  
  
  
  
  float scaleX =
    ((scaleX1 - 1.0f) * aCoeff1 + (scaleX2 - 1.0f) * aCoeff2) + 1.0f;
  float scaleY =
    ((scaleY1 - 1.0f) * aCoeff1 + (scaleY2 - 1.0f) * aCoeff2) + 1.0f;

  
  
  arr = AppendTransformFunction(eCSSKeyword_translate, resultTail);
  arr->Item(1).SetPercentValue(translateXPercent);
  arr->Item(2).SetPercentValue(translateYPercent);

  arr = AppendTransformFunction(eCSSKeyword_translate, resultTail);
  arr->Item(1).SetFloatValue(
    nsPresContext::AppUnitsToFloatCSSPixels(translateXCoord), eCSSUnit_Pixel);
  arr->Item(2).SetFloatValue(
    nsPresContext::AppUnitsToFloatCSSPixels(translateYCoord), eCSSUnit_Pixel);

  arr = AppendTransformFunction(eCSSKeyword_rotate, resultTail);
  arr->Item(1).SetFloatValue(rotate, eCSSUnit_Radian);

  arr = AppendTransformFunction(eCSSKeyword_skewx, resultTail);
  arr->Item(1).SetFloatValue(skewX, eCSSUnit_Radian);

  arr = AppendTransformFunction(eCSSKeyword_scale, resultTail);
  arr->Item(1).SetFloatValue(scaleX, eCSSUnit_Number);
  arr->Item(2).SetFloatValue(scaleY, eCSSUnit_Number);

  return result.forget();
}

static nsCSSValueList*
AddTransformLists(const nsCSSValueList* aList1, double aCoeff1,
                  const nsCSSValueList* aList2, double aCoeff2)
{
  nsAutoPtr<nsCSSValueList> result;
  nsCSSValueList **resultTail = getter_Transfers(result);

  do {
    const nsCSSValue::Array *a1 = aList1->mValue.GetArrayValue(),
                            *a2 = aList2->mValue.GetArrayValue();
    NS_ABORT_IF_FALSE(nsStyleTransformMatrix::TransformFunctionOf(a1) ==
                      nsStyleTransformMatrix::TransformFunctionOf(a2),
                      "transform function mismatch");

    nsCSSKeyword tfunc = nsStyleTransformMatrix::TransformFunctionOf(a1);
    nsRefPtr<nsCSSValue::Array> arr;
    if (tfunc != eCSSKeyword_matrix) {
      arr = AppendTransformFunction(tfunc, resultTail);
    }

    switch (tfunc) {
      case eCSSKeyword_translate: {
        NS_ABORT_IF_FALSE(a1->Count() == 2 || a1->Count() == 3,
                          "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 2 || a2->Count() == 3,
                          "unexpected count");

        
        
        
        nsCSSValue zero(0.0f, eCSSUnit_Pixel);
        
        AddTransformTranslate(a1->Count() == 3 ? a1->Item(2) : zero,
                              aCoeff1,
                              a2->Count() == 3 ? a2->Item(2) : zero,
                              aCoeff2,
                              arr->Item(2));

        
        
        AddTransformTranslate(a1->Item(1), aCoeff1, a2->Item(1), aCoeff2,
                              arr->Item(1));
        break;
      }
      case eCSSKeyword_translatex:
      case eCSSKeyword_translatey: {
        NS_ABORT_IF_FALSE(a1->Count() == 2, "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 2, "unexpected count");
        AddTransformTranslate(a1->Item(1), aCoeff1, a2->Item(1), aCoeff2,
                              arr->Item(1));
        break;
      }
      case eCSSKeyword_scale: {
        NS_ABORT_IF_FALSE(a1->Count() == 2 || a1->Count() == 3,
                          "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 2 || a2->Count() == 3,
                          "unexpected count");

        
        
        
        
        AddTransformScale(a1->Count() == 3 ? a1->Item(2) : a1->Item(1),
                          aCoeff1,
                          a2->Count() == 3 ? a2->Item(2) : a2->Item(1),
                          aCoeff2,
                          arr->Item(2));

        
        
        AddTransformScale(a1->Item(1), aCoeff1, a2->Item(1), aCoeff2,
                          arr->Item(1));

        break;
      }
      case eCSSKeyword_scalex:
      case eCSSKeyword_scaley: {
        NS_ABORT_IF_FALSE(a1->Count() == 2, "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 2, "unexpected count");

        AddTransformScale(a1->Item(1), aCoeff1, a2->Item(1), aCoeff2,
                          arr->Item(1));

        break;
      }
      
      
      
      
      
      case eCSSKeyword_skew: {
        NS_ABORT_IF_FALSE(a1->Count() == 2 || a1->Count() == 3,
                          "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 2 || a2->Count() == 3,
                          "unexpected count");

        nsCSSValue zero(0.0f, eCSSUnit_Radian);
        
        AddCSSValueAngle(a1->Count() == 3 ? a1->Item(2) : zero,
                         aCoeff1,
                         a2->Count() == 3 ? a2->Item(2) : zero,
                         aCoeff2,
                         arr->Item(2));

        
        
        AddCSSValueAngle(a1->Item(1), aCoeff1, a2->Item(1), aCoeff2,
                         arr->Item(1));

        break;
      }
      case eCSSKeyword_skewx:
      case eCSSKeyword_skewy:
      case eCSSKeyword_rotate: {
        NS_ABORT_IF_FALSE(a1->Count() == 2, "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 2, "unexpected count");

        AddCSSValueAngle(a1->Item(1), aCoeff1, a2->Item(1), aCoeff2,
                         arr->Item(1));

        break;
      }
      case eCSSKeyword_matrix: {
        NS_ABORT_IF_FALSE(a1->Count() == 7, "unexpected count");
        NS_ABORT_IF_FALSE(a2->Count() == 7, "unexpected count");

        PRBool dummy;
        nsStyleTransformMatrix matrix1, matrix2;
        matrix1.SetToTransformFunction(a1, nsnull, nsnull, dummy);
        matrix2.SetToTransformFunction(a2, nsnull, nsnull, dummy);

        *resultTail =
          AddTransformMatrix(matrix1, aCoeff1, matrix2, aCoeff2);

        while ((*resultTail)->mNext) {
          resultTail = &(*resultTail)->mNext;
        }

        break;
      }
      default:
        NS_ABORT_IF_FALSE(PR_FALSE, "unknown transform function");
    }

    aList1 = aList1->mNext;
    aList2 = aList2->mNext;
  } while (aList1);
  NS_ABORT_IF_FALSE(!aList2, "list length mismatch");

  return result.forget();
}

PRBool
nsStyleAnimation::AddWeighted(nsCSSProperty aProperty,
                              double aCoeff1, const Value& aValue1,
                              double aCoeff2, const Value& aValue2,
                              Value& aResultValue)
{
  Unit commonUnit =
    GetCommonUnit(aProperty, aValue1.GetUnit(), aValue2.GetUnit());
  
  
  
  

  switch (commonUnit) {
    case eUnit_Null:
    case eUnit_Auto:
    case eUnit_None:
    case eUnit_Normal:
    case eUnit_UnparsedString:
      return PR_FALSE;

    case eUnit_Enumerated:
      switch (aProperty) {
        case eCSSProperty_font_stretch: {
          
          PRInt32 result = NS_floor(aCoeff1 * double(aValue1.GetIntValue()) +
                                    aCoeff2 * double(aValue2.GetIntValue()));
          if (result < NS_STYLE_FONT_STRETCH_ULTRA_CONDENSED) {
            result = NS_STYLE_FONT_STRETCH_ULTRA_CONDENSED;
          } else if (result > NS_STYLE_FONT_STRETCH_ULTRA_EXPANDED) {
            result = NS_STYLE_FONT_STRETCH_ULTRA_EXPANDED;
          }
          aResultValue.SetIntValue(result, eUnit_Enumerated);
          return PR_TRUE;
        }
        default:
          return PR_FALSE;
      }
    case eUnit_Visibility: {
      PRInt32 val1 = aValue1.GetIntValue() == NS_STYLE_VISIBILITY_VISIBLE;
      PRInt32 val2 = aValue2.GetIntValue() == NS_STYLE_VISIBILITY_VISIBLE;
      double interp = aCoeff1 * val1 + aCoeff2 * val2;
      PRInt32 result = interp > 0.0 ? NS_STYLE_VISIBILITY_VISIBLE
                                    : NS_STYLE_VISIBILITY_HIDDEN;
      aResultValue.SetIntValue(result, eUnit_Visibility);
      return PR_TRUE;
    }
    case eUnit_Integer: {
      
      
      PRInt32 result = NS_floor(aCoeff1 * double(aValue1.GetIntValue()) +
                                aCoeff2 * double(aValue2.GetIntValue()));
      if (aProperty == eCSSProperty_font_weight) {
        NS_ASSERTION(result > 0, "unexpected value");
        result -= result % 100;
        if (result < 100) {
          result = 100;
        } else if (result > 900) {
          result = 900;
        }
      } else {
        result = RestrictValue(aProperty, result);
      }
      aResultValue.SetIntValue(result, eUnit_Integer);
      return PR_TRUE;
    }
    case eUnit_Coord: {
      aResultValue.SetCoordValue(RestrictValue(aProperty, NSToCoordRound(
        aCoeff1 * aValue1.GetCoordValue() +
        aCoeff2 * aValue2.GetCoordValue())));
      return PR_TRUE;
    }
    case eUnit_Percent: {
      aResultValue.SetPercentValue(RestrictValue(aProperty,
        aCoeff1 * aValue1.GetPercentValue() +
        aCoeff2 * aValue2.GetPercentValue()));
      return PR_TRUE;
    }
    case eUnit_Float: {
      aResultValue.SetFloatValue(RestrictValue(aProperty,
        aCoeff1 * aValue1.GetFloatValue() +
        aCoeff2 * aValue2.GetFloatValue()));
      return PR_TRUE;
    }
    case eUnit_Color: {
      nscolor color1 = aValue1.GetColorValue();
      nscolor color2 = aValue2.GetColorValue();
      
      
      
      

      
      
      double A1 = NS_GET_A(color1) * (1.0 / 255.0);
      double R1 = NS_GET_R(color1) * A1;
      double G1 = NS_GET_G(color1) * A1;
      double B1 = NS_GET_B(color1) * A1;
      double A2 = NS_GET_A(color2) * (1.0 / 255.0);
      double R2 = NS_GET_R(color2) * A2;
      double G2 = NS_GET_G(color2) * A2;
      double B2 = NS_GET_B(color2) * A2;
      double Aresf = (A1 * aCoeff1 + A2 * aCoeff2);
      nscolor resultColor;
      if (Aresf <= 0.0) {
        resultColor = NS_RGBA(0, 0, 0, 0);
      } else {
        if (Aresf > 1.0) {
          Aresf = 1.0;
        }
        double factor = 1.0 / Aresf;
        PRUint8 Ares = NSToIntRound(Aresf * 255.0);
        PRUint8 Rres = ClampColor((R1 * aCoeff1 + R2 * aCoeff2) * factor);
        PRUint8 Gres = ClampColor((G1 * aCoeff1 + G2 * aCoeff2) * factor);
        PRUint8 Bres = ClampColor((B1 * aCoeff1 + B2 * aCoeff2) * factor);
        resultColor = NS_RGBA(Rres, Gres, Bres, Ares);
      }
      aResultValue.SetColorValue(resultColor);
      return PR_TRUE;
    }
    case eUnit_Calc: {
      CalcValue v1 = ExtractCalcValue(aValue1);
      CalcValue v2 = ExtractCalcValue(aValue2);
      double len = aCoeff1 * v1.mLength + aCoeff2 * v2.mLength;
      double pct = aCoeff1 * v1.mPercent + aCoeff2 * v2.mPercent;
      PRBool hasPct = (aCoeff1 != 0.0 && v1.mHasPercent) ||
                      (aCoeff2 != 0.0 && v2.mHasPercent);
      nsCSSValue *val = new nsCSSValue();
      nsCSSValue::Array *arr = nsCSSValue::Array::Create(1);
      val->SetArrayValue(arr, eCSSUnit_Calc);
      if (hasPct) {
        nsCSSValue::Array *arr2 = nsCSSValue::Array::Create(2);
        arr2->Item(0).SetFloatValue(len, eCSSUnit_Pixel);
        arr2->Item(1).SetPercentValue(pct);
        arr->Item(0).SetArrayValue(arr2, eCSSUnit_Calc_Plus);
      } else {
        arr->Item(0).SetFloatValue(len, eCSSUnit_Pixel);
      }
      aResultValue.SetAndAdoptCSSValueValue(val, eUnit_Calc);
      return PR_TRUE;
    }
    case eUnit_CSSValuePair: {
      const nsCSSValuePair *pair1 = aValue1.GetCSSValuePairValue();
      const nsCSSValuePair *pair2 = aValue2.GetCSSValuePairValue();
      nsCSSUnit unit[2];
      unit[0] = GetCommonUnit(aProperty, pair1->mXValue.GetUnit(),
                              pair2->mXValue.GetUnit());
      unit[1] = GetCommonUnit(aProperty, pair1->mYValue.GetUnit(),
                              pair2->mYValue.GetUnit());
      if (unit[0] == eCSSUnit_Null || unit[1] == eCSSUnit_Null ||
          unit[0] == eCSSUnit_URL) {
        return PR_FALSE;
      }

      nsAutoPtr<nsCSSValuePair> result(new nsCSSValuePair);
      static nsCSSValue nsCSSValuePair::* const pairValues[2] = {
        &nsCSSValuePair::mXValue, &nsCSSValuePair::mYValue
      };
      PRUint32 restrictions = nsCSSProps::ValueRestrictions(aProperty);
      for (PRUint32 i = 0; i < 2; ++i) {
        nsCSSValue nsCSSValuePair::*member = pairValues[i];
        switch (unit[i]) {
          case eCSSUnit_Pixel:
            AddCSSValuePixel(aCoeff1, pair1->*member, aCoeff2, pair2->*member,
                             result->*member, restrictions);
            break;
          case eCSSUnit_Percent:
            AddCSSValuePercent(aCoeff1, pair1->*member,
                               aCoeff2, pair2->*member,
                               result->*member, restrictions);
            break;
          case eCSSUnit_Calc:
            AddCSSValueCanonicalCalc(aCoeff1, pair1->*member,
                                     aCoeff2, pair2->*member,
                                     result->*member);
            break;
          default:
            NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
            return PR_FALSE;
        }
      }

      aResultValue.SetAndAdoptCSSValuePairValue(result.forget(),
                                                eUnit_CSSValuePair);
      return PR_TRUE;
    }
    case eUnit_CSSRect: {
      NS_ABORT_IF_FALSE(nsCSSProps::ValueRestrictions(aProperty) == 0,
                        "must add code for handling value restrictions");
      const nsCSSRect *rect1 = aValue1.GetCSSRectValue();
      const nsCSSRect *rect2 = aValue2.GetCSSRectValue();
      if (rect1->mTop.GetUnit() != rect2->mTop.GetUnit() ||
          rect1->mRight.GetUnit() != rect2->mRight.GetUnit() ||
          rect1->mBottom.GetUnit() != rect2->mBottom.GetUnit() ||
          rect1->mLeft.GetUnit() != rect2->mLeft.GetUnit()) {
        
        return PR_FALSE;
      }

      nsAutoPtr<nsCSSRect> result(new nsCSSRect);
      for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(nsCSSRect::sides); ++i) {
        nsCSSValue nsCSSRect::*member = nsCSSRect::sides[i];
        NS_ABORT_IF_FALSE((rect1->*member).GetUnit() ==
                            (rect2->*member).GetUnit(),
                          "should have returned above");
        switch ((rect1->*member).GetUnit()) {
          case eCSSUnit_Pixel:
            AddCSSValuePixel(aCoeff1, rect1->*member, aCoeff2, rect2->*member,
                             result->*member);
            break;
          case eCSSUnit_Auto:
            if (float(aCoeff1 + aCoeff2) != 1.0f) {
              
              
              return PR_FALSE;
            }
            (result->*member).SetAutoValue();
            break;
          default:
            NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
            return PR_FALSE;
        }
      }

      aResultValue.SetAndAdoptCSSRectValue(result.forget(), eUnit_CSSRect);
      return PR_TRUE;
    }
    case eUnit_Dasharray: {
      const nsCSSValueList *list1 = aValue1.GetCSSValueListValue();
      const nsCSSValueList *list2 = aValue2.GetCSSValueListValue();

      PRUint32 len1 = 0, len2 = 0;
      for (const nsCSSValueList *v = list1; v; v = v->mNext) {
        ++len1;
      }
      for (const nsCSSValueList *v = list2; v; v = v->mNext) {
        ++len2;
      }
      NS_ABORT_IF_FALSE(len1 > 0 && len2 > 0, "unexpected length");
      if (list1->mValue.GetUnit() == eCSSUnit_None ||
          list2->mValue.GetUnit() == eCSSUnit_None) {
        
        NS_ABORT_IF_FALSE(
          (list1->mValue.GetUnit() != eCSSUnit_None || len1 == 1) &&
          (list2->mValue.GetUnit() != eCSSUnit_None || len2 == 1),
          "multi-value valuelist with 'none' as first element");
        return PR_FALSE;
      }

      nsAutoPtr<nsCSSValueList> result;
      nsCSSValueList **resultTail = getter_Transfers(result);
      for (PRUint32 i = 0, i_end = lcm(len1, len2); i != i_end; ++i) {
        const nsCSSValue &v1 = list1->mValue;
        const nsCSSValue &v2 = list2->mValue;
        NS_ABORT_IF_FALSE(v1.GetUnit() == eCSSUnit_Number ||
                          v1.GetUnit() == eCSSUnit_Percent, "unexpected");
        NS_ABORT_IF_FALSE(v2.GetUnit() == eCSSUnit_Number ||
                          v2.GetUnit() == eCSSUnit_Percent, "unexpected");
        if (v1.GetUnit() != v2.GetUnit()) {
          
          return PR_FALSE;
        }

        nsCSSValueList *item = new nsCSSValueList;
        if (!item) {
          return PR_FALSE;
        }
        *resultTail = item;
        resultTail = &item->mNext;

        if (v1.GetUnit() == eCSSUnit_Number) {
          AddCSSValueNumber(aCoeff1, v1, aCoeff2, v2, item->mValue,
                            CSS_PROPERTY_VALUE_NONNEGATIVE);
        } else {
          AddCSSValuePercent(aCoeff1, v1, aCoeff2, v2, item->mValue,
                             CSS_PROPERTY_VALUE_NONNEGATIVE);
        }

        list1 = list1->mNext;
        if (!list1) {
          list1 = aValue1.GetCSSValueListValue();
        }
        list2 = list2->mNext;
        if (!list2) {
          list2 = aValue2.GetCSSValueListValue();
        }
      }

      aResultValue.SetAndAdoptCSSValueListValue(result.forget(),
                                                eUnit_Dasharray);
      return PR_TRUE;
    }
    case eUnit_Shadow: {
      
      
      
      
      const nsCSSValueList *shadow1 = aValue1.GetCSSValueListValue();
      const nsCSSValueList *shadow2 = aValue2.GetCSSValueListValue();
      nsAutoPtr<nsCSSValueList> result;
      nsCSSValueList **resultTail = getter_Transfers(result);
      while (shadow1 && shadow2) {
        if (!AddShadowItems(aCoeff1, shadow1->mValue,
                            aCoeff2, shadow2->mValue,
                            resultTail)) {
          return PR_FALSE;
        }
        shadow1 = shadow1->mNext;
        shadow2 = shadow2->mNext;
      }
      if (shadow1 || shadow2) {
        const nsCSSValueList *longShadow;
        double longCoeff;
        if (shadow1) {
          longShadow = shadow1;
          longCoeff = aCoeff1;
        } else {
          longShadow = shadow2;
          longCoeff = aCoeff2;
        }

        while (longShadow) {
          
          
          
          if (!AddShadowItems(longCoeff, longShadow->mValue,
                              0.0, longShadow->mValue,
                              resultTail)) {
            return PR_FALSE;
          }

          longShadow = longShadow->mNext;
        }
      }
      aResultValue.SetAndAdoptCSSValueListValue(result.forget(), eUnit_Shadow);
      return PR_TRUE;
    }
    case eUnit_Transform: {
      const nsCSSValueList *list1 = aValue1.GetCSSValueListValue();
      const nsCSSValueList *list2 = aValue2.GetCSSValueListValue();

      
      
      
      
      
      
      
      nsAutoPtr<nsCSSValueList> result;
      if (list1->mValue.GetUnit() == eCSSUnit_None) {
        if (list2->mValue.GetUnit() == eCSSUnit_None) {
          result = new nsCSSValueList;
          if (result) {
            result->mValue.SetNoneValue();
          }
        } else {
          result = AddTransformLists(list2, aCoeff2, list2, 0);
        }
      } else {
        if (list2->mValue.GetUnit() == eCSSUnit_None) {
          result = AddTransformLists(list1, aCoeff1, list1, 0);
        } else {
          PRBool match = PR_TRUE;

          {
            const nsCSSValueList *item1 = list1, *item2 = list2;
            do {
              nsCSSKeyword func1 = nsStyleTransformMatrix::TransformFunctionOf(
                                     item1->mValue.GetArrayValue());
              nsCSSKeyword func2 = nsStyleTransformMatrix::TransformFunctionOf(
                                     item2->mValue.GetArrayValue());
              if (func1 != func2) {
                break;
              }

              item1 = item1->mNext;
              item2 = item2->mNext;
            } while (item1 && item2);
            if (item1 || item2) {
              
              match = PR_FALSE;
            }
          }

          if (match) {
            result = AddTransformLists(list1, aCoeff1, list2, aCoeff2);
          } else {
            PRBool dummy;
            nsStyleTransformMatrix matrix1 =
              nsStyleTransformMatrix::ReadTransforms(list1, nsnull, nsnull,
                                                     dummy),
                                   matrix2 =
              nsStyleTransformMatrix::ReadTransforms(list2, nsnull, nsnull,
                                                     dummy);
            result = AddTransformMatrix(matrix1, aCoeff1, matrix2, aCoeff2);
          }
        }
      }

      aResultValue.SetAndAdoptCSSValueListValue(result.forget(),
                                                eUnit_Transform);
      return PR_TRUE;
    }
    case eUnit_CSSValuePairList: {
      const nsCSSValuePairList *list1 = aValue1.GetCSSValuePairListValue();
      const nsCSSValuePairList *list2 = aValue2.GetCSSValuePairListValue();
      nsAutoPtr<nsCSSValuePairList> result;
      nsCSSValuePairList **resultTail = getter_Transfers(result);
      do {
        nsCSSValuePairList *item = new nsCSSValuePairList;
        if (!item) {
          return PR_FALSE;
        }
        *resultTail = item;
        resultTail = &item->mNext;

        static nsCSSValue nsCSSValuePairList::* const pairListValues[] = {
          &nsCSSValuePairList::mXValue,
          &nsCSSValuePairList::mYValue,
        };
        PRUint32 restrictions = nsCSSProps::ValueRestrictions(aProperty);
        for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(pairListValues); ++i) {
          const nsCSSValue &v1 = list1->*(pairListValues[i]);
          const nsCSSValue &v2 = list2->*(pairListValues[i]);
          nsCSSValue &vr = item->*(pairListValues[i]);
          nsCSSUnit unit =
            GetCommonUnit(aProperty, v1.GetUnit(), v2.GetUnit());
          if (unit == eCSSUnit_Null) {
            return PR_FALSE;
          }
          switch (unit) {
            case eCSSUnit_Pixel:
              AddCSSValuePixel(aCoeff1, v1, aCoeff2, v2, vr, restrictions);
              break;
            case eCSSUnit_Percent:
              AddCSSValuePercent(aCoeff1, v1, aCoeff2, v2, vr, restrictions);
              break;
            case eCSSUnit_Calc:
              AddCSSValueCanonicalCalc(aCoeff1, v1, aCoeff2, v2, vr);
              break;
            default:
              if (v1 != v2) {
                return PR_FALSE;
              }
              vr = v1;
              break;
          }
        }
        list1 = list1->mNext;
        list2 = list2->mNext;
      } while (list1 && list2);
      if (list1 || list2) {
        
        return PR_FALSE;
      }

      aResultValue.SetAndAdoptCSSValuePairListValue(result.forget());
      return PR_TRUE;
    }
  }

  NS_ABORT_IF_FALSE(false, "Can't interpolate using the given common unit");
  return PR_FALSE;
}

already_AddRefed<css::StyleRule>
BuildStyleRule(nsCSSProperty aProperty,
               dom::Element* aTargetElement,
               const nsAString& aSpecifiedValue,
               PRBool aUseSVGMode)
{
  
  nsAutoPtr<css::Declaration> declaration(new css::Declaration());
  declaration->InitializeEmpty();

  PRBool changed; 
  nsIDocument* doc = aTargetElement->GetOwnerDoc();
  nsCOMPtr<nsIURI> baseURI = aTargetElement->GetBaseURI();
  nsCSSParser parser(doc->CSSLoader());

  if (aUseSVGMode) {
#ifdef MOZ_SVG
    parser.SetSVGMode(PR_TRUE);
#else
    NS_NOTREACHED("aUseSVGMode should not be set");
#endif
  }

  nsCSSProperty propertyToCheck = nsCSSProps::IsShorthand(aProperty) ?
    nsCSSProps::SubpropertyEntryFor(aProperty)[0] : aProperty;

  
  
  if (NS_FAILED(parser.ParseProperty(aProperty, aSpecifiedValue,
                                     doc->GetDocumentURI(), baseURI,
                                     aTargetElement->NodePrincipal(),
                                     declaration, &changed, PR_FALSE)) ||
      
      !declaration->HasNonImportantValueFor(propertyToCheck)) {
    NS_WARNING("failure in BuildStyleRule");
    return nsnull;
  }

  nsRefPtr<css::StyleRule> rule = new css::StyleRule(nsnull, declaration.forget());
  return rule.forget();
}

inline
already_AddRefed<nsStyleContext>
LookupStyleContext(dom::Element* aElement)
{
  nsIDocument* doc = aElement->GetCurrentDoc();
  nsIPresShell* shell = doc->GetShell();
  if (!shell) {
    return nsnull;
  }
  return nsComputedDOMStyle::GetStyleContextForElement(aElement, nsnull, shell);
}





















already_AddRefed<nsStyleContext>
StyleWithDeclarationAdded(nsCSSProperty aProperty,
                          dom::Element* aTargetElement,
                          const nsAString& aSpecifiedValue,
                          PRBool aUseSVGMode)
{
  NS_ABORT_IF_FALSE(aTargetElement, "null target element");
  NS_ABORT_IF_FALSE(aTargetElement->GetCurrentDoc(),
                    "element needs to be in a document "
                    "if we're going to look up its style context");

  
  nsRefPtr<nsStyleContext> styleContext = LookupStyleContext(aTargetElement);
  if (!styleContext) {
    return nsnull;
  }

  
  nsRefPtr<css::StyleRule> styleRule =
    BuildStyleRule(aProperty, aTargetElement, aSpecifiedValue, aUseSVGMode);
  if (!styleRule) {
    return nsnull;
  }

  styleRule->RuleMatched();

  
  nsCOMArray<nsIStyleRule> ruleArray;
  ruleArray.AppendObject(styleRule);
  nsStyleSet* styleSet = styleContext->PresContext()->StyleSet();
  return styleSet->ResolveStyleByAddingRules(styleContext, ruleArray);
}

PRBool
nsStyleAnimation::ComputeValue(nsCSSProperty aProperty,
                               dom::Element* aTargetElement,
                               const nsAString& aSpecifiedValue,
                               PRBool aUseSVGMode,
                               Value& aComputedValue)
{
  NS_ABORT_IF_FALSE(aTargetElement, "null target element");
  NS_ABORT_IF_FALSE(aTargetElement->GetCurrentDoc(),
                    "we should only be able to actively animate nodes that "
                    "are in a document");

  nsCSSProperty propToParse =
    nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_REPORT_OTHER_NAME)
      ? nsCSSProps::OtherNameFor(aProperty) : aProperty;

  nsRefPtr<nsStyleContext> tmpStyleContext =
    StyleWithDeclarationAdded(propToParse, aTargetElement,
                              aSpecifiedValue, aUseSVGMode);
  if (!tmpStyleContext) {
    return PR_FALSE;
  }

 if (nsCSSProps::IsShorthand(aProperty) ||
     nsCSSProps::kAnimTypeTable[aProperty] == eStyleAnimType_None) {
    
    aComputedValue.SetUnparsedStringValue(nsString(aSpecifiedValue));
    return PR_TRUE;
  }
  
  return ExtractComputedValue(aProperty, tmpStyleContext, aComputedValue);
}

PRBool
nsStyleAnimation::UncomputeValue(nsCSSProperty aProperty,
                                 nsPresContext* aPresContext,
                                 const Value& aComputedValue,
                                 nsCSSValue& aSpecifiedValue)
{
  NS_ABORT_IF_FALSE(aPresContext, "null pres context");

  switch (aComputedValue.GetUnit()) {
    case eUnit_Normal:
      aSpecifiedValue.SetNormalValue();
      break;
    case eUnit_Auto:
      aSpecifiedValue.SetAutoValue();
      break;
    case eUnit_None:
      aSpecifiedValue.SetNoneValue();
      break;
    case eUnit_Enumerated:
    case eUnit_Visibility:
      aSpecifiedValue.
        SetIntValue(aComputedValue.GetIntValue(), eCSSUnit_Enumerated);
      break;
    case eUnit_Integer:
      aSpecifiedValue.
        SetIntValue(aComputedValue.GetIntValue(), eCSSUnit_Integer);
      break;
    case eUnit_Coord:
      nscoordToCSSValue(aComputedValue.GetCoordValue(), aSpecifiedValue);
      break;
    case eUnit_Percent:
      aSpecifiedValue.SetPercentValue(aComputedValue.GetPercentValue());
      break;
    case eUnit_Float:
      aSpecifiedValue.
        SetFloatValue(aComputedValue.GetFloatValue(), eCSSUnit_Number);
      break;
    case eUnit_Color:
      
      aSpecifiedValue.SetColorValue(aComputedValue.GetColorValue());
      break;
    case eUnit_Calc: {
      nsCSSValue *val = aComputedValue.GetCSSValueValue();
      NS_ABORT_IF_FALSE(val->GetUnit() == eCSSUnit_Calc, "unexpected unit");
      aSpecifiedValue = *val;
      break;
    }
    case eUnit_CSSValuePair: {
      
      
      
      
      const nsCSSValuePair* pair = aComputedValue.GetCSSValuePairValue();
      if (pair->mXValue == pair->mYValue) {
        aSpecifiedValue = pair->mXValue;
      } else {
        aSpecifiedValue.SetPairValue(pair);
      }
    } break;
    case eUnit_CSSRect: {
      nsCSSRect& rect = aSpecifiedValue.SetRectValue();
      rect = *aComputedValue.GetCSSRectValue();
    } break;
    case eUnit_Dasharray:
    case eUnit_Shadow:
    case eUnit_Transform:
      aSpecifiedValue.
        SetDependentListValue(aComputedValue.GetCSSValueListValue());
      break;
    case eUnit_CSSValuePairList:
      aSpecifiedValue.
        SetDependentPairListValue(aComputedValue.GetCSSValuePairListValue());
      break;
    default:
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsStyleAnimation::UncomputeValue(nsCSSProperty aProperty,
                                 nsPresContext* aPresContext,
                                 const Value& aComputedValue,
                                 nsAString& aSpecifiedValue)
{
  NS_ABORT_IF_FALSE(aPresContext, "null pres context");
  aSpecifiedValue.Truncate(); 

  if (aComputedValue.GetUnit() == eUnit_UnparsedString) {
    aComputedValue.GetStringValue(aSpecifiedValue);
    return PR_TRUE;
  }
  nsCSSValue val;
  if (!nsStyleAnimation::UncomputeValue(aProperty, aPresContext,
                                        aComputedValue, val)) {
    return PR_FALSE;
  }

  val.AppendToString(aProperty, aSpecifiedValue);
  return PR_TRUE;
}

inline const void*
StyleDataAtOffset(const void* aStyleStruct, ptrdiff_t aOffset)
{
  return reinterpret_cast<const char*>(aStyleStruct) + aOffset;
}

inline void*
StyleDataAtOffset(void* aStyleStruct, ptrdiff_t aOffset)
{
  return reinterpret_cast<char*>(aStyleStruct) + aOffset;
}

static void
ExtractBorderColor(nsStyleContext* aStyleContext, const void* aStyleBorder,
                   mozilla::css::Side aSide, nsStyleAnimation::Value& aComputedValue)
{
  nscolor color;
  PRBool foreground;
  static_cast<const nsStyleBorder*>(aStyleBorder)->
    GetBorderColor(aSide, color, foreground);
  if (foreground) {
    
    color = aStyleContext->GetStyleColor()->mColor;
  }
  aComputedValue.SetColorValue(color);
}

static PRBool
StyleCoordToValue(const nsStyleCoord& aCoord, nsStyleAnimation::Value& aValue)
{
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Normal:
      aValue.SetNormalValue();
      break;
    case eStyleUnit_Auto:
      aValue.SetAutoValue();
      break;
    case eStyleUnit_None:
      aValue.SetNoneValue();
      break;
    case eStyleUnit_Percent:
      aValue.SetPercentValue(aCoord.GetPercentValue());
      break;
    case eStyleUnit_Factor:
      aValue.SetFloatValue(aCoord.GetFactorValue());
      break;
    case eStyleUnit_Coord:
      aValue.SetCoordValue(aCoord.GetCoordValue());
      break;
    case eStyleUnit_Enumerated:
      aValue.SetIntValue(aCoord.GetIntValue(),
                         nsStyleAnimation::eUnit_Enumerated);
      break;
    case eStyleUnit_Integer:
      aValue.SetIntValue(aCoord.GetIntValue(),
                         nsStyleAnimation::eUnit_Integer);
      break;
    case eStyleUnit_Calc: {
      nsAutoPtr<nsCSSValue> val(new nsCSSValue);
      SetCalcValue(aCoord.GetCalcValue(), *val);
      aValue.SetAndAdoptCSSValueValue(val.forget(),
                                      nsStyleAnimation::eUnit_Calc);
      break;
    }
    default:
      return PR_FALSE;
  }
  return PR_TRUE;
}

static PRBool
StyleCoordToCSSValue(const nsStyleCoord& aCoord, nsCSSValue& aCSSValue)
{
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Coord:
      nscoordToCSSValue(aCoord.GetCoordValue(), aCSSValue);
      break;
    case eStyleUnit_Percent:
      aCSSValue.SetPercentValue(aCoord.GetPercentValue());
      break;
    case eStyleUnit_Calc:
      SetCalcValue(aCoord.GetCalcValue(), aCSSValue);
      break;
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
      return PR_FALSE;
  }
  return PR_TRUE;
}






static void
SubstitutePixelValues(nsStyleContext* aStyleContext,
                      const nsCSSValue& aInput, nsCSSValue& aOutput)
{
  if (aInput.IsCalcUnit()) {
    PRBool canStoreInRuleTree = PR_TRUE;
    nsRuleNode::ComputedCalc c =
      nsRuleNode::SpecifiedCalcToComputedCalc(aInput, aStyleContext,
                                              aStyleContext->PresContext(),
                                              canStoreInRuleTree);
    nsStyleCoord::Calc c2;
    c2.mLength = c.mLength;
    c2.mPercent = c.mPercent;
    c2.mHasPercent = PR_TRUE; 
    SetCalcValue(&c2, aOutput);
  } else if (aInput.UnitHasArrayValue()) {
    const nsCSSValue::Array *inputArray = aInput.GetArrayValue();
    nsRefPtr<nsCSSValue::Array> outputArray =
      nsCSSValue::Array::Create(inputArray->Count());
    for (size_t i = 0, i_end = inputArray->Count(); i < i_end; ++i) {
      SubstitutePixelValues(aStyleContext,
                            inputArray->Item(i), outputArray->Item(i));
    }
    aOutput.SetArrayValue(outputArray, aInput.GetUnit());
  } else if (aInput.IsLengthUnit() &&
             aInput.GetUnit() != eCSSUnit_Pixel) {
    PRBool canStoreInRuleTree = PR_TRUE;
    nscoord len = nsRuleNode::CalcLength(aInput, aStyleContext,
                                         aStyleContext->PresContext(),
                                         canStoreInRuleTree);
    aOutput.SetFloatValue(nsPresContext::AppUnitsToFloatCSSPixels(len),
                          eCSSUnit_Pixel);
  } else {
    aOutput = aInput;
  }
}

PRBool
nsStyleAnimation::ExtractComputedValue(nsCSSProperty aProperty,
                                       nsStyleContext* aStyleContext,
                                       Value& aComputedValue)
{
  NS_ABORT_IF_FALSE(0 <= aProperty &&
                    aProperty < eCSSProperty_COUNT_no_shorthands,
                    "bad property");
  const void* styleStruct =
    aStyleContext->GetStyleData(nsCSSProps::kSIDTable[aProperty]);
  ptrdiff_t ssOffset = nsCSSProps::kStyleStructOffsetTable[aProperty];
  nsStyleAnimType animType = nsCSSProps::kAnimTypeTable[aProperty];
  NS_ABORT_IF_FALSE(0 <= ssOffset || animType == eStyleAnimType_Custom,
                    "must be dealing with animatable property");
  switch (animType) {
    case eStyleAnimType_Custom:
      switch (aProperty) {
        
        
        

        #define BORDER_WIDTH_CASE(prop_, side_)                               \
        case prop_:                                                           \
          aComputedValue.SetCoordValue(                                       \
            static_cast<const nsStyleBorder*>(styleStruct)->                  \
              GetComputedBorder().side_);                                     \
          break;
        BORDER_WIDTH_CASE(eCSSProperty_border_bottom_width, bottom)
        BORDER_WIDTH_CASE(eCSSProperty_border_left_width_value, left)
        BORDER_WIDTH_CASE(eCSSProperty_border_right_width_value, right)
        BORDER_WIDTH_CASE(eCSSProperty_border_top_width, top)
        #undef BORDER_WIDTH_CASE

        case eCSSProperty__moz_column_rule_width:
          aComputedValue.SetCoordValue(
            static_cast<const nsStyleColumn*>(styleStruct)->
              GetComputedColumnRuleWidth());
          break;

        case eCSSProperty_border_bottom_color:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_BOTTOM,
                             aComputedValue);
          break;
        case eCSSProperty_border_left_color_value:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_LEFT,
                             aComputedValue);
          break;
        case eCSSProperty_border_right_color_value:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_RIGHT,
                             aComputedValue);
          break;
        case eCSSProperty_border_top_color:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_TOP,
                             aComputedValue);
          break;

        case eCSSProperty_outline_color: {
          const nsStyleOutline *styleOutline =
            static_cast<const nsStyleOutline*>(styleStruct);
          nscolor color;
        #ifdef GFX_HAS_INVERT
          
          
          styleOutline->GetOutlineColor(color);
        #else
          if (!styleOutline->GetOutlineColor(color))
            color = aStyleContext->GetStyleColor()->mColor;
        #endif
          aComputedValue.SetColorValue(color);
          break;
        }

        case eCSSProperty__moz_column_rule_color: {
          const nsStyleColumn *styleColumn =
            static_cast<const nsStyleColumn*>(styleStruct);
          nscolor color;
          if (styleColumn->mColumnRuleColorIsForeground) {
            color = aStyleContext->GetStyleColor()->mColor;
          } else {
            color = styleColumn->mColumnRuleColor;
          }
          aComputedValue.SetColorValue(color);
          break;
        }

        case eCSSProperty__moz_column_count: {
          const nsStyleColumn *styleColumn =
            static_cast<const nsStyleColumn*>(styleStruct);
          if (styleColumn->mColumnCount == NS_STYLE_COLUMN_COUNT_AUTO) {
            aComputedValue.SetAutoValue();
          } else {
            aComputedValue.SetIntValue(styleColumn->mColumnCount,
                                       eUnit_Integer);
          }
          break;
        }

        case eCSSProperty_text_decoration_color: {
          const nsStyleTextReset *styleTextReset =
            static_cast<const nsStyleTextReset*>(styleStruct);
          nscolor color;
          PRBool isForeground;
          styleTextReset->GetDecorationColor(color, isForeground);
          if (isForeground) {
            color = aStyleContext->GetStyleColor()->mColor;
          }
          aComputedValue.SetColorValue(color);
          break;
        }

        case eCSSProperty_text_decoration_style: {
          PRUint8 decorationStyle =
            static_cast<const nsStyleTextReset*>(styleStruct)->
              GetDecorationStyle();
          aComputedValue.SetIntValue(decorationStyle, eUnit_Enumerated);
          break;
        }

        case eCSSProperty_border_spacing: {
          const nsStyleTableBorder *styleTableBorder =
            static_cast<const nsStyleTableBorder*>(styleStruct);
          nsAutoPtr<nsCSSValuePair> pair(new nsCSSValuePair);
          if (!pair) {
            return PR_FALSE;
          }
          nscoordToCSSValue(styleTableBorder->mBorderSpacingX, pair->mXValue);
          nscoordToCSSValue(styleTableBorder->mBorderSpacingY, pair->mYValue);
          aComputedValue.SetAndAdoptCSSValuePairValue(pair.forget(),
                                                      eUnit_CSSValuePair);
          break;
        }

        case eCSSProperty__moz_transform_origin: {
          const nsStyleDisplay *styleDisplay =
            static_cast<const nsStyleDisplay*>(styleStruct);
          nsAutoPtr<nsCSSValuePair> pair(new nsCSSValuePair);
          if (!pair ||
              !StyleCoordToCSSValue(styleDisplay->mTransformOrigin[0],
                                    pair->mXValue) ||
              !StyleCoordToCSSValue(styleDisplay->mTransformOrigin[1],
                                    pair->mYValue)) {
            return PR_FALSE;
          }
          aComputedValue.SetAndAdoptCSSValuePairValue(pair.forget(),
                                                      eUnit_CSSValuePair);
          break;
        }

        case eCSSProperty_stroke_dasharray: {
          const nsStyleSVG *svg = static_cast<const nsStyleSVG*>(styleStruct);
          NS_ABORT_IF_FALSE((svg->mStrokeDasharray != nsnull) ==
                            (svg->mStrokeDasharrayLength != 0),
                            "pointer/length mismatch");
          nsAutoPtr<nsCSSValueList> result;
          if (svg->mStrokeDasharray) {
            NS_ABORT_IF_FALSE(svg->mStrokeDasharrayLength > 0,
                              "non-null list should have positive length");
            nsCSSValueList **resultTail = getter_Transfers(result);
            for (PRUint32 i = 0, i_end = svg->mStrokeDasharrayLength;
                 i != i_end; ++i) {
              nsCSSValueList *item = new nsCSSValueList;
              if (!item) {
                return PR_FALSE;
              }
              *resultTail = item;
              resultTail = &item->mNext;

              const nsStyleCoord &coord = svg->mStrokeDasharray[i];
              nsCSSValue &value = item->mValue;
              switch (coord.GetUnit()) {
                case eStyleUnit_Coord:
                  
                  
                  
                  value.SetFloatValue(nsPresContext::
                    AppUnitsToFloatCSSPixels(coord.GetCoordValue()),
                    eCSSUnit_Number);
                  break;
                case eStyleUnit_Factor:
                  value.SetFloatValue(coord.GetFactorValue(),
                                      eCSSUnit_Number);
                  break;
                case eStyleUnit_Percent:
                  value.SetPercentValue(coord.GetPercentValue());
                  break;
                default:
                  NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
                  return PR_FALSE;
              }
            }
          } else {
            result = new nsCSSValueList;
            if (!result) {
              return PR_FALSE;
            }
            result->mValue.SetNoneValue();
          }
          aComputedValue.SetAndAdoptCSSValueListValue(result.forget(),
                                                      eUnit_Dasharray);
          break;
        }

        case eCSSProperty_font_stretch: {
          PRInt16 stretch =
            static_cast<const nsStyleFont*>(styleStruct)->mFont.stretch;
          PR_STATIC_ASSERT(NS_STYLE_FONT_STRETCH_ULTRA_CONDENSED == -4);
          PR_STATIC_ASSERT(NS_STYLE_FONT_STRETCH_ULTRA_EXPANDED == 4);
          if (stretch < NS_STYLE_FONT_STRETCH_ULTRA_CONDENSED ||
              stretch > NS_STYLE_FONT_STRETCH_ULTRA_EXPANDED) {
            return PR_FALSE;
          }
          aComputedValue.SetIntValue(stretch, eUnit_Enumerated);
          return PR_TRUE;
        }

        case eCSSProperty_font_weight: {
          PRUint16 weight =
            static_cast<const nsStyleFont*>(styleStruct)->mFont.weight;
          if (weight % 100 != 0) {
            return PR_FALSE;
          }
          aComputedValue.SetIntValue(weight, eUnit_Integer);
          return PR_TRUE;
        }

        case eCSSProperty_image_region: {
          const nsStyleList *list =
            static_cast<const nsStyleList*>(styleStruct);
          const nsRect &srect = list->mImageRegion;
          if (srect.IsEmpty()) {
            aComputedValue.SetAutoValue();
            break;
          }

          nsCSSRect *vrect = new nsCSSRect;
          nscoordToCSSValue(srect.x, vrect->mLeft);
          nscoordToCSSValue(srect.y, vrect->mTop);
          nscoordToCSSValue(srect.XMost(), vrect->mRight);
          nscoordToCSSValue(srect.YMost(), vrect->mBottom);
          aComputedValue.SetAndAdoptCSSRectValue(vrect, eUnit_CSSRect);
          break;
        }

        case eCSSProperty_clip: {
          const nsStyleDisplay *display =
            static_cast<const nsStyleDisplay*>(styleStruct);
          if (!(display->mClipFlags & NS_STYLE_CLIP_RECT)) {
            aComputedValue.SetAutoValue();
          } else {
            nsCSSRect *vrect = new nsCSSRect;
            const nsRect &srect = display->mClip;
            if (display->mClipFlags & NS_STYLE_CLIP_TOP_AUTO) {
              vrect->mTop.SetAutoValue();
            } else {
              nscoordToCSSValue(srect.y, vrect->mTop);
            }
            if (display->mClipFlags & NS_STYLE_CLIP_RIGHT_AUTO) {
              vrect->mRight.SetAutoValue();
            } else {
              nscoordToCSSValue(srect.XMost(), vrect->mRight);
            }
            if (display->mClipFlags & NS_STYLE_CLIP_BOTTOM_AUTO) {
              vrect->mBottom.SetAutoValue();
            } else {
              nscoordToCSSValue(srect.YMost(), vrect->mBottom);
            }
            if (display->mClipFlags & NS_STYLE_CLIP_LEFT_AUTO) {
              vrect->mLeft.SetAutoValue();
            } else {
              nscoordToCSSValue(srect.x, vrect->mLeft);
            }
            aComputedValue.SetAndAdoptCSSRectValue(vrect, eUnit_CSSRect);
          }
          break;
        }

        case eCSSProperty_background_position: {
          const nsStyleBackground *bg =
            static_cast<const nsStyleBackground*>(styleStruct);
          nsAutoPtr<nsCSSValuePairList> result;
          nsCSSValuePairList **resultTail = getter_Transfers(result);
          NS_ABORT_IF_FALSE(bg->mPositionCount > 0, "unexpected count");
          for (PRUint32 i = 0, i_end = bg->mPositionCount; i != i_end; ++i) {
            nsCSSValuePairList *item = new nsCSSValuePairList;
            *resultTail = item;
            resultTail = &item->mNext;

            const nsStyleBackground::Position &pos = bg->mLayers[i].mPosition;
            
            
            if (!pos.mXPosition.mHasPercent) {
              NS_ABORT_IF_FALSE(pos.mXPosition.mPercent == 0.0f,
                                "Shouldn't have mPercent!");
              nscoordToCSSValue(pos.mXPosition.mLength, item->mXValue);
            } else if (pos.mXPosition.mLength == 0) {
              item->mXValue.SetPercentValue(pos.mXPosition.mPercent);
            } else {
              SetCalcValue(&pos.mXPosition, item->mXValue);
            }
            if (!pos.mYPosition.mHasPercent) {
              NS_ABORT_IF_FALSE(pos.mYPosition.mPercent == 0.0f,
                                "Shouldn't have mPercent!");
              nscoordToCSSValue(pos.mYPosition.mLength, item->mYValue);
            } else if (pos.mYPosition.mLength == 0) {
              item->mYValue.SetPercentValue(pos.mYPosition.mPercent);
            } else {
              SetCalcValue(&pos.mYPosition, item->mYValue);
            }
          }

          aComputedValue.SetAndAdoptCSSValuePairListValue(result.forget());
          break;
        }

        case eCSSProperty_background_size: {
          const nsStyleBackground *bg =
            static_cast<const nsStyleBackground*>(styleStruct);
          nsAutoPtr<nsCSSValuePairList> result;
          nsCSSValuePairList **resultTail = getter_Transfers(result);
          NS_ABORT_IF_FALSE(bg->mSizeCount > 0, "unexpected count");
          for (PRUint32 i = 0, i_end = bg->mSizeCount; i != i_end; ++i) {
            nsCSSValuePairList *item = new nsCSSValuePairList;
            *resultTail = item;
            resultTail = &item->mNext;

            const nsStyleBackground::Size &size = bg->mLayers[i].mSize;
            switch (size.mWidthType) {
              case nsStyleBackground::Size::eContain:
              case nsStyleBackground::Size::eCover:
                item->mXValue.SetIntValue(size.mWidthType,
                                          eCSSUnit_Enumerated);
                break;
              case nsStyleBackground::Size::eAuto:
                item->mXValue.SetAutoValue();
                break;
              case nsStyleBackground::Size::eLengthPercentage:
                if (size.mWidth.mLength == 0) {
                  item->mXValue.SetPercentValue(size.mWidth.mPercent);
                } else if (size.mWidth.mPercent == 0.0f) {
                  nscoordToCSSValue(size.mWidth.mLength, item->mXValue);
                } else {
                  nsStyleCoord::Calc calc;
                  calc.mLength = size.mWidth.mLength;
                  calc.mPercent = size.mWidth.mPercent;
                  calc.mHasPercent = PR_TRUE;
                  SetCalcValue(&calc, item->mXValue);
                }
                break;
            }

            switch (size.mHeightType) {
              case nsStyleBackground::Size::eContain:
              case nsStyleBackground::Size::eCover:
                
                break;
              case nsStyleBackground::Size::eAuto:
                item->mYValue.SetAutoValue();
                break;
              case nsStyleBackground::Size::eLengthPercentage:
                if (size.mHeight.mLength == 0) {
                  item->mYValue.SetPercentValue(size.mHeight.mPercent);
                } else if (size.mHeight.mPercent == 0.0f) {
                  nscoordToCSSValue(size.mHeight.mLength, item->mYValue);
                } else {
                  nsStyleCoord::Calc calc;
                  calc.mLength = size.mHeight.mLength;
                  calc.mPercent = size.mHeight.mPercent;
                  calc.mHasPercent = PR_TRUE;
                  SetCalcValue(&calc, item->mYValue);
                }
                break;
            }
          }

          aComputedValue.SetAndAdoptCSSValuePairListValue(result.forget());
          break;
        }

        case eCSSProperty__moz_transform: {
          const nsStyleDisplay *display =
            static_cast<const nsStyleDisplay*>(styleStruct);
          nsAutoPtr<nsCSSValueList> result;
          if (display->mSpecifiedTransform) {
            
            nsCSSValueList **resultTail = getter_Transfers(result);
            for (const nsCSSValueList *l = display->mSpecifiedTransform;
                 l; l = l->mNext) {
              nsCSSValueList *clone = new nsCSSValueList;
              *resultTail = clone;
              resultTail = &clone->mNext;

              SubstitutePixelValues(aStyleContext, l->mValue, clone->mValue);
            }
          } else {
            result = new nsCSSValueList();
            result->mValue.SetNoneValue();
          }

          aComputedValue.SetAndAdoptCSSValueListValue(result.forget(),
                                                      eUnit_Transform);
          break;
        }

        default:
          NS_ABORT_IF_FALSE(PR_FALSE, "missing property implementation");
          return PR_FALSE;
      };
      return PR_TRUE;
    case eStyleAnimType_Coord:
      return StyleCoordToValue(*static_cast<const nsStyleCoord*>(
        StyleDataAtOffset(styleStruct, ssOffset)), aComputedValue);
    case eStyleAnimType_Sides_Top:
    case eStyleAnimType_Sides_Right:
    case eStyleAnimType_Sides_Bottom:
    case eStyleAnimType_Sides_Left: {
      PR_STATIC_ASSERT(0 == NS_SIDE_TOP);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Right - eStyleAnimType_Sides_Top
                         == NS_SIDE_RIGHT);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Bottom - eStyleAnimType_Sides_Top
                         == NS_SIDE_BOTTOM);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Left - eStyleAnimType_Sides_Top
                         == NS_SIDE_LEFT);
      const nsStyleCoord &coord = static_cast<const nsStyleSides*>(
        StyleDataAtOffset(styleStruct, ssOffset))->
          Get(mozilla::css::Side(animType - eStyleAnimType_Sides_Top));
      return StyleCoordToValue(coord, aComputedValue);
    }
    case eStyleAnimType_Corner_TopLeft:
    case eStyleAnimType_Corner_TopRight:
    case eStyleAnimType_Corner_BottomRight:
    case eStyleAnimType_Corner_BottomLeft: {
      PR_STATIC_ASSERT(0 == NS_CORNER_TOP_LEFT);
      PR_STATIC_ASSERT(eStyleAnimType_Corner_TopRight -
                         eStyleAnimType_Corner_TopLeft
                       == NS_CORNER_TOP_RIGHT);
      PR_STATIC_ASSERT(eStyleAnimType_Corner_BottomRight -
                         eStyleAnimType_Corner_TopLeft
                       == NS_CORNER_BOTTOM_RIGHT);
      PR_STATIC_ASSERT(eStyleAnimType_Corner_BottomLeft -
                         eStyleAnimType_Corner_TopLeft
                       == NS_CORNER_BOTTOM_LEFT);
      const nsStyleCorners *corners = static_cast<const nsStyleCorners*>(
        StyleDataAtOffset(styleStruct, ssOffset));
      PRUint8 fullCorner = animType - eStyleAnimType_Corner_TopLeft;
      const nsStyleCoord &horiz =
        corners->Get(NS_FULL_TO_HALF_CORNER(fullCorner, PR_FALSE));
      const nsStyleCoord &vert =
        corners->Get(NS_FULL_TO_HALF_CORNER(fullCorner, PR_TRUE));
      nsAutoPtr<nsCSSValuePair> pair(new nsCSSValuePair);
      if (!pair ||
          !StyleCoordToCSSValue(horiz, pair->mXValue) ||
          !StyleCoordToCSSValue(vert, pair->mYValue)) {
        return PR_FALSE;
      }
      aComputedValue.SetAndAdoptCSSValuePairValue(pair.forget(),
                                                  eUnit_CSSValuePair);
      return PR_TRUE;
    }
    case eStyleAnimType_nscoord:
      aComputedValue.SetCoordValue(*static_cast<const nscoord*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      return PR_TRUE;
    case eStyleAnimType_EnumU8:
      aComputedValue.SetIntValue(*static_cast<const PRUint8*>(
        StyleDataAtOffset(styleStruct, ssOffset)), eUnit_Enumerated);
      return PR_TRUE;
    case eStyleAnimType_float:
      aComputedValue.SetFloatValue(*static_cast<const float*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      if (aProperty == eCSSProperty_font_size_adjust &&
          aComputedValue.GetFloatValue() == 0.0f) {
        
        
        
        
        aComputedValue.SetNoneValue();
      }
      return PR_TRUE;
    case eStyleAnimType_Color:
      aComputedValue.SetColorValue(*static_cast<const nscolor*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      return PR_TRUE;
    case eStyleAnimType_PaintServer: {
      const nsStyleSVGPaint &paint = *static_cast<const nsStyleSVGPaint*>(
        StyleDataAtOffset(styleStruct, ssOffset));
      if (paint.mType == eStyleSVGPaintType_Color) {
        aComputedValue.SetColorValue(paint.mPaint.mColor);
        return PR_TRUE;
      }
      if (paint.mType == eStyleSVGPaintType_Server) {
        if (!paint.mPaint.mPaintServer) {
          NS_WARNING("Null paint server");
          return PR_FALSE;
        }
        nsAutoPtr<nsCSSValuePair> pair(new nsCSSValuePair);
        nsRefPtr<nsStringBuffer> uriAsStringBuffer =
          GetURIAsUtf16StringBuffer(paint.mPaint.mPaintServer);
        NS_ENSURE_TRUE(!!uriAsStringBuffer, PR_FALSE);
        nsIDocument* doc = aStyleContext->PresContext()->Document();
        nsRefPtr<nsCSSValue::URL> url =
          new nsCSSValue::URL(paint.mPaint.mPaintServer,
                              uriAsStringBuffer,
                              doc->GetDocumentURI(),
                              doc->NodePrincipal());
        pair->mXValue.SetURLValue(url);
        pair->mYValue.SetColorValue(paint.mFallbackColor);
        aComputedValue.SetAndAdoptCSSValuePairValue(pair.forget(),
                                                    eUnit_CSSValuePair);
        return PR_TRUE;
      }
      NS_ABORT_IF_FALSE(paint.mType == eStyleSVGPaintType_None,
          "Unexpected SVG paint type");
      aComputedValue.SetNoneValue();
      return PR_TRUE;
    }
    case eStyleAnimType_Shadow: {
      const nsCSSShadowArray *shadowArray =
        *static_cast<const nsRefPtr<nsCSSShadowArray>*>(
          StyleDataAtOffset(styleStruct, ssOffset));
      if (!shadowArray) {
        aComputedValue.SetAndAdoptCSSValueListValue(nsnull, eUnit_Shadow);
        return PR_TRUE;
      }
      nsAutoPtr<nsCSSValueList> result;
      nsCSSValueList **resultTail = getter_Transfers(result);
      for (PRUint32 i = 0, i_end = shadowArray->Length(); i < i_end; ++i) {
        const nsCSSShadowItem *shadow = shadowArray->ShadowAt(i);
        
        nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(6);
        nscoordToCSSValue(shadow->mXOffset, arr->Item(0));
        nscoordToCSSValue(shadow->mYOffset, arr->Item(1));
        nscoordToCSSValue(shadow->mRadius, arr->Item(2));
        
        
        nscoordToCSSValue(shadow->mSpread, arr->Item(3));
        if (shadow->mHasColor) {
          arr->Item(4).SetColorValue(shadow->mColor);
        }
        if (shadow->mInset) {
          arr->Item(5).SetIntValue(NS_STYLE_BOX_SHADOW_INSET,
                                   eCSSUnit_Enumerated);
        }

        nsCSSValueList *resultItem = new nsCSSValueList;
        if (!resultItem) {
          return PR_FALSE;
        }
        resultItem->mValue.SetArrayValue(arr, eCSSUnit_Array);
        *resultTail = resultItem;
        resultTail = &resultItem->mNext;
      }
      aComputedValue.SetAndAdoptCSSValueListValue(result.forget(),
                                                  eUnit_Shadow);
      return PR_TRUE;
    }
    case eStyleAnimType_None:
      NS_NOTREACHED("shouldn't use on non-animatable properties");
  }
  return PR_FALSE;
}

nsStyleAnimation::Value::Value(PRInt32 aInt, Unit aUnit,
                               IntegerConstructorType)
{
  NS_ASSERTION(IsIntUnit(aUnit), "unit must be of integer type");
  mUnit = aUnit;
  mValue.mInt = aInt;
}

nsStyleAnimation::Value::Value(nscoord aLength, CoordConstructorType)
{
  mUnit = eUnit_Coord;
  mValue.mCoord = aLength;
}

nsStyleAnimation::Value::Value(float aPercent, PercentConstructorType)
{
  mUnit = eUnit_Percent;
  mValue.mFloat = aPercent;
}

nsStyleAnimation::Value::Value(float aFloat, FloatConstructorType)
{
  mUnit = eUnit_Float;
  mValue.mFloat = aFloat;
}

nsStyleAnimation::Value::Value(nscolor aColor, ColorConstructorType)
{
  mUnit = eUnit_Color;
  mValue.mColor = aColor;
}

nsStyleAnimation::Value&
nsStyleAnimation::Value::operator=(const Value& aOther)
{
  FreeValue();

  mUnit = aOther.mUnit;
  switch (mUnit) {
    case eUnit_Null:
    case eUnit_Normal:
    case eUnit_Auto:
    case eUnit_None:
      break;
    case eUnit_Enumerated:
    case eUnit_Visibility:
    case eUnit_Integer:
      mValue.mInt = aOther.mValue.mInt;
      break;
    case eUnit_Coord:
      mValue.mCoord = aOther.mValue.mCoord;
      break;
    case eUnit_Percent:
    case eUnit_Float:
      mValue.mFloat = aOther.mValue.mFloat;
      break;
    case eUnit_Color:
      mValue.mColor = aOther.mValue.mColor;
      break;
    case eUnit_Calc:
      NS_ABORT_IF_FALSE(aOther.mValue.mCSSValue, "values may not be null");
      mValue.mCSSValue = new nsCSSValue(*aOther.mValue.mCSSValue);
      if (!mValue.mCSSValue) {
        mUnit = eUnit_Null;
      }
      break;
    case eUnit_CSSValuePair:
      NS_ABORT_IF_FALSE(aOther.mValue.mCSSValuePair,
                        "value pairs may not be null");
      mValue.mCSSValuePair = new nsCSSValuePair(*aOther.mValue.mCSSValuePair);
      if (!mValue.mCSSValuePair) {
        mUnit = eUnit_Null;
      }
      break;
    case eUnit_CSSRect:
      NS_ABORT_IF_FALSE(aOther.mValue.mCSSRect, "rects may not be null");
      mValue.mCSSRect = new nsCSSRect(*aOther.mValue.mCSSRect);
      if (!mValue.mCSSRect) {
        mUnit = eUnit_Null;
      }
      break;
    case eUnit_Dasharray:
    case eUnit_Shadow:
    case eUnit_Transform:
      NS_ABORT_IF_FALSE(mUnit == eUnit_Shadow || aOther.mValue.mCSSValueList,
                        "value lists other than shadows may not be null");
      if (aOther.mValue.mCSSValueList) {
        mValue.mCSSValueList = aOther.mValue.mCSSValueList->Clone();
        if (!mValue.mCSSValueList) {
          mUnit = eUnit_Null;
        }
      } else {
        mValue.mCSSValueList = nsnull;
      }
      break;
    case eUnit_CSSValuePairList:
      NS_ABORT_IF_FALSE(aOther.mValue.mCSSValuePairList,
                        "value pair lists may not be null");
      mValue.mCSSValuePairList = aOther.mValue.mCSSValuePairList->Clone();
      if (!mValue.mCSSValuePairList) {
        mUnit = eUnit_Null;
      }
      break;
    case eUnit_UnparsedString:
      NS_ABORT_IF_FALSE(aOther.mValue.mString, "expecting non-null string");
      mValue.mString = aOther.mValue.mString;
      mValue.mString->AddRef();
      break;
  }

  return *this;
}

void
nsStyleAnimation::Value::SetNormalValue()
{
  FreeValue();
  mUnit = eUnit_Normal;
}

void
nsStyleAnimation::Value::SetAutoValue()
{
  FreeValue();
  mUnit = eUnit_Auto;
}

void
nsStyleAnimation::Value::SetNoneValue()
{
  FreeValue();
  mUnit = eUnit_None;
}

void
nsStyleAnimation::Value::SetIntValue(PRInt32 aInt, Unit aUnit)
{
  NS_ASSERTION(IsIntUnit(aUnit), "unit must be of integer type");
  FreeValue();
  mUnit = aUnit;
  mValue.mInt = aInt;
}

void
nsStyleAnimation::Value::SetCoordValue(nscoord aLength)
{
  FreeValue();
  mUnit = eUnit_Coord;
  mValue.mCoord = aLength;
}

void
nsStyleAnimation::Value::SetPercentValue(float aPercent)
{
  FreeValue();
  mUnit = eUnit_Percent;
  mValue.mFloat = aPercent;
}

void
nsStyleAnimation::Value::SetFloatValue(float aFloat)
{
  FreeValue();
  mUnit = eUnit_Float;
  mValue.mFloat = aFloat;
}

void
nsStyleAnimation::Value::SetColorValue(nscolor aColor)
{
  FreeValue();
  mUnit = eUnit_Color;
  mValue.mColor = aColor;
}

void
nsStyleAnimation::Value::SetUnparsedStringValue(const nsString& aString)
{
  FreeValue();
  mUnit = eUnit_UnparsedString;
  mValue.mString = nsCSSValue::BufferFromString(aString).get();
  if (NS_UNLIKELY(!mValue.mString)) {
    
    
    mUnit = eUnit_Null;
  }
}

void
nsStyleAnimation::Value::SetAndAdoptCSSValueValue(nsCSSValue *aValue,
                                                  Unit aUnit)
{
  FreeValue();
  NS_ABORT_IF_FALSE(IsCSSValueUnit(aUnit), "bad unit");
  NS_ABORT_IF_FALSE(aValue != nsnull, "values may not be null");
  mUnit = aUnit;
  mValue.mCSSValue = aValue; 
}

void
nsStyleAnimation::Value::SetAndAdoptCSSValuePairValue(
                           nsCSSValuePair *aValuePair, Unit aUnit)
{
  FreeValue();
  NS_ABORT_IF_FALSE(IsCSSValuePairUnit(aUnit), "bad unit");
  NS_ABORT_IF_FALSE(aValuePair != nsnull, "value pairs may not be null");
  mUnit = aUnit;
  mValue.mCSSValuePair = aValuePair; 
}

void
nsStyleAnimation::Value::SetAndAdoptCSSRectValue(nsCSSRect *aRect, Unit aUnit)
{
  FreeValue();
  NS_ABORT_IF_FALSE(IsCSSRectUnit(aUnit), "bad unit");
  NS_ABORT_IF_FALSE(aRect != nsnull, "value pairs may not be null");
  mUnit = aUnit;
  mValue.mCSSRect = aRect; 
}

void
nsStyleAnimation::Value::SetAndAdoptCSSValueListValue(
                           nsCSSValueList *aValueList, Unit aUnit)
{
  FreeValue();
  NS_ABORT_IF_FALSE(IsCSSValueListUnit(aUnit), "bad unit");
  NS_ABORT_IF_FALSE(aUnit != eUnit_Dasharray || aValueList != nsnull,
                    "dasharrays may not be null");
  mUnit = aUnit;
  mValue.mCSSValueList = aValueList; 
}

void
nsStyleAnimation::Value::SetAndAdoptCSSValuePairListValue(
                           nsCSSValuePairList *aValuePairList)
{
  FreeValue();
  NS_ABORT_IF_FALSE(aValuePairList, "may not be null");
  mUnit = eUnit_CSSValuePairList;
  mValue.mCSSValuePairList = aValuePairList; 
}

void
nsStyleAnimation::Value::FreeValue()
{
  if (IsCSSValueUnit(mUnit)) {
    delete mValue.mCSSValue;
  } else if (IsCSSValueListUnit(mUnit)) {
    delete mValue.mCSSValueList;
  } else if (IsCSSValuePairUnit(mUnit)) {
    delete mValue.mCSSValuePair;
  } else if (IsCSSRectUnit(mUnit)) {
    delete mValue.mCSSRect;
  } else if (IsCSSValuePairListUnit(mUnit)) {
    delete mValue.mCSSValuePairList;
  } else if (IsStringUnit(mUnit)) {
    NS_ABORT_IF_FALSE(mValue.mString, "expecting non-null string");
    mValue.mString->Release();
  }
}

PRBool
nsStyleAnimation::Value::operator==(const Value& aOther) const
{
  if (mUnit != aOther.mUnit) {
    return PR_FALSE;
  }

  switch (mUnit) {
    case eUnit_Null:
    case eUnit_Normal:
    case eUnit_Auto:
    case eUnit_None:
      return PR_TRUE;
    case eUnit_Enumerated:
    case eUnit_Visibility:
    case eUnit_Integer:
      return mValue.mInt == aOther.mValue.mInt;
    case eUnit_Coord:
      return mValue.mCoord == aOther.mValue.mCoord;
    case eUnit_Percent:
    case eUnit_Float:
      return mValue.mFloat == aOther.mValue.mFloat;
    case eUnit_Color:
      return mValue.mColor == aOther.mValue.mColor;
    case eUnit_Calc:
      return *mValue.mCSSValue == *aOther.mValue.mCSSValue;
    case eUnit_CSSValuePair:
      return *mValue.mCSSValuePair == *aOther.mValue.mCSSValuePair;
    case eUnit_CSSRect:
      return *mValue.mCSSRect == *aOther.mValue.mCSSRect;
    case eUnit_Dasharray:
    case eUnit_Shadow:
    case eUnit_Transform:
      return *mValue.mCSSValueList == *aOther.mValue.mCSSValueList;
    case eUnit_CSSValuePairList:
      return *mValue.mCSSValuePairList == *aOther.mValue.mCSSValuePairList;
    case eUnit_UnparsedString:
      return (NS_strcmp(GetStringBufferValue(),
                        aOther.GetStringBufferValue()) == 0);
  }

  NS_NOTREACHED("incomplete case");
  return PR_FALSE;
}

