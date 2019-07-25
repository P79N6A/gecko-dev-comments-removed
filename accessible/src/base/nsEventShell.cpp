





































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
  mProcessingStarted(PR_FALSE), mDocument(aDocument), mFlushingEventsCount(0)
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
  
  
  PrepareFlush();
}

void
nsAccEventQueue::Shutdown()
{
  mDocument = nsnull;
  mEvents.Clear();
}




void
nsAccEventQueue::PrepareFlush()
{
  
  
  if (mEvents.Length() > 0 && !mProcessingStarted) {
    NS_DISPATCH_RUNNABLEMETHOD(Flush, this)
    mProcessingStarted = PR_TRUE;
  }
}

void
nsAccEventQueue::Flush()
{
  
  
  if (!mDocument)
    return;

  nsCOMPtr<nsIPresShell> presShell = mDocument->GetPresShell();
  if (!presShell)
    return;

  
  
  
  
  
  presShell->FlushPendingNotifications(Flush_Layout);

  
  
  mFlushingEventsCount = mEvents.Length();
  NS_ASSERTION(mFlushingEventsCount,
               "How did we get here without events to fire?");

  for (PRUint32 index = 0; index < mFlushingEventsCount; index ++) {

    
    
    if (!mDocument || !mDocument->HasWeakShell())
      break;

    nsAccEvent *accEvent = mEvents[index];
    if (accEvent->mEventRule != nsAccEvent::eDoNotEmit)
      mDocument->ProcessPendingEvent(accEvent);
  }

  
  mProcessingStarted = PR_FALSE;

  
  
  
  
  if (mDocument && mDocument->HasWeakShell()) {
    mEvents.RemoveElementsAt(0, mFlushingEventsCount);
    mFlushingEventsCount = 0;
    PrepareFlush();
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
      for (PRInt32 index = tail - 1; index >= mFlushingEventsCount; index--) {
        nsAccEvent* thisEvent = mEvents[index];

        if (thisEvent->mEventType != tailEvent->mEventType)
          continue; 

        
        
        
        if (!thisEvent->mNode || !thisEvent->mNode->IsInDoc() ||
            thisEvent->mNode->GetOwnerDoc() != tailEvent->mNode->GetOwnerDoc())
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
          ApplyToSiblings(mFlushingEventsCount, index, thisEvent->mEventType,
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
      
      
      
      for (PRInt32 index = tail - 1; index >= mFlushingEventsCount; index--) {
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
      
      
      for (PRInt32 index = tail - 1; index >= mFlushingEventsCount; index--) {
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
  
  nsCOMPtr<nsAccReorderEvent> reorderEvent1 = do_QueryInterface(aAccEvent1);
  if (reorderEvent1->IsUnconditionalEvent()) {
    aAccEvent2->mEventRule = nsAccEvent::eDoNotEmit;
    return;
  }

  
  nsCOMPtr<nsAccReorderEvent> reorderEvent2 = do_QueryInterface(aAccEvent2);
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
  
  nsCOMPtr<nsAccReorderEvent> reorderEvent = do_QueryInterface(aAccEvent);
  if (reorderEvent->IsUnconditionalEvent())
    aDescendantAccEvent->mEventRule = nsAccEvent::eDoNotEmit;
}
