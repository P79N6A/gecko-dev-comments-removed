








































#ifndef mozilla_net_ChannelEventQueue_h
#define mozilla_net_ChannelEventQueue_h

#include <nsTArray.h>
#include <nsAutoPtr.h>

class nsIChannel;

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

class ChannelEventQueue
{
 public:
  ChannelEventQueue(nsIChannel *owner)
    : mForced(false)
    , mSuspended(false)
    , mFlushing(false)
    , mOwner(owner) {}

  ~ChannelEventQueue() {}

  
  
  inline bool ShouldEnqueue();

  
  
  inline void Enqueue(ChannelEvent* callback);

  
  
  
  
  
  inline void StartForcedQueueing();
  inline void EndForcedQueueing();

  
  
  
  
  
  
  
  
  inline void Suspend();
  inline void Resume();

 private:
  inline void MaybeFlushQueue();
  void FlushQueue();

  nsTArray<nsAutoPtr<ChannelEvent> > mEventQueue;

  bool mForced;
  bool mSuspended;
  bool mFlushing;

  
  nsIChannel *mOwner;

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
  NS_ABORT_IF_FALSE(!mSuspended,
                    "ChannelEventQueue::Suspend called recursively");

  mSuspended = true;
}

inline void
ChannelEventQueue::Resume()
{
  NS_ABORT_IF_FALSE(mSuspended,
                    "ChannelEventQueue::Resume called when not suspended!");

  mSuspended = false;
  MaybeFlushQueue();
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
  AutoEventEnqueuer(ChannelEventQueue &queue) : mEventQueue(queue) {
    mEventQueue.StartForcedQueueing();
  }
  ~AutoEventEnqueuer() {
    mEventQueue.EndForcedQueueing();
  }
 private:
  ChannelEventQueue &mEventQueue;
};

}
}

#endif
