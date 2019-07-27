





#include "ContentEventHandler.h"
#include "IMEContentObserver.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/TextComposition.h"
#include "mozilla/dom/Element.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMRange.h"
#include "nsIFrame.h"
#include "nsINode.h"
#include "nsIPresShell.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsISupports.h"
#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"

namespace mozilla {

using namespace widget;

NS_IMPL_CYCLE_COLLECTION(IMEContentObserver,
                         mWidget, mSelection,
                         mRootContent, mEditableNode, mDocShell)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(IMEContentObserver)
 NS_INTERFACE_MAP_ENTRY(nsISelectionListener)
 NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
 NS_INTERFACE_MAP_ENTRY(nsIReflowObserver)
 NS_INTERFACE_MAP_ENTRY(nsIScrollObserver)
 NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
 NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISelectionListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(IMEContentObserver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(IMEContentObserver)

IMEContentObserver::IMEContentObserver()
  : mESM(nullptr)
{
#ifdef DEBUG
  TestMergingTextChangeData();
#endif
}

void
IMEContentObserver::Init(nsIWidget* aWidget,
                         nsPresContext* aPresContext,
                         nsIContent* aContent)
{
  mESM = aPresContext->EventStateManager();
  mESM->OnStartToObserveContent(this);

  mWidget = aWidget;
  mEditableNode = IMEStateManager::GetRootEditableNode(aPresContext, aContent);
  if (!mEditableNode) {
    return;
  }

  nsIPresShell* presShell = aPresContext->PresShell();

  
  nsCOMPtr<nsISelectionController> selCon;
  if (mEditableNode->IsNodeOfType(nsINode::eCONTENT)) {
    nsIFrame* frame =
      static_cast<nsIContent*>(mEditableNode.get())->GetPrimaryFrame();
    NS_ENSURE_TRUE_VOID(frame);

    frame->GetSelectionController(aPresContext,
                                  getter_AddRefs(selCon));
  } else {
    
    selCon = do_QueryInterface(presShell);
  }
  NS_ENSURE_TRUE_VOID(selCon);

  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                       getter_AddRefs(mSelection));
  NS_ENSURE_TRUE_VOID(mSelection);

  nsCOMPtr<nsIDOMRange> selDomRange;
  if (NS_SUCCEEDED(mSelection->GetRangeAt(0, getter_AddRefs(selDomRange)))) {
    nsRange* selRange = static_cast<nsRange*>(selDomRange.get());
    NS_ENSURE_TRUE_VOID(selRange && selRange->GetStartParent());

    mRootContent = selRange->GetStartParent()->
                     GetSelectionRootContent(presShell);
  } else {
    mRootContent = mEditableNode->GetSelectionRootContent(presShell);
  }
  if (!mRootContent && mEditableNode->IsNodeOfType(nsINode::eDOCUMENT)) {
    
    
    return;
  }
  NS_ENSURE_TRUE_VOID(mRootContent);

  if (IMEStateManager::IsTestingIME()) {
    nsIDocument* doc = aPresContext->Document();
    (new AsyncEventDispatcher(doc, NS_LITERAL_STRING("MozIMEFocusIn"),
                              false, false))->RunDOMEventWhenSafe();
  }

  aWidget->NotifyIME(IMENotification(NOTIFY_IME_OF_FOCUS));

  
  
  
  if (!mRootContent) {
    return;
  }

  mDocShell = aPresContext->GetDocShell();

  ObserveEditableNode();
}

void
IMEContentObserver::ObserveEditableNode()
{
  MOZ_ASSERT(mSelection);
  MOZ_ASSERT(mRootContent);

  mUpdatePreference = mWidget->GetIMEUpdatePreference();
  if (mUpdatePreference.WantSelectionChange()) {
    
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSelection));
    NS_ENSURE_TRUE_VOID(selPrivate);
    nsresult rv = selPrivate->AddSelectionListener(this);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

  if (mUpdatePreference.WantTextChange()) {
    
    mRootContent->AddMutationObserver(this);
  }

  if (mUpdatePreference.WantPositionChanged() && mDocShell) {
    
    
    mDocShell->AddWeakScrollObserver(this);
    mDocShell->AddWeakReflowObserver(this);
  }
}

void
IMEContentObserver::Destroy()
{
  
  
  if (mRootContent) {
    if (IMEStateManager::IsTestingIME() && mEditableNode) {
      nsIDocument* doc = mEditableNode->OwnerDoc();
      (new AsyncEventDispatcher(doc, NS_LITERAL_STRING("MozIMEFocusOut"),
                                false, false))->RunDOMEventWhenSafe();
    }
    mWidget->NotifyIME(IMENotification(NOTIFY_IME_OF_BLUR));
  }
  
  mWidget = nullptr;
  if (mUpdatePreference.WantSelectionChange() && mSelection) {
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSelection));
    if (selPrivate) {
      selPrivate->RemoveSelectionListener(this);
    }
  }
  mSelection = nullptr;
  if (mUpdatePreference.WantTextChange() && mRootContent) {
    mRootContent->RemoveMutationObserver(this);
  }
  if (mUpdatePreference.WantPositionChanged() && mDocShell) {
    mDocShell->RemoveWeakScrollObserver(this);
    mDocShell->RemoveWeakReflowObserver(this);
  }
  mRootContent = nullptr;
  mEditableNode = nullptr;
  mDocShell = nullptr;
  mUpdatePreference.mWantUpdates = nsIMEUpdatePreference::NOTIFY_NOTHING;

  if (mESM) {
    mESM->OnStopObservingContent(this);
    mESM = nullptr;
  }
}

void
IMEContentObserver::DisconnectFromEventStateManager()
{
  mESM = nullptr;
}

bool
IMEContentObserver::IsManaging(nsPresContext* aPresContext,
                               nsIContent* aContent)
{
  if (!mSelection || !mRootContent || !mEditableNode) {
    return false; 
  }
  if (!mRootContent->IsInDoc()) {
    return false; 
  }
  return mEditableNode == IMEStateManager::GetRootEditableNode(aPresContext,
                                                               aContent);
}

bool
IMEContentObserver::IsEditorHandlingEventForComposition() const
{
  if (!mWidget) {
    return false;
  }
  nsRefPtr<TextComposition> composition =
    IMEStateManager::GetTextCompositionFor(mWidget);
  if (!composition) {
    return false;
  }
  return composition->IsEditorHandlingEvent();
}

nsresult
IMEContentObserver::GetSelectionAndRoot(nsISelection** aSelection,
                                        nsIContent** aRootContent) const
{
  if (!mEditableNode || !mSelection) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ASSERTION(mSelection && mRootContent, "uninitialized content observer");
  NS_ADDREF(*aSelection = mSelection);
  NS_ADDREF(*aRootContent = mRootContent);
  return NS_OK;
}


class SelectionChangeEvent : public nsRunnable
{
public:
  SelectionChangeEvent(IMEContentObserver* aDispatcher,
                       bool aCausedByComposition)
    : mDispatcher(aDispatcher)
    , mCausedByComposition(aCausedByComposition)
  {
    MOZ_ASSERT(mDispatcher);
  }

  NS_IMETHOD Run()
  {
    if (mDispatcher->GetWidget()) {
      IMENotification notification(NOTIFY_IME_OF_SELECTION_CHANGE);
      notification.mSelectionChangeData.mCausedByComposition =
         mCausedByComposition;
      mDispatcher->GetWidget()->NotifyIME(notification);
    }
    return NS_OK;
  }

private:
  nsRefPtr<IMEContentObserver> mDispatcher;
  bool mCausedByComposition;
};

nsresult
IMEContentObserver::NotifySelectionChanged(nsIDOMDocument* aDOMDocument,
                                           nsISelection* aSelection,
                                           int16_t aReason)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return NS_OK;
  }

  int32_t count = 0;
  nsresult rv = aSelection->GetRangeCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count > 0 && mWidget) {
    nsContentUtils::AddScriptRunner(
      new SelectionChangeEvent(this, causedByComposition));
  }
  return NS_OK;
}


class PositionChangeEvent MOZ_FINAL : public nsRunnable
{
public:
  PositionChangeEvent(IMEContentObserver* aDispatcher)
    : mDispatcher(aDispatcher)
  {
    MOZ_ASSERT(mDispatcher);
  }

  NS_IMETHOD Run()
  {
    if (mDispatcher->GetWidget()) {
      mDispatcher->GetWidget()->NotifyIME(
        IMENotification(NOTIFY_IME_OF_POSITION_CHANGE));
    }
    return NS_OK;
  }

private:
  nsRefPtr<IMEContentObserver> mDispatcher;
};

void
IMEContentObserver::ScrollPositionChanged()
{
  if (mWidget) {
    nsContentUtils::AddScriptRunner(new PositionChangeEvent(this));
  }
}

NS_IMETHODIMP
IMEContentObserver::Reflow(DOMHighResTimeStamp aStart,
                           DOMHighResTimeStamp aEnd)
{
  if (mWidget) {
    nsContentUtils::AddScriptRunner(new PositionChangeEvent(this));
  }
  return NS_OK;
}

NS_IMETHODIMP
IMEContentObserver::ReflowInterruptible(DOMHighResTimeStamp aStart,
                                        DOMHighResTimeStamp aEnd)
{
  if (mWidget) {
    nsContentUtils::AddScriptRunner(new PositionChangeEvent(this));
  }
  return NS_OK;
}


class TextChangeEvent : public nsRunnable
{
public:
  TextChangeEvent(IMEContentObserver* aDispatcher,
                  const IMEContentObserver::TextChangeData& aData)
    : mDispatcher(aDispatcher)
    , mData(aData)
  {
    MOZ_ASSERT(mDispatcher);
    MOZ_ASSERT(mData.mStored);
  }

  NS_IMETHOD Run()
  {
    if (mDispatcher->GetWidget()) {
      IMENotification notification(NOTIFY_IME_OF_TEXT_CHANGE);
      notification.mTextChangeData.mStartOffset = mData.mStartOffset;
      notification.mTextChangeData.mOldEndOffset = mData.mRemovedEndOffset;
      notification.mTextChangeData.mNewEndOffset = mData.mAddedEndOffset;
      notification.mTextChangeData.mCausedByComposition =
        mData.mCausedOnlyByComposition;
      mDispatcher->GetWidget()->NotifyIME(notification);
    }
    return NS_OK;
  }

private:
  nsRefPtr<IMEContentObserver> mDispatcher;
  IMEContentObserver::TextChangeData mData;
};

bool
IMEContentObserver::StoreTextChangeData(const TextChangeData& aTextChangeData)
{
  MOZ_ASSERT(aTextChangeData.mStartOffset <= aTextChangeData.mRemovedEndOffset,
             "end of removed text must be same or larger than start");
  MOZ_ASSERT(aTextChangeData.mStartOffset <= aTextChangeData.mAddedEndOffset,
             "end of added text must be same or larger than start");

  if (!mTextChangeData.mStored) {
    mTextChangeData = aTextChangeData;
    MOZ_ASSERT(mTextChangeData.mStored, "Why mStored is false?");
    return true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  

  
  
  
  
  

  const TextChangeData& newData = aTextChangeData;
  const TextChangeData oldData = mTextChangeData;

  mTextChangeData.mCausedOnlyByComposition =
    newData.mCausedOnlyByComposition && oldData.mCausedOnlyByComposition;

  if (newData.mStartOffset >= oldData.mAddedEndOffset) {
    
    
    
    
    
    
    mTextChangeData.mStartOffset = oldData.mStartOffset;
    
    
    
    
    uint32_t newRemovedEndOffsetInOldText =
      newData.mRemovedEndOffset - oldData.Difference();
    mTextChangeData.mRemovedEndOffset =
      std::max(newRemovedEndOffsetInOldText, oldData.mRemovedEndOffset);
    
    mTextChangeData.mAddedEndOffset = newData.mAddedEndOffset;
    return false;
  }

  if (newData.mStartOffset >= oldData.mStartOffset) {
    
    
    mTextChangeData.mStartOffset = oldData.mStartOffset;
    if (newData.mRemovedEndOffset >= oldData.mAddedEndOffset) {
      
      
      
      
      
      
      
      
      
      
      uint32_t newRemovedEndOffsetInOldText =
        newData.mRemovedEndOffset - oldData.Difference();
      mTextChangeData.mRemovedEndOffset =
        std::max(newRemovedEndOffsetInOldText, oldData.mRemovedEndOffset);
      
      
      
      
      mTextChangeData.mAddedEndOffset = newData.mAddedEndOffset;
      return false;
    }

    
    
    
    
    
    
    
    
    mTextChangeData.mRemovedEndOffset = oldData.mRemovedEndOffset;
    
    
    uint32_t oldAddedEndOffsetInNewText =
      oldData.mAddedEndOffset + newData.Difference();
    mTextChangeData.mAddedEndOffset =
      std::max(newData.mAddedEndOffset, oldAddedEndOffsetInNewText);
    return false;
  }

  if (newData.mRemovedEndOffset >= oldData.mStartOffset) {
    
    
    
    MOZ_ASSERT(newData.mStartOffset < oldData.mStartOffset,
      "new start offset should be less than old one here");
    mTextChangeData.mStartOffset = newData.mStartOffset;
    if (newData.mRemovedEndOffset >= oldData.mAddedEndOffset) {
      
      
      
      
      
      
      
      
      
      
      uint32_t newRemovedEndOffsetInOldText =
        newData.mRemovedEndOffset - oldData.Difference();
      mTextChangeData.mRemovedEndOffset =
        std::max(newRemovedEndOffsetInOldText, oldData.mRemovedEndOffset);
      
      
      
      
      
      mTextChangeData.mAddedEndOffset = newData.mAddedEndOffset;
      return false;
    }

    
    
    
    
    
    
    
    
    
    mTextChangeData.mRemovedEndOffset = oldData.mRemovedEndOffset;
    
    
    
    uint32_t oldAddedEndOffsetInNewText =
      oldData.mAddedEndOffset + newData.Difference();
    mTextChangeData.mAddedEndOffset =
      std::max(newData.mAddedEndOffset, oldAddedEndOffsetInNewText);
    return false;
  }

  
  
  
  
  
  MOZ_ASSERT(newData.mStartOffset < oldData.mStartOffset,
    "new start offset should be less than old one here");
  mTextChangeData.mStartOffset = newData.mStartOffset;
  MOZ_ASSERT(newData.mRemovedEndOffset < oldData.mRemovedEndOffset,
     "new removed end offset should be less than old one here");
  mTextChangeData.mRemovedEndOffset = oldData.mRemovedEndOffset;
  
  uint32_t oldAddedEndOffsetInNewText =
    oldData.mAddedEndOffset + newData.Difference();
  mTextChangeData.mAddedEndOffset =
    std::max(newData.mAddedEndOffset, oldAddedEndOffsetInNewText);

  return false;
}

void
IMEContentObserver::CharacterDataChanged(nsIDocument* aDocument,
                                         nsIContent* aContent,
                                         CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "character data changed for non-text node");

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.mStored && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0;
  
  nsresult rv =
    ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent, aContent,
                                                  aInfo->mChangeStart,
                                                  &offset,
                                                  LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  uint32_t oldEnd = offset + aInfo->mChangeEnd - aInfo->mChangeStart;
  uint32_t newEnd = offset + aInfo->mReplaceLength;

  TextChangeData data(offset, oldEnd, newEnd, causedByComposition);
  if (StoreTextChangeData(data)) {
    nsContentUtils::AddScriptRunner(new TextChangeEvent(this, mTextChangeData));
  }
}

void
IMEContentObserver::NotifyContentAdded(nsINode* aContainer,
                                       int32_t aStartIndex,
                                       int32_t aEndIndex)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.mStored && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0;
  nsresult rv =
    ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent, aContainer,
                                                  aStartIndex, &offset,
                                                  LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  
  nsIContent* childAtStart = aContainer->GetChildAt(aStartIndex);
  uint32_t addingLength = 0;
  rv = ContentEventHandler::GetFlatTextOffsetOfRange(childAtStart, aContainer,
                                                     aEndIndex, &addingLength,
                                                     LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  if (!addingLength) {
    return;
  }

  TextChangeData data(offset, offset, offset + addingLength,
                      causedByComposition);
  if (StoreTextChangeData(data)) {
    nsContentUtils::AddScriptRunner(new TextChangeEvent(this, mTextChangeData));
  }
}

void
IMEContentObserver::ContentAppended(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aFirstNewContent,
                                    int32_t aNewIndexInContainer)
{
  NotifyContentAdded(aContainer, aNewIndexInContainer,
                     aContainer->GetChildCount());
}

void
IMEContentObserver::ContentInserted(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    int32_t aIndexInContainer)
{
  NotifyContentAdded(NODE_FROM(aContainer, aDocument),
                     aIndexInContainer, aIndexInContainer + 1);
}

void
IMEContentObserver::ContentRemoved(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   int32_t aIndexInContainer,
                                   nsIContent* aPreviousSibling)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.mStored && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0;
  nsresult rv =
    ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent,
                                                  NODE_FROM(aContainer,
                                                            aDocument),
                                                  aIndexInContainer, &offset,
                                                  LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  
  int32_t nodeLength =
    aChild->IsNodeOfType(nsINode::eTEXT) ?
      static_cast<int32_t>(aChild->TextLength()) :
      std::max(static_cast<int32_t>(aChild->GetChildCount()), 1);
  MOZ_ASSERT(nodeLength >= 0, "The node length is out of range");
  uint32_t textLength = 0;
  rv = ContentEventHandler::GetFlatTextOffsetOfRange(aChild, aChild,
                                                     nodeLength, &textLength,
                                                     LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  if (!textLength) {
    return;
  }

  TextChangeData data(offset, offset + textLength, offset, causedByComposition);
  if (StoreTextChangeData(data)) {
    nsContentUtils::AddScriptRunner(new TextChangeEvent(this, mTextChangeData));
  }
}

static nsIContent*
GetContentBR(dom::Element* aElement)
{
  if (!aElement->IsNodeOfType(nsINode::eCONTENT)) {
    return nullptr;
  }
  nsIContent* content = static_cast<nsIContent*>(aElement);
  return content->IsHTML(nsGkAtoms::br) ? content : nullptr;
}

void
IMEContentObserver::AttributeWillChange(nsIDocument* aDocument,
                                        dom::Element* aElement,
                                        int32_t aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        int32_t aModType)
{
  nsIContent *content = GetContentBR(aElement);
  mPreAttrChangeLength = content ?
    ContentEventHandler::GetNativeTextLength(content) : 0;
}

void
IMEContentObserver::AttributeChanged(nsIDocument* aDocument,
                                     dom::Element* aElement,
                                     int32_t aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     int32_t aModType)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.mStored && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  nsIContent *content = GetContentBR(aElement);
  if (!content) {
    return;
  }

  uint32_t postAttrChangeLength =
    ContentEventHandler::GetNativeTextLength(content);
  if (postAttrChangeLength == mPreAttrChangeLength) {
    return;
  }
  uint32_t start;
  nsresult rv =
    ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent, content,
                                                  0, &start,
                                                  LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  TextChangeData data(start, start + mPreAttrChangeLength,
                      start + postAttrChangeLength, causedByComposition);
  if (StoreTextChangeData(data)) {
    nsContentUtils::AddScriptRunner(new TextChangeEvent(this, mTextChangeData));
  }
}

#ifdef DEBUG



void
IMEContentObserver::TestMergingTextChangeData()
{
  static bool gTestTextChangeEvent = true;
  if (!gTestTextChangeEvent) {
    return;
  }
  gTestTextChangeEvent = false;

  



  
  StoreTextChangeData(TextChangeData(10, 10, 20, false));
  StoreTextChangeData(TextChangeData(20, 20, 35, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 1-1-1: mStartOffset should be the first offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 10, 
    "Test 1-1-2: mRemovedEndOffset should be the first end of removed text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 35,
    "Test 1-1-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(10, 20, 10, false));
  StoreTextChangeData(TextChangeData(10, 30, 10, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 1-2-1: mStartOffset should be the first offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 40, 
    "Test 1-2-2: mRemovedEndOffset should be the the last end of removed text "
    "with already removed length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 10,
    "Test 1-2-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(10, 20, 10, false));
  StoreTextChangeData(TextChangeData(10, 15, 10, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 1-3-1: mStartOffset should be the first offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 25, 
    "Test 1-3-2: mRemovedEndOffset should be the the last end of removed text "
    "with already removed length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 10,
    "Test 1-3-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(10, 10, 20, false));
  StoreTextChangeData(TextChangeData(55, 55, 60, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 1-4-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 45, 
    "Test 1-4-2: mRemovedEndOffset should be the the largest end of removed "
    "text without already added length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 60,
    "Test 1-4-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(10, 20, 10, false));
  StoreTextChangeData(TextChangeData(55, 68, 55, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 1-5-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 78, 
    "Test 1-5-2: mRemovedEndOffset should be the the largest end of removed "
    "text with already removed length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 55,
    "Test 1-5-3: mAddedEndOffset should be the largest end of added text");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(30, 35, 32, false));
  StoreTextChangeData(TextChangeData(32, 32, 40, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 30,
    "Test 1-6-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 35, 
    "Test 1-6-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 40,
    "Test 1-6-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(30, 35, 32, false));
  StoreTextChangeData(TextChangeData(32, 32, 33, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 30,
    "Test 1-7-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 35, 
    "Test 1-7-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 33,
    "Test 1-7-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(30, 35, 30, false));
  StoreTextChangeData(TextChangeData(32, 34, 48, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 30,
    "Test 1-8-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 39, 
    "Test 1-8-2: mRemovedEndOffset should be the the first end of removed text "
    "without already removed text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 48,
    "Test 1-8-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(30, 35, 30, false));
  StoreTextChangeData(TextChangeData(32, 38, 36, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 30,
    "Test 1-9-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 43, 
    "Test 1-9-2: mRemovedEndOffset should be the the first end of removed text "
    "without already removed text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 36,
    "Test 1-9-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  



  
  
  StoreTextChangeData(TextChangeData(50, 50, 55, false));
  StoreTextChangeData(TextChangeData(53, 60, 54, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 50,
    "Test 2-1-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 55, 
    "Test 2-1-2: mRemovedEndOffset should be the the last end of removed text "
    "without already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 54,
    "Test 2-1-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 50, 55, false));
  StoreTextChangeData(TextChangeData(54, 62, 68, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 50,
    "Test 2-2-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 57, 
    "Test 2-2-2: mRemovedEndOffset should be the the last end of removed text "
    "without already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 68,
    "Test 2-2-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(36, 48, 45, false));
  StoreTextChangeData(TextChangeData(43, 50, 49, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 36,
    "Test 2-3-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 53, 
    "Test 2-3-2: mRemovedEndOffset should be the the last end of removed text "
    "without already removed text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 49,
    "Test 2-3-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(36, 52, 53, false));
  StoreTextChangeData(TextChangeData(43, 68, 61, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 36,
    "Test 2-4-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 67, 
    "Test 2-4-2: mRemovedEndOffset should be the the last end of removed text "
    "without already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 61,
    "Test 2-4-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  



  
  StoreTextChangeData(TextChangeData(10, 10, 20, false));
  StoreTextChangeData(TextChangeData(15, 15, 30, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 3-1-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 10,
    "Test 3-1-2: mRemovedEndOffset should be the the first end of removed text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 35, 
    "Test 3-1-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(50, 50, 55, false));
  StoreTextChangeData(TextChangeData(52, 53, 56, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 50,
    "Test 3-2-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 50,
    "Test 3-2-2: mRemovedEndOffset should be the the first end of removed text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 58, 
    "Test 3-2-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(36, 48, 45, false));
  StoreTextChangeData(TextChangeData(37, 38, 50, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 36,
    "Test 3-3-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 48,
    "Test 3-3-2: mRemovedEndOffset should be the the first end of removed text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 57, 
    "Test 3-3-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(32, 48, 53, false));
  StoreTextChangeData(TextChangeData(43, 50, 52, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 32,
    "Test 3-4-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 48,
    "Test 3-4-2: mRemovedEndOffset should be the the last end of removed text "
    "without already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 55, 
    "Test 3-4-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(36, 48, 50, false));
  StoreTextChangeData(TextChangeData(37, 49, 47, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 36,
    "Test 3-5-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 48,
    "Test 3-5-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 48, 
    "Test 3-5-3: mAddedEndOffset should be the first end of added text without "
    "removed text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(32, 48, 53, false));
  StoreTextChangeData(TextChangeData(43, 50, 47, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 32,
    "Test 3-6-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 48,
    "Test 3-6-2: mRemovedEndOffset should be the the last end of removed text "
    "without already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 50, 
    "Test 3-6-3: mAddedEndOffset should be the first end of added text without "
    "removed text length by the new change");
  mTextChangeData.mStored = false;

  



  
  StoreTextChangeData(TextChangeData(50, 50, 55, false));
  StoreTextChangeData(TextChangeData(44, 66, 68, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 44,
    "Test 4-1-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 61, 
    "Test 4-1-2: mRemovedEndOffset should be the the last end of removed text "
    "without already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 68,
    "Test 4-1-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 62, 50, false));
  StoreTextChangeData(TextChangeData(44, 66, 68, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 44,
    "Test 4-2-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 78, 
    "Test 4-2-2: mRemovedEndOffset should be the the last end of removed text "
    "without already removed text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 68,
    "Test 4-2-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 62, 60, false));
  StoreTextChangeData(TextChangeData(49, 128, 130, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 49,
    "Test 4-3-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 130, 
    "Test 4-3-2: mRemovedEndOffset should be the the last end of removed text "
    "without already removed text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 130,
    "Test 4-3-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 61, 73, false));
  StoreTextChangeData(TextChangeData(44, 100, 50, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 44,
    "Test 4-4-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 88, 
    "Test 4-4-2: mRemovedEndOffset should be the the last end of removed text "
    "with already added text length");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 50,
    "Test 4-4-3: mAddedEndOffset should be the last end of added text");
  mTextChangeData.mStored = false;

  



  
  StoreTextChangeData(TextChangeData(50, 50, 55, false));
  StoreTextChangeData(TextChangeData(48, 52, 49, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 48,
    "Test 5-1-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 50,
    "Test 5-1-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 52, 
    "Test 5-1-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 60, 58, false));
  StoreTextChangeData(TextChangeData(43, 50, 48, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 43,
    "Test 5-2-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 60,
    "Test 5-2-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 56, 
    "Test 5-2-3: mAddedEndOffset should be the first end of added text without "
    "removed text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 60, 68, false));
  StoreTextChangeData(TextChangeData(43, 55, 53, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 43,
    "Test 5-3-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 60,
    "Test 5-3-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 66, 
    "Test 5-3-3: mAddedEndOffset should be the first end of added text without "
    "removed text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 60, 58, false));
  StoreTextChangeData(TextChangeData(43, 50, 128, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 43,
    "Test 5-4-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 60,
    "Test 5-4-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 136, 
    "Test 5-4-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  
  StoreTextChangeData(TextChangeData(50, 60, 68, false));
  StoreTextChangeData(TextChangeData(43, 55, 65, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 43,
    "Test 5-5-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 60,
    "Test 5-5-2: mRemovedEndOffset should be the the first end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 78, 
    "Test 5-5-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  



  
  StoreTextChangeData(TextChangeData(30, 30, 45, false));
  StoreTextChangeData(TextChangeData(10, 10, 20, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 6-1-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 30,
    "Test 6-1-2: mRemovedEndOffset should be the the largest end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 55, 
    "Test 6-1-3: mAddedEndOffset should be the first end of added text with "
    "added text length by the new change");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(30, 35, 30, false));
  StoreTextChangeData(TextChangeData(10, 25, 10, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 10,
    "Test 6-2-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 35,
    "Test 6-2-2: mRemovedEndOffset should be the the largest end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 15, 
    "Test 6-2-3: mAddedEndOffset should be the first end of added text with "
    "removed text length by the new change");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(50, 65, 70, false));
  StoreTextChangeData(TextChangeData(13, 24, 15, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 13,
    "Test 6-3-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 65,
    "Test 6-3-2: mRemovedEndOffset should be the the largest end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 61, 
    "Test 6-3-3: mAddedEndOffset should be the first end of added text without "
    "removed text length by the new change");
  mTextChangeData.mStored = false;

  
  StoreTextChangeData(TextChangeData(50, 65, 70, false));
  StoreTextChangeData(TextChangeData(13, 24, 36, false));
  MOZ_ASSERT(mTextChangeData.mStartOffset == 13,
    "Test 6-4-1: mStartOffset should be the smallest offset");
  MOZ_ASSERT(mTextChangeData.mRemovedEndOffset == 65,
    "Test 6-4-2: mRemovedEndOffset should be the the largest end of removed "
    "text");
  MOZ_ASSERT(mTextChangeData.mAddedEndOffset == 82, 
    "Test 6-4-3: mAddedEndOffset should be the first end of added text without "
    "removed text length by the new change");
  mTextChangeData.mStored = false;
}
#endif 

} 
