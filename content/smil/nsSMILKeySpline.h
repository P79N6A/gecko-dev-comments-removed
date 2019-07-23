




































#ifndef NS_SMILKEYSPLINE_H_
#define NS_SMILKEYSPLINE_H_




class nsSMILKeySpline
{
public:
  nsSMILKeySpline() {  }

  





  nsSMILKeySpline(double aX1, double aY1,
                  double aX2, double aY2)
  {
    Init(aX1, aY1, aX2, aY2);
  }

  void Init(double aX1, double aY1,
            double aX2, double aY2);

  





  double GetSplineValue(double aX) const;

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
