




#if !defined(MediaQueue_h_)
#define MediaQueue_h_

#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TaskQueue.h"

#include "nsDeque.h"
#include "nsTArray.h"

namespace mozilla {


template <class T>
class MediaQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* aObject) {
    nsRefPtr<T> releaseMe = dont_AddRef(static_cast<T*>(aObject));
    return nullptr;
  }
};

template <class T> class MediaQueue : private nsDeque {
 public:

   MediaQueue()
     : nsDeque(new MediaQueueDeallocator<T>()),
       mReentrantMonitor("mediaqueue"),
       mEndOfStream(false)
   {}

  ~MediaQueue() {
    Reset();
  }

  inline int32_t GetSize() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return nsDeque::GetSize();
  }

  inline void Push(T* aItem) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    MOZ_ASSERT(aItem);
    NS_ADDREF(aItem);
    nsDeque::Push(aItem);
  }

  inline void PushFront(T* aItem) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    MOZ_ASSERT(aItem);
    NS_ADDREF(aItem);
    nsDeque::PushFront(aItem);
  }

  inline already_AddRefed<T> PopFront() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsRefPtr<T> rv = dont_AddRef(static_cast<T*>(nsDeque::PopFront()));
    if (rv) {
      NotifyPopListeners();
    }
    return rv.forget();
  }

  inline T* Peek() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return static_cast<T*>(nsDeque::Peek());
  }

  inline T* PeekFront() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return static_cast<T*>(nsDeque::PeekFront());
  }

  void Reset() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (GetSize() > 0) {
      nsRefPtr<T> x = PopFront();
    }
    mEndOfStream = false;
  }

  bool AtEndOfStream() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return GetSize() == 0 && mEndOfStream;
  }

  
  
  
  bool IsFinished() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    return mEndOfStream;
  }

  
  void Finish() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mEndOfStream = true;
  }

  
  int64_t Duration() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    if (GetSize() == 0) {
      return 0;
    }
    T* last = Peek();
    T* first = PeekFront();
    return last->GetEndTime() - first->mTime;
  }

  void LockedForEach(nsDequeFunctor& aFunctor) const {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    ForEach(aFunctor);
  }

  
  
  void GetElementsAfter(int64_t aTime, nsTArray<nsRefPtr<T>>* aResult) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    if (!GetSize())
      return;
    int32_t i;
    for (i = GetSize() - 1; i > 0; --i) {
      T* v = static_cast<T*>(ObjectAt(i));
      if (v->GetEndTime() < aTime)
        break;
    }
    
    
    for (; i < GetSize(); ++i) {
      nsRefPtr<T> elem = static_cast<T*>(ObjectAt(i));
      aResult->AppendElement(elem);
    }
  }

  void GetFirstElements(uint32_t aMaxElements, nsTArray<nsRefPtr<T>>* aResult) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    for (int32_t i = 0; i < (int32_t)aMaxElements && i < GetSize(); ++i) {
      *aResult->AppendElement() = static_cast<T*>(ObjectAt(i));
    }
  }

  uint32_t FrameCount() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    uint32_t frames = 0;
    for (int32_t i = 0; i < GetSize(); ++i) {
      T* v = static_cast<T*>(ObjectAt(i));
      frames += v->mFrames;
    }
    return frames;
  }

  void ClearListeners() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mPopListeners.Clear();
  }

  void AddPopListener(nsIRunnable* aRunnable, TaskQueue* aTarget) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mPopListeners.AppendElement(Listener(aRunnable, aTarget));
  }

private:
  mutable ReentrantMonitor mReentrantMonitor;

  struct Listener {
    Listener(nsIRunnable* aRunnable, TaskQueue* aTarget)
      : mRunnable(aRunnable)
      , mTarget(aTarget)
    {
    }
    Listener(const Listener& aOther)
      : mRunnable(aOther.mRunnable)
      , mTarget(aOther.mTarget)
    {
    }
    nsCOMPtr<nsIRunnable> mRunnable;
    RefPtr<TaskQueue> mTarget;
  };

  nsTArray<Listener> mPopListeners;

  void NotifyPopListeners() {
    for (uint32_t i = 0; i < mPopListeners.Length(); i++) {
      Listener& l = mPopListeners[i];
      nsCOMPtr<nsIRunnable> r = l.mRunnable;
      l.mTarget->Dispatch(r.forget());
    }
  }

  
  
  bool mEndOfStream;
};

} 

#endif
