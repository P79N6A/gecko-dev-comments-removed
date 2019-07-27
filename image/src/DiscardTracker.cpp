




#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "RasterImage.h"
#include "DiscardTracker.h"
#include "mozilla/Preferences.h"
#include "gfxPrefs.h"

namespace mozilla {
namespace image {

static const char* sDiscardTimeoutPref = "image.mem.min_discard_timeout_ms";

 LinkedList<DiscardTracker::Node> DiscardTracker::sDiscardableImages;
 nsCOMPtr<nsITimer> DiscardTracker::sTimer;
 bool DiscardTracker::sInitialized = false;
 bool DiscardTracker::sTimerOn = false;
 Atomic<bool> DiscardTracker::sDiscardRunnablePending(false);
 uint64_t DiscardTracker::sCurrentDecodedImageBytes = 0;
 uint32_t DiscardTracker::sMinDiscardTimeoutMs = 10000;
 PRLock * DiscardTracker::sAllocationLock = nullptr;
 Mutex* DiscardTracker::sNodeListMutex = nullptr;
 Atomic<bool> DiscardTracker::sShutdown(false);





NS_IMETHODIMP
DiscardTracker::DiscardRunnable::Run()
{
  sDiscardRunnablePending = false;

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

 bool
DiscardTracker::TryAllocation(uint64_t aBytes)
{
  MOZ_ASSERT(sInitialized);

  PR_Lock(sAllocationLock);
  bool enoughSpace =
    !gfxPrefs::ImageMemHardLimitDecodedImageKB() ||
    (gfxPrefs::ImageMemHardLimitDecodedImageKB() * 1024) - sCurrentDecodedImageBytes >= aBytes;

  if (enoughSpace) {
    sCurrentDecodedImageBytes += aBytes;
  }
  PR_Unlock(sAllocationLock);

  
  
  MaybeDiscardSoon();

  return enoughSpace;
}

 void
DiscardTracker::InformDeallocation(uint64_t aBytes)
{
  

  MOZ_ASSERT(sInitialized);

  PR_Lock(sAllocationLock);
  MOZ_ASSERT(aBytes <= sCurrentDecodedImageBytes);
  sCurrentDecodedImageBytes -= aBytes;
  PR_Unlock(sAllocationLock);
}




nsresult
DiscardTracker::Initialize()
{
  
  Preferences::RegisterCallback(DiscardTimeoutChangedCallback,
                                sDiscardTimeoutPref);

  
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
        sCurrentDecodedImageBytes > gfxPrefs::ImageMemMaxDecodedImageKB() * 1024) {

      
      
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
  
  
  if (sCurrentDecodedImageBytes > gfxPrefs::ImageMemMaxDecodedImageKB() * 1024 &&
      !sDiscardableImages.isEmpty()) {
    
    if (!sDiscardRunnablePending.exchange(true)) {
      nsRefPtr<DiscardRunnable> runnable = new DiscardRunnable();
      NS_DispatchToMainThread(runnable);
    }
  }
}

} 
} 
