




































#ifndef NS_SMILTIMECONTAINER_H_
#define NS_SMILTIMECONTAINER_H_

#include "nscore.h"
#include "nsSMILTypes.h"
#include "nsTPriorityQueue.h"
#include "nsAutoPtr.h"
#include "nsISMILAnimationElement.h"
#include "nsSMILMilestone.h"

class nsSMILTimeValue;






class nsSMILTimeContainer
{
public:
  nsSMILTimeContainer();
  virtual ~nsSMILTimeContainer();

  


  enum {
    PAUSE_BEGIN    =  1, 
    PAUSE_SCRIPT   =  2, 
    PAUSE_PAGEHIDE =  4, 
    PAUSE_USERPREF =  8, 
    PAUSE_IMAGE    = 16  
  };

  


  void Begin();

  







  virtual void Pause(PRUint32 aType);

  






  virtual void Resume(PRUint32 aType);

  







  bool IsPausedByType(PRUint32 aType) const { return mPauseState & aType; }

  






  bool IsPaused() const { return mPauseState != 0; }

  



  nsSMILTime GetCurrentTime() const;

  





  void SetCurrentTime(nsSMILTime aSeekTo);

  


  virtual nsSMILTime GetParentTime() const;

  






  nsSMILTimeValue ContainerToParentTime(nsSMILTime aContainerTime) const;

  






  nsSMILTimeValue ParentToContainerTime(nsSMILTime aParentTime) const;

  





  void SyncPauseTime();

  



  void Sample();

  





  bool NeedsSample() const { return !mPauseState || mNeedsPauseSample; }

  



  bool NeedsRewind() const { return mNeedsRewind; }
  void ClearNeedsRewind() { mNeedsRewind = PR_FALSE; }

  




  bool IsSeeking() const { return mIsSeeking; }
  void MarkSeekFinished() { mIsSeeking = PR_FALSE; }

  




  nsresult SetParent(nsSMILTimeContainer* aParent);

  







  bool AddMilestone(const nsSMILMilestone& aMilestone,
                      nsISMILAnimationElement& aElement);

  


  void ClearMilestones();

  








  bool GetNextMilestoneInParentTime(nsSMILMilestone& aNextMilestone) const;

  typedef nsTArray<nsRefPtr<nsISMILAnimationElement> > AnimElemArray;

  









  bool PopMilestoneElementsAtMilestone(const nsSMILMilestone& aMilestone,
                                         AnimElemArray& aMatchedElements);

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

protected:
  



  virtual void DoSample() { }

  




  


  virtual nsresult AddChild(nsSMILTimeContainer& aChild)
  {
    return NS_ERROR_FAILURE;
  }

  


  virtual void RemoveChild(nsSMILTimeContainer& aChild) { }

  


  void UpdateCurrentTime();

  



  void NotifyTimeChange();

  
  nsSMILTimeContainer* mParent;

  
  nsSMILTime mCurrentTime;

  
  
  
  
  
  
  nsSMILTime mParentOffset;

  
  nsSMILTime mPauseStart;

  
  bool mNeedsPauseSample;

  bool mNeedsRewind; 
  bool mIsSeeking; 

  
  PRUint32 mPauseState;

  struct MilestoneEntry
  {
    MilestoneEntry(nsSMILMilestone aMilestone,
                   nsISMILAnimationElement& aElement)
      : mMilestone(aMilestone), mTimebase(&aElement)
    { }

    bool operator<(const MilestoneEntry& aOther) const
    {
      return mMilestone < aOther.mMilestone;
    }

    nsSMILMilestone mMilestone; 
    nsRefPtr<nsISMILAnimationElement> mTimebase;
  };

  
  
  
  
  
  nsTPriorityQueue<MilestoneEntry> mMilestoneEntries;
};

#endif 
