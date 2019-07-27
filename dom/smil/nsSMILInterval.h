




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
  void Unlink(bool aFiltered = false);

  const nsSMILInstanceTime* Begin() const
  {
    MOZ_ASSERT(mBegin && mEnd,
               "Requesting Begin() on un-initialized instance time");
    return mBegin;
  }
  nsSMILInstanceTime* Begin();

  const nsSMILInstanceTime* End() const
  {
    MOZ_ASSERT(mBegin && mEnd,
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

  typedef nsTArray<nsRefPtr<nsSMILInstanceTime> > InstanceTimeList;

  void AddDependentTime(nsSMILInstanceTime& aTime);
  void RemoveDependentTime(const nsSMILInstanceTime& aTime);
  void GetDependentTimes(InstanceTimeList& aTimes);

  
  bool IsDependencyChainLink() const;

private:
  nsRefPtr<nsSMILInstanceTime> mBegin;
  nsRefPtr<nsSMILInstanceTime> mEnd;

  
  InstanceTimeList mDependentTimes;

  
  
  
  
  
  
  
  
  
  bool mBeginFixed;
  bool mEndFixed;
};

#endif 
