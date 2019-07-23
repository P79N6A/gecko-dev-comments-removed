




































#include "prtypes.h"
#include "nsDebug.h"
#include <math.h>












class nsSMILRepeatCount
{
public:
  nsSMILRepeatCount() : mCount(kNotSet) {}
  nsSMILRepeatCount(double aCount) : mCount(kNotSet) { SetCount(aCount); }

  operator double() const { return mCount; }
  PRBool IsDefinite() const {
    return mCount != kNotSet && mCount != kIndefinite;
  }
  PRBool IsIndefinite() const { return mCount == kIndefinite; }
  PRBool IsSet() const { return mCount != kNotSet; }

  nsSMILRepeatCount& operator=(double aCount)
  {
    SetCount(aCount);
    return *this;
  }
  void SetCount(double aCount)
  {
    NS_ASSERTION(aCount > 0.0, "Negative or zero repeat count.");
    mCount = aCount > 0.0 ? aCount : kNotSet;
  }
  void SetIndefinite() { mCount = kIndefinite; }
  void Unset() { mCount = kNotSet; }

private:
  static const double kNotSet;
  static const double kIndefinite;

  double  mCount;
};
