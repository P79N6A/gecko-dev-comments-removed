





#include "InputQueue.h"

#include "AsyncPanZoomController.h"
#include "gfxPrefs.h"
#include "InputBlockState.h"
#include "OverscrollHandoffState.h"

#define INPQ_LOG(...)


namespace mozilla {
namespace layers {

InputQueue::InputQueue()
{
}

InputQueue::~InputQueue() {
  mTouchBlockQueue.Clear();
}

nsEventStatus
InputQueue::ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget, const InputData& aEvent, uint64_t* aOutInputBlockId) {
  AsyncPanZoomController::AssertOnControllerThread();

  if (aEvent.mInputType != MULTITOUCH_INPUT) {
    aTarget->HandleInputEvent(aEvent);
    
    
    return nsEventStatus_eConsumeDoDefault;
  }

  TouchBlockState* block = nullptr;
  if (aEvent.AsMultiTouchInput().mType == MultiTouchInput::MULTITOUCH_START) {
    block = StartNewTouchBlock(aTarget, false);
    INPQ_LOG("started new touch block %p for target %p\n", block, aTarget.get());

    
    
    
    
    
    
    if (block == CurrentTouchBlock()) {
      if (block->GetOverscrollHandoffChain()->HasFastMovingApzc()) {
        
        
        block->DisallowSingleTap();
      }
      block->GetOverscrollHandoffChain()->CancelAnimations();
    }

    if (aTarget->NeedToWaitForContent()) {
      
      
      ScheduleContentResponseTimeout(aTarget, block->GetBlockId());
    } else {
      
      
      
      INPQ_LOG("not waiting for content response on block %p\n", block);
      block->TimeoutContentResponse();
    }
  } else if (mTouchBlockQueue.IsEmpty()) {
    NS_WARNING("Received a non-start touch event while no touch blocks active!");
  } else {
    
    block = mTouchBlockQueue.LastElement().get();
    INPQ_LOG("received new event in block %p\n", block);
  }

  if (!block) {
    return nsEventStatus_eIgnore;
  }
  if (aOutInputBlockId) {
    *aOutInputBlockId = block->GetBlockId();
  }

  nsEventStatus result = aTarget->ArePointerEventsConsumable(block, aEvent.AsMultiTouchInput().mTouches.Length())
      ? nsEventStatus_eConsumeDoDefault
      : nsEventStatus_eIgnore;

  if (block == CurrentTouchBlock() && block->IsReadyForHandling()) {
    INPQ_LOG("current touch block is ready with preventdefault %d\n",
        block->IsDefaultPrevented());
    if (block->IsDefaultPrevented()) {
      return result;
    }
    aTarget->HandleInputEvent(aEvent);
    return result;
  }

  
  block->AddEvent(aEvent.AsMultiTouchInput());
  return result;
}

uint64_t
InputQueue::InjectNewTouchBlock(AsyncPanZoomController* aTarget)
{
  TouchBlockState* block = StartNewTouchBlock(aTarget, true);
  INPQ_LOG("%p injecting new touch block with id %" PRIu64 " and target %p\n",
    this, block->GetBlockId(), aTarget);
  ScheduleContentResponseTimeout(aTarget, block->GetBlockId());
  return block->GetBlockId();
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
      INPQ_LOG("discarding depleted touch block %p\n", mTouchBlockQueue[0].get());
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
InputQueue::ScheduleContentResponseTimeout(const nsRefPtr<AsyncPanZoomController>& aTarget, uint64_t aInputBlockId) {
  INPQ_LOG("scheduling content response timeout for target %p\n", aTarget.get());
  aTarget->PostDelayedTask(
    NewRunnableMethod(this, &InputQueue::ContentResponseTimeout, aInputBlockId),
    gfxPrefs::APZContentResponseTimeout());
}

void
InputQueue::ContentResponseTimeout(const uint64_t& aInputBlockId) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got a content response timeout; block=%" PRIu64 "\n", aInputBlockId);
  bool success = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->GetBlockId() == aInputBlockId) {
      success = mTouchBlockQueue[i]->TimeoutContentResponse();
      break;
    }
  }
  if (success) {
    ProcessPendingInputBlocks();
  }
}

void
InputQueue::ContentReceivedTouch(uint64_t aInputBlockId, bool aPreventDefault) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got a content response; block=%" PRIu64 "\n", aInputBlockId);
  bool success = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->GetBlockId() == aInputBlockId) {
      success = mTouchBlockQueue[i]->SetContentResponse(aPreventDefault);
      break;
    }
  }
  if (success) {
    ProcessPendingInputBlocks();
  }
}

void
InputQueue::SetAllowedTouchBehavior(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aBehaviors) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got allowed touch behaviours; block=%" PRIu64 "\n", aInputBlockId);
  bool success = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->GetBlockId() == aInputBlockId) {
      success = mTouchBlockQueue[i]->SetAllowedTouchBehaviors(aBehaviors);
      break;
    }
  }
  if (success) {
    ProcessPendingInputBlocks();
  } else {
    NS_WARNING("INPQ received useless SetAllowedTouchBehavior");
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

    INPQ_LOG("processing input block %p; preventDefault %d target %p\n",
        curBlock, curBlock->IsDefaultPrevented(),
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

    
    
    INPQ_LOG("discarding depleted touch block %p\n", curBlock);
    mTouchBlockQueue.RemoveElementAt(0);
  }
}

} 
} 
