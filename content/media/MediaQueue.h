




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
  virtual void* operator() (void* anObject) {
    delete static_cast<T*>(anObject);
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
    nsDeque::Push(aItem);
  }

  inline void PushFront(T* aItem) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    MOZ_ASSERT(aItem);
    nsDeque::PushFront(aItem);
  }

  inline T* PopFront() {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    T* rv = static_cast<T*>(nsDeque::PopFront());
    if (rv) {
      NotifyPopListeners();
    }
    return rv;
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
      T* x = PopFront();
      delete x;
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
    if (GetSize() < 2) {
      return 0;
    }
    T* last = Peek();
    T* first = PeekFront();
    return last->mTime - first->mTime;
  }

  void LockedForEach(nsDequeFunctor& aFunctor) const {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    ForEach(aFunctor);
  }

  
  
  void GetElementsAfter(int64_t aTime, nsTArray<T*>* aResult) {
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
      aResult->AppendElement(static_cast<T*>(ObjectAt(i)));
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

  void AddPopListener(nsIRunnable* aRunnable, MediaTaskQueue* aTaskQueue) {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mPopListeners.AppendElement(Listener(aRunnable, aTaskQueue));
  }

private:
  mutable ReentrantMonitor mReentrantMonitor;

  struct Listener {
    Listener(nsIRunnable* aRunnable, MediaTaskQueue* aTaskQueue)
      : mRunnable(aRunnable)
      , mTarget(aTaskQueue)
    {
    }
    Listener(const Listener& aOther)
      : mRunnable(aOther.mRunnable)
      , mTarget(aOther.mTarget)
    {
    }
    RefPtr<nsIRunnable> mRunnable;
    RefPtr<MediaTaskQueue> mTarget;
  };

  nsTArray<Listener> mPopListeners;

  void NotifyPopListeners() {
    for (uint32_t i = 0; i < mPopListeners.Length(); i++) {
      Listener& l = mPopListeners[i];
      l.mTarget->Dispatch(l.mRunnable);
    }
  }

  
  
  bool mEndOfStream;
};

} 

#endif
