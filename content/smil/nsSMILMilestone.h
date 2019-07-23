




































#ifndef NS_SMILMILESTONE_H_
#define NS_SMILMILESTONE_H_


























class nsSMILMilestone
{
public:
  nsSMILMilestone(nsSMILTime aTime, PRBool aIsEnd)
    : mTime(aTime), mIsEnd(aIsEnd)
  { }

  nsSMILMilestone()
    : mTime(0), mIsEnd(PR_FALSE)
  { }

  PRBool operator==(const nsSMILMilestone& aOther) const
  {
    return mTime == aOther.mTime && mIsEnd == aOther.mIsEnd;
  }

  PRBool operator!=(const nsSMILMilestone& aOther) const
  {
    return !(*this == aOther);
  }

  PRBool operator<(const nsSMILMilestone& aOther) const
  {
    
    return mTime < aOther.mTime ||
          (mTime == aOther.mTime && mIsEnd && !aOther.mIsEnd);
  }

  PRBool operator<=(const nsSMILMilestone& aOther) const
  {
    return *this == aOther || *this < aOther;
  }

  PRBool operator>=(const nsSMILMilestone& aOther) const
  {
    return !(*this < aOther);
  }

  nsSMILTime   mTime;  
                       
  PRPackedBool mIsEnd; 
                       
};

#endif 
