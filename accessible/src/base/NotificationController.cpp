





































#include "NotificationController.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "nsEventShell.h"
#include "nsTextAccessible.h"






NotificationController::NotificationController(nsDocAccessible* aDocument,
                                               nsIPresShell* aPresShell) :
  mObservingState(eNotObservingRefresh), mDocument(aDocument),
  mPresShell(aPresShell), mTreeConstructedState(eTreeConstructionPending)
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
  
  if (mTreeConstructedState == eTreeConstructionPending)
    return;

  nsRefPtr<ContentInsertion> insertion =
    new ContentInsertion(mDocument, aContainer, aStartChildNode, aEndChildNode);

  if (insertion && mContentInsertions.AppendElement(insertion))
    ScheduleProcessing();
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
  nsCOMPtr<nsIPresShell_MOZILLA_2_0_BRANCH2> presShell =
    do_QueryInterface(mPresShell);
  return presShell->IsLayoutFlushObserver() ||
    mObservingState == eRefreshProcessingForUpdate ||
    mContentInsertions.Length() != 0 || mNotifications.Length() != 0 ||
    mTextHash.Count() != 0;
}




void
NotificationController::WillRefresh(mozilla::TimeStamp aTime)
{
  
  
  NS_ASSERTION(mDocument,
               "The document was shut down while refresh observer is attached!");
  if (!mDocument)
    return;

  
  
  mObservingState = eRefreshProcessingForUpdate;

  
  if (mTreeConstructedState == eTreeConstructionPending) {
    
    
    if (!mDocument->IsBoundToParent())
      return;

#ifdef DEBUG_NOTIFICATIONS
    printf("\ninitial tree created, document: %p, document node: %p\n",
           mDocument.get(), mDocument->GetDocumentNode());
#endif

    mTreeConstructedState = eTreeConstructed;
    mDocument->CacheChildrenInSubtree(mDocument);

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

  
  PRUint32 childDocCount = mHangingChildDocuments.Length();
  for (PRUint32 idx = 0; idx < childDocCount; idx++) {
    nsDocAccessible* childDoc = mHangingChildDocuments[idx];

    nsIContent* ownerContent = mDocument->GetDocumentNode()->
      FindContentForSubDocument(childDoc->GetDocumentNode());
    if (ownerContent) {
      nsAccessible* outerDocAcc = mDocument->GetAccessible(ownerContent);
      if (outerDocAcc && outerDocAcc->AppendChild(childDoc)) {
        if (mDocument->AppendChildDocument(childDoc)) {
          
          
          nsRefPtr<AccEvent> reorderEvent =
              new AccEvent(nsIAccessibleEvent::EVENT_REORDER, outerDocAcc,
                           eAutoDetect, AccEvent::eCoalesceFromSameSubtree);
          if (reorderEvent)
            QueueEvent(reorderEvent);

          continue;
        }
        outerDocAcc->RemoveChild(childDoc);
      }

      
      childDoc->Shutdown();
    }
  }
  mHangingChildDocuments.Clear();

  
  nsTArray < nsRefPtr<Notification> > notifications;
  notifications.SwapElements(mNotifications);

  PRUint32 notificationCount = notifications.Length();
  for (PRUint32 idx = 0; idx < notificationCount; idx++) {
    notifications[idx]->Process();
    if (!mDocument)
      return;
  }

  
  
  mObservingState = eRefreshObserving;

  
  nsTArray<nsRefPtr<AccEvent> > events;
  events.SwapElements(mEvents);

  PRUint32 eventCount = events.Length();
  for (PRUint32 idx = 0; idx < eventCount; idx++) {
    AccEvent* accEvent = events[idx];
    if (accEvent->mEventRule != AccEvent::eDoNotEmit) {
      mDocument->ProcessPendingEvent(accEvent);

      AccMutationEvent* showOrhideEvent = downcast_accEvent(accEvent);
      if (showOrhideEvent) {
        if (showOrhideEvent->mTextChangeEvent)
          mDocument->ProcessPendingEvent(showOrhideEvent->mTextChangeEvent);
      }
    }
    if (!mDocument)
      return;
  }

  
  
  if (mContentInsertions.Length() == 0 && mNotifications.Length() == 0 &&
      mEvents.Length() == 0 &&
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

  
  
  if (!tailEvent->mNode)
    return;

  switch(tailEvent->mEventRule) {
    case AccEvent::eCoalesceFromSameSubtree:
    {
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        AccEvent* thisEvent = mEvents[index];

        if (thisEvent->mEventType != tailEvent->mEventType)
          continue; 

        
        
        
        if (!thisEvent->mNode ||
            thisEvent->mNode->GetOwnerDoc() != tailEvent->mNode->GetOwnerDoc())
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
          if (thisEvent->mAccessible->GetParent() ==
              tailEvent->mAccessible->GetParent()) {
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

    case AccEvent::eCoalesceFromSameDocument:
    {
      
      
      
      for (PRInt32 index = tail - 1; index >= 0; index--) {
        AccEvent* thisEvent = mEvents[index];
        if (thisEvent->mEventType == tailEvent->mEventType &&
            thisEvent->mEventRule == tailEvent->mEventRule &&
            thisEvent->GetDocAccessible() == tailEvent->GetDocAccessible()) {
          thisEvent->mEventRule = AccEvent::eDoNotEmit;
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

  if (aTailEvent->mAccessible->GetIndexInParent() ==
      aThisEvent->mAccessible->GetIndexInParent() + 1) {
    
    
    aTailEvent->mAccessible->AppendTextTo(textEvent->mModifiedText);

  } else if (aTailEvent->mAccessible->GetIndexInParent() ==
             aThisEvent->mAccessible->GetIndexInParent() -1) {
    
    
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
      PRBool isEmpty = PR_FALSE;
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








class TextUpdater
{
public:
  TextUpdater(nsDocAccessible* aDocument, nsTextAccessible* aTextLeaf) :
    mDocument(aDocument), mTextLeaf(aTextLeaf) { }
  ~TextUpdater() { mDocument = nsnull; mTextLeaf = nsnull; }

  



  void Run(const nsAString& aNewText);

private:
  TextUpdater();
  TextUpdater(const TextUpdater&);
  TextUpdater& operator = (const TextUpdater&);

  


  void FindDiffNFireEvents(const nsDependentSubstring& aStr1,
                           const nsDependentSubstring& aStr2,
                           PRUint32** aMatrix,
                           PRUint32 aStartOffset);

  


  enum ChangeType {
    eNoChange,
    eInsertion,
    eRemoval,
    eSubstitution
  };

  


  inline void MayFireEvent(nsAString* aInsertedText, nsAString* aRemovedText,
                           PRUint32 aOffset, ChangeType* aChange)
  {
    if (*aChange == eNoChange)
      return;

    if (*aChange == eRemoval || *aChange == eSubstitution) {
      FireEvent(*aRemovedText, aOffset, PR_FALSE);
      aRemovedText->Truncate();
    }

    if (*aChange == eInsertion || *aChange == eSubstitution) {
      FireEvent(*aInsertedText, aOffset, PR_TRUE);
      aInsertedText->Truncate();
    }

    *aChange = eNoChange;
  }

  


  void FireEvent(const nsAString& aModText, PRUint32 aOffset, PRBool aType);

private:
  nsDocAccessible* mDocument;
  nsTextAccessible* mTextLeaf;
};

void
TextUpdater::Run(const nsAString& aNewText)
{
  NS_ASSERTION(mTextLeaf, "No text leaf accessible?");

  const nsString& oldText = mTextLeaf->Text();
  PRUint32 oldLen = oldText.Length(), newLen = aNewText.Length();
  PRUint32 minLen = oldLen < newLen ? oldLen : newLen;

  
  PRUint32 skipIdx = 0;
  for (; skipIdx < minLen; skipIdx++) {
    if (aNewText[skipIdx] != oldText[skipIdx])
      break;
  }

  
  if (skipIdx == minLen) {
    if (oldLen == newLen)
      return;

    
    if (oldLen < newLen) {
      FireEvent(Substring(aNewText, oldLen), oldLen, PR_TRUE);
      mTextLeaf->SetText(aNewText);
      return;
    }

    
    FireEvent(Substring(oldText, newLen), newLen, PR_FALSE);
    mTextLeaf->SetText(aNewText);
    return;
  }

  
  PRUint32 endIdx = minLen;
  if (oldLen < newLen) {
    PRUint32 delta = newLen - oldLen;
    for (; endIdx > skipIdx; endIdx--) {
      if (aNewText[endIdx + delta] != oldText[endIdx])
        break;
    }
  } else {
    PRUint32 delta = oldLen - newLen;
    for (; endIdx > skipIdx; endIdx--) {
      if (aNewText[endIdx] != oldText[endIdx + delta])
        break;
    }
  }
  PRUint32 oldEndIdx = oldLen - minLen + endIdx;
  PRUint32 newEndIdx = newLen - minLen + endIdx;

  
  
  

  const nsDependentSubstring& str1 =
    Substring(oldText, skipIdx, oldEndIdx - skipIdx);
  const nsDependentSubstring& str2 =
    Substring(aNewText, skipIdx, newEndIdx - skipIdx);

  
  PRUint32 len1 = str1.Length() + 1, len2 = str2.Length() + 1;

  PRUint32** matrix = new PRUint32*[len1];
  for (PRUint32 i = 0; i < len1; i++)
    matrix[i] = new PRUint32[len2];

  matrix[0][0] = 0;

  for (PRUint32 i = 1; i < len1; i++)
    matrix[i][0] = i;

  for (PRUint32 j = 1; j < len2; j++)
    matrix[0][j] = j;

  for (PRUint32 i = 1; i < len1; i++) {
    for (PRUint32 j = 1; j < len2; j++) {
      if (str1[i - 1] != str2[j - 1]) {
        PRUint32 left = matrix[i - 1][j];
        PRUint32 up = matrix[i][j - 1];

        PRUint32 upleft = matrix[i - 1][j - 1];
        matrix[i][j] =
            (left < up ? (upleft < left ? upleft : left) :
                (upleft < up ? upleft : up)) + 1;
      } else {
        matrix[i][j] = matrix[i - 1][j - 1];
      }
    }
  }

  FindDiffNFireEvents(str1, str2, matrix, skipIdx);

  for (PRUint32 i = 0; i < len1; i++)
    delete[] matrix[i];
  delete[] matrix;

  mTextLeaf->SetText(aNewText);
}

void
TextUpdater::FindDiffNFireEvents(const nsDependentSubstring& aStr1,
                                 const nsDependentSubstring& aStr2,
                                 PRUint32** aMatrix,
                                 PRUint32 aStartOffset)
{
  
  ChangeType change = eNoChange;
  nsAutoString insertedText;
  nsAutoString removedText;
  PRUint32 offset = 0;

  PRInt32 i = aStr1.Length(), j = aStr2.Length();
  while (i >= 0 && j >= 0) {
    if (aMatrix[i][j] == 0) {
      MayFireEvent(&insertedText, &removedText, offset + aStartOffset, &change);
      return;
    }

    
    if (i >= 1 && j >= 1) {
      
      if (aStr1[i - 1] == aStr2[j - 1]) {
        MayFireEvent(&insertedText, &removedText, offset + aStartOffset, &change);

        i--; j--;
        continue;
      }

      
      if (aMatrix[i][j] == aMatrix[i - 1][j - 1] + 1) {
        if (change != eSubstitution)
          MayFireEvent(&insertedText, &removedText, offset + aStartOffset, &change);

        offset = j - 1;
        insertedText.Append(aStr1[i - 1]);
        removedText.Append(aStr2[offset]);
        change = eSubstitution;

        i--; j--;
        continue;
      }
    }

    
    if (j >= 1 && aMatrix[i][j] == aMatrix[i][j - 1] + 1) {
      if (change != eInsertion)
        MayFireEvent(&insertedText, &removedText, offset + aStartOffset, &change);

      offset = j - 1;
      insertedText.Insert(aStr2[offset], 0);
      change = eInsertion;

      j--;
      continue;
    }

    
    if (i >= 1 && aMatrix[i][j] == aMatrix[i - 1][j] + 1) {
      if (change != eRemoval) {
        MayFireEvent(&insertedText, &removedText, offset + aStartOffset, &change);

        offset = j;
      }

      removedText.Insert(aStr1[i - 1], 0);
      change = eRemoval;

      i--;
      continue;
    }

    NS_NOTREACHED("Huh?");
    return;
  }

  MayFireEvent(&insertedText, &removedText, offset + aStartOffset, &change);
}

void
TextUpdater::FireEvent(const nsAString& aModText, PRUint32 aOffset,
                       PRBool aIsInserted)
{
  nsAccessible* parent = mTextLeaf->GetParent();
  NS_ASSERTION(parent, "No parent for text leaf!");

  nsHyperTextAccessible* hyperText = parent->AsHyperText();
  NS_ASSERTION(hyperText, "Text leaf parnet is not hyper text!");

  PRInt32 textLeafOffset = hyperText->GetChildOffset(mTextLeaf, PR_TRUE);
  NS_ASSERTION(textLeafOffset != -1,
               "Text leaf hasn't offset within hyper text!");

  
  nsRefPtr<AccEvent> textChangeEvent =
    new AccTextChangeEvent(hyperText, textLeafOffset + aOffset, aModText,
                           aIsInserted);
  mDocument->FireDelayedAccessibleEvent(textChangeEvent);

  
  if (hyperText->Role() == nsIAccessibleRole::ROLE_ENTRY) {
    nsRefPtr<AccEvent> valueChangeEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, hyperText,
                   eAutoDetect, AccEvent::eRemoveDupes);
    mDocument->FireDelayedAccessibleEvent(valueChangeEvent);
  }
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

    TextUpdater updater(document, textAcc->AsTextLeaf());
    updater.Run(text);

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
    nsTArray<nsCOMPtr<nsIContent> > insertedContents;
    insertedContents.AppendElement(textNode);
    document->ProcessContentInserted(container, &insertedContents);
  }

  return PL_DHASH_NEXT;
}





NotificationController::ContentInsertion::
  ContentInsertion(nsDocAccessible* aDocument, nsAccessible* aContainer,
                   nsIContent* aStartChildNode, nsIContent* aEndChildNode) :
  mDocument(aDocument), mContainer(aContainer)
{
  nsIContent* node = aStartChildNode;
  while (node != aEndChildNode) {
    mInsertedContent.AppendElement(node);
    node = node->GetNextSibling();
  }
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

