




































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
    PAUSE_BEGIN    = 1,
    PAUSE_SCRIPT   = 2,
    PAUSE_PAGEHIDE = 4,
    PAUSE_USERPREF = 8
  };

  


  void Begin();

  







  virtual void Pause(PRUint32 aType);

  






  virtual void Resume(PRUint32 aType);

  







  PRBool IsPausedByType(PRUint32 aType) const { return mPauseState & aType; }

  






  PRBool IsPaused() const { return mPauseState != 0; }

  



  nsSMILTime GetCurrentTime() const;

  





  void SetCurrentTime(nsSMILTime aSeekTo);

  


  virtual nsSMILTime GetParentTime() const;

  






  nsSMILTimeValue ContainerToParentTime(nsSMILTime aContainerTime) const;

  






  nsSMILTimeValue ParentToContainerTime(nsSMILTime aParentTime) const;

  





  void SyncPauseTime();

  



  void Sample();

  





  PRBool NeedsSample() const { return !mPauseState || mNeedsPauseSample; }

  




  nsresult SetParent(nsSMILTimeContainer* aParent);

  







  PRBool AddMilestone(const nsSMILMilestone& aMilestone,
                      nsISMILAnimationElement& aElement);

  


  void ClearMilestones();

  








  PRBool GetNextMilestoneInParentTime(nsSMILMilestone& aNextMilestone) const;

  typedef nsTArray<nsRefPtr<nsISMILAnimationElement> > AnimElemArray;

  









  PRBool PopMilestoneElementsAtMilestone(const nsSMILMilestone& aMilestone,
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

  
  PRPackedBool mNeedsPauseSample;

  
  PRUint32 mPauseState;

  struct MilestoneEntry
  {
    MilestoneEntry(nsSMILMilestone aMilestone,
                   nsISMILAnimationElement& aElement)
      : mMilestone(aMilestone), mTimebase(&aElement)
    { }

    PRBool operator<(const MilestoneEntry& aOther) const
    {
      return mMilestone < aOther.mMilestone;
    }

    nsSMILMilestone mMilestone; 
    nsRefPtr<nsISMILAnimationElement> mTimebase;
  };

  
  
  
  
  
  nsTPriorityQueue<MilestoneEntry> mMilestoneEntries;
};

#endif 
