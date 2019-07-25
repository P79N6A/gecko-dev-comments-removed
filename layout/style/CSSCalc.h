



































#ifndef CSSCalc_h_
#define CSSCalc_h_

#include "nsCSSValue.h"
#include <math.h>

namespace mozilla {

namespace css {














































template <class CalcOps>
static typename CalcOps::result_type
ComputeCalc(const nsCSSValue& aValue, CalcOps &aOps)
{
  switch (aValue.GetUnit()) {
    case eCSSUnit_Calc: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 1, "unexpected length");
      return ComputeCalc(arr->Item(0), aOps);
    }
    case eCSSUnit_Calc_Plus:
    case eCSSUnit_Calc_Minus: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      typename CalcOps::result_type lhs = ComputeCalc(arr->Item(0), aOps),
                                    rhs = ComputeCalc(arr->Item(1), aOps);
      return aOps.MergeAdditive(aValue.GetUnit(), lhs, rhs);
    }
    case eCSSUnit_Calc_Times_L: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      float lhs = aOps.ComputeNumber(arr->Item(0));
      typename CalcOps::result_type rhs = ComputeCalc(arr->Item(1), aOps);
      return aOps.MergeMultiplicativeL(aValue.GetUnit(), lhs, rhs);
    }
    case eCSSUnit_Calc_Times_R:
    case eCSSUnit_Calc_Divided: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      typename CalcOps::result_type lhs = ComputeCalc(arr->Item(0), aOps);
      float rhs = aOps.ComputeNumber(arr->Item(1));
      return aOps.MergeMultiplicativeR(aValue.GetUnit(), lhs, rhs);
    }
    case eCSSUnit_Calc_Minimum:
    case eCSSUnit_Calc_Maximum: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      typename CalcOps::result_type result = ComputeCalc(arr->Item(0), aOps);
      for (size_t i = 1, i_end = arr->Count(); i < i_end; ++i) {
        typename CalcOps::result_type tmp = ComputeCalc(arr->Item(i), aOps);
        result = aOps.MergeAdditive(aValue.GetUnit(), result, tmp);
      }
      return result;
    }
    default: {
      return aOps.ComputeLeafValue(aValue);
    }
  }
}







struct BasicCoordCalcOps
{
  typedef nscoord result_type;

  result_type
  MergeAdditive(nsCSSUnit aCalcFunction,
                result_type aValue1, result_type aValue2)
  {
    if (aCalcFunction == eCSSUnit_Calc_Plus) {
      return NSCoordSaturatingAdd(aValue1, aValue2);
    }
    if (aCalcFunction == eCSSUnit_Calc_Minus) {
      return NSCoordSaturatingSubtract(aValue1, aValue2, 0);
    }
    if (aCalcFunction == eCSSUnit_Calc_Minimum) {
      return NS_MIN(aValue1, aValue2);
    }
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Maximum,
                      "unexpected unit");
    return NS_MAX(aValue1, aValue2);
  }

  result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_L,
                      "unexpected unit");
    return NSCoordSaturatingMultiply(aValue2, aValue1);
  }

  result_type
  MergeMultiplicativeR(nsCSSUnit aCalcFunction,
                       result_type aValue1, float aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_R ||
                      aCalcFunction == eCSSUnit_Calc_Divided,
                      "unexpected unit");
    if (aCalcFunction == eCSSUnit_Calc_Divided) {
      aValue2 = 1.0f / aValue2;
    }
    return NSCoordSaturatingMultiply(aValue1, aValue2);
  }
};

struct BasicFloatCalcOps
{
  typedef float result_type;

  result_type
  MergeAdditive(nsCSSUnit aCalcFunction,
                result_type aValue1, result_type aValue2)
  {
    if (aCalcFunction == eCSSUnit_Calc_Plus) {
      return aValue1 + aValue2;
    }
    if (aCalcFunction == eCSSUnit_Calc_Minus) {
      return aValue1 - aValue2;
    }
    if (aCalcFunction == eCSSUnit_Calc_Minimum) {
      return NS_MIN(aValue1, aValue2);
    }
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Maximum,
                      "unexpected unit");
    return NS_MAX(aValue1, aValue2);
  }

  result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_L,
                      "unexpected unit");
    return aValue1 * aValue2;
  }

  result_type
  MergeMultiplicativeR(nsCSSUnit aCalcFunction,
                       result_type aValue1, float aValue2)
  {
    if (aCalcFunction == eCSSUnit_Calc_Times_R) {
      return aValue1 * aValue2;
    }
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Divided,
                      "unexpected unit");
    return aValue1 / aValue2;
  }
};





struct NumbersAlreadyNormalizedOps
{
  float ComputeNumber(const nsCSSValue& aValue)
  {
    NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Number, "unexpected unit");
    return aValue.GetFloatValue();
  }
};

#define CHECK_UNIT(u_)                                                        \
  PR_STATIC_ASSERT(int(eCSSUnit_Calc_##u_) + 14 == int(eStyleUnit_Calc_##u_));\
  PR_STATIC_ASSERT(eCSSUnit_Calc_##u_ >= eCSSUnit_Calc_Plus);                 \
  PR_STATIC_ASSERT(eCSSUnit_Calc_##u_ <= eCSSUnit_Calc_Maximum);

CHECK_UNIT(Plus)
CHECK_UNIT(Minus)
CHECK_UNIT(Times_L)
CHECK_UNIT(Times_R)
CHECK_UNIT(Divided)
CHECK_UNIT(Minimum)
CHECK_UNIT(Maximum)

#undef CHECK_UNIT

inline nsStyleUnit
ConvertCalcUnit(nsCSSUnit aUnit)
{
  NS_ABORT_IF_FALSE(eCSSUnit_Calc_Plus <= aUnit &&
                    aUnit <= eCSSUnit_Calc_Maximum, "out of range");
  return nsStyleUnit(aUnit + 14);
}

inline nsCSSUnit
ConvertCalcUnit(nsStyleUnit aUnit)
{
  NS_ABORT_IF_FALSE(eStyleUnit_Calc_Plus <= aUnit &&
                    aUnit <= eStyleUnit_Calc_Maximum, "out of range");
  return nsCSSUnit(aUnit - 14);
}

}

}

#endif 
