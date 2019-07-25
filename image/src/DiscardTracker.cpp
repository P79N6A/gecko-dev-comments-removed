




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
 bool DiscardTracker::sDiscardRunnablePending = false;
 PRInt64 DiscardTracker::sCurrentDecodedImageBytes = 0;
 PRUint32 DiscardTracker::sMinDiscardTimeoutMs = 10000;
 PRUint32 DiscardTracker::sMaxDecodedImageKB = 42 * 1024;





NS_IMETHODIMP
DiscardTracker::DiscardRunnable::Run()
{
  sDiscardRunnablePending = false;
  DiscardTracker::DiscardNow();
  return NS_OK;
}

int
DiscardTimeoutChangedCallback(const char* aPref, void *aClosure)
{
  DiscardTracker::ReloadTimeout();
  return 0;
}

nsresult
DiscardTracker::Reset(Node *node)
{
  
  
  
  MOZ_ASSERT(node->img);
  MOZ_ASSERT(node->img->CanDiscard());
  MOZ_ASSERT(!node->img->mAnim);

  
  nsresult rv;
  if (NS_UNLIKELY(!sInitialized)) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  bool wasInList = node->isInList();
  if (wasInList) {
    node->remove();
  }
  node->timestamp = TimeStamp::Now();
  sDiscardableImages.insertFront(node);

  
  
  
  if (!wasInList) {
    MaybeDiscardSoon();
  }

  
  rv = EnableTimer();
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

void
DiscardTracker::Remove(Node *node)
{
  if (node->isInList())
    node->remove();

  if (sDiscardableImages.isEmpty())
    DisableTimer();
}




void
DiscardTracker::Shutdown()
{
  if (sTimer) {
    sTimer->Cancel();
    sTimer = NULL;
  }
}




void
DiscardTracker::DiscardAll()
{
  if (!sInitialized)
    return;

  
  
  Node *n;
  while ((n = sDiscardableImages.popFirst())) {
    n->img->Discard();
  }

  
  DisableTimer();
}

void
DiscardTracker::InformAllocation(PRInt64 bytes)
{
  

  sCurrentDecodedImageBytes += bytes;
  MOZ_ASSERT(sCurrentDecodedImageBytes >= 0);

  
  
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

  
  ReloadTimeout();

  
  sInitialized = true;

  return NS_OK;
}




void
DiscardTracker::ReloadTimeout()
{
  
  PRInt32 discardTimeout;
  nsresult rv = Preferences::GetInt(sDiscardTimeoutPref, &discardTimeout);

  
  if (!NS_SUCCEEDED(rv) || discardTimeout <= 0)
    return;

  
  if ((PRUint32) discardTimeout == sMinDiscardTimeoutMs)
    return;

  
  sMinDiscardTimeoutMs = (PRUint32) discardTimeout;

  
  DisableTimer();
  EnableTimer();
}




nsresult
DiscardTracker::EnableTimer()
{
  
  if (sTimerOn)
    return NS_OK;
  sTimerOn = true;

  
  
  
  return sTimer->InitWithFuncCallback(TimerCallback,
                                      nsnull,
                                      sMinDiscardTimeoutMs / 2,
                                      nsITimer::TYPE_REPEATING_SLACK);
}




void
DiscardTracker::DisableTimer()
{
  
  if (!sTimerOn)
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
      !sDiscardableImages.isEmpty() && !sDiscardRunnablePending) {
    sDiscardRunnablePending = true;
    nsRefPtr<DiscardRunnable> runnable = new DiscardRunnable();
    NS_DispatchToCurrentThread(runnable);
  }
}

} 
} 
