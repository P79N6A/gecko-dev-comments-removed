







































#ifndef mozilla_net_ChannelEventQueue_h
#define mozilla_net_ChannelEventQueue_h

namespace mozilla {
namespace net {

class ChannelEvent
{
 public:
  ChannelEvent() { MOZ_COUNT_CTOR(ChannelEvent); }
  virtual ~ChannelEvent() { MOZ_COUNT_DTOR(ChannelEvent); }
  virtual void Run() = 0;
};







class ChannelEventQueue
{
 public:
  ChannelEventQueue() : mQueuePhase(PHASE_UNQUEUED) {}
  ~ChannelEventQueue() {}
  
 protected:
  void BeginEventQueueing();
  void EndEventQueueing();
  void EnqueueEvent(ChannelEvent* callback);
  bool ShouldEnqueue();

  
  
  virtual void FlushEventQueue() = 0;

  nsTArray<nsAutoPtr<ChannelEvent> > mEventQueue;
  enum {
    PHASE_UNQUEUED,
    PHASE_QUEUEING,
    PHASE_FINISHED_QUEUEING,
    PHASE_FLUSHING
  } mQueuePhase;

  friend class AutoEventEnqueuer;
};

inline void
ChannelEventQueue::BeginEventQueueing()
{
  if (mQueuePhase != PHASE_UNQUEUED)
    return;
  
  mQueuePhase = PHASE_QUEUEING;
}

inline void
ChannelEventQueue::EndEventQueueing()
{
  if (mQueuePhase != PHASE_QUEUEING)
    return;

  mQueuePhase = PHASE_FINISHED_QUEUEING;
}

inline bool
ChannelEventQueue::ShouldEnqueue()
{
  return mQueuePhase != PHASE_UNQUEUED;
}

inline void
ChannelEventQueue::EnqueueEvent(ChannelEvent* callback)
{
  mEventQueue.AppendElement(callback);
}



class AutoEventEnqueuer 
{
 public:
  AutoEventEnqueuer(ChannelEventQueue* queue) : mEventQueue(queue) 
  {
    mEventQueue->BeginEventQueueing();
  }
  ~AutoEventEnqueuer() 
  { 
    mEventQueue->EndEventQueueing();
    mEventQueue->FlushEventQueue(); 
  }
 private:
  ChannelEventQueue *mEventQueue;
};

}
}

#endif
