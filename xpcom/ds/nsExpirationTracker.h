





#ifndef NSEXPIRATIONTRACKER_H_
#define NSEXPIRATIONTRACKER_H_

#include "prlog.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "mozilla/Attributes.h"
#include "mozilla/Services.h"






struct nsExpirationState
{
  enum
  {
    NOT_TRACKED = (1U << 4) - 1,
    MAX_INDEX_IN_GENERATION = (1U << 28) - 1
  };

  nsExpirationState() : mGeneration(NOT_TRACKED) {}
  bool IsTracked() { return mGeneration != NOT_TRACKED; }

  


  uint32_t mGeneration:4;
  uint32_t mIndexInGeneration:28;
};































template<class T, uint32_t K>
class nsExpirationTracker
{
public:
  






  explicit nsExpirationTracker(uint32_t aTimerPeriod)
    : mTimerPeriod(aTimerPeriod)
    , mNewestGeneration(0)
    , mInAgeOneGeneration(false)
  {
    static_assert(K >= 2 && K <= nsExpirationState::NOT_TRACKED,
                  "Unsupported number of generations (must be 2 <= K <= 15)");
    mObserver = new ExpirationTrackerObserver();
    mObserver->Init(this);
  }
  ~nsExpirationTracker()
  {
    if (mTimer) {
      mTimer->Cancel();
    }
    mObserver->Destroy();
  }

  




  nsresult AddObject(T* aObj)
  {
    nsExpirationState* state = aObj->GetExpirationState();
    NS_ASSERTION(!state->IsTracked(),
                 "Tried to add an object that's already tracked");
    nsTArray<T*>& generation = mGenerations[mNewestGeneration];
    uint32_t index = generation.Length();
    if (index > nsExpirationState::MAX_INDEX_IN_GENERATION) {
      NS_WARNING("More than 256M elements tracked, this is probably a problem");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (index == 0) {
      
      nsresult rv = CheckStartTimer();
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
    if (!generation.AppendElement(aObj)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    state->mGeneration = mNewestGeneration;
    state->mIndexInGeneration = index;
    return NS_OK;
  }

  


  void RemoveObject(T* aObj)
  {
    nsExpirationState* state = aObj->GetExpirationState();
    NS_ASSERTION(state->IsTracked(), "Tried to remove an object that's not tracked");
    nsTArray<T*>& generation = mGenerations[state->mGeneration];
    uint32_t index = state->mIndexInGeneration;
    NS_ASSERTION(generation.Length() > index &&
                 generation[index] == aObj, "Object is lying about its index");
    
    uint32_t last = generation.Length() - 1;
    T* lastObj = generation[last];
    generation[index] = lastObj;
    lastObj->GetExpirationState()->mIndexInGeneration = index;
    generation.RemoveElementAt(last);
    state->mGeneration = nsExpirationState::NOT_TRACKED;
    
    
    
    
    
  }

  



  nsresult MarkUsed(T* aObj)
  {
    nsExpirationState* state = aObj->GetExpirationState();
    if (mNewestGeneration == state->mGeneration) {
      return NS_OK;
    }
    RemoveObject(aObj);
    return AddObject(aObj);
  }

  



  void AgeOneGeneration()
  {
    if (mInAgeOneGeneration) {
      NS_WARNING("Can't reenter AgeOneGeneration from NotifyExpired");
      return;
    }

    mInAgeOneGeneration = true;
    uint32_t reapGeneration =
      mNewestGeneration > 0 ? mNewestGeneration - 1 : K - 1;
    nsTArray<T*>& generation = mGenerations[reapGeneration];
    
    
    
    
    
    
    
    
    size_t index = generation.Length();
    for (;;) {
      
      
      index = XPCOM_MIN(index, generation.Length());
      if (index == 0) {
        break;
      }
      --index;
      NotifyExpired(generation[index]);
    }
    
    
    if (!generation.IsEmpty()) {
      NS_WARNING("Expired objects were not removed or marked used");
    }
    
    
    generation.Compact();
    mNewestGeneration = reapGeneration;
    mInAgeOneGeneration = false;
  }

  






  void AgeAllGenerations()
  {
    uint32_t i;
    for (i = 0; i < K; ++i) {
      AgeOneGeneration();
    }
  }

  class Iterator
  {
  private:
    nsExpirationTracker<T, K>* mTracker;
    uint32_t mGeneration;
    uint32_t mIndex;
  public:
    explicit Iterator(nsExpirationTracker<T, K>* aTracker)
      : mTracker(aTracker)
      , mGeneration(0)
      , mIndex(0)
    {
    }

    T* Next()
    {
      while (mGeneration < K) {
        nsTArray<T*>* generation = &mTracker->mGenerations[mGeneration];
        if (mIndex < generation->Length()) {
          ++mIndex;
          return (*generation)[mIndex - 1];
        }
        ++mGeneration;
        mIndex = 0;
      }
      return nullptr;
    }
  };

  friend class Iterator;

  bool IsEmpty()
  {
    for (uint32_t i = 0; i < K; ++i) {
      if (!mGenerations[i].IsEmpty()) {
        return false;
      }
    }
    return true;
  }

protected:
  
























  virtual void NotifyExpired(T* aObj) = 0;

private:
  class ExpirationTrackerObserver;
  nsRefPtr<ExpirationTrackerObserver> mObserver;
  nsTArray<T*>       mGenerations[K];
  nsCOMPtr<nsITimer> mTimer;
  uint32_t           mTimerPeriod;
  uint32_t           mNewestGeneration;
  bool               mInAgeOneGeneration;

  



  class ExpirationTrackerObserver final : public nsIObserver
  {
  public:
    void Init(nsExpirationTracker<T, K>* aObj)
    {
      mOwner = aObj;
      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      if (obs) {
        obs->AddObserver(this, "memory-pressure", false);
      }
    }
    void Destroy()
    {
      mOwner = nullptr;
      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      if (obs) {
        obs->RemoveObserver(this, "memory-pressure");
      }
    }
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
  private:
    nsExpirationTracker<T, K>* mOwner;
  };

  static void TimerCallback(nsITimer* aTimer, void* aThis)
  {
    nsExpirationTracker* tracker = static_cast<nsExpirationTracker*>(aThis);
    tracker->AgeOneGeneration();
    
    if (tracker->IsEmpty()) {
      tracker->mTimer->Cancel();
      tracker->mTimer = nullptr;
    }
  }

  nsresult CheckStartTimer()
  {
    if (mTimer || !mTimerPeriod) {
      return NS_OK;
    }
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mTimer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTimer->InitWithFuncCallback(TimerCallback, this, mTimerPeriod,
                                 nsITimer::TYPE_REPEATING_SLACK);
    return NS_OK;
  }
};

template<class T, uint32_t K>
NS_IMETHODIMP
nsExpirationTracker<T, K>::ExpirationTrackerObserver::Observe(
    nsISupports* aSubject, const char* aTopic, const char16_t* aData)
{
  if (!strcmp(aTopic, "memory-pressure") && mOwner) {
    mOwner->AgeAllGenerations();
  }
  return NS_OK;
}

template<class T, uint32_t K>
NS_IMETHODIMP_(MozExternalRefCountType)
nsExpirationTracker<T, K>::ExpirationTrackerObserver::AddRef(void)
{
  MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");
  NS_ASSERT_OWNINGTHREAD(ExpirationTrackerObserver);
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "ExpirationTrackerObserver", sizeof(*this));
  return mRefCnt;
}

template<class T, uint32_t K>
NS_IMETHODIMP_(MozExternalRefCountType)
nsExpirationTracker<T, K>::ExpirationTrackerObserver::Release(void)
{
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");
  NS_ASSERT_OWNINGTHREAD(ExpirationTrackerObserver);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "ExpirationTrackerObserver");
  if (mRefCnt == 0) {
    NS_ASSERT_OWNINGTHREAD(ExpirationTrackerObserver);
    mRefCnt = 1; 
    delete (this);
    return 0;
  }
  return mRefCnt;
}

template<class T, uint32_t K>
NS_IMETHODIMP
nsExpirationTracker<T, K>::ExpirationTrackerObserver::QueryInterface(
    REFNSIID aIID, void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr,
               "QueryInterface requires a non-NULL destination!");
  nsresult rv = NS_ERROR_FAILURE;
  NS_INTERFACE_TABLE(ExpirationTrackerObserver, nsIObserver)
  return rv;
}

#endif 
