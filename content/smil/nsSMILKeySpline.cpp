




































#include "nsSMILKeySpline.h"
#include <math.h>

#define NEWTON_ITERATIONS   4

const double nsSMILKeySpline::kSampleStepSize = 
                                        1.0 / double(kSplineTableSize - 1);

nsSMILKeySpline::nsSMILKeySpline(double aX1,
                                 double aY1,
                                 double aX2,
                                 double aY2)
: mX1(aX1),
  mY1(aY1),
  mX2(aX2),
  mY2(aY2)
{
  if (mX1 != mY1 || mX2 != mY2)
    CalcSampleValues();
}

double
nsSMILKeySpline::GetSplineValue(double aX) const
{
  if (mX1 == mY1 && mX2 == mY2)
    return aX;

  return CalcBezier(GetTForX(aX), mY1, mY2);
}

void
nsSMILKeySpline::CalcSampleValues()
{
  for (int i = 0; i < kSplineTableSize; ++i) {
    mSampleValues[i] = CalcBezier(double(i) * kSampleStepSize, mX1, mX2);
  }
}

 double
nsSMILKeySpline::CalcBezier(double aT,
                            double aA1,
                            double aA2)
{
  
  return ((A(aA1, aA2)*aT + B(aA1, aA2))*aT + C(aA1))*aT;
}

 double
nsSMILKeySpline::GetSlope(double aT,
                          double aA1,
                          double aA2)
{
  double denom = (3.0 * A(aA1, aA2)*aT*aT + 2.0 * B(aA1, aA2) * aT + C(aA1)); 
  return (denom == 0.0) ? 0.0 : 1.0 / denom;
}

double
nsSMILKeySpline::GetTForX(double aX) const
{
  int i;

  
  
  
  
  
  
  for (i = 0; i < kSplineTableSize - 2 && mSampleValues[i] < aX; ++i);
  double currentT = 
    double(i) * kSampleStepSize + (aX - mSampleValues[i]) * kSampleStepSize;

  
  for (i = 0; i < NEWTON_ITERATIONS; ++i) {
    double currentX = CalcBezier(currentT, mX1, mX2);
    double currentSlope = GetSlope(currentT, mX1, mX2);

    if (currentSlope == 0.0)
      return currentT;

    currentT -= (currentX - aX) * currentSlope;
  }

  return currentT;
}
