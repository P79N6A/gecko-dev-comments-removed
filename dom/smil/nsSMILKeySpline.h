





#ifndef NS_SMILKEYSPLINE_H_
#define NS_SMILKEYSPLINE_H_

#include "mozilla/ArrayUtils.h"
#include "mozilla/PodOperations.h"




class nsSMILKeySpline
{
public:
  nsSMILKeySpline() {  }

  





  nsSMILKeySpline(double aX1, double aY1,
                  double aX2, double aY2)
  {
    Init(aX1, aY1, aX2, aY2);
  }

  double X1() const { return mX1; }
  double Y1() const { return mY1; }
  double X2() const { return mX2; }
  double Y2() const { return mY2; }

  void Init(double aX1, double aY1,
            double aX2, double aY2);

  





  double GetSplineValue(double aX) const;

  void GetSplineDerivativeValues(double aX, double& aDX, double& aDY) const;

  bool operator==(const nsSMILKeySpline& aOther) const {
    return mX1 == aOther.mX1 &&
           mY1 == aOther.mY1 &&
           mX2 == aOther.mX2 &&
           mY2 == aOther.mY2;
  }
  bool operator!=(const nsSMILKeySpline& aOther) const {
    return !(*this == aOther);
  }

private:
  void
  CalcSampleValues();

  


  static double
  CalcBezier(double aT, double aA1, double aA2);

  


  static double
  GetSlope(double aT, double aA1, double aA2);

  double
  GetTForX(double aX) const;

  double
  NewtonRaphsonIterate(double aX, double aGuessT) const;

  double
  BinarySubdivide(double aX, double aA, double aB) const;

  static double
  A(double aA1, double aA2)
  {
    return 1.0 - 3.0 * aA2 + 3.0 * aA1;
  }

  static double
  B(double aA1, double aA2)
  {
    return 3.0 * aA2 - 6.0 * aA1;
  }

  static double
  C(double aA1)
  {
    return 3.0 * aA1;
  }

  double               mX1;
  double               mY1;
  double               mX2;
  double               mY2;

  enum { kSplineTableSize = 11 };
  double               mSampleValues[kSplineTableSize];

  static const double  kSampleStepSize;
};

#endif 
