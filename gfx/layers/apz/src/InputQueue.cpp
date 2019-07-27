





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
InputQueue::ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget,
                              bool aTargetConfirmed,
                              const InputData& aEvent,
                              uint64_t* aOutInputBlockId) {
  AsyncPanZoomController::AssertOnControllerThread();

  if (aEvent.mInputType != MULTITOUCH_INPUT) {
    
    
    
    
    return aTarget->HandleInputEvent(aEvent);
  }

  TouchBlockState* block = nullptr;
  if (aEvent.AsMultiTouchInput().mType == MultiTouchInput::MULTITOUCH_START) {
    block = StartNewTouchBlock(aTarget, aTargetConfirmed, false);
    INPQ_LOG("started new touch block %p for target %p\n", block, aTarget.get());

    
    
    
    
    
    
    if (block == CurrentTouchBlock()) {
      
      
      
      if (block->GetOverscrollHandoffChain()->HasFastMovingApzc()) {
        
        
        block->DisallowSingleTap();
      }
      block->GetOverscrollHandoffChain()->CancelAnimations();
    }

    bool waitForMainThread = !aTargetConfirmed;
    if (!gfxPrefs::LayoutEventRegionsEnabled()) {
      waitForMainThread |= aTarget->NeedToWaitForContent();
    }
    if (waitForMainThread) {
      
      
      
      
      ScheduleMainThreadTimeout(aTarget, block->GetBlockId());
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

  
  
  
  
  nsRefPtr<AsyncPanZoomController> target = block->GetTargetApzc();

  nsEventStatus result = nsEventStatus_eIgnore;
  
  
  
  if (target && target->ArePointerEventsConsumable(block, aEvent.AsMultiTouchInput().mTouches.Length())) {
    result = nsEventStatus_eConsumeDoDefault;
  }

  if (block == CurrentTouchBlock() && block->IsReadyForHandling()) {
    INPQ_LOG("current touch block is ready with target %p preventdefault %d\n",
        target.get(), block->IsDefaultPrevented());
    if (!target || block->IsDefaultPrevented()) {
      return result;
    }
    target->HandleInputEvent(aEvent);
    return result;
  }

  
  block->AddEvent(aEvent.AsMultiTouchInput());
  return result;
}

uint64_t
InputQueue::InjectNewTouchBlock(AsyncPanZoomController* aTarget)
{
  TouchBlockState* block = StartNewTouchBlock(aTarget,
     true,
     true);
  INPQ_LOG("%p injecting new touch block with id %" PRIu64 " and target %p\n",
    this, block->GetBlockId(), aTarget);
  ScheduleMainThreadTimeout(aTarget, block->GetBlockId());
  return block->GetBlockId();
}

TouchBlockState*
InputQueue::StartNewTouchBlock(const nsRefPtr<AsyncPanZoomController>& aTarget,
                               bool aTargetConfirmed,
                               bool aCopyAllowedTouchBehaviorFromCurrent)
{
  TouchBlockState* newBlock = new TouchBlockState(aTarget, aTargetConfirmed);
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
InputQueue::ScheduleMainThreadTimeout(const nsRefPtr<AsyncPanZoomController>& aTarget, uint64_t aInputBlockId) {
  INPQ_LOG("scheduling main thread timeout for target %p\n", aTarget.get());
  aTarget->PostDelayedTask(
    NewRunnableMethod(this, &InputQueue::MainThreadTimeout, aInputBlockId),
    gfxPrefs::APZContentResponseTimeout());
}

void
InputQueue::MainThreadTimeout(const uint64_t& aInputBlockId) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got a main thread timeout; block=%" PRIu64 "\n", aInputBlockId);
  bool success = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->GetBlockId() == aInputBlockId) {
      
      
      
      success = mTouchBlockQueue[i]->TimeoutContentResponse();
      success |= mTouchBlockQueue[i]->SetConfirmedTargetApzc(mTouchBlockQueue[i]->GetTargetApzc());
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
InputQueue::SetConfirmedTargetApzc(uint64_t aInputBlockId, const nsRefPtr<AsyncPanZoomController>& aTargetApzc) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got a target apzc; block=%" PRIu64 " guid=%s\n",
    aInputBlockId, aTargetApzc ? Stringify(aTargetApzc->GetGuid()).c_str() : "");
  bool success = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->GetBlockId() == aInputBlockId) {
      success = mTouchBlockQueue[i]->SetConfirmedTargetApzc(aTargetApzc);
      break;
    }
  }
  if (success) {
    ProcessPendingInputBlocks();
  } else {
    NS_WARNING("INPQ received useless SetConfirmedTargetApzc");
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
    
    
    if (!target) {
      curBlock->DropEvents();
    } else if (curBlock->IsDefaultPrevented()) {
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
