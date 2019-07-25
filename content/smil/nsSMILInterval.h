




































#ifndef NS_SMILINTERVAL_H_
#define NS_SMILINTERVAL_H_

#include "nsSMILInstanceTime.h"
#include "nsTArray.h"










class nsSMILInterval
{
public:
  nsSMILInterval();
  nsSMILInterval(const nsSMILInterval& aOther);
  ~nsSMILInterval();
  void NotifyChanged(const nsSMILTimeContainer* aContainer);
  void Unlink(PRBool aFiltered = PR_FALSE);

  const nsSMILInstanceTime* Begin() const
  {
    NS_ABORT_IF_FALSE(mBegin && mEnd,
        "Requesting Begin() on un-initialized instance time");
    return mBegin;
  }
  nsSMILInstanceTime* Begin();

  const nsSMILInstanceTime* End() const
  {
    NS_ABORT_IF_FALSE(mBegin && mEnd,
        "Requesting End() on un-initialized instance time");
    return mEnd;
  }
  nsSMILInstanceTime* End();

  void SetBegin(nsSMILInstanceTime& aBegin);
  void SetEnd(nsSMILInstanceTime& aEnd);
  void Set(nsSMILInstanceTime& aBegin, nsSMILInstanceTime& aEnd)
  {
    SetBegin(aBegin);
    SetEnd(aEnd);
  }

  void FixBegin();
  void FixEnd();

  void AddDependentTime(nsSMILInstanceTime& aTime);
  void RemoveDependentTime(const nsSMILInstanceTime& aTime);

  
  PRBool IsDependencyChainLink() const;

private:
  nsRefPtr<nsSMILInstanceTime> mBegin;
  nsRefPtr<nsSMILInstanceTime> mEnd;

  typedef nsTArray<nsRefPtr<nsSMILInstanceTime> > InstanceTimeList;

  
  InstanceTimeList mDependentTimes;

  
  
  
  
  
  
  
  
  
  PRPackedBool mBeginFixed;
  PRPackedBool mEndFixed;

  
  
  
  
  
  
  
  
  
  
  
  
  PRPackedBool mBeginObjectChanged;
  PRPackedBool mEndObjectChanged;
};

#endif 
