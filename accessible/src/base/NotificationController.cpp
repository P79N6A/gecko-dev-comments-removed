




#include "NotificationController.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "DocAccessible.h"
#include "nsEventShell.h"
#include "FocusManager.h"
#include "Role.h"
#include "TextLeafAccessible.h"
#include "TextUpdater.h"

#ifdef DEBUG
#include "Logging.h"
#endif

#include "mozilla/dom/Element.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;
using namespace mozilla::a11y;



const unsigned int kSelChangeCountToPack = 5;





NotificationController::NotificationController(DocAccessible* aDocument,
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

NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(NotificationController)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(NotificationController)
  if (tmp->mDocument)
    tmp->Shutdown();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(NotificationController)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mDocument");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mDocument.get()));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_MEMBER(mHangingChildDocuments,
                                                    DocAccessible)
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

  
  int32_t childDocCount = mHangingChildDocuments.Length();
  for (int32_t idx = childDocCount - 1; idx >= 0; idx--) {
    if (!mHangingChildDocuments[idx]->IsDefunct())
      mHangingChildDocuments[idx]->Shutdown();
  }

  mHangingChildDocuments.Clear();

  mDocument = nullptr;
  mPresShell = nullptr;

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
NotificationController::ScheduleChildDocBinding(DocAccessible* aDocument)
{
  
  mHangingChildDocuments.AppendElement(aDocument);
  ScheduleProcessing();
}

void
NotificationController::ScheduleContentInsertion(Accessible* aContainer,
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
    !mDocument->HasLoadState(DocAccessible::eTreeConstructed);
}




void
NotificationController::WillRefresh(mozilla::TimeStamp aTime)
{
  Telemetry::AutoTimer<Telemetry::A11Y_UPDATE_TIME> updateTimer;

  
  
  NS_ASSERTION(mDocument,
               "The document was shut down while refresh observer is attached!");
  if (!mDocument)
    return;

  
  
  mObservingState = eRefreshProcessingForUpdate;

  
  if (!mDocument->HasLoadState(DocAccessible::eTreeConstructed)) {
    
    
    if (!mDocument->IsBoundToParent()) {
      mObservingState = eRefreshObserving;
      return;
    }

#ifdef DEBUG
    if (logging::IsEnabled(logging::eTree)) {
      logging::MsgBegin("TREE", "initial tree created");
      logging::Address("document", mDocument);
      logging::MsgEnd();
    }
#endif

    mDocument->DoInitialUpdate();

    NS_ASSERTION(mContentInsertions.Length() == 0,
                 "Pending content insertions while initial accessible tree isn't created!");
  }

  
  
  
  
  
  
  
  

  
  nsTArray<nsRefPtr<ContentInsertion> > contentInsertions;
  contentInsertions.SwapElements(mContentInsertions);

  uint32_t insertionCount = contentInsertions.Length();
  for (uint32_t idx = 0; idx < insertionCount; idx++) {
    contentInsertions[idx]->Process();
    if (!mDocument)
      return;
  }

  
  mTextHash.EnumerateEntries(TextEnumerator, mDocument);
  mTextHash.Clear();

  
  uint32_t hangingDocCnt = mHangingChildDocuments.Length();
  for (uint32_t idx = 0; idx < hangingDocCnt; idx++) {
    DocAccessible* childDoc = mHangingChildDocuments[idx];
    if (childDoc->IsDefunct())
      continue;

    nsIContent* ownerContent = mDocument->GetDocumentNode()->
      FindContentForSubDocument(childDoc->GetDocumentNode());
    if (ownerContent) {
      Accessible* outerDocAcc = mDocument->GetAccessible(ownerContent);
      if (outerDocAcc && outerDocAcc->AppendChild(childDoc)) {
        if (mDocument->AppendChildDocument(childDoc))
          continue;

        outerDocAcc->RemoveChild(childDoc);
      }

      
      childDoc->Shutdown();
    }
  }
  mHangingChildDocuments.Clear();

  
  
  if (mDocument->HasLoadState(DocAccessible::eReady) &&
      !mDocument->HasLoadState(DocAccessible::eCompletelyLoaded) &&
      hangingDocCnt == 0) {
    uint32_t childDocCnt = mDocument->ChildDocumentCount(), childDocIdx = 0;
    for (; childDocIdx < childDocCnt; childDocIdx++) {
      DocAccessible* childDoc = mDocument->GetChildDocumentAt(childDocIdx);
      if (!childDoc->HasLoadState(DocAccessible::eCompletelyLoaded))
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

  uint32_t notificationCount = notifications.Length();
  for (uint32_t idx = 0; idx < notificationCount; idx++) {
    notifications[idx]->Process();
    if (!mDocument)
      return;
  }

  
  
  mDocument->ProcessInvalidationList();

  
  
  mObservingState = eRefreshObserving;

  
  nsTArray<nsRefPtr<AccEvent> > events;
  events.SwapElements(mEvents);

  uint32_t eventCount = events.Length();
#ifdef DEBUG
  if (eventCount > 0 && logging::IsEnabled(logging::eEvents)) {
    logging::MsgBegin("EVENTS", "events processing");
    logging::Address("document", mDocument);
    logging::MsgEnd();
  }
#endif

  for (uint32_t idx = 0; idx < eventCount; idx++) {
    AccEvent* accEvent = events[idx];
    if (accEvent->mEventRule != AccEvent::eDoNotEmit) {
      Accessible* target = accEvent->GetAccessible();
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
      mDocument->HasLoadState(DocAccessible::eCompletelyLoaded) &&
      mPresShell->RemoveRefreshObserver(this, Flush_Display)) {
    mObservingState = eNotObservingRefresh;
  }
}




void
NotificationController::CoalesceEvents()
{
  uint32_t numQueuedEvents = mEvents.Length();
  int32_t tail = numQueuedEvents - 1;
  AccEvent* tailEvent = mEvents[tail];

  switch(tailEvent->mEventRule) {
    case AccEvent::eCoalesceFromSameSubtree:
    {
      
      
      if (!tailEvent->mNode)
        return;

      for (int32_t index = tail - 1; index >= 0; index--) {
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
      
      for (int32_t index = tail - 1; index >= 0; index--) {
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
      
      
      for (int32_t index = tail - 1; index >= 0; index--) {
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
      int32_t index = tail - 1;
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
NotificationController::ApplyToSiblings(uint32_t aStart, uint32_t aEnd,
                                        uint32_t aEventType, nsINode* aNode,
                                        AccEvent::EEventRule aEventRule)
{
  for (uint32_t index = aStart; index < aEnd; index ++) {
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
                                                int32_t aThisIndex)
{
  aTailEvent->mPreceedingCount = aThisEvent->mPreceedingCount + 1;

  
  
  if (aTailEvent->mPreceedingCount >= kSelChangeCountToPack) {
    aTailEvent->mEventType = nsIAccessibleEvent::EVENT_SELECTION_WITHIN;
    aTailEvent->mAccessible = aTailEvent->mWidget;
    aThisEvent->mEventRule = AccEvent::eDoNotEmit;

    
    
    if (aThisEvent->mEventType != nsIAccessibleEvent::EVENT_SELECTION_WITHIN) {
      for (int32_t jdx = aThisIndex - 1; jdx >= 0; jdx--) {
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
NotificationController::CoalesceTextChangeEventsFor(AccHideEvent* aTailEvent,
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
  DocAccessible* document = aEvent->GetDocAccessible();
  Accessible* container = document->GetContainerAccessible(aEvent->mNode);
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




PLDHashOperator
NotificationController::TextEnumerator(nsCOMPtrHashKey<nsIContent>* aEntry,
                                       void* aUserArg)
{
  DocAccessible* document = static_cast<DocAccessible*>(aUserArg);
  nsIContent* textNode = aEntry->GetKey();
  Accessible* textAcc = document->GetAccessible(textNode);

  
  
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
    containerNode->AsElement() : nullptr;

  nsAutoString text;
  textFrame->GetRenderedText(&text);

  
  if (textAcc) {
    if (text.IsEmpty()) {
#ifdef DEBUG
      if (logging::IsEnabled(logging::eTree | logging::eText)) {
        logging::MsgBegin("TREE", "text node lost its content");
        logging::Node("container", containerElm);
        logging::Node("content", textNode);
        logging::MsgEnd();
      }
#endif

      document->ContentRemoved(containerElm, textNode);
      return PL_DHASH_NEXT;
    }

    
#ifdef DEBUG
    if (logging::IsEnabled(logging::eText)) {
      logging::MsgBegin("TEXT", "text may be changed");
      logging::Node("container", containerElm);
      logging::Node("content", textNode);
      logging::MsgEntry("old text '%s'",
                        NS_ConvertUTF16toUTF8(textAcc->AsTextLeaf()->Text()).get());
      logging::MsgEntry("new text: '%s'",
                        NS_ConvertUTF16toUTF8(text).get());
      logging::MsgEnd();
    }
#endif

    TextUpdater::Run(document, textAcc->AsTextLeaf(), text);
    return PL_DHASH_NEXT;
  }

  
  if (!text.IsEmpty()) {
#ifdef DEBUG
    if (logging::IsEnabled(logging::eTree | logging::eText)) {
      logging::MsgBegin("TREE", "text node gains new content");
      logging::Node("container", containerElm);
      logging::Node("content", textNode);
      logging::MsgEnd();
    }
#endif

    
    Accessible* container = document->GetAccessibleOrContainer(containerNode);
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
  ContentInsertion(DocAccessible* aDocument, Accessible* aContainer) :
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

NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(NotificationController::ContentInsertion)

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
  mDocument->ProcessContentInserted(mContainer, &mInsertedContent);

  mDocument = nullptr;
  mContainer = nullptr;
  mInsertedContent.Clear();
}

