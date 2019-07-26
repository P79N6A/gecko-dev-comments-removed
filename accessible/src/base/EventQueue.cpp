




#include "EventQueue.h"

#include "Accessible-inl.h"
#include "DocAccessible-inl.h"
#include "nsEventShell.h"

using namespace mozilla;
using namespace mozilla::a11y;



const unsigned int kSelChangeCountToPack = 5;





bool
EventQueue::PushEvent(AccEvent* aEvent)
{
  NS_ASSERTION((aEvent->mAccessible && aEvent->mAccessible->IsApplication()) ||
               aEvent->GetDocAccessible() == mDocument,
               "Queued event belongs to another document!");

  if (!mEvents.AppendElement(aEvent))
    return false;

  
  CoalesceEvents();

  
  
  AccMutationEvent* showOrHideEvent = downcast_accEvent(aEvent);
  if (showOrHideEvent && !showOrHideEvent->mTextChangeEvent)
    CreateTextChangeEventFor(showOrHideEvent);

  return true;
}




void
EventQueue::CoalesceEvents()
{
  NS_ASSERTION(mEvents.Length(), "There should be at least one pending event!");
  uint32_t tail = mEvents.Length() - 1;
  AccEvent* tailEvent = mEvents[tail];

  switch(tailEvent->mEventRule) {
    case AccEvent::eCoalesceReorder:
      CoalesceReorderEvents(tailEvent);
      break; 

    case AccEvent::eCoalesceMutationTextChange:
    {
      for (uint32_t index = tail - 1; index < tail; index--) {
        AccEvent* thisEvent = mEvents[index];
        if (thisEvent->mEventRule != tailEvent->mEventRule)
          continue;

        
        if (thisEvent->mEventType != tailEvent->mEventType)
          continue;

        
        
        
        if (thisEvent->mAccessible == tailEvent->mAccessible)
          thisEvent->mEventRule = AccEvent::eDoNotEmit;

        AccMutationEvent* tailMutationEvent = downcast_accEvent(tailEvent);
        AccMutationEvent* thisMutationEvent = downcast_accEvent(thisEvent);
        if (tailMutationEvent->mParent != thisMutationEvent->mParent)
          continue;

        
        if (thisMutationEvent->IsHide()) {
          AccHideEvent* tailHideEvent = downcast_accEvent(tailEvent);
          AccHideEvent* thisHideEvent = downcast_accEvent(thisEvent);
          CoalesceTextChangeEventsFor(tailHideEvent, thisHideEvent);
          break;
        }

        AccShowEvent* tailShowEvent = downcast_accEvent(tailEvent);
        AccShowEvent* thisShowEvent = downcast_accEvent(thisEvent);
        CoalesceTextChangeEventsFor(tailShowEvent, thisShowEvent);
        break;
      }
    } break; 

    case AccEvent::eCoalesceOfSameType:
    {
      
      for (uint32_t index = tail - 1; index < tail; index--) {
        AccEvent* accEvent = mEvents[index];
        if (accEvent->mEventType == tailEvent->mEventType &&
          accEvent->mEventRule == tailEvent->mEventRule) {
          accEvent->mEventRule = AccEvent::eDoNotEmit;
          return;
        }
      }
    } break; 

    case AccEvent::eCoalesceSelectionChange:
    {
      AccSelChangeEvent* tailSelChangeEvent = downcast_accEvent(tailEvent);
      for (uint32_t index = tail - 1; index < tail; index--) {
        AccEvent* thisEvent = mEvents[index];
        if (thisEvent->mEventRule == tailEvent->mEventRule) {
          AccSelChangeEvent* thisSelChangeEvent =
            downcast_accEvent(thisEvent);

          
          if (tailSelChangeEvent->mWidget == thisSelChangeEvent->mWidget) {
            CoalesceSelChangeEvents(tailSelChangeEvent, thisSelChangeEvent, index);
            return;
          }
        }
      }

    } break; 

    case AccEvent::eCoalesceStateChange:
    {
      
      
      
      for (uint32_t index = tail - 1; index < tail; index--) {
        AccEvent* thisEvent = mEvents[index];
        if (thisEvent->mEventRule != AccEvent::eDoNotEmit &&
            thisEvent->mEventType == tailEvent->mEventType &&
            thisEvent->mAccessible == tailEvent->mAccessible) {
          AccStateChangeEvent* thisSCEvent = downcast_accEvent(thisEvent);
          AccStateChangeEvent* tailSCEvent = downcast_accEvent(tailEvent);
          if (thisSCEvent->mState == tailSCEvent->mState) {
            thisEvent->mEventRule = AccEvent::eDoNotEmit;
            if (thisSCEvent->mIsEnabled != tailSCEvent->mIsEnabled)
              tailEvent->mEventRule = AccEvent::eDoNotEmit;
          }
        }
      }
      break; 
    }

    case AccEvent::eRemoveDupes:
    {
      
      
      for (uint32_t index = tail - 1; index < tail; index--) {
        AccEvent* accEvent = mEvents[index];
        if (accEvent->mEventType == tailEvent->mEventType &&
          accEvent->mEventRule == tailEvent->mEventRule &&
          accEvent->mAccessible == tailEvent->mAccessible) {
          tailEvent->mEventRule = AccEvent::eDoNotEmit;
          return;
        }
      }
    } break; 

    default:
      break; 
  } 
}

void
EventQueue::CoalesceReorderEvents(AccEvent* aTailEvent)
{
  uint32_t count = mEvents.Length();
  for (uint32_t index = count - 2; index < count; index--) {
    AccEvent* thisEvent = mEvents[index];

    
    if (thisEvent->mEventType != aTailEvent->mEventType ||
        thisEvent->mAccessible->IsApplication())
      continue;

    
    
    if (!thisEvent->mAccessible->IsDoc() &&
        !thisEvent->mAccessible->IsInDocument()) {
      thisEvent->mEventRule = AccEvent::eDoNotEmit;
      continue;
    }

    
    if (thisEvent->mAccessible == aTailEvent->mAccessible) {
      if (thisEvent->mEventRule == AccEvent::eDoNotEmit) {
        AccReorderEvent* tailReorder = downcast_accEvent(aTailEvent);
        tailReorder->DoNotEmitAll();
      } else {
        thisEvent->mEventRule = AccEvent::eDoNotEmit;
      }

      return;
    }

    
    
    
    
    
    
    
    Accessible* thisParent = thisEvent->mAccessible;
    while (thisParent && thisParent != mDocument) {
      if (thisParent->Parent() == aTailEvent->mAccessible) {
        AccReorderEvent* tailReorder = downcast_accEvent(aTailEvent);
        uint32_t eventType = tailReorder->IsShowHideEventTarget(thisParent);

        if (eventType == nsIAccessibleEvent::EVENT_SHOW) {
           NS_ERROR("Accessible tree was created after it was modified! Huh?");
        } else if (eventType == nsIAccessibleEvent::EVENT_HIDE) {
          AccReorderEvent* thisReorder = downcast_accEvent(thisEvent);
          thisReorder->DoNotEmitAll();
        } else {
          thisEvent->mEventRule = AccEvent::eDoNotEmit;
        }

        return;
      }

      thisParent = thisParent->Parent();
    }

    
    
    
    
    
    
    
    Accessible* tailParent = aTailEvent->mAccessible;
    while (tailParent && tailParent != mDocument) {
      if (tailParent->Parent() == thisEvent->mAccessible) {
        AccReorderEvent* thisReorder = downcast_accEvent(thisEvent);
        AccReorderEvent* tailReorder = downcast_accEvent(aTailEvent);
        uint32_t eventType = thisReorder->IsShowHideEventTarget(tailParent);
        if (eventType == nsIAccessibleEvent::EVENT_SHOW)
          tailReorder->DoNotEmitAll();
        else if (eventType == nsIAccessibleEvent::EVENT_HIDE)
          NS_ERROR("Accessible tree was modified after it was removed! Huh?");
        else
          aTailEvent->mEventRule = AccEvent::eDoNotEmit;

        return;
      }

      tailParent = tailParent->Parent();
    }

  } 
}

void
EventQueue::CoalesceSelChangeEvents(AccSelChangeEvent* aTailEvent,
                                    AccSelChangeEvent* aThisEvent,
                                    uint32_t aThisIndex)
{
  aTailEvent->mPreceedingCount = aThisEvent->mPreceedingCount + 1;

  
  
  if (aTailEvent->mPreceedingCount >= kSelChangeCountToPack) {
    aTailEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION_WITHIN;
    aTailEvent->mAccessible = aTailEvent->mWidget;
    aThisEvent->mEventRule = AccEvent::eDoNotEmit;

    
    
    if (aThisEvent->mEventType != nsIAccessibleEvent::EVENT_SELECTION_WITHIN) {
      for (uint32_t jdx = aThisIndex - 1; jdx < aThisIndex; jdx--) {
        AccEvent* prevEvent = mEvents[jdx];
        if (prevEvent->mEventRule == aTailEvent->mEventRule) {
          AccSelChangeEvent* prevSelChangeEvent =
            downcast_accEvent(prevEvent);
          if (prevSelChangeEvent->mWidget == aTailEvent->mWidget)
            prevSelChangeEvent->mEventRule = AccEvent::eDoNotEmit;
        }
      }
    }
    return;
  }

  
  
  if (aTailEvent->mPreceedingCount == 1 &&
      aTailEvent->mItem != aThisEvent->mItem) {
    if (aTailEvent->mSelChangeType == AccSelChangeEvent::eSelectionAdd &&
        aThisEvent->mSelChangeType == AccSelChangeEvent::eSelectionRemove) {
      aThisEvent->mEventRule = AccEvent::eDoNotEmit;
      aTailEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION;
      aTailEvent->mPackedEvent = aThisEvent;
      return;
    }

    if (aThisEvent->mSelChangeType == AccSelChangeEvent::eSelectionAdd &&
        aTailEvent->mSelChangeType == AccSelChangeEvent::eSelectionRemove) {
      aTailEvent->mEventRule = AccEvent::eDoNotEmit;
      aThisEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION;
      aThisEvent->mPackedEvent = aThisEvent;
      return;
    }
  }

  
  
  if (aThisEvent->mEventType == nsIAccessibleEvent::EVENT_SELECTION) {
    if (aThisEvent->mPackedEvent) {
      aThisEvent->mPackedEvent->mEventType =
        aThisEvent->mPackedEvent->mSelChangeType == AccSelChangeEvent::eSelectionAdd ?
          nsIAccessibleEvent::EVENT_SELECTION_ADD :
          nsIAccessibleEvent::EVENT_SELECTION_REMOVE;

      aThisEvent->mPackedEvent->mEventRule =
        AccEvent::eCoalesceSelectionChange;

      aThisEvent->mPackedEvent = nullptr;
    }

    aThisEvent->mEventType =
      aThisEvent->mSelChangeType == AccSelChangeEvent::eSelectionAdd ?
        nsIAccessibleEvent::EVENT_SELECTION_ADD :
        nsIAccessibleEvent::EVENT_SELECTION_REMOVE;

    return;
  }

  
  
  if (aTailEvent->mEventType == nsIAccessibleEvent::EVENT_SELECTION)
    aTailEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION_ADD;
}

void
EventQueue::CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
                                        AccHideEvent* aThisEvent)
{
  
  

  AccTextChangeEvent* textEvent = aThisEvent->mTextChangeEvent;
  if (!textEvent)
    return;

  if (aThisEvent->mNextSibling == aTailEvent->mAccessible) {
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText);

  } else if (aThisEvent->mPrevSibling == aTailEvent->mAccessible) {
    uint32_t oldLen = textEvent->GetLength();
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText);
    textEvent->mStart -= textEvent->GetLength() - oldLen;
  }

  aTailEvent->mTextChangeEvent.swap(aThisEvent->mTextChangeEvent);
}

void
EventQueue::CoalesceTextChangeEventsFor(AccShowEvent* aTailEvent,
                                        AccShowEvent* aThisEvent)
{
  AccTextChangeEvent* textEvent = aThisEvent->mTextChangeEvent;
  if (!textEvent)
    return;

  if (aTailEvent->mAccessible->IndexInParent() ==
      aThisEvent->mAccessible->IndexInParent() + 1) {
    
    
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText);

  } else if (aTailEvent->mAccessible->IndexInParent() ==
             aThisEvent->mAccessible->IndexInParent() -1) {
    
    
    nsAutoString startText;
    aTailEvent->mAccessible->AppendTextTo(startText);
    textEvent->mModifiedText = startText + textEvent->mModifiedText;
    textEvent->mStart -= startText.Length();
  }

  aTailEvent->mTextChangeEvent.swap(aThisEvent->mTextChangeEvent);
}

void
EventQueue::CreateTextChangeEventFor(AccMutationEvent* aEvent)
{
  Accessible* container = aEvent->mAccessible->Parent();
  if (!container)
    return;

  HyperTextAccessible* textAccessible = container->AsHyperText();
  if (!textAccessible)
    return;

  
  if (aEvent->mAccessible->Role() == roles::WHITESPACE) {
    nsCOMPtr<nsIEditor> editor = textAccessible->GetEditor();
    if (editor) {
      bool isEmpty = false;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty)
        return;
    }
  }

  int32_t offset = textAccessible->GetChildOffset(aEvent->mAccessible);

  nsAutoString text;
  aEvent->mAccessible->AppendTextTo(text);
  if (text.IsEmpty())
    return;

  aEvent->mTextChangeEvent =
    new AccTextChangeEvent(textAccessible, offset, text, aEvent->IsShow(),
                           aEvent->mIsFromUserInput ? eFromUserInput : eNoUserInput);
}




void
EventQueue::ProcessEventQueue()
{
  
  nsTArray<nsRefPtr<AccEvent> > events;
  events.SwapElements(mEvents);

  uint32_t eventCount = events.Length();
#ifdef A11Y_LOG
  if (eventCount > 0 && logging::IsEnabled(logging::eEvents)) {
    logging::MsgBegin("EVENTS", "events processing");
    logging::Address("document", mDocument);
    logging::MsgEnd();
  }
#endif

  for (uint32_t idx = 0; idx < eventCount; idx++) {
    AccEvent* event = events[idx];
    if (event->mEventRule != AccEvent::eDoNotEmit) {
      Accessible* target = event->GetAccessible();
      if (!target || target->IsDefunct())
        continue;

      
      if (event->mEventType == nsIAccessibleEvent::EVENT_FOCUS) {
        FocusMgr()->ProcessFocusEvent(event);
        continue;
      }

      
      if (event->mEventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED) {
        AccCaretMoveEvent* caretMoveEvent = downcast_accEvent(event);
        HyperTextAccessible* hyperText = target->AsHyperText();
        if (hyperText &&
            NS_SUCCEEDED(hyperText->GetCaretOffset(&caretMoveEvent->mCaretOffset))) {

          nsEventShell::FireEvent(caretMoveEvent);

          
          int32_t selectionCount;
          hyperText->GetSelectionCount(&selectionCount);
          if (selectionCount)
            nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED,
                                    hyperText);
        }
        continue;
      }

      nsEventShell::FireEvent(event);

      
      AccMutationEvent* mutationEvent = downcast_accEvent(event);
      if (mutationEvent) {
        if (mutationEvent->mTextChangeEvent)
          nsEventShell::FireEvent(mutationEvent->mTextChangeEvent);
      }
    }

    if (event->mEventType == nsIAccessibleEvent::EVENT_HIDE)
      mDocument->ShutdownChildrenInSubtree(event->mAccessible);

    if (!mDocument)
      return;
  }
}
