





































#ifndef NSEXPIRATIONTRACKER_H_
#define NSEXPIRATIONTRACKER_H_

#include "prlog.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"






struct nsExpirationState {
  enum { NOT_TRACKED = (1U << 4) - 1,
         MAX_INDEX_IN_GENERATION = (1U << 28) - 1 };

  nsExpirationState() : mGeneration(NOT_TRACKED) {}
  PRBool IsTracked() { return mGeneration != NOT_TRACKED; }

  


  PRUint32 mGeneration:4;
  PRUint32 mIndexInGeneration:28;
};































template <class T, PRUint32 K> class nsExpirationTracker {
  public:
    






    nsExpirationTracker(PRUint32 aTimerPeriod)
      : mTimerPeriod(aTimerPeriod), mNewestGeneration(0),
        mInAgeOneGeneration(PR_FALSE) {
      PR_STATIC_ASSERT(K >= 2 && K <= nsExpirationState::NOT_TRACKED);
    }
    ~nsExpirationTracker() {
      if (mTimer) {
        mTimer->Cancel();
      }
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
      
      mInAgeOneGeneration = PR_TRUE;
      PRUint32 reapGeneration = 
        mNewestGeneration > 0 ? mNewestGeneration - 1 : K - 1;
      nsTArray<T*>& generation = mGenerations[reapGeneration];
      
      
      
      
      
      
      
      
      PRUint32 index = generation.Length();
      for (;;) {
        
        
        index = PR_MIN(index, generation.Length());
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
      mInAgeOneGeneration = PR_FALSE;
    }

    






    void AgeAllGenerations() {
      PRUint32 i;
      for (i = 0; i < K; ++i) {
        AgeOneGeneration();
      }
    }

  protected:
    
























    virtual void NotifyExpired(T* aObj) = 0;

  private:
    nsTArray<T*>       mGenerations[K];
    nsCOMPtr<nsITimer> mTimer;
    PRUint32           mTimerPeriod;
    PRUint32           mNewestGeneration;
    PRPackedBool       mInAgeOneGeneration;

    static void TimerCallback(nsITimer* aTimer, void* aThis) {
      nsExpirationTracker* tracker = static_cast<nsExpirationTracker*>(aThis);
      tracker->AgeOneGeneration();
      
      PRUint32 i;
      for (i = 0; i < K; ++i) {
        if (!tracker->mGenerations[i].IsEmpty())
          return;
      }
      tracker->mTimer->Cancel();
      tracker->mTimer = nsnull;
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

#endif 
