



































#ifndef CSSCalc_h_
#define CSSCalc_h_

#include "nsCSSValue.h"
#include "nsStyleCoord.h"
#include <math.h>

namespace mozilla {

namespace css {




























































template <class CalcOps>
static typename CalcOps::result_type
ComputeCalc(const typename CalcOps::input_type& aValue, CalcOps &aOps)
{
  switch (CalcOps::GetUnit(aValue)) {
    case eCSSUnit_Calc: {
      typename CalcOps::input_array_type *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 1, "unexpected length");
      return ComputeCalc(arr->Item(0), aOps);
    }
    case eCSSUnit_Calc_Plus:
    case eCSSUnit_Calc_Minus: {
      typename CalcOps::input_array_type *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      typename CalcOps::result_type lhs = ComputeCalc(arr->Item(0), aOps),
                                    rhs = ComputeCalc(arr->Item(1), aOps);
      return aOps.MergeAdditive(CalcOps::GetUnit(aValue), lhs, rhs);
    }
    case eCSSUnit_Calc_Times_L: {
      typename CalcOps::input_array_type *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      float lhs = aOps.ComputeNumber(arr->Item(0));
      typename CalcOps::result_type rhs = ComputeCalc(arr->Item(1), aOps);
      return aOps.MergeMultiplicativeL(CalcOps::GetUnit(aValue), lhs, rhs);
    }
    case eCSSUnit_Calc_Times_R:
    case eCSSUnit_Calc_Divided: {
      typename CalcOps::input_array_type *arr = aValue.GetArrayValue();
      NS_ABORT_IF_FALSE(arr->Count() == 2, "unexpected length");
      typename CalcOps::result_type lhs = ComputeCalc(arr->Item(0), aOps);
      float rhs = aOps.ComputeNumber(arr->Item(1));
      return aOps.MergeMultiplicativeR(CalcOps::GetUnit(aValue), lhs, rhs);
    }
    case eCSSUnit_Calc_Minimum:
    case eCSSUnit_Calc_Maximum: {
      typename CalcOps::input_array_type *arr = aValue.GetArrayValue();
      typename CalcOps::result_type result = ComputeCalc(arr->Item(0), aOps);
      for (size_t i = 1, i_end = arr->Count(); i < i_end; ++i) {
        typename CalcOps::result_type tmp = ComputeCalc(arr->Item(i), aOps);
        result = aOps.MergeAdditive(CalcOps::GetUnit(aValue), result, tmp);
      }
      return result;
    }
    default: {
      return aOps.ComputeLeafValue(aValue);
    }
  }
}

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




struct CSSValueInputCalcOps
{
  typedef nsCSSValue input_type;
  typedef nsCSSValue::Array input_array_type;

  static nsCSSUnit GetUnit(const nsCSSValue& aValue)
  {
    return aValue.GetUnit();
  }

};




struct StyleCoordInputCalcOps
{
  typedef nsStyleCoord input_type;
  typedef nsStyleCoord::Array input_array_type;

  static nsCSSUnit GetUnit(const nsStyleCoord& aValue)
  {
    if (aValue.IsCalcUnit()) {
      return css::ConvertCalcUnit(aValue.GetUnit());
    }
    return eCSSUnit_Null;
  }

  float ComputeNumber(const nsStyleCoord& aValue)
  {
    NS_ABORT_IF_FALSE(PR_FALSE, "SpecifiedToComputedCalcOps should not "
                                "leave numbers in structure");
    return 0.0f;
  }
};







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





struct NumbersAlreadyNormalizedOps : public CSSValueInputCalcOps
{
  float ComputeNumber(const nsCSSValue& aValue)
  {
    NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Number, "unexpected unit");
    return aValue.GetFloatValue();
  }
};

























template <class CalcOps>
static void
SerializeCalcInternal(const typename CalcOps::input_type& aValue, CalcOps &aOps);



template <class CalcOps>
static void
SerializeCalc(const typename CalcOps::input_type& aValue, CalcOps &aOps)
{
  aOps.Append("-moz-");
  nsCSSUnit unit = CalcOps::GetUnit(aValue);
  if (unit != eCSSUnit_Calc_Minimum && unit != eCSSUnit_Calc_Maximum) {
    aOps.Append("calc(");
  }
  if (unit == eCSSUnit_Calc) {
    const typename CalcOps::input_array_type *array = aValue.GetArrayValue();
    NS_ABORT_IF_FALSE(array->Count() == 1, "unexpected length");
    SerializeCalcInternal(array->Item(0), aOps);
  } else {
    SerializeCalcInternal(aValue, aOps);
  }
  if (unit != eCSSUnit_Calc_Minimum && unit != eCSSUnit_Calc_Maximum) {
    aOps.Append(")");
  }
}

static inline PRBool
IsCalcAdditiveUnit(nsCSSUnit aUnit)
{
  return aUnit == eCSSUnit_Calc_Plus ||
         aUnit == eCSSUnit_Calc_Minus;
}

static inline PRBool
IsCalcMultiplicativeUnit(nsCSSUnit aUnit)
{
  return aUnit == eCSSUnit_Calc_Times_L ||
         aUnit == eCSSUnit_Calc_Times_R ||
         aUnit == eCSSUnit_Calc_Divided;
}



template <class CalcOps>
 void
SerializeCalcInternal(const typename CalcOps::input_type& aValue, CalcOps &aOps)
{
  nsCSSUnit unit = CalcOps::GetUnit(aValue);
  if (eCSSUnit_Calc_Minimum == unit || eCSSUnit_Calc_Maximum == unit) {
    const typename CalcOps::input_array_type *array = aValue.GetArrayValue();
    if (eCSSUnit_Calc_Minimum == unit) {
      aOps.Append("min(");
    } else {
      aOps.Append("max(");
    }

    for (size_t i = 0, i_end = array->Count(); i < i_end; ++i) {
      if (i != 0) {
        aOps.Append(", ");
      }
      SerializeCalcInternal(array->Item(i), aOps);
    }

    aOps.Append(")");
  } else if (IsCalcAdditiveUnit(unit)) {
    const typename CalcOps::input_array_type *array = aValue.GetArrayValue();
    NS_ABORT_IF_FALSE(array->Count() == 2, "unexpected length");

    SerializeCalcInternal(array->Item(0), aOps);

    if (eCSSUnit_Calc_Plus == unit) {
      aOps.Append(" + ");
    } else {
      NS_ABORT_IF_FALSE(eCSSUnit_Calc_Minus == unit, "unexpected unit");
      aOps.Append(" - ");
    }

    PRBool needParens = IsCalcAdditiveUnit(CalcOps::GetUnit(array->Item(1)));
    if (needParens) {
      aOps.Append("(");
    }
    SerializeCalcInternal(array->Item(1), aOps);
    if (needParens) {
      aOps.Append(")");
    }
  } else if (IsCalcMultiplicativeUnit(unit)) {
    const typename CalcOps::input_array_type *array = aValue.GetArrayValue();
    NS_ABORT_IF_FALSE(array->Count() == 2, "unexpected length");

    PRBool needParens = IsCalcAdditiveUnit(CalcOps::GetUnit(array->Item(0)));
    if (needParens) {
      aOps.Append("(");
    }
    if (unit == eCSSUnit_Calc_Times_L) {
      aOps.AppendNumber(array->Item(0));
    } else {
      SerializeCalcInternal(array->Item(0), aOps);
    }
    if (needParens) {
      aOps.Append(")");
    }

    if (eCSSUnit_Calc_Times_L == unit || eCSSUnit_Calc_Times_R == unit) {
      aOps.Append(" * ");
    } else {
      NS_ABORT_IF_FALSE(eCSSUnit_Calc_Divided == unit, "unexpected unit");
      aOps.Append(" / ");
    }

    nsCSSUnit subUnit = CalcOps::GetUnit(array->Item(1));
    needParens = IsCalcAdditiveUnit(subUnit) ||
                 IsCalcMultiplicativeUnit(subUnit);
    if (needParens) {
      aOps.Append("(");
    }
    if (unit == eCSSUnit_Calc_Times_L) {
      SerializeCalcInternal(array->Item(1), aOps);
    } else {
      aOps.AppendNumber(array->Item(1));
    }
    if (needParens) {
      aOps.Append(")");
    }
  } else {
    aOps.AppendLeafValue(aValue);
  }
}

}

}

#endif 
