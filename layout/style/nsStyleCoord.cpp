






































#include "nsStyleCoord.h"
#include "nsString.h"
#include "nsCRT.h"
#include "prlog.h"
#include "nsMathUtils.h"
#include "nsStyleContext.h"

nsStyleCoord::nsStyleCoord(nsStyleUnit aUnit)
  : mUnit(aUnit)
{
  NS_ASSERTION(aUnit < eStyleUnit_Percent, "not a valueless unit");
  if (aUnit >= eStyleUnit_Percent) {
    mUnit = eStyleUnit_Null;
  }
  mValue.mInt = 0;
}

nsStyleCoord::nsStyleCoord(PRInt32 aValue, nsStyleUnit aUnit)
  : mUnit(aUnit)
{
  
  
  NS_ASSERTION((aUnit == eStyleUnit_Enumerated) ||
               (aUnit == eStyleUnit_Integer), "not an int value");
  if ((aUnit == eStyleUnit_Enumerated) ||
      (aUnit == eStyleUnit_Integer)) {
    mValue.mInt = aValue;
  }
  else {
    mUnit = eStyleUnit_Null;
    mValue.mInt = 0;
  }
}

nsStyleCoord::nsStyleCoord(float aValue, nsStyleUnit aUnit)
  : mUnit(aUnit)
{
  if (aUnit < eStyleUnit_Percent || aUnit >= eStyleUnit_Coord) {
    NS_NOTREACHED("not a float value");
    Reset();
  } else {
    mValue.mFloat = aValue;
  }
}




nsStyleCoord& nsStyleCoord::operator=(const nsStyleCoord& aCopy)
{
  mUnit = aCopy.mUnit;
  if ((eStyleUnit_Percent <= mUnit) && (mUnit < eStyleUnit_Coord)) {
    mValue.mFloat = aCopy.mValue.mFloat;
  }
  else if (IsPointerValue()) {
    mValue.mPointer = aCopy.mValue.mPointer;
  }
  else {
    mValue.mInt = aCopy.mValue.mInt;
  }
  return *this;
}

bool nsStyleCoord::operator==(const nsStyleCoord& aOther) const
{
  if (mUnit != aOther.mUnit) {
    return false;
  }
  switch (mUnit) {
    case eStyleUnit_Null:
    case eStyleUnit_Normal:
    case eStyleUnit_Auto:
    case eStyleUnit_None:
      return true;
    case eStyleUnit_Percent:
    case eStyleUnit_Factor:
    case eStyleUnit_Degree:
    case eStyleUnit_Grad:
    case eStyleUnit_Radian:
      return mValue.mFloat == aOther.mValue.mFloat;
    case eStyleUnit_Coord:
    case eStyleUnit_Integer:
    case eStyleUnit_Enumerated:
      return mValue.mInt == aOther.mValue.mInt;
    case eStyleUnit_Calc:
      return *this->GetCalcValue() == *aOther.GetCalcValue();
  }
  NS_ABORT_IF_FALSE(false, "unexpected unit");
  return false;
}

void nsStyleCoord::Reset()
{
  mUnit = eStyleUnit_Null;
  mValue.mInt = 0;
}

void nsStyleCoord::SetCoordValue(nscoord aValue)
{
  mUnit = eStyleUnit_Coord;
  mValue.mInt = aValue;
}

void nsStyleCoord::SetIntValue(PRInt32 aValue, nsStyleUnit aUnit)
{
  NS_ASSERTION((aUnit == eStyleUnit_Enumerated) ||
               (aUnit == eStyleUnit_Integer), "not an int value");
  if ((aUnit == eStyleUnit_Enumerated) ||
      (aUnit == eStyleUnit_Integer)) {
    mUnit = aUnit;
    mValue.mInt = aValue;
  }
  else {
    Reset();
  }
}

void nsStyleCoord::SetPercentValue(float aValue)
{
  mUnit = eStyleUnit_Percent;
  mValue.mFloat = aValue;
}

void nsStyleCoord::SetFactorValue(float aValue)
{
  mUnit = eStyleUnit_Factor;
  mValue.mFloat = aValue;
}

void nsStyleCoord::SetAngleValue(float aValue, nsStyleUnit aUnit)
{
  if (aUnit == eStyleUnit_Degree ||
      aUnit == eStyleUnit_Grad ||
      aUnit == eStyleUnit_Radian) {
    mUnit = aUnit;
    mValue.mFloat = aValue;
  } else {
    NS_NOTREACHED("not an angle value");
    Reset();
  }
}

void nsStyleCoord::SetCalcValue(Calc* aValue)
{
  mUnit = eStyleUnit_Calc;
  mValue.mPointer = aValue;
}

void nsStyleCoord::SetNormalValue()
{
  mUnit = eStyleUnit_Normal;
  mValue.mInt = 0;
}

void nsStyleCoord::SetAutoValue()
{
  mUnit = eStyleUnit_Auto;
  mValue.mInt = 0;
}

void nsStyleCoord::SetNoneValue()
{
  mUnit = eStyleUnit_None;
  mValue.mInt = 0;
}



double
nsStyleCoord::GetAngleValueInRadians() const
{
  double angle = mValue.mFloat;

  switch (GetUnit()) {
  case eStyleUnit_Radian: return angle;
  case eStyleUnit_Degree: return angle * M_PI / 180.0;
  case eStyleUnit_Grad:   return angle * M_PI / 200.0;

  default:
    NS_NOTREACHED("unrecognized angular unit");
    return 0.0;
  }
}


#define COMPARE_INDEXED_COORD(i)                                              \
  PR_BEGIN_MACRO                                                              \
  if (mUnits[i] != aOther.mUnits[i])                                          \
    return false;                                                          \
  if ((eStyleUnit_Percent <= mUnits[i]) &&                                    \
      (mUnits[i] < eStyleUnit_Coord)) {                                       \
    if (mValues[i].mFloat != aOther.mValues[i].mFloat)                        \
      return false;                                                        \
  }                                                                           \
  else {                                                                      \
    if (mValues[i].mInt != aOther.mValues[i].mInt)                            \
      return false;                                                        \
  }                                                                           \
  PR_END_MACRO


nsStyleSides::nsStyleSides()
{
  memset(this, 0x00, sizeof(nsStyleSides));
}

bool nsStyleSides::operator==(const nsStyleSides& aOther) const
{
  NS_FOR_CSS_SIDES(i) {
    COMPARE_INDEXED_COORD(i);
  }
  return true;
}

void nsStyleSides::Reset()
{
  memset(this, 0x00, sizeof(nsStyleSides));
}

nsStyleCorners::nsStyleCorners()
{
  memset(this, 0x00, sizeof(nsStyleCorners));
}

bool
nsStyleCorners::operator==(const nsStyleCorners& aOther) const
{
  NS_FOR_CSS_HALF_CORNERS(i) {
    COMPARE_INDEXED_COORD(i);
  }
  return true;
}

void nsStyleCorners::Reset()
{
  memset(this, 0x00, sizeof(nsStyleCorners));
}


#define CASE(side, result)                                                    \
  PR_STATIC_ASSERT(NS_SIDE_IS_VERTICAL(side) == result)
CASE(NS_SIDE_TOP,    false);
CASE(NS_SIDE_RIGHT,  true);
CASE(NS_SIDE_BOTTOM, false);
CASE(NS_SIDE_LEFT,   true);
#undef CASE

#define CASE(corner, result)                                                  \
  PR_STATIC_ASSERT(NS_HALF_CORNER_IS_X(corner) == result)
CASE(NS_CORNER_TOP_LEFT_X,     true);
CASE(NS_CORNER_TOP_LEFT_Y,     false);
CASE(NS_CORNER_TOP_RIGHT_X,    true);
CASE(NS_CORNER_TOP_RIGHT_Y,    false);
CASE(NS_CORNER_BOTTOM_RIGHT_X, true);
CASE(NS_CORNER_BOTTOM_RIGHT_Y, false);
CASE(NS_CORNER_BOTTOM_LEFT_X,  true);
CASE(NS_CORNER_BOTTOM_LEFT_Y,  false);
#undef CASE


#define CASE(corner, result)                                                  \
  PR_STATIC_ASSERT(NS_HALF_TO_FULL_CORNER(corner) == result)
CASE(NS_CORNER_TOP_LEFT_X,     NS_CORNER_TOP_LEFT);
CASE(NS_CORNER_TOP_LEFT_Y,     NS_CORNER_TOP_LEFT);
CASE(NS_CORNER_TOP_RIGHT_X,    NS_CORNER_TOP_RIGHT);
CASE(NS_CORNER_TOP_RIGHT_Y,    NS_CORNER_TOP_RIGHT);
CASE(NS_CORNER_BOTTOM_RIGHT_X, NS_CORNER_BOTTOM_RIGHT);
CASE(NS_CORNER_BOTTOM_RIGHT_Y, NS_CORNER_BOTTOM_RIGHT);
CASE(NS_CORNER_BOTTOM_LEFT_X,  NS_CORNER_BOTTOM_LEFT);
CASE(NS_CORNER_BOTTOM_LEFT_Y,  NS_CORNER_BOTTOM_LEFT);
#undef CASE


#define CASE(corner, vert, result)                                            \
  PR_STATIC_ASSERT(NS_FULL_TO_HALF_CORNER(corner, vert) == result)
CASE(NS_CORNER_TOP_LEFT,     false, NS_CORNER_TOP_LEFT_X);
CASE(NS_CORNER_TOP_LEFT,     true,  NS_CORNER_TOP_LEFT_Y);
CASE(NS_CORNER_TOP_RIGHT,    false, NS_CORNER_TOP_RIGHT_X);
CASE(NS_CORNER_TOP_RIGHT,    true,  NS_CORNER_TOP_RIGHT_Y);
CASE(NS_CORNER_BOTTOM_RIGHT, false, NS_CORNER_BOTTOM_RIGHT_X);
CASE(NS_CORNER_BOTTOM_RIGHT, true,  NS_CORNER_BOTTOM_RIGHT_Y);
CASE(NS_CORNER_BOTTOM_LEFT,  false, NS_CORNER_BOTTOM_LEFT_X);
CASE(NS_CORNER_BOTTOM_LEFT,  true,  NS_CORNER_BOTTOM_LEFT_Y);
#undef CASE


#define CASE(side, second, result)                                            \
  PR_STATIC_ASSERT(NS_SIDE_TO_FULL_CORNER(side, second) == result)
CASE(NS_SIDE_TOP,    false, NS_CORNER_TOP_LEFT);
CASE(NS_SIDE_TOP,    true,  NS_CORNER_TOP_RIGHT);

CASE(NS_SIDE_RIGHT,  false, NS_CORNER_TOP_RIGHT);
CASE(NS_SIDE_RIGHT,  true,  NS_CORNER_BOTTOM_RIGHT);

CASE(NS_SIDE_BOTTOM, false, NS_CORNER_BOTTOM_RIGHT);
CASE(NS_SIDE_BOTTOM, true,  NS_CORNER_BOTTOM_LEFT);

CASE(NS_SIDE_LEFT,   false, NS_CORNER_BOTTOM_LEFT);
CASE(NS_SIDE_LEFT,   true,  NS_CORNER_TOP_LEFT);
#undef CASE

#define CASE(side, second, parallel, result)                                  \
  PR_STATIC_ASSERT(NS_SIDE_TO_HALF_CORNER(side, second, parallel) == result)
CASE(NS_SIDE_TOP,    false, true,  NS_CORNER_TOP_LEFT_X);
CASE(NS_SIDE_TOP,    false, false, NS_CORNER_TOP_LEFT_Y);
CASE(NS_SIDE_TOP,    true,  true,  NS_CORNER_TOP_RIGHT_X);
CASE(NS_SIDE_TOP,    true,  false, NS_CORNER_TOP_RIGHT_Y);

CASE(NS_SIDE_RIGHT,  false, false, NS_CORNER_TOP_RIGHT_X);
CASE(NS_SIDE_RIGHT,  false, true,  NS_CORNER_TOP_RIGHT_Y);
CASE(NS_SIDE_RIGHT,  true,  false, NS_CORNER_BOTTOM_RIGHT_X);
CASE(NS_SIDE_RIGHT,  true,  true,  NS_CORNER_BOTTOM_RIGHT_Y);

CASE(NS_SIDE_BOTTOM, false, true,  NS_CORNER_BOTTOM_RIGHT_X);
CASE(NS_SIDE_BOTTOM, false, false, NS_CORNER_BOTTOM_RIGHT_Y);
CASE(NS_SIDE_BOTTOM, true,  true,  NS_CORNER_BOTTOM_LEFT_X);
CASE(NS_SIDE_BOTTOM, true,  false, NS_CORNER_BOTTOM_LEFT_Y);

CASE(NS_SIDE_LEFT,   false, false, NS_CORNER_BOTTOM_LEFT_X);
CASE(NS_SIDE_LEFT,   false, true,  NS_CORNER_BOTTOM_LEFT_Y);
CASE(NS_SIDE_LEFT,   true,  false, NS_CORNER_TOP_LEFT_X);
CASE(NS_SIDE_LEFT,   true,  true,  NS_CORNER_TOP_LEFT_Y);
#undef CASE
