




#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "RasterImage.h"
#include "DiscardTracker.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace image {

static const char* sDiscardTimeoutPref = "image.mem.min_discard_timeout_ms";

 LinkedList<DiscardTracker::Node> DiscardTracker::sDiscardableImages;
 nsCOMPtr<nsITimer> DiscardTracker::sTimer;
 bool DiscardTracker::sInitialized = false;
 bool DiscardTracker::sTimerOn = false;
 Atomic<int32_t> DiscardTracker::sDiscardRunnablePending(0);
 int64_t DiscardTracker::sCurrentDecodedImageBytes = 0;
 uint32_t DiscardTracker::sMinDiscardTimeoutMs = 10000;
 uint32_t DiscardTracker::sMaxDecodedImageKB = 42 * 1024;
 PRLock * DiscardTracker::sAllocationLock = nullptr;
 mozilla::Mutex* DiscardTracker::sNodeListMutex = nullptr;
 Atomic<uint32_t> DiscardTracker::sShutdown(0);





NS_IMETHODIMP
DiscardTracker::DiscardRunnable::Run()
{
  sDiscardRunnablePending = 0;

  DiscardTracker::DiscardNow();
  return NS_OK;
}

void
DiscardTimeoutChangedCallback(const char* aPref, void *aClosure)
{
  DiscardTracker::ReloadTimeout();
}

nsresult
DiscardTracker::Reset(Node *node)
{
  
  
  
  MutexAutoLock lock(*sNodeListMutex);
  MOZ_ASSERT(sInitialized);
  MOZ_ASSERT(node->img);
  MOZ_ASSERT(node->img->CanDiscard());
  MOZ_ASSERT(!node->img->mAnim);

  
  bool wasInList = node->isInList();
  if (wasInList) {
    node->remove();
  }
  node->timestamp = TimeStamp::Now();
  sDiscardableImages.insertFront(node);

  
  
  
  if (!wasInList) {
    MaybeDiscardSoon();
  }

  
  nsresult rv = EnableTimer();
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

void
DiscardTracker::Remove(Node *node)
{
  if (sShutdown) {
    
    return;
  }
  MutexAutoLock lock(*sNodeListMutex);

  if (node->isInList())
    node->remove();

  if (sDiscardableImages.isEmpty())
    DisableTimer();
}




void
DiscardTracker::Shutdown()
{
  sShutdown = true;

  if (sTimer) {
    sTimer->Cancel();
    sTimer = nullptr;
  }

  
  
  DiscardAll();

  delete sNodeListMutex;
  sNodeListMutex = nullptr;
}




void
DiscardTracker::DiscardAll()
{
  MutexAutoLock lock(*sNodeListMutex);

  if (!sInitialized)
    return;

  
  
  Node *n;
  while ((n = sDiscardableImages.popFirst())) {
    n->img->Discard();
  }

  
  DisableTimer();
}

void
DiscardTracker::InformAllocation(int64_t bytes)
{
  

  MOZ_ASSERT(sInitialized);

  PR_Lock(sAllocationLock);
  sCurrentDecodedImageBytes += bytes;
  MOZ_ASSERT(sCurrentDecodedImageBytes >= 0);
  PR_Unlock(sAllocationLock);

  
  
  MaybeDiscardSoon();
}




nsresult
DiscardTracker::Initialize()
{
  
  Preferences::RegisterCallback(DiscardTimeoutChangedCallback,
                                sDiscardTimeoutPref);

  Preferences::AddUintVarCache(&sMaxDecodedImageKB,
                              "image.mem.max_decoded_image_kb",
                              50 * 1024);

  
  sTimer = do_CreateInstance("@mozilla.org/timer;1");

  
  sAllocationLock = PR_NewLock();

  
  MOZ_ASSERT(!sNodeListMutex);
  sNodeListMutex = new Mutex("image::DiscardTracker");

  
  sInitialized = true;

  
  ReloadTimeout();

  return NS_OK;
}




void
DiscardTracker::ReloadTimeout()
{
  
  int32_t discardTimeout;
  nsresult rv = Preferences::GetInt(sDiscardTimeoutPref, &discardTimeout);

  
  if (!NS_SUCCEEDED(rv) || discardTimeout <= 0)
    return;

  
  if ((uint32_t) discardTimeout == sMinDiscardTimeoutMs)
    return;

  
  sMinDiscardTimeoutMs = (uint32_t) discardTimeout;

  
  DisableTimer();
  EnableTimer();
}




nsresult
DiscardTracker::EnableTimer()
{
  
  
  
  if (sTimerOn || !sInitialized || !sTimer)
    return NS_OK;

  sTimerOn = true;

  
  
  
  return sTimer->InitWithFuncCallback(TimerCallback,
                                      nullptr,
                                      sMinDiscardTimeoutMs / 2,
                                      nsITimer::TYPE_REPEATING_SLACK);
}




void
DiscardTracker::DisableTimer()
{
  
  if (!sTimerOn || !sTimer)
    return;
  sTimerOn = false;

  
  sTimer->Cancel();
}






void
DiscardTracker::TimerCallback(nsITimer *aTimer, void *aClosure)
{
  DiscardNow();
}

void
DiscardTracker::DiscardNow()
{
  
  
  
  

  TimeStamp now = TimeStamp::Now();
  Node* node;
  while ((node = sDiscardableImages.getLast())) {
    if ((now - node->timestamp).ToMilliseconds() > sMinDiscardTimeoutMs ||
        sCurrentDecodedImageBytes > sMaxDecodedImageKB * 1024) {

      
      
      node->img->Discard();

      
      
      Remove(node);
    }
    else {
      break;
    }
  }

  
  if (sDiscardableImages.isEmpty())
    DisableTimer();
}

void
DiscardTracker::MaybeDiscardSoon()
{
  
  
  if (sCurrentDecodedImageBytes > sMaxDecodedImageKB * 1024 &&
      !sDiscardableImages.isEmpty()) {
    
    if (!sDiscardRunnablePending.exchange(1)) {
      nsRefPtr<DiscardRunnable> runnable = new DiscardRunnable();
      NS_DispatchToMainThread(runnable);
    }
  }
}

} 
} 
