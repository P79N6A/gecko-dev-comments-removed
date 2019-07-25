




































#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "RasterImage.h"
#include "DiscardTracker.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace imagelib {

static PRBool sInitialized = PR_FALSE;
static PRBool sTimerOn = PR_FALSE;
static PRUint32 sMinDiscardTimeoutMs = 10000; 
static nsITimer *sTimer = nsnull;
static struct DiscardTrackerNode sHead, sSentinel, sTail;





nsresult
DiscardTracker::Reset(DiscardTrackerNode *node)
{
  nsresult rv;
#ifdef DEBUG
  PRBool isSentinel = (node == &sSentinel);

  
  NS_ABORT_IF_FALSE(isSentinel || node->curr, "Node doesn't point to anything!");

  
  NS_ABORT_IF_FALSE(isSentinel || node->curr->CanDiscard(),
                    "trying to reset discarding but can't discard!");

  
  NS_ABORT_IF_FALSE(isSentinel || !node->curr->mAnim,
                    "Trying to reset discarding on animated image!");
#endif

  
  if (NS_UNLIKELY(!sInitialized)) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  Remove(node);

  
  node->prev = sTail.prev;
  node->next = &sTail;
  node->prev->next = sTail.prev = node;

  
  rv = TimerOn();
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}




void
DiscardTracker::Remove(DiscardTrackerNode *node)
{
  NS_ABORT_IF_FALSE(node != nsnull, "Can't pass null node");

  
  if ((node->prev == nsnull) || (node->next == nsnull)) {
    NS_ABORT_IF_FALSE(node->prev == node->next,
                      "Node is half in a list!");
    return;
  }

  
  node->prev->next = node->next;
  node->next->prev = node->prev;

  
  node->prev = node->next = nsnull;
}




nsresult
DiscardTracker::Initialize()
{
  nsresult rv;

  
  sHead.curr = sTail.curr = sSentinel.curr = nsnull;
  sHead.prev = sTail.next = nsnull;
  sHead.next = sTail.prev = &sSentinel;
  sSentinel.prev = &sHead;
  sSentinel.next = &sTail;

  
  ReloadTimeout();

  
  nsCOMPtr<nsITimer> t = do_CreateInstance("@mozilla.org/timer;1");
  NS_ENSURE_TRUE(t, NS_ERROR_OUT_OF_MEMORY);
  t.forget(&sTimer);
  rv = TimerOn();
  NS_ENSURE_SUCCESS(rv, rv);

  
  sInitialized = PR_TRUE;

  return NS_OK;
}




void
DiscardTracker::Shutdown()
{
  if (sTimer) {
    sTimer->Cancel();
    NS_RELEASE(sTimer);
    sTimer = nsnull;
  }
}




void
DiscardTracker::ReloadTimeout()
{
  nsresult rv;

  
  PRInt32 discardTimeout;
  rv = Preferences::GetInt(DISCARD_TIMEOUT_PREF, &discardTimeout);

  
  if (!NS_SUCCEEDED(rv) || discardTimeout <= 0)
    return;

  
  if ((PRUint32) discardTimeout == sMinDiscardTimeoutMs)
    return;

  
  sMinDiscardTimeoutMs = (PRUint32) discardTimeout;

  
  if (sTimerOn) {
    TimerOff();
    TimerOn();
  }
}




nsresult
DiscardTracker::TimerOn()
{
  
  if (sTimerOn)
    return NS_OK;
  sTimerOn = PR_TRUE;

  
  return sTimer->InitWithFuncCallback(TimerCallback,
                                      nsnull,
                                      sMinDiscardTimeoutMs,
                                      nsITimer::TYPE_REPEATING_SLACK);
}




void
DiscardTracker::TimerOff()
{
  
  if (!sTimerOn)
    return;
  sTimerOn = PR_FALSE;

  
  sTimer->Cancel();
}






void
DiscardTracker::TimerCallback(nsITimer *aTimer, void *aClosure)
{
  DiscardTrackerNode *node;

  
  for (node = sSentinel.prev; node != &sHead; node = sSentinel.prev) {
    NS_ABORT_IF_FALSE(node->curr, "empty node!");
    Remove(node);
    node->curr->Discard();
  }

  
  Reset(&sSentinel);

  
  
  if (sSentinel.prev == &sHead)
    TimerOff();
}

} 
} 
