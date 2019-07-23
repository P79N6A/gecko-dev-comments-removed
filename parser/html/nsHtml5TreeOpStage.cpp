




































#include "nsHtml5TreeOpStage.h"

nsHtml5TreeOpStage::nsHtml5TreeOpStage()
 : mMutex("nsHtml5TreeOpStage mutex")
{
}
    
nsHtml5TreeOpStage::~nsHtml5TreeOpStage()
{
}

void
nsHtml5TreeOpStage::MaybeFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  mozilla::MutexAutoLock autoLock(mMutex);
  if (mOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
  }  
}

void
nsHtml5TreeOpStage::ForcedFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  mozilla::MutexAutoLock autoLock(mMutex);
  if (mOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
    return;
  }
  mOpQueue.MoveElementsFrom(aOpQueue);
}
    
void
nsHtml5TreeOpStage::RetrieveOperations(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  mozilla::MutexAutoLock autoLock(mMutex);
  if (aOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
    return;
  }
  aOpQueue.MoveElementsFrom(mOpQueue);
}
