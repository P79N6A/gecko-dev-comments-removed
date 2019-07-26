




#ifndef NS_SMILTIMECONTAINER_H_
#define NS_SMILTIMECONTAINER_H_

#include "mozilla/dom/SVGAnimationElement.h"
#include "nscore.h"
#include "nsSMILTypes.h"
#include "nsTPriorityQueue.h"
#include "nsAutoPtr.h"
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

  







  virtual void Pause(uint32_t aType);

  






  virtual void Resume(uint32_t aType);

  







  bool IsPausedByType(uint32_t aType) const { return mPauseState & aType; }

  






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
  void ClearNeedsRewind() { mNeedsRewind = false; }

  




  bool IsSeeking() const { return mIsSeeking; }
  void MarkSeekFinished() { mIsSeeking = false; }

  




  nsresult SetParent(nsSMILTimeContainer* aParent);

  







  bool AddMilestone(const nsSMILMilestone& aMilestone,
                    mozilla::dom::SVGAnimationElement& aElement);

  


  void ClearMilestones();

  








  bool GetNextMilestoneInParentTime(nsSMILMilestone& aNextMilestone) const;

  typedef nsTArray<nsRefPtr<mozilla::dom::SVGAnimationElement> > AnimElemArray;

  









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

  
  uint32_t mPauseState;

  struct MilestoneEntry
  {
    MilestoneEntry(nsSMILMilestone aMilestone,
                   mozilla::dom::SVGAnimationElement& aElement)
      : mMilestone(aMilestone), mTimebase(&aElement)
    { }

    bool operator<(const MilestoneEntry& aOther) const
    {
      return mMilestone < aOther.mMilestone;
    }

    nsSMILMilestone mMilestone; 
    nsRefPtr<mozilla::dom::SVGAnimationElement> mTimebase;
  };

  
  
  
  
  
  nsTPriorityQueue<MilestoneEntry> mMilestoneEntries;
};

#endif 
