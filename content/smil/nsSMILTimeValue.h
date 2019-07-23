




































#ifndef NS_SMILTIMEVALUE_H_
#define NS_SMILTIMEVALUE_H_

#include "prtypes.h"
#include "prlong.h"
#include "nsSMILTypes.h"

























































class nsSMILTimeValue
{
public:
  
  nsSMILTimeValue();

  PRBool            IsIndefinite() const { return mState == STATE_INDEFINITE; }
  void              SetIndefinite();

  PRBool            IsResolved() const { return mState == STATE_RESOLVED; }
  void              SetUnresolved();

  nsSMILTime        GetMillis() const;
  void              SetMillis(nsSMILTime aMillis);

  PRInt8            CompareTo(const nsSMILTimeValue& aOther) const;

private:
  PRInt8            Cmp(PRInt64 aA, PRInt64 aB) const;

  static nsSMILTime kUnresolvedSeconds;

  nsSMILTime        mMilliseconds;
  enum {
    STATE_RESOLVED,
    STATE_INDEFINITE,
    STATE_UNRESOLVED
  } mState;
};

#endif 
