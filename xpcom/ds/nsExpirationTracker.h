





































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
#include "mozilla/Services.h"






struct nsExpirationState {
  enum { NOT_TRACKED = (1U << 4) - 1,
         MAX_INDEX_IN_GENERATION = (1U << 28) - 1 };

  nsExpirationState() : mGeneration(NOT_TRACKED) {}
  bool IsTracked() { return mGeneration != NOT_TRACKED; }

  


  PRUint32 mGeneration:4;
  PRUint32 mIndexInGeneration:28;
};































template <class T, PRUint32 K> class nsExpirationTracker {
  public:
    






    nsExpirationTracker(PRUint32 aTimerPeriod)
      : mTimerPeriod(aTimerPeriod), mNewestGeneration(0),
        mInAgeOneGeneration(false) {
      PR_STATIC_ASSERT(K >= 2 && K <= nsExpirationState::NOT_TRACKED);
      mObserver = new ExpirationTrackerObserver();
      mObserver->Init(this);
    }
    ~nsExpirationTracker() {
      if (mTimer) {
        mTimer->Cancel();
      }
      mObserver->Destroy();
    }

    




    nsresult AddObject(T* aObj) {
      nsExpirationState* state = aObj->GetExpirationState();
      NS_ASSERTION(!state->IsTracked(), "Tried to add an object that's already tracked");
      nsTArray<T*>& generation = mGenerations[mNewestGeneration];
      PRUint32 index = generation.Length();
      if (index > nsExpirationState::MAX_INDEX_IN_GENERATION) {
        NS_WARNING("More than 256M elements tracked, this is probably a problem");
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (index == 0) {
        
        nsresult rv = CheckStartTimer();
        if (NS_FAILED(rv))
          return rv;
      }
      if (!generation.AppendElement(aObj))
        return NS_ERROR_OUT_OF_MEMORY;
      state->mGeneration = mNewestGeneration;
      state->mIndexInGeneration = index;
      return NS_OK;
    }

    


    void RemoveObject(T* aObj) {
      nsExpirationState* state = aObj->GetExpirationState();
      NS_ASSERTION(state->IsTracked(), "Tried to remove an object that's not tracked");
      nsTArray<T*>& generation = mGenerations[state->mGeneration];
      PRUint32 index = state->mIndexInGeneration;
      NS_ASSERTION(generation.Length() > index &&
                   generation[index] == aObj, "Object is lying about its index");
      
      PRUint32 last = generation.Length() - 1;
      T* lastObj = generation[last];
      generation[index] = lastObj;
      lastObj->GetExpirationState()->mIndexInGeneration = index;
      generation.RemoveElementAt(last);
      state->mGeneration = nsExpirationState::NOT_TRACKED;
      
      
      
      
      
    }

    



    nsresult MarkUsed(T* aObj) {
      nsExpirationState* state = aObj->GetExpirationState();
      if (mNewestGeneration == state->mGeneration)
        return NS_OK;
      RemoveObject(aObj);
      return AddObject(aObj);
    }

    



    void AgeOneGeneration() {
      if (mInAgeOneGeneration) {
        NS_WARNING("Can't reenter AgeOneGeneration from NotifyExpired");
        return;
      }
      
      mInAgeOneGeneration = true;
      PRUint32 reapGeneration = 
        mNewestGeneration > 0 ? mNewestGeneration - 1 : K - 1;
      nsTArray<T*>& generation = mGenerations[reapGeneration];
      
      
      
      
      
      
      
      
      PRUint32 index = generation.Length();
      for (;;) {
        
        
        index = NS_MIN(index, generation.Length());
        if (index == 0)
          break;
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

    






    void AgeAllGenerations() {
      PRUint32 i;
      for (i = 0; i < K; ++i) {
        AgeOneGeneration();
      }
    }
    
    class Iterator {
    private:
      nsExpirationTracker<T,K>* mTracker;
      PRUint32                  mGeneration;
      PRUint32                  mIndex;
    public:
      Iterator(nsExpirationTracker<T,K>* aTracker)
        : mTracker(aTracker), mGeneration(0), mIndex(0) {}
      T* Next() {
        while (mGeneration < K) {
          nsTArray<T*>* generation = &mTracker->mGenerations[mGeneration];
          if (mIndex < generation->Length()) {
            ++mIndex;
            return (*generation)[mIndex - 1];
          }
          ++mGeneration;
          mIndex = 0;
        }
        return nsnull;
      }
    };
    
    friend class Iterator;

    bool IsEmpty() {
      for (PRUint32 i = 0; i < K; ++i) {
        if (!mGenerations[i].IsEmpty())
          return false;
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
    PRUint32           mTimerPeriod;
    PRUint32           mNewestGeneration;
    bool               mInAgeOneGeneration;

    



    class ExpirationTrackerObserver : public nsIObserver {
    public:
      void Init(nsExpirationTracker<T,K> *obj) {
        mOwner = obj;
        nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
        if (obs) {
          obs->AddObserver(this, "memory-pressure", false);
        }
      }
      void Destroy() {
        nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
        if (obs)
          obs->RemoveObserver(this, "memory-pressure");
      }
      NS_DECL_ISUPPORTS
      NS_DECL_NSIOBSERVER
    private:
      nsExpirationTracker<T,K> *mOwner;
    };
  
    static void TimerCallback(nsITimer* aTimer, void* aThis) {
      nsExpirationTracker* tracker = static_cast<nsExpirationTracker*>(aThis);
      tracker->AgeOneGeneration();
      
      if (tracker->IsEmpty()) {
        tracker->mTimer->Cancel();
        tracker->mTimer = nsnull;
      }
    }

    nsresult CheckStartTimer() {
      if (mTimer || !mTimerPeriod)
        return NS_OK;
      mTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (!mTimer)
        return NS_ERROR_OUT_OF_MEMORY;
      mTimer->InitWithFuncCallback(TimerCallback, this, mTimerPeriod,
                                   nsITimer::TYPE_REPEATING_SLACK);
      return NS_OK;
    }
};

template<class T, PRUint32 K>
NS_IMETHODIMP
nsExpirationTracker<T, K>::ExpirationTrackerObserver::Observe(nsISupports     *aSubject,
                                                              const char      *aTopic,
                                                              const PRUnichar *aData)
{
  if (!strcmp(aTopic, "memory-pressure"))
    mOwner->AgeAllGenerations();
  return NS_OK;
}

template <class T, PRUint32 K>
NS_IMETHODIMP_(nsrefcnt)
nsExpirationTracker<T,K>::ExpirationTrackerObserver::AddRef(void)
{
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(ExpirationTrackerObserver);
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "ExpirationTrackerObserver", sizeof(*this));
  return mRefCnt;
}

template <class T, PRUint32 K>
NS_IMETHODIMP_(nsrefcnt)
nsExpirationTracker<T,K>::ExpirationTrackerObserver::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD_AND_NOT_CCTHREAD(ExpirationTrackerObserver);
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

template <class T, PRUint32 K>
NS_IMETHODIMP
nsExpirationTracker<T,K>::ExpirationTrackerObserver::QueryInterface(REFNSIID aIID, 
                                                                    void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr,
               "QueryInterface requires a non-NULL destination!");            
  nsresult rv = NS_ERROR_FAILURE;
  NS_INTERFACE_TABLE1(ExpirationTrackerObserver, nsIObserver)
  return rv;
}

#endif 
