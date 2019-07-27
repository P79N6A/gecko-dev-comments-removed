





#include "ContentEventHandler.h"
#include "IMEContentObserver.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"
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
#include "nsWeakReference.h"
#include "WritingModes.h"

namespace mozilla {

using namespace widget;





NS_IMPL_CYCLE_COLLECTION_CLASS(IMEContentObserver)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(IMEContentObserver)
  nsAutoScriptBlocker scriptBlocker;

  tmp->NotifyIMEOfBlur();
  tmp->UnregisterObservers();

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSelection)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRootContent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mEditableNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocShell)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mEditor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mEndOfAddedTextCache.mContainerNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mStartOfRemovingTextRangeCache.mContainerNode)

  tmp->mUpdatePreference.mWantUpdates = nsIMEUpdatePreference::NOTIFY_NOTHING;
  tmp->mESM = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(IMEContentObserver)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWidget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSelection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRootContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEditableNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocShell)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEditor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEndOfAddedTextCache.mContainerNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(
    mStartOfRemovingTextRangeCache.mContainerNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(IMEContentObserver)
 NS_INTERFACE_MAP_ENTRY(nsISelectionListener)
 NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
 NS_INTERFACE_MAP_ENTRY(nsIReflowObserver)
 NS_INTERFACE_MAP_ENTRY(nsIScrollObserver)
 NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
 NS_INTERFACE_MAP_ENTRY(nsIEditorObserver)
 NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISelectionListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(IMEContentObserver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(IMEContentObserver)

IMEContentObserver::IMEContentObserver()
  : mESM(nullptr)
  , mSuppressNotifications(0)
  , mPreCharacterDataChangeLength(-1)
  , mIsObserving(false)
  , mIMEHasFocus(false)
  , mIsFocusEventPending(false)
  , mIsSelectionChangeEventPending(false)
  , mSelectionChangeCausedOnlyByComposition(false)
  , mIsPositionChangeEventPending(false)
  , mIsFlushingPendingNotifications(false)
{
#ifdef DEBUG
  mTextChangeData.Test();
#endif
}

void
IMEContentObserver::Init(nsIWidget* aWidget,
                         nsPresContext* aPresContext,
                         nsIContent* aContent,
                         nsIEditor* aEditor)
{
  MOZ_ASSERT(aEditor, "aEditor must not be null");

  State state = GetState();
  if (NS_WARN_IF(state == eState_Observing)) {
    return; 
  }

  bool firstInitialization = state != eState_StoppedObserving;
  if (!firstInitialization) {
    
    
    UnregisterObservers();
    
    mRootContent = nullptr;
    mEditor = nullptr;
    mSelection = nullptr;
    mDocShell = nullptr;
  }

  mESM = aPresContext->EventStateManager();
  mESM->OnStartToObserveContent(this);

  mWidget = aWidget;

  mEditableNode =
    IMEStateManager::GetRootEditableNode(aPresContext, aContent);
  if (!mEditableNode) {
    return;
  }

  mEditor = aEditor;

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

  if (firstInitialization) {
    MaybeNotifyIMEOfFocusSet();

    
    
    
    
    if (GetState() != eState_Initializing) {
      return;
    }

    
    
    
    if (!mRootContent) {
      return;
    }
  }

  mDocShell = aPresContext->GetDocShell();

  ObserveEditableNode();

  
  
  FlushMergeableNotifications();
}

void
IMEContentObserver::ObserveEditableNode()
{
  MOZ_RELEASE_ASSERT(mEditor);
  MOZ_RELEASE_ASSERT(mSelection);
  MOZ_RELEASE_ASSERT(mRootContent);
  MOZ_RELEASE_ASSERT(GetState() != eState_Observing);

  mIsObserving = true;
  mEditor->AddEditorObserver(this);

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
IMEContentObserver::NotifyIMEOfBlur()
{
  
  nsCOMPtr<nsIWidget> widget;
  mWidget.swap(widget);

  
  if (!mIMEHasFocus) {
    return;
  }

  
  MOZ_RELEASE_ASSERT(widget);

  
  
  
  
  
  
  
  
  mIMEHasFocus = false;
  IMEStateManager::NotifyIME(IMENotification(NOTIFY_IME_OF_BLUR), widget);
}

void
IMEContentObserver::UnregisterObservers()
{
  if (!mIsObserving) {
    return;
  }
  mIsObserving = false;

  if (mEditor) {
    mEditor->RemoveEditorObserver(this);
  }

  if (mUpdatePreference.WantSelectionChange() && mSelection) {
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSelection));
    if (selPrivate) {
      selPrivate->RemoveSelectionListener(this);
    }
  }

  if (mUpdatePreference.WantTextChange() && mRootContent) {
    mRootContent->RemoveMutationObserver(this);
  }

  if (mUpdatePreference.WantPositionChanged() && mDocShell) {
    mDocShell->RemoveWeakScrollObserver(this);
    mDocShell->RemoveWeakReflowObserver(this);
  }
}

nsPresContext*
IMEContentObserver::GetPresContext() const
{
  return mESM ? mESM->GetPresContext() : nullptr;
}

void
IMEContentObserver::Destroy()
{
  

  NotifyIMEOfBlur();
  UnregisterObservers();

  mEditor = nullptr;
  mWidget = nullptr;
  mSelection = nullptr;
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
IMEContentObserver::MaybeReinitialize(nsIWidget* aWidget,
                                      nsPresContext* aPresContext,
                                      nsIContent* aContent,
                                      nsIEditor* aEditor)
{
  if (!IsObservingContent(aPresContext, aContent)) {
    return false;
  }

  if (GetState() == eState_StoppedObserving) {
    Init(aWidget, aPresContext, aContent, aEditor);
  }
  return IsManaging(aPresContext, aContent);
}

bool
IMEContentObserver::IsManaging(nsPresContext* aPresContext,
                               nsIContent* aContent)
{
  return GetState() == eState_Observing &&
         IsObservingContent(aPresContext, aContent);
}

IMEContentObserver::State
IMEContentObserver::GetState() const
{
  if (!mSelection || !mRootContent || !mEditableNode) {
    return eState_NotObserving; 
  }
  if (!mRootContent->IsInComposedDoc()) {
    
    return eState_StoppedObserving;
  }
  return mIsObserving ? eState_Observing : eState_Initializing;
}

bool
IMEContentObserver::IsObservingContent(nsPresContext* aPresContext,
                                       nsIContent* aContent) const
{
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
    MaybeNotifyIMEOfSelectionChange(causedByComposition);
  }
  return NS_OK;
}

void
IMEContentObserver::ScrollPositionChanged()
{
  MaybeNotifyIMEOfPositionChange();
}

NS_IMETHODIMP
IMEContentObserver::Reflow(DOMHighResTimeStamp aStart,
                           DOMHighResTimeStamp aEnd)
{
  MaybeNotifyIMEOfPositionChange();
  return NS_OK;
}

NS_IMETHODIMP
IMEContentObserver::ReflowInterruptible(DOMHighResTimeStamp aStart,
                                        DOMHighResTimeStamp aEnd)
{
  MaybeNotifyIMEOfPositionChange();
  return NS_OK;
}

bool
IMEContentObserver::OnMouseButtonEvent(nsPresContext* aPresContext,
                                       WidgetMouseEvent* aMouseEvent)
{
  if (!mUpdatePreference.WantMouseButtonEventOnChar()) {
    return false;
  }
  if (!aMouseEvent->mFlags.mIsTrusted ||
      aMouseEvent->mFlags.mDefaultPrevented ||
      !aMouseEvent->widget) {
    return false;
  }
  
  switch (aMouseEvent->message) {
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_BUTTON_DOWN:
      break;
    default:
      return false;
  }
  if (NS_WARN_IF(!mWidget) || NS_WARN_IF(mWidget->Destroyed())) {
    return false;
  }

  nsRefPtr<IMEContentObserver> kungFuDeathGrip(this);

  WidgetQueryContentEvent charAtPt(true, NS_QUERY_CHARACTER_AT_POINT,
                                   aMouseEvent->widget);
  charAtPt.refPoint = aMouseEvent->refPoint;
  ContentEventHandler handler(aPresContext);
  handler.OnQueryCharacterAtPoint(&charAtPt);
  if (NS_WARN_IF(!charAtPt.mSucceeded) ||
      charAtPt.mReply.mOffset == WidgetQueryContentEvent::NOT_FOUND) {
    return false;
  }

  
  
  if (!mWidget || NS_WARN_IF(mWidget->Destroyed())) {
    return false;
  }

  
  
  nsIWidget* topLevelWidget = mWidget->GetTopLevelWidget();
  if (topLevelWidget && topLevelWidget != mWidget) {
    charAtPt.mReply.mRect.MoveBy(
      topLevelWidget->WidgetToScreenOffset() -
        mWidget->WidgetToScreenOffset());
  }
  
  
  if (aMouseEvent->widget != mWidget) {
    charAtPt.refPoint += aMouseEvent->widget->WidgetToScreenOffset() -
      mWidget->WidgetToScreenOffset();
  }

  IMENotification notification(NOTIFY_IME_OF_MOUSE_BUTTON_EVENT);
  notification.mMouseButtonEventData.mEventMessage = aMouseEvent->message;
  notification.mMouseButtonEventData.mOffset = charAtPt.mReply.mOffset;
  notification.mMouseButtonEventData.mCursorPos.Set(
    LayoutDeviceIntPoint::ToUntyped(charAtPt.refPoint));
  notification.mMouseButtonEventData.mCharRect.Set(
    LayoutDevicePixel::ToUntyped(charAtPt.mReply.mRect));
  notification.mMouseButtonEventData.mButton = aMouseEvent->button;
  notification.mMouseButtonEventData.mButtons = aMouseEvent->buttons;
  notification.mMouseButtonEventData.mModifiers = aMouseEvent->modifiers;

  nsresult rv = IMEStateManager::NotifyIME(notification, mWidget);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  bool consumed = (rv == NS_SUCCESS_EVENT_CONSUMED);
  aMouseEvent->mFlags.mDefaultPrevented = consumed;
  return consumed;
}

void
IMEContentObserver::CharacterDataWillChange(nsIDocument* aDocument,
                                            nsIContent* aContent,
                                            CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "character data changed for non-text node");
  MOZ_ASSERT(mPreCharacterDataChangeLength < 0,
             "CharacterDataChanged() should've reset "
             "mPreCharacterDataChangeLength");

  mEndOfAddedTextCache.Clear();
  mStartOfRemovingTextRangeCache.Clear();

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.IsValid() && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  mPreCharacterDataChangeLength =
    ContentEventHandler::GetNativeTextLength(aContent, aInfo->mChangeStart,
                                             aInfo->mChangeEnd);
  MOZ_ASSERT(mPreCharacterDataChangeLength >=
               aInfo->mChangeEnd - aInfo->mChangeStart,
             "The computed length must be same as or larger than XP length");
}

void
IMEContentObserver::CharacterDataChanged(nsIDocument* aDocument,
                                         nsIContent* aContent,
                                         CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "character data changed for non-text node");

  mEndOfAddedTextCache.Clear();
  mStartOfRemovingTextRangeCache.Clear();

  int64_t removedLength = mPreCharacterDataChangeLength;
  mPreCharacterDataChangeLength = -1;

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.IsValid() && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  MOZ_ASSERT(removedLength >= 0,
             "mPreCharacterDataChangeLength should've been set by "
             "CharacterDataWillChange()");

  uint32_t offset = 0;
  
  nsresult rv =
    ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent, aContent,
                                                  aInfo->mChangeStart,
                                                  &offset,
                                                  LINE_BREAK_TYPE_NATIVE);
  NS_ENSURE_SUCCESS_VOID(rv);

  uint32_t newLength =
    ContentEventHandler::GetNativeTextLength(aContent, aInfo->mChangeStart,
                                             aInfo->mChangeStart +
                                               aInfo->mReplaceLength);

  uint32_t oldEnd = offset + static_cast<uint32_t>(removedLength);
  uint32_t newEnd = offset + newLength;

  TextChangeData data(offset, oldEnd, newEnd, causedByComposition);
  MaybeNotifyIMEOfTextChange(data);
}

void
IMEContentObserver::NotifyContentAdded(nsINode* aContainer,
                                       int32_t aStartIndex,
                                       int32_t aEndIndex)
{
  mStartOfRemovingTextRangeCache.Clear();

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.IsValid() && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  uint32_t offset = 0;
  nsresult rv = NS_OK;
  if (!mEndOfAddedTextCache.Match(aContainer, aStartIndex)) {
    mEndOfAddedTextCache.Clear();
    rv = ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent, aContainer,
                                                       aStartIndex, &offset,
                                                       LINE_BREAK_TYPE_NATIVE);
    if (NS_WARN_IF(NS_FAILED((rv)))) {
      return;
    }
  } else {
    offset = mEndOfAddedTextCache.mFlatTextLength;
  }

  
  nsIContent* childAtStart = aContainer->GetChildAt(aStartIndex);
  uint32_t addingLength = 0;
  rv = ContentEventHandler::GetFlatTextOffsetOfRange(childAtStart, aContainer,
                                                     aEndIndex, &addingLength,
                                                     LINE_BREAK_TYPE_NATIVE);
  if (NS_WARN_IF(NS_FAILED((rv)))) {
    mEndOfAddedTextCache.Clear();
    return;
  }

  
  
  
  
  mEndOfAddedTextCache.Cache(aContainer, aEndIndex, offset + addingLength);

  if (!addingLength) {
    return;
  }

  TextChangeData data(offset, offset, offset + addingLength,
                      causedByComposition);
  MaybeNotifyIMEOfTextChange(data);
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
  mEndOfAddedTextCache.Clear();

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.IsValid() && causedByComposition &&
      !mUpdatePreference.WantChangesCausedByComposition()) {
    return;
  }

  nsINode* containerNode = NODE_FROM(aContainer, aDocument);

  uint32_t offset = 0;
  nsresult rv = NS_OK;
  if (!mStartOfRemovingTextRangeCache.Match(containerNode, aIndexInContainer)) {
    rv =
      ContentEventHandler::GetFlatTextOffsetOfRange(mRootContent, containerNode,
                                                    aIndexInContainer, &offset,
                                                    LINE_BREAK_TYPE_NATIVE);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mStartOfRemovingTextRangeCache.Clear();
      return;
    }
    mStartOfRemovingTextRangeCache.Cache(containerNode, aIndexInContainer,
                                         offset);
  } else {
    offset = mStartOfRemovingTextRangeCache.mFlatTextLength;
  }

  
  int32_t nodeLength =
    aChild->IsNodeOfType(nsINode::eTEXT) ?
      static_cast<int32_t>(aChild->TextLength()) :
      std::max(static_cast<int32_t>(aChild->GetChildCount()), 1);
  MOZ_ASSERT(nodeLength >= 0, "The node length is out of range");
  uint32_t textLength = 0;
  rv = ContentEventHandler::GetFlatTextOffsetOfRange(aChild, aChild,
                                                     nodeLength, &textLength,
                                                     LINE_BREAK_TYPE_NATIVE);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mStartOfRemovingTextRangeCache.Clear();
    return;
  }

  if (!textLength) {
    return;
  }

  TextChangeData data(offset, offset + textLength, offset, causedByComposition);
  MaybeNotifyIMEOfTextChange(data);
}

static nsIContent*
GetContentBR(dom::Element* aElement)
{
  if (!aElement->IsNodeOfType(nsINode::eCONTENT)) {
    return nullptr;
  }
  nsIContent* content = static_cast<nsIContent*>(aElement);
  return content->IsHTMLElement(nsGkAtoms::br) ? content : nullptr;
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
  mEndOfAddedTextCache.Clear();
  mStartOfRemovingTextRangeCache.Clear();

  bool causedByComposition = IsEditorHandlingEventForComposition();
  if (!mTextChangeData.IsValid() && causedByComposition &&
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
  MaybeNotifyIMEOfTextChange(data);
}

NS_IMETHODIMP
IMEContentObserver::EditAction()
{
  mEndOfAddedTextCache.Clear();
  mStartOfRemovingTextRangeCache.Clear();
  FlushMergeableNotifications();
  return NS_OK;
}

NS_IMETHODIMP
IMEContentObserver::BeforeEditAction()
{
  mEndOfAddedTextCache.Clear();
  mStartOfRemovingTextRangeCache.Clear();
  return NS_OK;
}

NS_IMETHODIMP
IMEContentObserver::CancelEditAction()
{
  mEndOfAddedTextCache.Clear();
  mStartOfRemovingTextRangeCache.Clear();
  FlushMergeableNotifications();
  return NS_OK;
}

void
IMEContentObserver::PostFocusSetNotification()
{
  mIsFocusEventPending = true;
}

void
IMEContentObserver::PostTextChangeNotification(
                      const TextChangeDataBase& aTextChangeData)
{
  mTextChangeData += aTextChangeData;
  MOZ_ASSERT(mTextChangeData.IsValid(),
             "mTextChangeData must have text change data");
}

void
IMEContentObserver::PostSelectionChangeNotification(bool aCausedByComposition)
{
  if (!mIsSelectionChangeEventPending) {
    mSelectionChangeCausedOnlyByComposition = aCausedByComposition;
  } else {
    mSelectionChangeCausedOnlyByComposition =
      mSelectionChangeCausedOnlyByComposition && aCausedByComposition;
  }
  mIsSelectionChangeEventPending = true;
}

void
IMEContentObserver::PostPositionChangeNotification()
{
  mIsPositionChangeEventPending = true;
}

bool
IMEContentObserver::IsReflowLocked() const
{
  nsPresContext* presContext = GetPresContext();
  if (NS_WARN_IF(!presContext)) {
    return false;
  }
  nsIPresShell* presShell = presContext->GetPresShell();
  if (NS_WARN_IF(!presShell)) {
    return false;
  }
  
  
  
  return presShell->IsReflowLocked();
}

bool
IMEContentObserver::IsSafeToNotifyIME() const
{
  
  
  if (!mWidget) {
    return false;
  }

  
  if (mSuppressNotifications) {
    return false;
  }

  if (!mESM || NS_WARN_IF(!GetPresContext())) {
    return false;
  }

  
  
  if (IsReflowLocked()) {
    return false;
  }

  
  bool isInEditAction = false;
  if (mEditor && NS_SUCCEEDED(mEditor->GetIsInEditAction(&isInEditAction)) &&
      isInEditAction) {
    return false;
  }

  return true;
}

void
IMEContentObserver::FlushMergeableNotifications()
{
  if (!IsSafeToNotifyIME()) {
    
    return;
  }

  
  
  
  

  if (mIsFlushingPendingNotifications) {
    
    return;
  }

  AutoRestore<bool> flusing(mIsFlushingPendingNotifications);
  mIsFlushingPendingNotifications = true;

  
  

  if (mIsFocusEventPending) {
    mIsFocusEventPending = false;
    nsContentUtils::AddScriptRunner(new FocusSetEvent(this));
    
    
    ClearPendingNotifications();
    return;
  }

  if (mTextChangeData.IsValid()) {
    nsContentUtils::AddScriptRunner(new TextChangeEvent(this, mTextChangeData));
  }

  
  
  
  if (mIsSelectionChangeEventPending) {
    mIsSelectionChangeEventPending = false;
    nsContentUtils::AddScriptRunner(
      new SelectionChangeEvent(this, mSelectionChangeCausedOnlyByComposition));
  }

  if (mIsPositionChangeEventPending) {
    mIsPositionChangeEventPending = false;
    nsContentUtils::AddScriptRunner(new PositionChangeEvent(this));
  }

  
  if (mTextChangeData.IsValid() ||
      mIsSelectionChangeEventPending ||
      mIsPositionChangeEventPending) {
    nsRefPtr<AsyncMergeableNotificationsFlusher> asyncFlusher =
      new AsyncMergeableNotificationsFlusher(this);
    NS_DispatchToCurrentThread(asyncFlusher);
  }
}





bool
IMEContentObserver::AChangeEvent::CanNotifyIME() const
{
  if (NS_WARN_IF(!mIMEContentObserver)) {
    return false;
  }
  State state = mIMEContentObserver->GetState();
  
  if (state == eState_NotObserving) {
    return false;
  }
  
  if (mChangeEventType == eChangeEventType_Focus) {
    return !NS_WARN_IF(mIMEContentObserver->mIMEHasFocus);
  }
  
  if (!mIMEContentObserver->mIMEHasFocus) {
    return false;
  }

  
  MOZ_ASSERT(mIMEContentObserver->mWidget);

  return true;
}

bool
IMEContentObserver::AChangeEvent::IsSafeToNotifyIME() const
{
  if (NS_WARN_IF(!nsContentUtils::IsSafeToRunScript())) {
    return false;
  }
  State state = mIMEContentObserver->GetState();
  if (mChangeEventType == eChangeEventType_Focus) {
    if (NS_WARN_IF(state != eState_Initializing && state != eState_Observing)) {
      return false;
    }
  } else if (state != eState_Observing) {
    return false;
  }
  return mIMEContentObserver->IsSafeToNotifyIME();
}





NS_IMETHODIMP
IMEContentObserver::FocusSetEvent::Run()
{
  if (!CanNotifyIME()) {
    
    
    mIMEContentObserver->ClearPendingNotifications();
    return NS_OK;
  }

  if (!IsSafeToNotifyIME()) {
    mIMEContentObserver->PostFocusSetNotification();
    return NS_OK;
  }

  mIMEContentObserver->mIMEHasFocus = true;
  IMEStateManager::NotifyIME(IMENotification(NOTIFY_IME_OF_FOCUS),
                             mIMEContentObserver->mWidget);
  return NS_OK;
}





NS_IMETHODIMP
IMEContentObserver::SelectionChangeEvent::Run()
{
  if (!CanNotifyIME()) {
    return NS_OK;
  }

  if (!IsSafeToNotifyIME()) {
    mIMEContentObserver->PostSelectionChangeNotification(mCausedByComposition);
    return NS_OK;
  }

  
  
  WidgetQueryContentEvent selection(true, NS_QUERY_SELECTED_TEXT,
                                    mIMEContentObserver->mWidget);
  ContentEventHandler handler(mIMEContentObserver->GetPresContext());
  handler.OnQuerySelectedText(&selection);
  if (NS_WARN_IF(!selection.mSucceeded)) {
    return NS_OK;
  }

  
  if (!CanNotifyIME()) {
    return NS_OK;
  }

  IMENotification notification(NOTIFY_IME_OF_SELECTION_CHANGE);
  notification.mSelectionChangeData.mOffset =
    selection.mReply.mOffset;
  notification.mSelectionChangeData.mLength =
    selection.mReply.mString.Length();
  notification.mSelectionChangeData.SetWritingMode(
                                      selection.GetWritingMode());
  notification.mSelectionChangeData.mReversed = selection.mReply.mReversed;
  notification.mSelectionChangeData.mCausedByComposition =
    mCausedByComposition;
  IMEStateManager::NotifyIME(notification, mIMEContentObserver->mWidget);
  return NS_OK;
}





NS_IMETHODIMP
IMEContentObserver::TextChangeEvent::Run()
{
  if (!CanNotifyIME()) {
    return NS_OK;
  }

  if (!IsSafeToNotifyIME()) {
    mIMEContentObserver->PostTextChangeNotification(mTextChangeData);
    return NS_OK;
  }

  IMENotification notification(NOTIFY_IME_OF_TEXT_CHANGE);
  notification.mTextChangeData = mTextChangeData;
  IMEStateManager::NotifyIME(notification, mIMEContentObserver->mWidget);
  return NS_OK;
}





NS_IMETHODIMP
IMEContentObserver::PositionChangeEvent::Run()
{
  if (!CanNotifyIME()) {
    return NS_OK;
  }

  if (!IsSafeToNotifyIME()) {
    mIMEContentObserver->PostPositionChangeNotification();
    return NS_OK;
  }

  IMEStateManager::NotifyIME(IMENotification(NOTIFY_IME_OF_POSITION_CHANGE),
                             mIMEContentObserver->mWidget);
  return NS_OK;
}





NS_IMETHODIMP
IMEContentObserver::AsyncMergeableNotificationsFlusher::Run()
{
  if (!CanNotifyIME()) {
    return NS_OK;
  }

  mIMEContentObserver->FlushMergeableNotifications();
  return NS_OK;
}

} 
