





#include "AccEvent.h"

#include "ApplicationAccessibleWrap.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "nsIAccessibleText.h"
#include "nsAccEvent.h"
#include "States.h"

#include "nsEventStateManager.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsIDOMXULMultSelectCntrlEl.h"
#endif

using namespace mozilla::a11y;








AccEvent::AccEvent(uint32_t aEventType, Accessible* aAccessible,
                   EIsFromUserInput aIsFromUserInput, EEventRule aEventRule) :
  mEventType(aEventType), mEventRule(aEventRule), mAccessible(aAccessible)
{
  if (aIsFromUserInput == eAutoDetect)
    mIsFromUserInput = nsEventStateManager::IsHandlingUserInput();
  else
    mIsFromUserInput = aIsFromUserInput == eFromUserInput ? true : false;
}




already_AddRefed<nsAccEvent>
AccEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccEvent(this);
  NS_IF_ADDREF(event);
  return event;
}




NS_IMPL_CYCLE_COLLECTION_1(AccEvent, mAccessible)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AccEvent, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AccEvent, Release)





already_AddRefed<nsAccEvent>
AccStateChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccStateChangeEvent(this);
  NS_IF_ADDREF(event);
  return event;
}













AccTextChangeEvent::
  AccTextChangeEvent(Accessible* aAccessible, int32_t aStart,
                     const nsAString& aModifiedText, bool aIsInserted,
                     EIsFromUserInput aIsFromUserInput)
  : AccEvent(aIsInserted ?
             static_cast<uint32_t>(nsIAccessibleEvent::EVENT_TEXT_INSERTED) :
             static_cast<uint32_t>(nsIAccessibleEvent::EVENT_TEXT_REMOVED),
             aAccessible, aIsFromUserInput, eAllowDupes)
  , mStart(aStart)
  , mIsInserted(aIsInserted)
  , mModifiedText(aModifiedText)
{
  
  
   mIsFromUserInput = mAccessible->State() &
    (states::FOCUSED | states::EDITABLE);
}

already_AddRefed<nsAccEvent>
AccTextChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccTextChangeEvent(this);
  NS_IF_ADDREF(event);
  return event;
}






uint32_t
AccReorderEvent::IsShowHideEventTarget(const Accessible* aTarget) const
{
  uint32_t count = mDependentEvents.Length();
  for (uint32_t index = count - 1; index < count; index--) {
    if (mDependentEvents[index]->mAccessible == aTarget) {
      uint32_t eventType = mDependentEvents[index]->mEventType;
      if (eventType == nsIAccessibleEvent::EVENT_SHOW ||
          eventType == nsIAccessibleEvent::EVENT_HIDE) {
        return mDependentEvents[index]->mEventType;
      }
    }
  }

  return 0;
}





AccHideEvent::
  AccHideEvent(Accessible* aTarget, nsINode* aTargetNode) :
  AccMutationEvent(::nsIAccessibleEvent::EVENT_HIDE, aTarget, aTargetNode)
{
  mNextSibling = mAccessible->NextSibling();
  mPrevSibling = mAccessible->PrevSibling();
}

already_AddRefed<nsAccEvent>
AccHideEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccHideEvent(this);
  NS_ADDREF(event);
  return event;
}






AccShowEvent::
  AccShowEvent(Accessible* aTarget, nsINode* aTargetNode) :
  AccMutationEvent(::nsIAccessibleEvent::EVENT_SHOW, aTarget, aTargetNode)
{
}






already_AddRefed<nsAccEvent>
AccCaretMoveEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccCaretMoveEvent(this);
  NS_IF_ADDREF(event);
  return event;
}






AccSelChangeEvent::
  AccSelChangeEvent(Accessible* aWidget, Accessible* aItem,
                    SelChangeType aSelChangeType) :
    AccEvent(0, aItem, eAutoDetect, eCoalesceSelectionChange),
    mWidget(aWidget), mItem(aItem), mSelChangeType(aSelChangeType),
    mPreceedingCount(0), mPackedEvent(nullptr)
{
  if (aSelChangeType == eSelectionAdd) {
    if (mWidget->GetSelectedItem(1))
      mEventType = nsIAccessibleEvent::EVENT_SELECTION_ADD;
    else
      mEventType = nsIAccessibleEvent::EVENT_SELECTION;
  } else {
    mEventType = nsIAccessibleEvent::EVENT_SELECTION_REMOVE;
  }
}






AccTableChangeEvent::
  AccTableChangeEvent(Accessible* aAccessible, uint32_t aEventType,
                      int32_t aRowOrColIndex, int32_t aNumRowsOrCols) :
  AccEvent(aEventType, aAccessible),
  mRowOrColIndex(aRowOrColIndex), mNumRowsOrCols(aNumRowsOrCols)
{
}

already_AddRefed<nsAccEvent>
AccTableChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccTableChangeEvent(this);
  NS_IF_ADDREF(event);
  return event;
}






AccVCChangeEvent::
  AccVCChangeEvent(Accessible* aAccessible,
                   nsIAccessible* aOldAccessible,
                   int32_t aOldStart, int32_t aOldEnd,
                   int16_t aReason) :
    AccEvent(::nsIAccessibleEvent::EVENT_VIRTUALCURSOR_CHANGED, aAccessible),
    mOldAccessible(aOldAccessible), mOldStart(aOldStart), mOldEnd(aOldEnd),
    mReason(aReason)
{
}

already_AddRefed<nsAccEvent>
AccVCChangeEvent::CreateXPCOMObject()
{
  nsAccEvent* event = new nsAccVirtualCursorChangeEvent(this);
  NS_ADDREF(event);
  return event;
}
