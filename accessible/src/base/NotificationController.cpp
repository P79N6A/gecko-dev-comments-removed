





































#include "NotificationController.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "nsEventShell.h"
#include "nsTextAccessible.h"
#include "FocusManager.h"
#include "TextUpdater.h"

#include "mozilla/dom/Element.h"

using namespace mozilla::a11y;



const unsigned int kSelChangeCountToPack = 5;





NotificationController::NotificationController(nsDocAccessible* aDocument,
                                               nsIPresShell* aPresShell) :
  mObservingState(eNotObservingRefresh), mDocument(aDocument),
  mPresShell(aPresShell)
{
  mTextHash.Init();

  
  ScheduleProcessing();
}

NotificationController::~NotificationController()
{
  NS_ASSERTION(!mDocument, "Controller wasn't shutdown properly!");
  if (mDocument)
    Shutdown();
}




NS_IMPL_ADDREF(NotificationController)
NS_IMPL_RELEASE(NotificationController)

NS_IMPL_CYCLE_COLLECTION_CLASS(NotificationController)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(NotificationController)
  if (tmp->mDocument)
    tmp->Shutdown();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(NotificationController)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mDocument");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mDocument.get()));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(mHangingChildDocuments,
                                                    nsDocAccessible)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(mContentInsertions,
                                                    ContentInsertion)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(mEvents, AccEvent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(NotificationController, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(NotificationController, Release)




void
NotificationController::Shutdown()
{
  if (mObservingState != eNotObservingRefresh &&
      mPresShell->RemoveRefreshObserver(this, Flush_Display)) {
    mObservingState = eNotObservingRefresh;
  }

  
  PRInt32 childDocCount = mHangingChildDocuments.Length();
  for (PRInt32 idx = childDocCount - 1; idx >= 0; idx--)
    mHangingChildDocuments[idx]->Shutdown();

  mHangingChildDocuments.Clear();

  mDocument = nsnull;
  mPresShell = nsnull;

  mTextHash.Clear();
  mContentInsertions.Clear();
  mNotifications.Clear();
  mEvents.Clear();
}

void
NotificationController::QueueEvent(AccEvent* aEvent)
{
  if (!mEvents.AppendElement(aEvent))
    return;

  
  CoalesceEvents();

  
  
  AccMutationEvent* showOrHideEvent = downcast_accEvent(aEvent);
  if (showOrHideEvent && !showOrHideEvent->mTextChangeEvent)
    CreateTextChangeEventFor(showOrHideEvent);

  ScheduleProcessing();
}

void
NotificationController::ScheduleChildDocBinding(nsDocAccessible* aDocument)
{
  
  mHangingChildDocuments.AppendElement(aDocument);
  ScheduleProcessing();
}

void
NotificationController::ScheduleContentInsertion(nsAccessible* aContainer,
                                                 nsIContent* aStartChildNode,
                                                 nsIContent* aEndChildNode)
{
  nsRefPtr<ContentInsertion> insertion = new ContentInsertion(mDocument,
                                                              aContainer);
  if (insertion && insertion->InitChildList(aStartChildNode, aEndChildNode) &&
      mContentInsertions.AppendElement(insertion)) {
    ScheduleProcessing();
  }
}




void
NotificationController::ScheduleProcessing()
{
  
  
  if (mObservingState == eNotObservingRefresh) {
    if (mPresShell->AddRefreshObserver(this, Flush_Display))
      mObservingState = eRefreshObserving;
  }
}

bool
NotificationController::IsUpdatePending()
{
  return mPresShell->IsLayoutFlushObserver() ||
    mObservingState == eRefreshProcessingForUpdate ||
    mContentInsertions.Length() != 0 || mNotifications.Length() != 0 ||
    mTextHash.Count() != 0 ||
    !mDocument->HasLoadState(nsDocAccessible::eTreeConstructed);
}




void
NotificationController::WillRefresh(mozilla::TimeStamp aTime)
{
  
  
  NS_ASSERTION(mDocument,
               "The document was shut down while refresh observer is attached!");
  if (!mDocument)
    return;

  
  
  mObservingState = eRefreshProcessingForUpdate;

  
  if (!mDocument->HasLoadState(nsDocAccessible::eTreeConstructed)) {
    
    
    if (!mDocument->IsBoundToParent()) {
      mObservingState = eRefreshObserving;
      return;
    }

#ifdef DEBUG_NOTIFICATIONS
    printf("\ninitial tree created, document: %p, document node: %p\n",
           mDocument.get(), mDocument->GetDocumentNode());
#endif

    mDocument->DoInitialUpdate();

    NS_ASSERTION(mContentInsertions.Length() == 0,
                 "Pending content insertions while initial accessible tree isn't created!");
  }

  
  
  
  
  
  
  
  

  
  nsTArray<nsRefPtr<ContentInsertion> > contentInsertions;
  contentInsertions.SwapElements(mContentInsertions);

  PRUint32 insertionCount = contentInsertions.Length();
  for (PRUint32 idx = 0; idx < insertionCount; idx++) {
    contentInsertions[idx]->Process();
    if (!mDocument)
      return;
  }

  
  mTextHash.EnumerateEntries(TextEnumerator, mDocument);
  mTextHash.Clear();

  
  PRUint32 hangingDocCnt = mHangingChildDocuments.Length();
  for (PRUint32 idx = 0; idx < hangingDocCnt; idx++) {
    nsDocAccessible* childDoc = mHangingChildDocuments[idx];

    nsIContent* ownerContent = mDocument->GetDocumentNode()->
      FindContentForSubDocument(childDoc->GetDocumentNode());
    if (ownerContent) {
      nsAccessible* outerDocAcc = mDocument->GetAccessible(ownerContent);
      if (outerDocAcc && outerDocAcc->AppendChild(childDoc)) {
        if (mDocument->AppendChildDocument(childDoc))
          continue;

        outerDocAcc->RemoveChild(childDoc);
      }

      
      childDoc->Shutdown();
    }
  }
  mHangingChildDocuments.Clear();

  
  
  if (mDocument->HasLoadState(nsDocAccessible::eReady) &&
      !mDocument->HasLoadState(nsDocAccessible::eCompletelyLoaded) &&
      hangingDocCnt == 0) {
    PRUint32 childDocCnt = mDocument->ChildDocumentCount(), childDocIdx = 0;
    for (; childDocIdx < childDocCnt; childDocIdx++) {
      nsDocAccessible* childDoc = mDocument->GetChildDocumentAt(childDocIdx);
      if (!childDoc->HasLoadState(nsDocAccessible::eCompletelyLoaded))
        break;
    }

    if (childDocIdx == childDocCnt) {
      mDocument->ProcessLoad();
      if (!mDocument)
        return;
    }
  }

  
  nsTArray < nsRefPtr<Notification> > notifications;
  notifications.SwapElements(mNotifications);

  PRUint32 notificationCount = notifications.Length();
  for (PRUint32 idx = 0; idx < notificationCount; idx++) {
    notifications[idx]->Process();
    if (!mDocument)
      return;
  }

  
  
  mDocument->ProcessInvalidationList();

  
  
  mObservingState = eRefreshObserving;

  
  nsTArray<nsRefPtr<AccEvent> > events;
  events.SwapElements(mEvents);

  PRUint32 eventCount = events.Length();
  for (PRUint32 idx = 0; idx < eventCount; idx++) {
    AccEvent* accEvent = events[idx];
    if (accEvent->mEventRule != AccEvent::eDoNotEmit) {
      nsAccessible* target = accEvent->GetAccessible();
      if (!target || target->IsDefunct())
        continue;

      
      if (accEvent->mEventType == nsIAccessibleEvent::EVENT_FOCUS) {
        FocusMgr()->ProcessFocusEvent(accEvent);
        continue;
      }

      mDocument->ProcessPendingEvent(accEvent);

      
      AccMutationEvent* showOrHideEvent = downcast_accEvent(accEvent);
      if (showOrHideEvent) {
        if (showOrHideEvent->mTextChangeEvent)
          mDocument->ProcessPendingEvent(showOrHideEvent->mTextChangeEvent);
      }
    }
    if (!mDocument)
      return;
  }

  
  
  if (mContentInsertions.Length() == 0 && mNotifications.Length() == 0 &&
      mEvents.Length() == 0 && mTextHash.Count() == 0 &&
      mHangingChildDocuments.Length() == 0 &&
      mDocument->HasLoadState(nsDocAccessible::eCompletelyLoaded) &&
      mPresShell->RemoveRefreshObserver(this, Flush_Display)) {
    mObservingState = eNotObservingRefresh;
  }
}




void
NotificationController::CoalesceEvents()
{
  PRUint32 numQueuedEvents = mEvents.Length();
  PRInt32 tail = numQueuedEvents - 1;
  AccEvent* tailEvent = mEvents[tail];

  switch(tailEvent->mEventRule) {
    case AccEvent::eCoalesceFromSameSubtree:
    {
      
      
      if (!tailEvent->mNode)
        return;

      for (PRInt32 index = tail - 1; index >= 0; index--) {
        AccEvent* thisEvent = mEvents[index];

        if (thisEvent->mEventType != tailEvent->mEventType)
          continue; 

        
        
        
        if (!thisEvent->mNode ||
            thisEvent->mNode->OwnerDoc() != tailEvent->mNode->OwnerDoc())
          continue;

        
        if (thisEvent->mNode == tailEvent->mNode) {
          thisEvent->mEventRule = AccEvent::eDoNotEmit;
          return;
        }

        
        
        

        
        if (tailEvent->mEventType == nsIAccessibleEvent::EVENT_HIDE) {
          AccHideEvent* tailHideEvent = downcast_accEvent(tailEvent);
          AccHideEvent* thisHideEvent = downcast_accEvent(thisEvent);
          if (thisHideEvent->mParent == tailHideEvent->mParent) {
            tailEvent->mEventRule = thisEvent->mEventRule;

            
            if (tailEvent->mEventRule != AccEvent::eDoNotEmit)
              CoalesceTextChangeEventsFor(tailHideEvent, thisHideEvent);

            return;
          }
        } else if (tailEvent->mEventType == nsIAccessibleEvent::EVENT_SHOW) {
          if (thisEvent->mAccessible->Parent() ==
              tailEvent->mAccessible->Parent()) {
            tailEvent->mEventRule = thisEvent->mEventRule;

            
            if (tailEvent->mEventRule != AccEvent::eDoNotEmit) {
              AccShowEvent* tailShowEvent = downcast_accEvent(tailEvent);
              AccShowEvent* thisShowEvent = downcast_accEvent(thisEvent);
              CoalesceTextChangeEventsFor(tailShowEvent, thisShowEvent);
            }

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

        
        

        
        
        
        
        
        
        if (tailEvent->mEventType != nsIAccessibleEvent::EVENT_HIDE &&
            nsCoreUtils::IsAncestorOf(thisEvent->mNode, tailEvent->mNode)) {
          tailEvent->mEventRule = AccEvent::eDoNotEmit;
          return;
        }

        
        
        
        if (nsCoreUtils::IsAncestorOf(tailEvent->mNode, thisEvent->mNode)) {
          thisEvent->mEventRule = AccEvent::eDoNotEmit;
          ApplyToSiblings(0, index, thisEvent->mEventType,
                          thisEvent->mNode, AccEvent::eDoNotEmit);
          continue;
        }

      } 

    } break; 

    case AccEvent::eCoalesceOfSameType:
    {
      
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        AccEvent* accEvent = mEvents[index];
        if (accEvent->mEventType == tailEvent->mEventType &&
            accEvent->mEventRule == tailEvent->mEventRule) {
          accEvent->mEventRule = AccEvent::eDoNotEmit;
          return;
        }
      }
    } break; 

    case AccEvent::eRemoveDupes:
    {
      
      
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        AccEvent* accEvent = mEvents[index];
        if (accEvent->mEventType == tailEvent->mEventType &&
            accEvent->mEventRule == tailEvent->mEventRule &&
            accEvent->mNode == tailEvent->mNode) {
          tailEvent->mEventRule = AccEvent::eDoNotEmit;
          return;
        }
      }
    } break; 

    case AccEvent::eCoalesceSelectionChange:
    {
      AccSelChangeEvent* tailSelChangeEvent = downcast_accEvent(tailEvent);
      PRInt32 index = tail - 1;
      for (; index >= 0; index--) {
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

    default:
      break; 
  } 
}

void
NotificationController::ApplyToSiblings(PRUint32 aStart, PRUint32 aEnd,
                                        PRUint32 aEventType, nsINode* aNode,
                                        AccEvent::EEventRule aEventRule)
{
  for (PRUint32 index = aStart; index < aEnd; index ++) {
    AccEvent* accEvent = mEvents[index];
    if (accEvent->mEventType == aEventType &&
        accEvent->mEventRule != AccEvent::eDoNotEmit && accEvent->mNode &&
        accEvent->mNode->GetNodeParent() == aNode->GetNodeParent()) {
      accEvent->mEventRule = aEventRule;
    }
  }
}

void
NotificationController::CoalesceSelChangeEvents(AccSelChangeEvent* aTailEvent,
                                                AccSelChangeEvent* aThisEvent,
                                                PRInt32 aThisIndex)
{
  aTailEvent->mPreceedingCount = aThisEvent->mPreceedingCount + 1;

  
  
  if (aTailEvent->mPreceedingCount >= kSelChangeCountToPack) {
    aTailEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION_WITHIN;
    aTailEvent->mAccessible = aTailEvent->mWidget;
    aThisEvent->mEventRule = AccEvent::eDoNotEmit;

    
    
    if (aThisEvent->mEventType != nsIAccessibleEvent::EVENT_SELECTION_WITHIN) {
      for (PRInt32 jdx = aThisIndex - 1; jdx >= 0; jdx--) {
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
          static_cast<PRUint32>(nsIAccessibleEvent::EVENT_SELECTION_ADD) :
          static_cast<PRUint32>(nsIAccessibleEvent::EVENT_SELECTION_REMOVE);

      aThisEvent->mPackedEvent->mEventRule =
        AccEvent::eCoalesceSelectionChange;

      aThisEvent->mPackedEvent = nsnull;
    }

    aThisEvent->mEventType =
      aThisEvent->mSelChangeType == AccSelChangeEvent::eSelectionAdd ?
        static_cast<PRUint32>(nsIAccessibleEvent::EVENT_SELECTION_ADD) :
        static_cast<PRUint32>(nsIAccessibleEvent::EVENT_SELECTION_REMOVE);

    return;
  }

  
  
  if (aTailEvent->mEventType == nsIAccessibleEvent::EVENT_SELECTION)
    aTailEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION_ADD;
}

void
NotificationController::CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
                                                    AccHideEvent* aThisEvent)
{
  
  

  AccTextChangeEvent* textEvent = aThisEvent->mTextChangeEvent;
  if (!textEvent)
    return;

  if (aThisEvent->mNextSibling == aTailEvent->mAccessible) {
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText);

  } else if (aThisEvent->mPrevSibling == aTailEvent->mAccessible) {
    PRUint32 oldLen = textEvent->GetLength();
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText);
    textEvent->mStart -= textEvent->GetLength() - oldLen;
  }

  aTailEvent->mTextChangeEvent.swap(aThisEvent->mTextChangeEvent);
}

void
NotificationController::CoalesceTextChangeEventsFor(AccShowEvent* aTailEvent,
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
NotificationController::CreateTextChangeEventFor(AccMutationEvent* aEvent)
{
  nsAccessible* container =
    GetAccService()->GetContainerAccessible(aEvent->mNode,
                                            aEvent->mAccessible->GetWeakShell());
  if (!container)
    return;

  nsHyperTextAccessible* textAccessible = container->AsHyperText();
  if (!textAccessible)
    return;

  
  if (aEvent->mAccessible->Role() == nsIAccessibleRole::ROLE_WHITESPACE) {
    nsCOMPtr<nsIEditor> editor;
    textAccessible->GetAssociatedEditor(getter_AddRefs(editor));
    if (editor) {
      bool isEmpty = false;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty)
        return;
    }
  }

  PRInt32 offset = textAccessible->GetChildOffset(aEvent->mAccessible);

  nsAutoString text;
  aEvent->mAccessible->AppendTextTo(text);
  if (text.IsEmpty())
    return;

  aEvent->mTextChangeEvent =
    new AccTextChangeEvent(textAccessible, offset, text, aEvent->IsShow(),
                           aEvent->mIsFromUserInput ? eFromUserInput : eNoUserInput);
}




PLDHashOperator
NotificationController::TextEnumerator(nsCOMPtrHashKey<nsIContent>* aEntry,
                                       void* aUserArg)
{
  nsDocAccessible* document = static_cast<nsDocAccessible*>(aUserArg);
  nsIContent* textNode = aEntry->GetKey();
  nsAccessible* textAcc = document->GetAccessible(textNode);

  
  
  nsINode* containerNode = textNode->GetNodeParent();
  if (!containerNode) {
    NS_ASSERTION(!textAcc,
                 "Text node was removed but accessible is kept alive!");
    return PL_DHASH_NEXT;
  }

  nsIFrame* textFrame = textNode->GetPrimaryFrame();
  if (!textFrame) {
    NS_ASSERTION(!textAcc,
                 "Text node isn't rendered but accessible is kept alive!");
    return PL_DHASH_NEXT;
  }

  nsIContent* containerElm = containerNode->IsElement() ?
    containerNode->AsElement() : nsnull;

  nsAutoString text;
  textFrame->GetRenderedText(&text);

  
  if (textAcc) {
    if (text.IsEmpty()) {
#ifdef DEBUG_NOTIFICATIONS
      PRUint32 index = containerNode->IndexOf(textNode);

      nsCAutoString tag;
      nsCAutoString id;
      if (containerElm) {
        containerElm->Tag()->ToUTF8String(tag);
        nsIAtom* atomid = containerElm->GetID();
        if (atomid)
          atomid->ToUTF8String(id);
      }

      printf("\npending text node removal: container: %s@id='%s', index in container: %d\n\n",
             tag.get(), id.get(), index);
#endif

      document->ContentRemoved(containerElm, textNode);
      return PL_DHASH_NEXT;
    }

    
#ifdef DEBUG_TEXTCHANGE
      PRUint32 index = containerNode->IndexOf(textNode);

      nsCAutoString tag;
      nsCAutoString id;
      if (containerElm) {
        containerElm->Tag()->ToUTF8String(tag);
        nsIAtom* atomid = containerElm->GetID();
        if (atomid)
          atomid->ToUTF8String(id);
      }

      printf("\ntext may be changed: container: %s@id='%s', index in container: %d, old text '%s', new text: '%s'\n\n",
             tag.get(), id.get(), index,
             NS_ConvertUTF16toUTF8(textAcc->AsTextLeaf()->Text()).get(),
             NS_ConvertUTF16toUTF8(text).get());
#endif

    TextUpdater::Run(document, textAcc->AsTextLeaf(), text);
    return PL_DHASH_NEXT;
  }

  
  if (!text.IsEmpty()) {
#ifdef DEBUG_NOTIFICATIONS
      PRUint32 index = containerNode->IndexOf(textNode);

      nsCAutoString tag;
      nsCAutoString id;
      if (containerElm) {
        containerElm->Tag()->ToUTF8String(tag);
        nsIAtom* atomid = containerElm->GetID();
        if (atomid)
          atomid->ToUTF8String(id);
      }

      printf("\npending text node insertion: container: %s@id='%s', index in container: %d\n\n",
             tag.get(), id.get(), index);
#endif

    
    nsAccessible* container = document->GetAccessibleOrContainer(containerNode);
    NS_ASSERTION(container,
                 "Text node having rendered text hasn't accessible document!");
    if (container) {
      nsTArray<nsCOMPtr<nsIContent> > insertedContents;
      insertedContents.AppendElement(textNode);
      document->ProcessContentInserted(container, &insertedContents);
    }
  }

  return PL_DHASH_NEXT;
}





NotificationController::ContentInsertion::
  ContentInsertion(nsDocAccessible* aDocument, nsAccessible* aContainer) :
  mDocument(aDocument), mContainer(aContainer)
{
}

bool
NotificationController::ContentInsertion::
  InitChildList(nsIContent* aStartChildNode, nsIContent* aEndChildNode)
{
  bool haveToUpdate = false;

  nsIContent* node = aStartChildNode;
  while (node != aEndChildNode) {
    
    
    
    if (node->GetPrimaryFrame()) {
      if (mInsertedContent.AppendElement(node))
        haveToUpdate = true;
    }

    node = node->GetNextSibling();
  }

  return haveToUpdate;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(NotificationController::ContentInsertion)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(NotificationController::ContentInsertion)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContainer)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(NotificationController::ContentInsertion)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mContainer");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mContainer.get()));
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(NotificationController::ContentInsertion,
                                     AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(NotificationController::ContentInsertion,
                                       Release)

void
NotificationController::ContentInsertion::Process()
{
#ifdef DEBUG_NOTIFICATIONS
  nsIContent* firstChildNode = mInsertedContent[0];

  nsCAutoString tag;
  firstChildNode->Tag()->ToUTF8String(tag);

  nsIAtom* atomid = firstChildNode->GetID();
  nsCAutoString id;
  if (atomid)
    atomid->ToUTF8String(id);

  nsCAutoString ctag;
  nsCAutoString cid;
  nsIAtom* catomid = nsnull;
  if (mContainer->IsContent()) {
    mContainer->GetContent()->Tag()->ToUTF8String(ctag);
    catomid = mContainer->GetContent()->GetID();
    if (catomid)
      catomid->ToUTF8String(cid);
  }

  printf("\npending content insertion: %s@id='%s', container: %s@id='%s', inserted content amount: %d\n\n",
         tag.get(), id.get(), ctag.get(), cid.get(), mInsertedContent.Length());
#endif

  mDocument->ProcessContentInserted(mContainer, &mInsertedContent);

  mDocument = nsnull;
  mContainer = nsnull;
  mInsertedContent.Clear();
}

