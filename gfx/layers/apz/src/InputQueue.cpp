





#include "InputQueue.h"

#include "AsyncPanZoomController.h"
#include "gfxPrefs.h"
#include "InputBlockState.h"
#include "OverscrollHandoffState.h"

#define INPQ_LOG(...)


namespace mozilla {
namespace layers {

InputQueue::InputQueue()
  : mTouchBlockBalance(0)
{
}

InputQueue::~InputQueue() {
  mTouchBlockQueue.Clear();
}

nsEventStatus
InputQueue::ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget, const InputData& aEvent) {
  AsyncPanZoomController::AssertOnControllerThread();

  if (aEvent.mInputType != MULTITOUCH_INPUT) {
    aTarget->HandleInputEvent(aEvent);
    
    
    return nsEventStatus_eConsumeDoDefault;
  }

  TouchBlockState* block = nullptr;
  if (aEvent.AsMultiTouchInput().mType == MultiTouchInput::MULTITOUCH_START) {
    block = StartNewTouchBlock(aTarget, false);
    INPQ_LOG("%p started new touch block %p for target %p\n", this, block, aTarget.get());

    
    
    
    
    
    
    if (block == CurrentTouchBlock()) {
      if (block->GetOverscrollHandoffChain()->HasFastMovingApzc()) {
        
        
        block->DisallowSingleTap();
      }
      block->GetOverscrollHandoffChain()->CancelAnimations();
    }

    if (aTarget->NeedToWaitForContent()) {
      
      
      ScheduleContentResponseTimeout(aTarget);
    } else {
      
      
      
      INPQ_LOG("%p not waiting for content response on block %p\n", this, block);
      mTouchBlockBalance++;
      block->TimeoutContentResponse();
    }
  } else if (mTouchBlockQueue.IsEmpty()) {
    NS_WARNING("Received a non-start touch event while no touch blocks active!");
  } else {
    
    block = mTouchBlockQueue.LastElement().get();
    INPQ_LOG("%p received new event in block %p\n", this, block);
  }

  if (!block) {
    return nsEventStatus_eIgnore;
  }

  nsEventStatus result = aTarget->ArePointerEventsConsumable(block, aEvent.AsMultiTouchInput().mTouches.Length())
      ? nsEventStatus_eConsumeDoDefault
      : nsEventStatus_eIgnore;

  if (block == CurrentTouchBlock() && block->IsReadyForHandling()) {
    INPQ_LOG("%p's current touch block is ready with preventdefault %d\n",
        this, block->IsDefaultPrevented());
    if (block->IsDefaultPrevented()) {
      return result;
    }
    aTarget->HandleInputEvent(aEvent);
    return result;
  }

  
  block->AddEvent(aEvent.AsMultiTouchInput());
  return result;
}

void
InputQueue::InjectNewTouchBlock(AsyncPanZoomController* aTarget)
{
  StartNewTouchBlock(aTarget, true);
  ScheduleContentResponseTimeout(aTarget);
}

TouchBlockState*
InputQueue::StartNewTouchBlock(const nsRefPtr<AsyncPanZoomController>& aTarget, bool aCopyAllowedTouchBehaviorFromCurrent)
{
  TouchBlockState* newBlock = new TouchBlockState(aTarget);
  if (gfxPrefs::TouchActionEnabled() && aCopyAllowedTouchBehaviorFromCurrent) {
    newBlock->CopyAllowedTouchBehaviorsFrom(*CurrentTouchBlock());
  }

  
  
  while (!mTouchBlockQueue.IsEmpty()) {
    if (mTouchBlockQueue[0]->IsReadyForHandling() && !mTouchBlockQueue[0]->HasEvents()) {
      INPQ_LOG("%p discarding depleted touch block %p\n", this, mTouchBlockQueue[0].get());
      mTouchBlockQueue.RemoveElementAt(0);
    } else {
      break;
    }
  }

  
  mTouchBlockQueue.AppendElement(newBlock);
  return newBlock;
}

TouchBlockState*
InputQueue::CurrentTouchBlock() const
{
  AsyncPanZoomController::AssertOnControllerThread();

  MOZ_ASSERT(!mTouchBlockQueue.IsEmpty());
  return mTouchBlockQueue[0].get();
}

bool
InputQueue::HasReadyTouchBlock() const
{
  return !mTouchBlockQueue.IsEmpty() && mTouchBlockQueue[0]->IsReadyForHandling();
}

void
InputQueue::ScheduleContentResponseTimeout(const nsRefPtr<AsyncPanZoomController>& aTarget) {
  INPQ_LOG("%p scheduling content response timeout for target %p\n", this, aTarget.get());
  aTarget->PostDelayedTask(
    NewRunnableMethod(this, &InputQueue::ContentResponseTimeout),
    gfxPrefs::APZContentResponseTimeout());
}

void
InputQueue::ContentResponseTimeout() {
  AsyncPanZoomController::AssertOnControllerThread();

  mTouchBlockBalance++;
  INPQ_LOG("%p got a content response timeout; balance %d\n", this, mTouchBlockBalance);
  if (mTouchBlockBalance > 0) {
    
    
    bool found = false;
    for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
      if (mTouchBlockQueue[i]->TimeoutContentResponse()) {
        found = true;
        break;
      }
    }
    if (found) {
      ProcessPendingInputBlocks();
    } else {
      NS_WARNING("INPQ received more ContentResponseTimeout calls than it has unprocessed touch blocks\n");
    }
  }
}

void
InputQueue::ContentReceivedTouch(bool aPreventDefault) {
  AsyncPanZoomController::AssertOnControllerThread();

  mTouchBlockBalance--;
  INPQ_LOG("%p got a content response; balance %d\n", this, mTouchBlockBalance);
  if (mTouchBlockBalance < 0) {
    
    
    bool found = false;
    for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
      if (mTouchBlockQueue[i]->SetContentResponse(aPreventDefault)) {
        found = true;
        break;
      }
    }
    if (found) {
      ProcessPendingInputBlocks();
    } else {
      NS_WARNING("INPQ received more ContentReceivedTouch calls than it has unprocessed touch blocks\n");
    }
  }
}

void
InputQueue::SetAllowedTouchBehavior(const nsTArray<TouchBehaviorFlags>& aBehaviors) {
  AsyncPanZoomController::AssertOnControllerThread();

  bool found = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->SetAllowedTouchBehaviors(aBehaviors)) {
      found = true;
      break;
    }
  }
  if (found) {
    ProcessPendingInputBlocks();
  } else {
    NS_WARNING("INPQ received more SetAllowedTouchBehavior calls than it has unprocessed touch blocks\n");
  }
}

void
InputQueue::ProcessPendingInputBlocks() {
  AsyncPanZoomController::AssertOnControllerThread();

  while (true) {
    TouchBlockState* curBlock = CurrentTouchBlock();
    if (!curBlock->IsReadyForHandling()) {
      break;
    }

    INPQ_LOG("%p processing input block %p; preventDefault %d target %p\n",
        this, curBlock, curBlock->IsDefaultPrevented(),
        curBlock->GetTargetApzc().get());
    nsRefPtr<AsyncPanZoomController> target = curBlock->GetTargetApzc();
    if (curBlock->IsDefaultPrevented()) {
      curBlock->DropEvents();
      target->ResetInputState();
    } else {
      while (curBlock->HasEvents()) {
        target->HandleInputEvent(curBlock->RemoveFirstEvent());
      }
    }
    MOZ_ASSERT(!curBlock->HasEvents());

    if (mTouchBlockQueue.Length() == 1) {
      
      
      
      
      break;
    }

    
    
    INPQ_LOG("%p discarding depleted touch block %p\n", this, curBlock);
    mTouchBlockQueue.RemoveElementAt(0);
  }
}

} 
} 
