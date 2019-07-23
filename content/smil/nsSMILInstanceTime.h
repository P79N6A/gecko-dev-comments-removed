




































#ifndef NS_SMILINSTANCETIME_H_
#define NS_SMILINSTANCETIME_H_

#include "nsISupports.h"
#include "nsSMILTimeValue.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"

class nsSMILTimeValueSpec;










class nsSMILInstanceTime
{
public:
  nsSMILInstanceTime(const nsSMILTimeValue& aTime,
                     nsSMILTimeValueSpec* aCreator,
                     PRBool aClearOnReset = PR_FALSE);

  ~nsSMILInstanceTime();

  const nsSMILTimeValue& Time() const { return mTime; }

  PRBool                 ClearOnReset() const { return mClearOnReset; }

  

  
  class Comparator {
    public:
      PRBool Equals(const nsSMILInstanceTime& aElem1,
                    const nsSMILInstanceTime& aElem2) const {
        return (aElem1.Time().CompareTo(aElem2.Time()) == 0);
      }
      PRBool LessThan(const nsSMILInstanceTime& aElem1,
                      const nsSMILInstanceTime& aElem2) const {
        return (aElem1.Time().CompareTo(aElem2.Time()) < 0);
      }
  };

protected:
  nsSMILTimeValue     mTime;

  



  PRPackedBool        mClearOnReset;
};

#endif 
