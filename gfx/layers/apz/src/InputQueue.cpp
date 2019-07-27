





#include "InputQueue.h"

#include "AsyncPanZoomController.h"
#include "gfxPrefs.h"
#include "InputBlockState.h"
#include "LayersLogging.h"
#include "OverscrollHandoffState.h"

#define INPQ_LOG(...)


namespace mozilla {
namespace layers {

InputQueue::InputQueue()
{
}

InputQueue::~InputQueue() {
  mInputBlockQueue.Clear();
}

nsEventStatus
InputQueue::ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget,
                              bool aTargetConfirmed,
                              const InputData& aEvent,
                              uint64_t* aOutInputBlockId) {
  AsyncPanZoomController::AssertOnControllerThread();

  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      const MultiTouchInput& event = aEvent.AsMultiTouchInput();
      return ReceiveTouchInput(aTarget, aTargetConfirmed, event, aOutInputBlockId);
    }

    case SCROLLWHEEL_INPUT: {
      const ScrollWheelInput& event = aEvent.AsScrollWheelInput();
      return ReceiveScrollWheelInput(aTarget, aTargetConfirmed, event, aOutInputBlockId);
    }

    default:
      
      
      
      
      return aTarget->HandleInputEvent(aEvent);
  }
}

bool
InputQueue::MaybeHandleCurrentBlock(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                           CancelableBlockState *block,
                                           const InputData& aEvent) {
  if (block == CurrentBlock() && block->IsReadyForHandling()) {
    INPQ_LOG("current block is ready with target %p preventdefault %d\n",
        aTarget.get(), block->IsDefaultPrevented());
    if (!aTarget || block->IsDefaultPrevented()) {
      return true;
    }
    aTarget->HandleInputEvent(aEvent);
    return true;
  }
  return false;
}

nsEventStatus
InputQueue::ReceiveTouchInput(const nsRefPtr<AsyncPanZoomController>& aTarget,
                              bool aTargetConfirmed,
                              const MultiTouchInput& aEvent,
                              uint64_t* aOutInputBlockId) {
  TouchBlockState* block = nullptr;
  if (aEvent.mType == MultiTouchInput::MULTITOUCH_START) {
    block = StartNewTouchBlock(aTarget, aTargetConfirmed, false);
    INPQ_LOG("started new touch block %p for target %p\n", block, aTarget.get());

    CancelAnimationsForNewBlock(block);
    MaybeRequestContentResponse(aTarget, block);
  } else {
    if (!mInputBlockQueue.IsEmpty()) {
      block = mInputBlockQueue.LastElement().get()->AsTouchBlock();
    }

    if (!block) {
      NS_WARNING("Received a non-start touch event while no touch blocks active!");
      return nsEventStatus_eIgnore;
    }

    INPQ_LOG("received new event in block %p\n", block);
  }

  if (aOutInputBlockId) {
    *aOutInputBlockId = block->GetBlockId();
  }

  
  
  
  
  nsRefPtr<AsyncPanZoomController> target = block->GetTargetApzc();

  nsEventStatus result = nsEventStatus_eIgnore;

  
  
  
  if (block->IsDuringFastMotion()) {
    result = nsEventStatus_eConsumeNoDefault;
  } else if (target && target->ArePointerEventsConsumable(block, aEvent.AsMultiTouchInput().mTouches.Length())) {
    result = nsEventStatus_eConsumeDoDefault;
  }
  if (!MaybeHandleCurrentBlock(target, block, aEvent)) {
    block->AddEvent(aEvent.AsMultiTouchInput());
  }
  return result;
}

nsEventStatus
InputQueue::ReceiveScrollWheelInput(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                    bool aTargetConfirmed,
                                    const ScrollWheelInput& aEvent,
                                    uint64_t* aOutInputBlockId) {
  WheelBlockState* block = nullptr;
  if (!mInputBlockQueue.IsEmpty()) {
    block = mInputBlockQueue.LastElement().get()->AsWheelBlock();
  }

  if (!block) {
    block = new WheelBlockState(aTarget, aTargetConfirmed);
    INPQ_LOG("started new scroll wheel block %p for target %p\n", block, aTarget.get());

    SweepDepletedBlocks();
    mInputBlockQueue.AppendElement(block);

    CancelAnimationsForNewBlock(block);
    MaybeRequestContentResponse(aTarget, block);
  } else {
    INPQ_LOG("received new event in block %p\n", block);
  }

  if (aOutInputBlockId) {
    *aOutInputBlockId = block->GetBlockId();
  }

  
  
  
  
  nsRefPtr<AsyncPanZoomController> target = block->GetTargetApzc();

  if (!MaybeHandleCurrentBlock(target, block, aEvent)) {
    block->AddEvent(aEvent.AsScrollWheelInput());
  }

  return nsEventStatus_eIgnore;
}

void
InputQueue::CancelAnimationsForNewBlock(CancelableBlockState* aBlock)
{
  
  
  
  
  
  
  if (aBlock == CurrentBlock()) {
    
    
    
    if (aBlock->GetOverscrollHandoffChain()->HasFastMovingApzc()) {
      
      
      if (TouchBlockState* touch = aBlock->AsTouchBlock()) {
        touch->SetDuringFastMotion();
        INPQ_LOG("block %p tagged as fast-motion\n", touch);
      }
    }
    aBlock->GetOverscrollHandoffChain()->CancelAnimations();
  }
}

void
InputQueue::MaybeRequestContentResponse(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                        CancelableBlockState* aBlock)
{
  bool waitForMainThread = !aBlock->IsTargetConfirmed();
  if (!gfxPrefs::LayoutEventRegionsEnabled()) {
    waitForMainThread |= aTarget->NeedToWaitForContent();
  }
  if (aBlock->AsTouchBlock() && aBlock->AsTouchBlock()->IsDuringFastMotion()) {
    aBlock->SetConfirmedTargetApzc(aTarget);
    waitForMainThread = false;
  }

  if (waitForMainThread) {
    
    
    
    
    ScheduleMainThreadTimeout(aTarget, aBlock->GetBlockId());
  } else {
    
    
    
    INPQ_LOG("not waiting for content response on block %p\n", block);
    aBlock->TimeoutContentResponse();
  }
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

void
InputQueue::SweepDepletedBlocks()
{
  
  
  while (!mInputBlockQueue.IsEmpty()) {
    CancelableBlockState* block = mInputBlockQueue[0].get();
    if (!block->IsReadyForHandling() || block->HasEvents()) {
      break;
    }

    INPQ_LOG("discarding depleted %s block %p\n", block->Type(), block);
    mInputBlockQueue.RemoveElementAt(0);
  }
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

  SweepDepletedBlocks();

  
  mInputBlockQueue.AppendElement(newBlock);
  return newBlock;
}

CancelableBlockState*
InputQueue::CurrentBlock() const
{
  AsyncPanZoomController::AssertOnControllerThread();

  MOZ_ASSERT(!mInputBlockQueue.IsEmpty());
  return mInputBlockQueue[0].get();
}

TouchBlockState*
InputQueue::CurrentTouchBlock() const
{
  TouchBlockState *block = CurrentBlock()->AsTouchBlock();
  MOZ_ASSERT(block);
  return block;
}

bool
InputQueue::HasReadyTouchBlock() const
{
  return !mInputBlockQueue.IsEmpty() &&
         mInputBlockQueue[0]->AsTouchBlock() &&
         mInputBlockQueue[0]->IsReadyForHandling();
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
  for (size_t i = 0; i < mInputBlockQueue.Length(); i++) {
    if (mInputBlockQueue[i]->GetBlockId() == aInputBlockId) {
      
      
      
      success = mInputBlockQueue[i]->TimeoutContentResponse();
      success |= mInputBlockQueue[i]->SetConfirmedTargetApzc(mInputBlockQueue[i]->GetTargetApzc());
      break;
    }
  }
  if (success) {
    ProcessInputBlocks();
  }
}

void
InputQueue::ContentReceivedInputBlock(uint64_t aInputBlockId, bool aPreventDefault) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got a content response; block=%" PRIu64 "\n", aInputBlockId);
  bool success = false;
  for (size_t i = 0; i < mInputBlockQueue.Length(); i++) {
    if (mInputBlockQueue[i]->GetBlockId() == aInputBlockId) {
      CancelableBlockState* block = mInputBlockQueue[i].get();
      success = block->SetContentResponse(aPreventDefault);
      break;
    }
  }
  if (success) {
    ProcessInputBlocks();
  }
}

void
InputQueue::SetConfirmedTargetApzc(uint64_t aInputBlockId, const nsRefPtr<AsyncPanZoomController>& aTargetApzc) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got a target apzc; block=%" PRIu64 " guid=%s\n",
    aInputBlockId, aTargetApzc ? Stringify(aTargetApzc->GetGuid()).c_str() : "");
  bool success = false;
  for (size_t i = 0; i < mInputBlockQueue.Length(); i++) {
    if (mInputBlockQueue[i]->GetBlockId() == aInputBlockId) {
      success = mInputBlockQueue[i]->SetConfirmedTargetApzc(aTargetApzc);
      break;
    }
  }
  if (success) {
    ProcessInputBlocks();
  } else {
    NS_WARNING("INPQ received useless SetConfirmedTargetApzc");
  }
}

void
InputQueue::SetAllowedTouchBehavior(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aBehaviors) {
  AsyncPanZoomController::AssertOnControllerThread();

  INPQ_LOG("got allowed touch behaviours; block=%" PRIu64 "\n", aInputBlockId);
  bool success = false;
  for (size_t i = 0; i < mInputBlockQueue.Length(); i++) {
    if (mInputBlockQueue[i]->GetBlockId() == aInputBlockId) {
      TouchBlockState *block = mInputBlockQueue[i]->AsTouchBlock();
      if (block) {
        success = block->SetAllowedTouchBehaviors(aBehaviors);
      } else {
        NS_WARNING("input block is not a touch block");
      }
      break;
    }
  }
  if (success) {
    ProcessInputBlocks();
  } else {
    NS_WARNING("INPQ received useless SetAllowedTouchBehavior");
  }
}

void
InputQueue::ProcessInputBlocks() {
  AsyncPanZoomController::AssertOnControllerThread();

  do {
    CancelableBlockState* curBlock = CurrentBlock();
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
      curBlock->HandleEvents(target);
    }
    MOZ_ASSERT(!curBlock->HasEvents());

    if (mInputBlockQueue.Length() == 1 && curBlock->MustStayActive()) {
      
      
      
      
      break;
    }

    
    
    INPQ_LOG("discarding depleted touch block %p\n", curBlock);
    mInputBlockQueue.RemoveElementAt(0);
  } while (!mInputBlockQueue.IsEmpty());
}

} 
} 
