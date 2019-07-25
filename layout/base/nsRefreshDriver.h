









































#ifndef nsRefreshDriver_h_
#define nsRefreshDriver_h_

#include "mozilla/TimeStamp.h"
#include "mozFlushType.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

class nsPresContext;
class nsIPresShell;
class nsIDocument;
class imgIRequest;






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

  















  bool AddRefreshObserver(nsARefreshObserver *aObserver,
                            mozFlushType aFlushType);
  bool RemoveRefreshObserver(nsARefreshObserver *aObserver,
                               mozFlushType aFlushType);

  













  bool AddImageRequest(imgIRequest* aRequest);
  void RemoveImageRequest(imgIRequest* aRequest);
  void ClearAllImageRequests();

  


  bool AddStyleFlushObserver(nsIPresShell* aShell) {
    NS_ASSERTION(!mStyleFlushObservers.Contains(aShell),
		 "Double-adding style flush observer");
    bool appended = mStyleFlushObservers.AppendElement(aShell) != nsnull;
    EnsureTimerStarted(false);
    return appended;
  }
  void RemoveStyleFlushObserver(nsIPresShell* aShell) {
    mStyleFlushObservers.RemoveElement(aShell);
  }
  bool AddLayoutFlushObserver(nsIPresShell* aShell) {
    NS_ASSERTION(!IsLayoutFlushObserver(aShell),
		 "Double-adding layout flush observer");
    bool appended = mLayoutFlushObservers.AppendElement(aShell) != nsnull;
    EnsureTimerStarted(false);
    return appended;
  }
  void RemoveLayoutFlushObserver(nsIPresShell* aShell) {
    mLayoutFlushObservers.RemoveElement(aShell);
  }
  bool IsLayoutFlushObserver(nsIPresShell* aShell) {
    return mLayoutFlushObservers.Contains(aShell);
  }

  


  void ScheduleFrameRequestCallbacks(nsIDocument* aDocument);

  


  void RevokeFrameRequestCallbacks(nsIDocument* aDocument);

  




  void Disconnect() {
    StopTimer();
    mPresContext = nsnull;
  }

  



  void Freeze();

  



  void Thaw();

  



  void SetThrottled(bool aThrottled);

  


  nsPresContext* PresContext() const { return mPresContext; }

#ifdef DEBUG
  


  bool IsRefreshObserver(nsARefreshObserver *aObserver,
			   mozFlushType aFlushType);
#endif

private:
  typedef nsTObserverArray<nsARefreshObserver*> ObserverArray;
  typedef nsTHashtable<nsISupportsHashKey> RequestTable;

  void EnsureTimerStarted(bool aAdjustingTimer);
  void StopTimer();

  PRUint32 ObserverCount() const;
  PRUint32 ImageRequestCount() const;
  static PLDHashOperator ImageRequestEnumerator(nsISupportsHashKey* aEntry,
                                          void* aUserArg);
  void UpdateMostRecentRefresh();
  ObserverArray& ArrayFor(mozFlushType aFlushType);
  
  void DoRefresh();

  PRInt32 GetRefreshTimerInterval() const;
  PRInt32 GetRefreshTimerType() const;

  bool HaveFrameRequestCallbacks() const {
    return mFrameRequestCallbackDocs.Length() != 0;
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
  RequestTable mRequests;

  nsAutoTArray<nsIPresShell*, 16> mStyleFlushObservers;
  nsAutoTArray<nsIPresShell*, 16> mLayoutFlushObservers;
  
  nsTArray<nsIDocument*> mFrameRequestCallbackDocs;

  
  
  mutable PRInt32 mLastTimerInterval;

  
  struct ImageRequestParameters {
      mozilla::TimeStamp ts;
  };
};

#endif 
