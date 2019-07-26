





#include "IMEContentObserver.h"
#include "mozilla/dom/Element.h"
#include "nsAutoPtr.h"
#include "nsAsyncDOMEvent.h"
#include "nsContentEventHandler.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMRange.h"
#include "nsIFrame.h"
#include "nsIMEStateManager.h"
#include "nsINode.h"
#include "nsIPresShell.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsISupports.h"
#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "TextComposition.h"

namespace mozilla {

using namespace widget;

NS_IMPL_ISUPPORTS5(IMEContentObserver,
                   nsIMutationObserver,
                   nsISelectionListener,
                   nsIReflowObserver,
                   nsIScrollObserver,
                   nsISupportsWeakReference)

IMEContentObserver::IMEContentObserver()
{
}

void
IMEContentObserver::Init(nsIWidget* aWidget,
                         nsPresContext* aPresContext,
                         nsIContent* aContent)
{
  mWidget = aWidget;
  mEditableNode =
    nsIMEStateManager::GetRootEditableNode(aPresContext, aContent);
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
                       getter_AddRefs(mSel));
  NS_ENSURE_TRUE_VOID(mSel);

  nsCOMPtr<nsIDOMRange> selDomRange;
  if (NS_SUCCEEDED(mSel->GetRangeAt(0, getter_AddRefs(selDomRange)))) {
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

  if (nsIMEStateManager::IsTestingIME()) {
    nsIDocument* doc = aPresContext->Document();
    (new nsAsyncDOMEvent(doc, NS_LITERAL_STRING("MozIMEFocusIn"),
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
  MOZ_ASSERT(mSel);
  MOZ_ASSERT(mRootContent);

  mUpdatePreference = mWidget->GetIMEUpdatePreference();
  if (mUpdatePreference.WantSelectionChange()) {
    
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSel));
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
IMEContentObserver::Destroy(void)
{
  
  
  if (mRootContent) {
    if (nsIMEStateManager::IsTestingIME() && mEditableNode) {
      nsIDocument* doc = mEditableNode->OwnerDoc();
      (new nsAsyncDOMEvent(doc, NS_LITERAL_STRING("MozIMEFocusOut"),
                           false, false))->RunDOMEventWhenSafe();
    }
    mWidget->NotifyIME(IMENotification(NOTIFY_IME_OF_BLUR));
  }
  
  mWidget = nullptr;
  if (mUpdatePreference.WantSelectionChange() && mSel) {
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSel));
    if (selPrivate)
      selPrivate->RemoveSelectionListener(this);
  }
  mSel = nullptr;
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
}

bool
IMEContentObserver::IsManaging(nsPresContext* aPresContext,
                               nsIContent* aContent)
{
  if (!mSel || !mRootContent || !mEditableNode) {
    return false; 
  }
  if (!mRootContent->IsInDoc()) {
    return false; 
  }
  return mEditableNode == nsIMEStateManager::GetRootEditableNode(aPresContext,
                                                                 aContent);
}

bool
IMEContentObserver::IsEditorHandlingEventForComposition() const
{
  if (!mWidget) {
    return false;
  }
  nsRefPtr<TextComposition> composition =
    nsIMEStateManager::GetTextCompositionFor(mWidget);
  if (!composition) {
    return false;
  }
  return composition->IsEditorHandlingEvent();
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
    if (mDispatcher->mWidget) {
      IMENotification notification(NOTIFY_IME_OF_SELECTION_CHANGE);
      notification.mSelectionChangeData.mCausedByComposition =
         mCausedByComposition;
      mDispatcher->mWidget->NotifyIME(notification);
    }
    return NS_OK;
  }

private:
  nsRefPtr<IMEContentObserver> mDispatcher;
  bool mCausedByComposition;
};

nsresult
IMEContentObserver::NotifySelectionChanged(nsIDOMDocument* aDoc,
                                           nsISelection* aSel,
                                           int16_t aReason)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return NS_OK;
  }

  int32_t count = 0;
  nsresult rv = aSel->GetRangeCount(&count);
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
    if (mDispatcher->mWidget) {
      mDispatcher->mWidget->NotifyIME(
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
                  uint32_t aStart, uint32_t aOldEnd, uint32_t aNewEnd,
                  bool aCausedByComposition)
    : mDispatcher(aDispatcher)
    , mStart(aStart)
    , mOldEnd(aOldEnd)
    , mNewEnd(aNewEnd)
    , mCausedByComposition(aCausedByComposition)
  {
    MOZ_ASSERT(mDispatcher);
  }

  NS_IMETHOD Run()
  {
    if (mDispatcher->mWidget) {
      IMENotification notification(NOTIFY_IME_OF_TEXT_CHANGE);
      notification.mTextChangeData.mStartOffset = mStart;
      notification.mTextChangeData.mOldEndOffset = mOldEnd;
      notification.mTextChangeData.mNewEndOffset = mNewEnd;
      notification.mTextChangeData.mCausedByComposition = mCausedByComposition;
      mDispatcher->mWidget->NotifyIME(notification);
    }
    return NS_OK;
  }

private:
  nsRefPtr<IMEContentObserver> mDispatcher;
  uint32_t mStart, mOldEnd, mNewEnd;
  bool mCausedByComposition;
};

void
IMEContentObserver::CharacterDataChanged(nsIDocument* aDocument,
                                         nsIContent* aContent,
                                         CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "character data changed for non-text node");

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0;
  
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, aContent, aInfo->mChangeStart, &offset)))
    return;

  uint32_t oldEnd = offset + aInfo->mChangeEnd - aInfo->mChangeStart;
  uint32_t newEnd = offset + aInfo->mReplaceLength;

  nsContentUtils::AddScriptRunner(
    new TextChangeEvent(this, offset, oldEnd, newEnd, causedByComposition));
}

void
IMEContentObserver::NotifyContentAdded(nsINode* aContainer,
                                       int32_t aStartIndex,
                                       int32_t aEndIndex)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0, newOffset = 0;
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, aContainer, aStartIndex, &offset)))
    return;

  
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    aContainer->GetChildAt(aStartIndex),
                    aContainer, aEndIndex, &newOffset)))
    return;

  
  if (newOffset) {
    nsContentUtils::AddScriptRunner(
      new TextChangeEvent(this, offset, offset, offset + newOffset,
                          causedByComposition));
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
  if (causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0, childOffset = 1;
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, NODE_FROM(aContainer, aDocument),
                    aIndexInContainer, &offset)))
    return;

  
  if (aChild->IsNodeOfType(nsINode::eTEXT))
    childOffset = aChild->TextLength();
  else if (0 < aChild->GetChildCount())
    childOffset = aChild->GetChildCount();

  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    aChild, aChild, childOffset, &childOffset)))
    return;

  
  if (childOffset) {
    nsContentUtils::AddScriptRunner(
      new TextChangeEvent(this, offset, offset + childOffset, offset,
                          causedByComposition));
  }
}

static nsIContent*
GetContentBR(dom::Element* aElement)
{
  if (!aElement->IsNodeOfType(nsINode::eCONTENT)) {
    return nullptr;
  }
  nsIContent *content = static_cast<nsIContent*>(aElement);
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
    nsContentEventHandler::GetNativeTextLength(content) : 0;
}

void
IMEContentObserver::AttributeChanged(nsIDocument* aDocument,
                                     dom::Element* aElement,
                                     int32_t aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     int32_t aModType)
{
  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  nsIContent *content = GetContentBR(aElement);
  if (!content) {
    return;
  }
  uint32_t postAttrChangeLength =
    nsContentEventHandler::GetNativeTextLength(content);
  if (postAttrChangeLength != mPreAttrChangeLength) {
    uint32_t start;
    if (NS_SUCCEEDED(nsContentEventHandler::GetFlatTextOffsetOfRange(
        mRootContent, content, 0, &start))) {
      nsContentUtils::AddScriptRunner(new TextChangeEvent(this, start,
        start + mPreAttrChangeLength, start + postAttrChangeLength,
        causedByComposition));
    }
  }
}

} 
