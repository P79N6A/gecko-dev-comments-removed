






#ifndef mozilla_net_ChannelEventQueue_h
#define mozilla_net_ChannelEventQueue_h

#include <nsTArray.h>
#include <nsAutoPtr.h>

class nsISupports;
class nsIEventTarget;
class nsIThread;

namespace mozilla {
namespace net {

class ChannelEvent
{
 public:
  ChannelEvent() { MOZ_COUNT_CTOR(ChannelEvent); }
  virtual ~ChannelEvent() { MOZ_COUNT_DTOR(ChannelEvent); }
  virtual void Run() = 0;
};








class AutoEventEnqueuerBase;

class ChannelEventQueue MOZ_FINAL
{
  NS_INLINE_DECL_REFCOUNTING(ChannelEventQueue)

 public:
  explicit ChannelEventQueue(nsISupports *owner)
    : mSuspendCount(0)
    , mSuspended(false)
    , mForced(false)
    , mFlushing(false)
    , mOwner(owner) {}

  
  
  inline bool ShouldEnqueue();

  
  
  inline void Enqueue(ChannelEvent* callback);

  
  
  
  
  
  inline void StartForcedQueueing();
  inline void EndForcedQueueing();

  
  
  
  inline void Suspend();
  
  
  void Resume();

  
  nsresult RetargetDeliveryTo(nsIEventTarget* aTargetThread);

 private:
  
  ~ChannelEventQueue()
  {
  }

  inline void MaybeFlushQueue();
  void FlushQueue();
  inline void CompleteResume();

  nsTArray<nsAutoPtr<ChannelEvent> > mEventQueue;

  uint32_t mSuspendCount;
  bool     mSuspended;
  bool mForced;
  bool mFlushing;

  
  nsISupports *mOwner;

  
  nsCOMPtr<nsIThread> mTargetThread;

  friend class AutoEventEnqueuer;
};

inline bool
ChannelEventQueue::ShouldEnqueue()
{
  bool answer =  mForced || mSuspended || mFlushing;

  NS_ABORT_IF_FALSE(answer == true || mEventQueue.IsEmpty(),
                    "Should always enqueue if ChannelEventQueue not empty");

  return answer;
}

inline void
ChannelEventQueue::Enqueue(ChannelEvent* callback)
{
  mEventQueue.AppendElement(callback);
}

inline void
ChannelEventQueue::StartForcedQueueing()
{
  mForced = true;
}

inline void
ChannelEventQueue::EndForcedQueueing()
{
  mForced = false;
  MaybeFlushQueue();
}

inline void
ChannelEventQueue::Suspend()
{
  mSuspended = true;
  mSuspendCount++;
}

inline void
ChannelEventQueue::CompleteResume()
{
  
  if (!mSuspendCount) {
    
    
    
    mSuspended = false;
    MaybeFlushQueue();
  }
}

inline void
ChannelEventQueue::MaybeFlushQueue()
{
  
  
  if (!mForced && !mFlushing && !mSuspended && !mEventQueue.IsEmpty())
    FlushQueue();
}




class AutoEventEnqueuer
{
 public:
  explicit AutoEventEnqueuer(ChannelEventQueue *queue) : mEventQueue(queue) {
    mEventQueue->StartForcedQueueing();
  }
  ~AutoEventEnqueuer() {
    mEventQueue->EndForcedQueueing();
  }
 private:
  ChannelEventQueue* mEventQueue;
};

}
}

#endif
