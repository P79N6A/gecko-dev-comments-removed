





#include "SameProcessMessageQueue.h"

using namespace mozilla;
using namespace mozilla::dom;

SameProcessMessageQueue* SameProcessMessageQueue::sSingleton;

SameProcessMessageQueue::SameProcessMessageQueue()
{
}

SameProcessMessageQueue::~SameProcessMessageQueue()
{
  
  
  
  NS_WARN_IF_FALSE(mQueue.IsEmpty(), "Shouldn't send messages during shutdown");
  sSingleton = nullptr;
}

void
SameProcessMessageQueue::Push(Runnable* aRunnable)
{
  mQueue.AppendElement(aRunnable);
  NS_DispatchToCurrentThread(aRunnable);
}

void
SameProcessMessageQueue::Flush()
{
  nsTArray<nsRefPtr<Runnable>> queue;
  mQueue.SwapElements(queue);
  for (size_t i = 0; i < queue.Length(); i++) {
    queue[i]->Run();
  }
}

 SameProcessMessageQueue*
SameProcessMessageQueue::Get()
{
  if (!sSingleton) {
    sSingleton = new SameProcessMessageQueue();
  }
  return sSingleton;
}

SameProcessMessageQueue::Runnable::Runnable()
 : mDispatched(false)
{
}

NS_IMPL_ISUPPORTS(SameProcessMessageQueue::Runnable, nsIRunnable)

nsresult
SameProcessMessageQueue::Runnable::Run()
{
  if (mDispatched) {
    return NS_OK;
  }

  SameProcessMessageQueue* queue = SameProcessMessageQueue::Get();
  queue->mQueue.RemoveElement(this);

  mDispatched = true;
  return HandleMessage();
}
