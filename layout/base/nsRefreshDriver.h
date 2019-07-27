










#ifndef nsRefreshDriver_h_
#define nsRefreshDriver_h_

#include "mozilla/TimeStamp.h"
#include "mozFlushType.h"
#include "nsTObserverArray.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "GeckoProfiler.h"
#include "mozilla/layers/TransactionIdAllocator.h"

class nsPresContext;
class nsIPresShell;
class nsIDocument;
class imgIRequest;
class nsIRunnable;

namespace mozilla {
class RefreshDriverTimer;
namespace layout {
class VsyncChild;
}
}






class nsARefreshObserver {
public:
  
  
  
  
  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) = 0;

  virtual void WillRefresh(mozilla::TimeStamp aTime) = 0;
};






class nsAPostRefreshObserver {
public:
  virtual void DidRefresh() = 0;
};

class nsRefreshDriver final : public mozilla::layers::TransactionIdAllocator,
                              public nsARefreshObserver
{
public:
  explicit nsRefreshDriver(nsPresContext *aPresContext);
  ~nsRefreshDriver();

  static void InitializeStatics();
  static void Shutdown();

  



  void AdvanceTimeAndRefresh(int64_t aMilliseconds);
  void RestoreNormalRefresh();
  void DoTick();
  bool IsTestControllingRefreshesEnabled() const
  {
    return mTestControllingRefreshes;
  }

  






  mozilla::TimeStamp MostRecentRefresh() const;
  


  int64_t MostRecentRefreshEpochTime() const;

  

















  bool AddRefreshObserver(nsARefreshObserver *aObserver,
                          mozFlushType aFlushType);
  bool RemoveRefreshObserver(nsARefreshObserver *aObserver,
                             mozFlushType aFlushType);

  




  void AddPostRefreshObserver(nsAPostRefreshObserver *aObserver);
  void RemovePostRefreshObserver(nsAPostRefreshObserver *aObserver);

  













  bool AddImageRequest(imgIRequest* aRequest);
  void RemoveImageRequest(imgIRequest* aRequest);

  


  bool AddStyleFlushObserver(nsIPresShell* aShell) {
    NS_ASSERTION(!mStyleFlushObservers.Contains(aShell),
		 "Double-adding style flush observer");
    
    
    
    
    if (!mStyleCause) {
      mStyleCause = profiler_get_backtrace();
    }
    bool appended = mStyleFlushObservers.AppendElement(aShell) != nullptr;
    EnsureTimerStarted();

    return appended;
  }
  void RemoveStyleFlushObserver(nsIPresShell* aShell) {
    mStyleFlushObservers.RemoveElement(aShell);
  }
  bool AddLayoutFlushObserver(nsIPresShell* aShell) {
    NS_ASSERTION(!IsLayoutFlushObserver(aShell),
		 "Double-adding layout flush observer");
    
    
    
    
    if (!mReflowCause) {
      mReflowCause = profiler_get_backtrace();
    }
    bool appended = mLayoutFlushObservers.AppendElement(aShell) != nullptr;
    EnsureTimerStarted();
    return appended;
  }
  void RemoveLayoutFlushObserver(nsIPresShell* aShell) {
    mLayoutFlushObservers.RemoveElement(aShell);
  }
  bool IsLayoutFlushObserver(nsIPresShell* aShell) {
    return mLayoutFlushObservers.Contains(aShell);
  }
  bool AddPresShellToInvalidateIfHidden(nsIPresShell* aShell) {
    NS_ASSERTION(!mPresShellsToInvalidateIfHidden.Contains(aShell),
		 "Double-adding style flush observer");
    bool appended = mPresShellsToInvalidateIfHidden.AppendElement(aShell) != nullptr;
    EnsureTimerStarted();
    return appended;
  }
  void RemovePresShellToInvalidateIfHidden(nsIPresShell* aShell) {
    mPresShellsToInvalidateIfHidden.RemoveElement(aShell);
  }

  


  void ScheduleViewManagerFlush();
  void RevokeViewManagerFlush() {
    mViewManagerFlushIsPending = false;
  }
  bool ViewManagerFlushIsPending() {
    return mViewManagerFlushIsPending;
  }

  


  void ScheduleFrameRequestCallbacks(nsIDocument* aDocument);

  


  void RevokeFrameRequestCallbacks(nsIDocument* aDocument);

  




  void Disconnect() {
    StopTimer();
    mPresContext = nullptr;
  }

  bool IsFrozen() { return mFreezeCount > 0; }

  





  void Freeze();

  




  void Thaw();

  



  void SetThrottled(bool aThrottled);

  


  nsPresContext* PresContext() const { return mPresContext; }

  





  static void PVsyncActorCreated(mozilla::layout::VsyncChild* aVsyncChild);

#ifdef DEBUG
  


  bool IsRefreshObserver(nsARefreshObserver *aObserver,
			   mozFlushType aFlushType);
#endif

  


  static int32_t DefaultInterval();

  bool IsInRefresh() { return mInRefresh; }

  
  virtual uint64_t GetTransactionId() override;
  void NotifyTransactionCompleted(uint64_t aTransactionId) override;
  void RevokeTransactionId(uint64_t aTransactionId) override;
  mozilla::TimeStamp GetTransactionStart() override;

  bool IsWaitingForPaint(mozilla::TimeStamp aTime);

  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override { return TransactionIdAllocator::AddRef(); }
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override { return TransactionIdAllocator::Release(); }
  virtual void WillRefresh(mozilla::TimeStamp aTime) override;
private:
  typedef nsTObserverArray<nsARefreshObserver*> ObserverArray;
  typedef nsTHashtable<nsISupportsHashKey> RequestTable;
  struct ImageStartData {
    ImageStartData()
    {
    }

    mozilla::Maybe<mozilla::TimeStamp> mStartTime;
    RequestTable mEntries;
  };
  typedef nsClassHashtable<nsUint32HashKey, ImageStartData> ImageStartTable;

  void RunFrameRequestCallbacks(int64_t aNowEpoch, mozilla::TimeStamp aNowTime);

  void Tick(int64_t aNowEpoch, mozilla::TimeStamp aNowTime);

  enum EnsureTimerStartedFlags {
    eNone = 0,
    eAdjustingTimer = 1 << 0,
    eAllowTimeToGoBackwards = 1 << 1
  };
  void EnsureTimerStarted(EnsureTimerStartedFlags aFlags = eNone);
  void StopTimer();

  uint32_t ObserverCount() const;
  uint32_t ImageRequestCount() const;
  static PLDHashOperator ImageRequestEnumerator(nsISupportsHashKey* aEntry,
                                                void* aUserArg);
  static PLDHashOperator StartTableRequestCounter(const uint32_t& aKey,
                                                  ImageStartData* aEntry,
                                                  void* aUserArg);
  static PLDHashOperator StartTableRefresh(const uint32_t& aKey,
                                           ImageStartData* aEntry,
                                           void* aUserArg);
  static PLDHashOperator BeginRefreshingImages(nsISupportsHashKey* aEntry,
                                               void* aUserArg);
  ObserverArray& ArrayFor(mozFlushType aFlushType);
  
  void DoRefresh();

  double GetRefreshTimerInterval() const;
  double GetRegularTimerInterval(bool *outIsDefault = nullptr) const;
  static double GetThrottledTimerInterval();

  bool HaveFrameRequestCallbacks() const {
    return mFrameRequestCallbackDocs.Length() != 0;
  }

  void FinishedWaitingForTransaction();

  mozilla::RefreshDriverTimer* ChooseTimer() const;
  mozilla::RefreshDriverTimer* mActiveTimer;

  ProfilerBacktrace* mReflowCause;
  ProfilerBacktrace* mStyleCause;

  nsPresContext *mPresContext; 
                               

  nsRefPtr<nsRefreshDriver> mRootRefresh;

  
  uint64_t mPendingTransaction;
  
  uint64_t mCompletedTransaction;

  uint32_t mFreezeCount;

  
  
  const mozilla::TimeDuration mThrottledFrameRequestInterval;

  bool mThrottled;
  bool mTestControllingRefreshes;
  bool mViewManagerFlushIsPending;
  bool mRequestedHighPrecision;
  bool mInRefresh;

  
  
  bool mWaitingForTransaction;
  
  
  
  bool mSkippedPaints;

  int64_t mMostRecentRefreshEpochTime;
  mozilla::TimeStamp mMostRecentRefresh;
  mozilla::TimeStamp mMostRecentTick;
  mozilla::TimeStamp mTickStart;
  mozilla::TimeStamp mNextThrottledFrameRequestTick;

  
  ObserverArray mObservers[3];
  RequestTable mRequests;
  ImageStartTable mStartTable;

  nsAutoTArray<nsIPresShell*, 16> mStyleFlushObservers;
  nsAutoTArray<nsIPresShell*, 16> mLayoutFlushObservers;
  nsAutoTArray<nsIPresShell*, 16> mPresShellsToInvalidateIfHidden;
  
  nsTArray<nsIDocument*> mFrameRequestCallbackDocs;
  nsTArray<nsIDocument*> mThrottledFrameRequestCallbackDocs;
  nsTArray<nsAPostRefreshObserver*> mPostRefreshObservers;

  
  struct ImageRequestParameters {
    mozilla::TimeStamp mCurrent;
    mozilla::TimeStamp mPrevious;
    RequestTable* mRequests;
    mozilla::TimeStamp mDesired;
  };

  friend class mozilla::RefreshDriverTimer;

  
  void ConfigureHighPrecision();
  void SetHighPrecisionTimersEnabled(bool aEnable);
};

#endif 
