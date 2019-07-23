




































#include "nsHtml5TreeOpStage.h"

nsHtml5TreeOpStage::nsHtml5TreeOpStage()
 : mMutex("nsHtml5TreeOpStage mutex")
{
}
    
nsHtml5TreeOpStage::~nsHtml5TreeOpStage()
{
}

void
nsHtml5TreeOpStage::MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  mozilla::MutexAutoLock autoLock(mMutex);
  if (mOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
    return;
  }
  mOpQueue.MoveElementsFrom(aOpQueue);
}
    
void
nsHtml5TreeOpStage::MoveOpsTo(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  mozilla::MutexAutoLock autoLock(mMutex);
  if (aOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
    return;
  }
  aOpQueue.MoveElementsFrom(mOpQueue);
}

#ifdef DEBUG
void
nsHtml5TreeOpStage::AssertEmpty()
{
  mozilla::MutexAutoLock autoLock(mMutex);
  
  NS_ASSERTION(mOpQueue.IsEmpty(), "The stage was supposed to be empty.");
}
#endif
