




#include "NotificationController.h"

#include "Accessible-inl.h"
#include "DocAccessible-inl.h"
#include "TextLeafAccessible.h"
#include "TextUpdater.h"

#include "mozilla/dom/Element.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;
using namespace mozilla::a11y;



const unsigned int kSelChangeCountToPack = 5;





NotificationController::NotificationController(DocAccessible* aDocument,
                                               nsIPresShell* aPresShell) :
  EventQueue(aDocument), mObservingState(eNotObservingRefresh),
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




NS_IMPL_CYCLE_COLLECTING_NATIVE_ADDREF(NotificationController)
NS_IMPL_CYCLE_COLLECTING_NATIVE_RELEASE(NotificationController)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(NotificationController)
  if (tmp->mDocument)
    tmp->Shutdown();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(NotificationController)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mHangingChildDocuments)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContentInsertions)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEvents)
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

#ifdef A11Y_LOG
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

  
  if (!(mDocument->mDocFlags & DocAccessible::eScrollInitialized))
    mDocument->AddScrollListener();

  
  
  
  
  
  
  
  

  
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

    nsIContent* ownerContent = mDocument->DocumentNode()->
      FindContentForSubDocument(childDoc->DocumentNode());
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

  ProcessEventQueue();
  if (!mDocument)
    return;

  
  
  if (mContentInsertions.Length() == 0 && mNotifications.Length() == 0 &&
      mEvents.Length() == 0 && mTextHash.Count() == 0 &&
      mHangingChildDocuments.Length() == 0 &&
      mDocument->HasLoadState(DocAccessible::eCompletelyLoaded) &&
      mPresShell->RemoveRefreshObserver(this, Flush_Display)) {
    mObservingState = eNotObservingRefresh;
  }
}




PLDHashOperator
NotificationController::TextEnumerator(nsCOMPtrHashKey<nsIContent>* aEntry,
                                       void* aUserArg)
{
  DocAccessible* document = static_cast<DocAccessible*>(aUserArg);
  nsIContent* textNode = aEntry->GetKey();
  Accessible* textAcc = document->GetAccessible(textNode);

  
  
  nsINode* containerNode = textNode->GetParentNode();
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
#ifdef A11Y_LOG
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

    
#ifdef A11Y_LOG
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
#ifdef A11Y_LOG
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

NS_IMPL_CYCLE_COLLECTION_1(NotificationController::ContentInsertion,
                           mContainer)

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

