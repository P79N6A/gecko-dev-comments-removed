




































#ifndef NS_SMILMILESTONE_H_
#define NS_SMILMILESTONE_H_


























class nsSMILMilestone
{
public:
  nsSMILMilestone(nsSMILTime aTime, bool aIsEnd)
    : mTime(aTime), mIsEnd(aIsEnd)
  { }

  nsSMILMilestone()
    : mTime(0), mIsEnd(PR_FALSE)
  { }

  bool operator==(const nsSMILMilestone& aOther) const
  {
    return mTime == aOther.mTime && mIsEnd == aOther.mIsEnd;
  }

  bool operator!=(const nsSMILMilestone& aOther) const
  {
    return !(*this == aOther);
  }

  bool operator<(const nsSMILMilestone& aOther) const
  {
    
    return mTime < aOther.mTime ||
          (mTime == aOther.mTime && mIsEnd && !aOther.mIsEnd);
  }

  bool operator<=(const nsSMILMilestone& aOther) const
  {
    return *this == aOther || *this < aOther;
  }

  bool operator>=(const nsSMILMilestone& aOther) const
  {
    return !(*this < aOther);
  }

  nsSMILTime   mTime;  
                       
  bool mIsEnd; 
                       
};

#endif 
