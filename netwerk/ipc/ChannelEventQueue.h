







































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







template<class T> class AutoEventEnqueuerBase;

template<class T>
class ChannelEventQueue
{
 public:
  ChannelEventQueue(T* self) : mQueuePhase(PHASE_UNQUEUED)
                             , mSelf(self) {}
  ~ChannelEventQueue() {}
  
 protected:
  void BeginEventQueueing();
  void EndEventQueueing();
  void EnqueueEvent(ChannelEvent* callback);
  bool ShouldEnqueue();
  void FlushEventQueue();

  nsTArray<nsAutoPtr<ChannelEvent> > mEventQueue;
  enum {
    PHASE_UNQUEUED,
    PHASE_QUEUEING,
    PHASE_FINISHED_QUEUEING,
    PHASE_FLUSHING
  } mQueuePhase;

  typedef AutoEventEnqueuerBase<T> AutoEventEnqueuer;

 private:
  T* mSelf;

  friend class AutoEventEnqueuerBase<T>;
};

template<class T> inline void
ChannelEventQueue<T>::BeginEventQueueing()
{
  if (mQueuePhase != PHASE_UNQUEUED)
    return;
  
  mQueuePhase = PHASE_QUEUEING;
}

template<class T> inline void
ChannelEventQueue<T>::EndEventQueueing()
{
  if (mQueuePhase != PHASE_QUEUEING)
    return;

  mQueuePhase = PHASE_FINISHED_QUEUEING;
}

template<class T> inline bool
ChannelEventQueue<T>::ShouldEnqueue()
{
  return mQueuePhase != PHASE_UNQUEUED || mSelf->IsSuspended();
}

template<class T> inline void
ChannelEventQueue<T>::EnqueueEvent(ChannelEvent* callback)
{
  mEventQueue.AppendElement(callback);
}

template<class T> void
ChannelEventQueue<T>::FlushEventQueue()
{
  NS_ABORT_IF_FALSE(mQueuePhase != PHASE_UNQUEUED,
                    "Queue flushing should not occur if PHASE_UNQUEUED");
  
  
  if (mQueuePhase != PHASE_FINISHED_QUEUEING || mSelf->IsSuspended())
    return;
  
  nsRefPtr<T> kungFuDeathGrip(mSelf);
  if (mEventQueue.Length() > 0) {
    
    
    
    mQueuePhase = PHASE_FLUSHING;
    
    PRUint32 i;
    for (i = 0; i < mEventQueue.Length(); i++) {
      mEventQueue[i]->Run();
      if (mSelf->IsSuspended())
        break;
    }

    
    if (i < mEventQueue.Length())
      i++;

    mEventQueue.RemoveElementsAt(0, i);
  }

  if (mSelf->IsSuspended())
    mQueuePhase = PHASE_QUEUEING;
  else
    mQueuePhase = PHASE_UNQUEUED;
}



template<class T>
class AutoEventEnqueuerBase
{
 public:
  AutoEventEnqueuerBase(ChannelEventQueue<T>* queue) : mEventQueue(queue) 
  {
    mEventQueue->BeginEventQueueing();
  }
  ~AutoEventEnqueuerBase() 
  { 
    mEventQueue->EndEventQueueing();
    mEventQueue->FlushEventQueue(); 
  }
 private:
  ChannelEventQueue<T> *mEventQueue;
};

}
}

#endif
