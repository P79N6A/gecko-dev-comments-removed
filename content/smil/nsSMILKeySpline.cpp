




































#include "nsSMILKeySpline.h"
#include "prtypes.h"
#include <math.h>

#define NEWTON_ITERATIONS          4
#define NEWTON_MIN_SLOPE           0.02
#define SUBDIVISION_PRECISION      0.0000001
#define SUBDIVISION_MAX_ITERATIONS 10

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
  for (PRUint32 i = 0; i < kSplineTableSize; ++i) {
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
  return 3.0 * A(aA1, aA2)*aT*aT + 2.0 * B(aA1, aA2) * aT + C(aA1);
}

double
nsSMILKeySpline::GetTForX(double aX) const
{
  
  double intervalStart = 0.0;
  const double* currentSample = &mSampleValues[1];
  const double* const lastSample = &mSampleValues[kSplineTableSize - 1];
  for (; currentSample != lastSample && *currentSample <= aX;
        ++currentSample) {
    intervalStart += kSampleStepSize;
  }
  --currentSample; 

  
  double dist = (aX - *currentSample) /
                (*(currentSample+1) - *currentSample);
  double guessForT = intervalStart + dist * kSampleStepSize;

  
  
  
  double initialSlope = GetSlope(guessForT, mX1, mX2);
  if (initialSlope >= NEWTON_MIN_SLOPE) {
    return NewtonRaphsonIterate(aX, guessForT);
  } else if (initialSlope == 0.0) {
    return guessForT;
  } else {
    return BinarySubdivide(aX, intervalStart, intervalStart + kSampleStepSize);
  }
}

double
nsSMILKeySpline::NewtonRaphsonIterate(double aX, double aGuessT) const
{
  
  for (PRUint32 i = 0; i < NEWTON_ITERATIONS; ++i) {
    
    
    double currentX = CalcBezier(aGuessT, mX1, mX2) - aX;
    double currentSlope = GetSlope(aGuessT, mX1, mX2);

    if (currentSlope == 0.0)
      return aGuessT;

    aGuessT -= currentX / currentSlope;
  }

  return aGuessT;
}

double
nsSMILKeySpline::BinarySubdivide(double aX, double aA, double aB) const
{
  double currentX;
  double currentT;
  PRUint32 i = 0;

  do
  {
    currentT = aA + (aB - aA) / 2.0;
    currentX = CalcBezier(currentT, mX1, mX2) - aX;

    if (currentX > 0.0) {
      aB = currentT;
    } else {
      aA = currentT;
    }
  } while (fabs(currentX) > SUBDIVISION_PRECISION
           && ++i < SUBDIVISION_MAX_ITERATIONS);

  return currentT;
}
