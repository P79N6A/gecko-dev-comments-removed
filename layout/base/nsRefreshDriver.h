









































#ifndef nsRefreshDriver_h_
#define nsRefreshDriver_h_

#include "mozilla/TimeStamp.h"
#include "mozFlushType.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsPresContext;
class nsIPresShell;
class nsIDocument;






class nsARefreshObserver {
public:
  
  
  
  
  
  
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;

  virtual void WillRefresh(mozilla::TimeStamp aTime) = 0;
};

class nsRefreshDriver : public nsITimerCallback {
public:
  nsRefreshDriver(nsPresContext *aPresContext);
  ~nsRefreshDriver();

  static void InitializeStatics();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITIMERCALLBACK

  



  void AdvanceTimeAndRefresh(PRInt64 aMilliseconds);
  void RestoreNormalRefresh();

  






  mozilla::TimeStamp MostRecentRefresh() const;
  


  PRInt64 MostRecentRefreshEpochTime() const;

  















  PRBool AddRefreshObserver(nsARefreshObserver *aObserver,
                            mozFlushType aFlushType);
  PRBool RemoveRefreshObserver(nsARefreshObserver *aObserver,
                               mozFlushType aFlushType);

  


  PRBool AddStyleFlushObserver(nsIPresShell* aShell) {
    NS_ASSERTION(!mStyleFlushObservers.Contains(aShell),
		 "Double-adding style flush observer");
    PRBool appended = mStyleFlushObservers.AppendElement(aShell) != nsnull;
    EnsureTimerStarted(false);
    return appended;
  }
  void RemoveStyleFlushObserver(nsIPresShell* aShell) {
    mStyleFlushObservers.RemoveElement(aShell);
  }
  PRBool AddLayoutFlushObserver(nsIPresShell* aShell) {
    NS_ASSERTION(!IsLayoutFlushObserver(aShell),
		 "Double-adding layout flush observer");
    PRBool appended = mLayoutFlushObservers.AppendElement(aShell) != nsnull;
    EnsureTimerStarted(false);
    return appended;
  }
  void RemoveLayoutFlushObserver(nsIPresShell* aShell) {
    mLayoutFlushObservers.RemoveElement(aShell);
  }
  PRBool IsLayoutFlushObserver(nsIPresShell* aShell) {
    return mLayoutFlushObservers.Contains(aShell);
  }

  


  PRBool ScheduleBeforePaintEvent(nsIDocument* aDocument);

  


  void ScheduleAnimationFrameListeners(nsIDocument* aDocument);

  


  void RevokeBeforePaintEvent(nsIDocument* aDocument);

  


  void RevokeAnimationFrameListeners(nsIDocument* aDocument);

  




  void Disconnect() {
    StopTimer();
    mPresContext = nsnull;
  }

  



  void Freeze();

  



  void Thaw();

  



  void SetThrottled(bool aThrottled);

  


  nsPresContext* PresContext() const { return mPresContext; }

#ifdef DEBUG
  


  PRBool IsRefreshObserver(nsARefreshObserver *aObserver,
			   mozFlushType aFlushType);
#endif

private:
  typedef nsTObserverArray<nsARefreshObserver*> ObserverArray;

  void EnsureTimerStarted(bool aAdjustingTimer);
  void StopTimer();
  PRUint32 ObserverCount() const;
  void UpdateMostRecentRefresh();
  ObserverArray& ArrayFor(mozFlushType aFlushType);
  
  void DoRefresh();

  PRInt32 GetRefreshTimerInterval() const;
  PRInt32 GetRefreshTimerType() const;

  bool HaveAnimationFrameListeners() const {
    return mAnimationFrameListenerDocs.Length() != 0;
  }

  nsCOMPtr<nsITimer> mTimer;
  mozilla::TimeStamp mMostRecentRefresh; 
  PRInt64 mMostRecentRefreshEpochTime;   
                                         

  nsPresContext *mPresContext; 
                               

  bool mFrozen;
  bool mThrottled;
  bool mTestControllingRefreshes;
  


  bool mTimerIsPrecise;

  
  ObserverArray mObservers[3];
  nsAutoTArray<nsIPresShell*, 16> mStyleFlushObservers;
  nsAutoTArray<nsIPresShell*, 16> mLayoutFlushObservers;
  
  nsTArray< nsCOMPtr<nsIDocument> > mBeforePaintTargets;
  
  nsTArray<nsIDocument*> mAnimationFrameListenerDocs;

  
  
  mutable PRInt32 mLastTimerInterval;
};

#endif 
