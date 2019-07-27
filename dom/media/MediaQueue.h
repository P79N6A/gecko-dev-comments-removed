




#if !defined(MediaQueue_h_)
#define MediaQueue_h_

#include "nsDeque.h"
#include "nsTArray.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "MediaTaskQueue.h"

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

  inline void Empty() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    nsDeque::Empty();
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

  void AddPopListener(nsIRunnable* aRunnable, MediaTaskQueue* aTarget) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mPopListeners.AppendElement(Listener(aRunnable, aTarget));
  }

private:
  mutable ReentrantMonitor mReentrantMonitor;

  struct Listener {
    Listener(nsIRunnable* aRunnable, MediaTaskQueue* aTarget)
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
    RefPtr<MediaTaskQueue> mTarget;
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
