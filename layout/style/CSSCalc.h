



































#ifndef CSSCalc_h_
#define CSSCalc_h_

#include "nsCSSValue.h"
#include <math.h>

namespace mozilla {

namespace css {















































template <class CalcOps>
static typename CalcOps::result_type
ComputeCalc(const nsCSSValue& aValue,
            const typename CalcOps::ComputeData &aClosure)
{
  switch (aValue.GetUnit()) {
    case eCSSUnit_Calc: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 1, "unexpected length");
      return ComputeCalc<CalcOps>(arr->Item(0), aClosure);
    }
    case eCSSUnit_Calc_Plus:
    case eCSSUnit_Calc_Minus: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      typename CalcOps::result_type
        lhs = ComputeCalc<CalcOps>(arr->Item(0), aClosure),
        rhs = ComputeCalc<CalcOps>(arr->Item(1), aClosure);
      return CalcOps::MergeAdditive(aValue.GetUnit(), lhs, rhs);
    }
    case eCSSUnit_Calc_Times_L: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      float lhs = CalcOps::ComputeNumber(arr->Item(0));
      typename CalcOps::result_type rhs =
        ComputeCalc<CalcOps>(arr->Item(1), aClosure);
      return CalcOps::MergeMultiplicativeL(aValue.GetUnit(), lhs, rhs);
    }
    case eCSSUnit_Calc_Times_R:
    case eCSSUnit_Calc_Divided: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      typename CalcOps::result_type lhs =
        ComputeCalc<CalcOps>(arr->Item(0), aClosure);
      float rhs = CalcOps::ComputeNumber(arr->Item(1));
      return CalcOps::MergeMultiplicativeR(aValue.GetUnit(), lhs, rhs);
    }
    case eCSSUnit_Calc_Minimum:
    case eCSSUnit_Calc_Maximum: {
      nsCSSValue::Array *arr = aValue.GetArrayValue();
      typename CalcOps::result_type result =
        ComputeCalc<CalcOps>(arr->Item(0), aClosure);
      for (PRUint32 i = 1, i_end = arr->Count(); i < i_end; ++i) {
        typename CalcOps::result_type tmp =
          ComputeCalc<CalcOps>(arr->Item(i), aClosure);
        result = CalcOps::MergeAdditive(aValue.GetUnit(), result, tmp);
      }
      return result;
    }
    default: {
      return CalcOps::ComputeLeafValue(aValue, aClosure);
    }
  }
}






template <typename T>
struct BasicCalcOpsAdditive
{
  typedef T result_type;

  static result_type
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
};

struct BasicCoordCalcOps : public BasicCalcOpsAdditive<nscoord>
{
  static result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_L,
                      "unexpected unit");
    return NSToCoordRound(aValue1 * aValue2);
  }

  static result_type
  MergeMultiplicativeR(nsCSSUnit aCalcFunction,
                       result_type aValue1, float aValue2)
  {
    if (aCalcFunction == eCSSUnit_Calc_Times_R) {
      return NSToCoordRound(aValue1 * aValue2);
    }
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Divided,
                      "unexpected unit");
    return NSToCoordRound(aValue1 / aValue2);
  }
};

struct BasicFloatCalcOps : public BasicCalcOpsAdditive<float>
{
  static result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_L,
                      "unexpected unit");
    return aValue1 * aValue2;
  }

  static result_type
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
  static float ComputeNumber(const nsCSSValue& aValue)
  {
    NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Number, "unexpected unit");
    return aValue.GetFloatValue();
  }
};

}

}

#endif 
