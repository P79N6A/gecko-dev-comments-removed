





































#include "nsEventShell.h"

#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"





void
nsEventShell::FireEvent(nsAccEvent *aEvent)
{
  if (!aEvent)
    return;

  nsAccessible *accessible = aEvent->GetAccessible();
  NS_ENSURE_TRUE(accessible,);

  nsINode* node = aEvent->GetNode();
  if (node) {
    sEventTargetNode = node;
    sEventFromUserInput = aEvent->IsFromUserInput();
  }

  accessible->HandleAccEvent(aEvent);

  sEventTargetNode = nsnull;
}

void
nsEventShell::FireEvent(PRUint32 aEventType, nsAccessible *aAccessible,
                        PRBool aIsAsynch, EIsFromUserInput aIsFromUserInput)
{
  NS_ENSURE_TRUE(aAccessible,);

  nsRefPtr<nsAccEvent> event = new nsAccEvent(aEventType, aAccessible,
                                              aIsAsynch, aIsFromUserInput);

  FireEvent(event);
}

void 
nsEventShell::GetEventAttributes(nsINode *aNode,
                                 nsIPersistentProperties *aAttributes)
{
  if (aNode != sEventTargetNode)
    return;

  nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::eventFromInput,
                         sEventFromUserInput ? NS_LITERAL_STRING("true") :
                                               NS_LITERAL_STRING("false"));
}




PRBool nsEventShell::sEventFromUserInput = PR_FALSE;
nsCOMPtr<nsINode> nsEventShell::sEventTargetNode;






nsAccEventQueue::nsAccEventQueue(nsDocAccessible *aDocument):
  mObservingRefresh(PR_FALSE), mDocument(aDocument)
{
}

nsAccEventQueue::~nsAccEventQueue()
{
  NS_ASSERTION(!mDocument, "Queue wasn't shut down!");
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsAccEventQueue)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccEventQueue)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsAccEventQueue)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mDocument");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mDocument.get()));

  PRUint32 i, length = tmp->mEvents.Length();
  for (i = 0; i < length; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvents[i]");
    cb.NoteXPCOMChild(tmp->mEvents[i].get());
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsAccEventQueue)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mEvents)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAccEventQueue)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsAccEventQueue)




void
nsAccEventQueue::Push(nsAccEvent *aEvent)
{
  mEvents.AppendElement(aEvent);

  
  CoalesceEvents();

  
  
  AccHideEvent* hideEvent = downcast_accEvent(aEvent);
  if (hideEvent && !hideEvent->mTextChangeEvent)
    CreateTextChangeEventFor(hideEvent);

  
  PrepareFlush();
}

void
nsAccEventQueue::Shutdown()
{
  if (mObservingRefresh) {
    nsCOMPtr<nsIPresShell> shell = mDocument->GetPresShell();
    if (!shell ||
        shell->RemoveRefreshObserver(this, Flush_Display)) {
      mObservingRefresh = PR_FALSE;
    }
  }
  mDocument = nsnull;
  mEvents.Clear();
}




void
nsAccEventQueue::PrepareFlush()
{
  
  
  if (mEvents.Length() > 0 && !mObservingRefresh) {
    nsCOMPtr<nsIPresShell> shell = mDocument->GetPresShell();
    
    
    if (shell &&
        shell->AddRefreshObserver(this, Flush_Display)) {
      mObservingRefresh = PR_TRUE;
    }
  }
}

void
nsAccEventQueue::WillRefresh(mozilla::TimeStamp aTime)
{
  
  
  if (!mDocument)
    return;

  
  
  nsTArray < nsRefPtr<nsAccEvent> > events;
  events.SwapElements(mEvents);
  PRUint32 length = events.Length();
  NS_ASSERTION(length, "How did we get here without events to fire?");

  for (PRUint32 index = 0; index < length; index ++) {

    nsAccEvent *accEvent = events[index];
    if (accEvent->mEventRule != nsAccEvent::eDoNotEmit) {
      mDocument->ProcessPendingEvent(accEvent);

      AccHideEvent* hideEvent = downcast_accEvent(accEvent);
      if (hideEvent) {
        if (hideEvent->mTextChangeEvent)
          mDocument->ProcessPendingEvent(hideEvent->mTextChangeEvent);
      }
    }

    
    if (!mDocument)
      return;
  }

  if (mEvents.Length() == 0) {
    nsCOMPtr<nsIPresShell> shell = mDocument->GetPresShell();
    if (!shell ||
        shell->RemoveRefreshObserver(this, Flush_Display)) {
      mObservingRefresh = PR_FALSE;
    }
  }
}

void
nsAccEventQueue::CoalesceEvents()
{
  PRUint32 numQueuedEvents = mEvents.Length();
  PRInt32 tail = numQueuedEvents - 1;
  nsAccEvent* tailEvent = mEvents[tail];

  
  
  if (!tailEvent->mNode)
    return;

  switch(tailEvent->mEventRule) {
    case nsAccEvent::eCoalesceFromSameSubtree:
    {
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        nsAccEvent* thisEvent = mEvents[index];

        if (thisEvent->mEventType != tailEvent->mEventType)
          continue; 

        
        
        
        if (!thisEvent->mNode ||
            thisEvent->mNode->GetOwnerDoc() != tailEvent->mNode->GetOwnerDoc())
          continue;

        
        
        

        
        
        
        

        
        if (tailEvent->mEventType == nsIAccessibleEvent::EVENT_HIDE) {
          AccHideEvent* tailHideEvent = downcast_accEvent(tailEvent);
          AccHideEvent* thisHideEvent = downcast_accEvent(thisEvent);
          if (thisHideEvent->mParent == tailHideEvent->mParent) {
            tailEvent->mEventRule = thisEvent->mEventRule;

            
            if (tailEvent->mEventRule != nsAccEvent::eDoNotEmit)
              CoalesceTextChangeEventsFor(tailHideEvent, thisHideEvent);

            return;
          }
        }

        
        if (!thisEvent->mNode->IsInDoc())
          continue;

        
        if (thisEvent->mNode->GetNodeParent() ==
            tailEvent->mNode->GetNodeParent()) {
          tailEvent->mEventRule = thisEvent->mEventRule;
          return;
        }

        
        PRBool thisCanBeDescendantOfTail = PR_FALSE;

        
        if (thisEvent->mEventRule == nsAccEvent::eDoNotEmit) {
          
          
          
          

          
          
          

          if (thisEvent->mNode == tailEvent->mNode) {
            thisEvent->mEventRule = nsAccEvent::eDoNotEmit;
            return;
          }

        } else {
          
          
          
          

          
          
          
          if (thisEvent->mNode == tailEvent->mNode) {
            
            
            if (thisEvent->mEventType == nsIAccessibleEvent::EVENT_REORDER) {
              CoalesceReorderEventsFromSameSource(thisEvent, tailEvent);
              if (tailEvent->mEventRule != nsAccEvent::eDoNotEmit)
                continue;
            }
            else {
              tailEvent->mEventRule = nsAccEvent::eDoNotEmit;
            }

            return;
          }

          
          

          
          
          
          
          
          
          
          thisCanBeDescendantOfTail =
            tailEvent->mEventType != nsIAccessibleEvent::EVENT_SHOW ||
            tailEvent->mIsAsync;
        }

        
        
        
        
        
        
        if (tailEvent->mEventType != nsIAccessibleEvent::EVENT_HIDE &&
            nsCoreUtils::IsAncestorOf(thisEvent->mNode, tailEvent->mNode)) {

          if (thisEvent->mEventType == nsIAccessibleEvent::EVENT_REORDER) {
            CoalesceReorderEventsFromSameTree(thisEvent, tailEvent);
            if (tailEvent->mEventRule != nsAccEvent::eDoNotEmit)
              continue;

            return;
          }

          tailEvent->mEventRule = nsAccEvent::eDoNotEmit;
          return;
        }

#ifdef DEBUG
        if (tailEvent->mEventType == nsIAccessibleEvent::EVENT_HIDE &&
            nsCoreUtils::IsAncestorOf(thisEvent->mNode, tailEvent->mNode)) {
          NS_NOTREACHED("More older hide event target is an ancestor of recent hide event target!");
        }
#endif

        
        
        if (thisCanBeDescendantOfTail &&
            nsCoreUtils::IsAncestorOf(tailEvent->mNode, thisEvent->mNode)) {

          if (thisEvent->mEventType == nsIAccessibleEvent::EVENT_REORDER) {
            CoalesceReorderEventsFromSameTree(tailEvent, thisEvent);
            if (tailEvent->mEventRule != nsAccEvent::eDoNotEmit)
              continue;

            return;
          }

          
          
          thisEvent->mEventRule = nsAccEvent::eDoNotEmit;
          ApplyToSiblings(0, index, thisEvent->mEventType,
                          thisEvent->mNode, nsAccEvent::eDoNotEmit);
          continue;
        }

#ifdef DEBUG
        if (!thisCanBeDescendantOfTail &&
            nsCoreUtils::IsAncestorOf(tailEvent->mNode, thisEvent->mNode)) {
          NS_NOTREACHED("Older event target is a descendant of recent event target!");
        }
#endif

      } 

    } break; 

    case nsAccEvent::eCoalesceFromSameDocument:
    {
      
      
      
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        nsAccEvent* thisEvent = mEvents[index];
        if (thisEvent->mEventType == tailEvent->mEventType &&
            thisEvent->mEventRule == tailEvent->mEventRule &&
            thisEvent->GetDocAccessible() == tailEvent->GetDocAccessible()) {
          thisEvent->mEventRule = nsAccEvent::eDoNotEmit;
          return;
        }
      }
    } break; 

    case nsAccEvent::eRemoveDupes:
    {
      
      
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        nsAccEvent* accEvent = mEvents[index];
        if (accEvent->mEventType == tailEvent->mEventType &&
            accEvent->mEventRule == tailEvent->mEventRule &&
            accEvent->mNode == tailEvent->mNode) {
          tailEvent->mEventRule = nsAccEvent::eDoNotEmit;
          return;
        }
      }
    } break; 

    default:
      break; 
  } 
}

void
nsAccEventQueue::ApplyToSiblings(PRUint32 aStart, PRUint32 aEnd,
                                 PRUint32 aEventType, nsINode* aNode,
                                 nsAccEvent::EEventRule aEventRule)
{
  for (PRUint32 index = aStart; index < aEnd; index ++) {
    nsAccEvent* accEvent = mEvents[index];
    if (accEvent->mEventType == aEventType &&
        accEvent->mEventRule != nsAccEvent::eDoNotEmit &&
        accEvent->mNode->GetNodeParent() == aNode->GetNodeParent()) {
      accEvent->mEventRule = aEventRule;
    }
  }
}

void
nsAccEventQueue::CoalesceReorderEventsFromSameSource(nsAccEvent *aAccEvent1,
                                                     nsAccEvent *aAccEvent2)
{
  
  nsAccReorderEvent *reorderEvent1 = downcast_accEvent(aAccEvent1);
  if (reorderEvent1->IsUnconditionalEvent()) {
    aAccEvent2->mEventRule = nsAccEvent::eDoNotEmit;
    return;
  }

  
  nsAccReorderEvent *reorderEvent2 = downcast_accEvent(aAccEvent2);
  if (reorderEvent2->IsUnconditionalEvent()) {
    aAccEvent1->mEventRule = nsAccEvent::eDoNotEmit;
    return;
  }

  
  if (reorderEvent1->HasAccessibleInReasonSubtree())
    aAccEvent2->mEventRule = nsAccEvent::eDoNotEmit;
  else
    aAccEvent1->mEventRule = nsAccEvent::eDoNotEmit;
}

void
nsAccEventQueue::CoalesceReorderEventsFromSameTree(nsAccEvent *aAccEvent,
                                                   nsAccEvent *aDescendantAccEvent)
{
  
  nsAccReorderEvent *reorderEvent = downcast_accEvent(aAccEvent);
  if (reorderEvent->IsUnconditionalEvent())
    aDescendantAccEvent->mEventRule = nsAccEvent::eDoNotEmit;
}

void
nsAccEventQueue::CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
                                             AccHideEvent* aThisEvent)
{
  
  

  nsAccTextChangeEvent* textEvent = aThisEvent->mTextChangeEvent;
  if (!textEvent)
    return;

  if (aThisEvent->mNextSibling == aTailEvent->mAccessible) {
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText,
                                          0, PR_UINT32_MAX);

  } else if (aThisEvent->mPrevSibling == aTailEvent->mAccessible) {
    PRUint32 oldLen = textEvent->GetLength();
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText,
                                          0, PR_UINT32_MAX);
    textEvent->mStart -= textEvent->GetLength() - oldLen;
  }

  aTailEvent->mTextChangeEvent.swap(aThisEvent->mTextChangeEvent);
}

void
nsAccEventQueue::CreateTextChangeEventFor(AccHideEvent* aEvent)
{
  nsRefPtr<nsHyperTextAccessible> textAccessible = do_QueryObject(
    GetAccService()->GetContainerAccessible(aEvent->mNode,
                                            aEvent->mAccessible->GetWeakShell()));
  if (!textAccessible)
    return;

  
  if (nsAccUtils::Role(aEvent->mAccessible) ==
      nsIAccessibleRole::ROLE_WHITESPACE) {
    nsCOMPtr<nsIEditor> editor;
    textAccessible->GetAssociatedEditor(getter_AddRefs(editor));
    if (editor) {
      PRBool isEmpty = PR_FALSE;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty)
        return;
    }
  }

  PRInt32 offset = textAccessible->GetChildOffset(aEvent->mAccessible);

  nsAutoString text;
  aEvent->mAccessible->AppendTextTo(text, 0, PR_UINT32_MAX);
  if (text.IsEmpty())
    return;

  aEvent->mTextChangeEvent =
    new nsAccTextChangeEvent(textAccessible, offset, text, PR_FALSE,
                             aEvent->mIsAsync,
                             aEvent->mIsFromUserInput ? eFromUserInput : eNoUserInput);
}
