





#include "ContentEventHandler.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIEditor.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"

using namespace mozilla::widget;

namespace mozilla {

#define IDEOGRAPHIC_SPACE (NS_LITERAL_STRING("\x3000"))





TextComposition::TextComposition(nsPresContext* aPresContext,
                                 nsINode* aNode,
                                 WidgetCompositionEvent* aCompositionEvent)
  : mPresContext(aPresContext)
  , mNode(aNode)
  , mNativeContext(
      aCompositionEvent->widget->GetInputContext().mNativeIMEContext)
  , mCompositionStartOffset(0)
  , mCompositionTargetOffset(0)
  , mIsSynthesizedForTests(aCompositionEvent->mFlags.mIsSynthesizedForTests)
  , mIsComposing(false)
  , mIsEditorHandlingEvent(false)
  , mIsRequestingCommit(false)
  , mIsRequestingCancel(false)
  , mRequestedToCommitOrCancel(false)
  , mWasNativeCompositionEndEventDiscarded(false)
{
}

void
TextComposition::Destroy()
{
  mPresContext = nullptr;
  mNode = nullptr;
  
  
}

bool
TextComposition::MatchesNativeContext(nsIWidget* aWidget) const
{
  return mNativeContext == aWidget->GetInputContext().mNativeIMEContext;
}

bool
TextComposition::IsValidStateForComposition(nsIWidget* aWidget) const
{
  return !Destroyed() && aWidget && !aWidget->Destroyed() &&
         mPresContext->GetPresShell() &&
         !mPresContext->GetPresShell()->IsDestroying();
}

bool
TextComposition::MaybeDispatchCompositionUpdate(
                   const WidgetCompositionEvent* aCompositionEvent)
{
  if (!IsValidStateForComposition(aCompositionEvent->widget)) {
    return false;
  }

  if (mLastData == aCompositionEvent->mData) {
    return true;
  }
  CloneAndDispatchAs(aCompositionEvent, NS_COMPOSITION_UPDATE);
  return IsValidStateForComposition(aCompositionEvent->widget);
}

BaseEventFlags
TextComposition::CloneAndDispatchAs(
                   const WidgetCompositionEvent* aCompositionEvent,
                   uint32_t aMessage,
                   nsEventStatus* aStatus,
                   EventDispatchingCallback* aCallBack)
{
  MOZ_ASSERT(IsValidStateForComposition(aCompositionEvent->widget),
             "Should be called only when it's safe to dispatch an event");

  WidgetCompositionEvent compositionEvent(aCompositionEvent->mFlags.mIsTrusted,
                                          aMessage, aCompositionEvent->widget);
  compositionEvent.time = aCompositionEvent->time;
  compositionEvent.timeStamp = aCompositionEvent->timeStamp;
  compositionEvent.mData = aCompositionEvent->mData;
  compositionEvent.mFlags.mIsSynthesizedForTests =
    aCompositionEvent->mFlags.mIsSynthesizedForTests;

  nsEventStatus dummyStatus = nsEventStatus_eConsumeNoDefault;
  nsEventStatus* status = aStatus ? aStatus : &dummyStatus;
  if (aMessage == NS_COMPOSITION_UPDATE) {
    mLastData = compositionEvent.mData;
  }
  EventDispatcher::Dispatch(mNode, mPresContext,
                            &compositionEvent, nullptr, status, aCallBack);
  return compositionEvent.mFlags;
}

void
TextComposition::OnCompositionEventDiscarded(
                   const WidgetCompositionEvent* aCompositionEvent)
{
  
  

  MOZ_ASSERT(aCompositionEvent->mFlags.mIsTrusted,
             "Shouldn't be called with untrusted event");

  
  
  
  
  if (!aCompositionEvent->CausesDOMCompositionEndEvent()) {
    return;
  }

  mWasNativeCompositionEndEventDiscarded = true;
}

void
TextComposition::DispatchCompositionEvent(
                   WidgetCompositionEvent* aCompositionEvent,
                   nsEventStatus* aStatus,
                   EventDispatchingCallback* aCallBack,
                   bool aIsSynthesized)
{
  if (aCompositionEvent->message == NS_COMPOSITION_COMMIT_AS_IS) {
    NS_ASSERTION(!aCompositionEvent->mRanges,
                 "mRanges of NS_COMPOSITION_COMMIT_AS_IS should be null");
    aCompositionEvent->mRanges = nullptr;
    NS_ASSERTION(aCompositionEvent->mData.IsEmpty(),
                 "mData of NS_COMPOSITION_COMMIT_AS_IS should be empty string");
    if (mLastData == IDEOGRAPHIC_SPACE) {
      
      
      
      aCompositionEvent->mData.Truncate();
    } else {
      aCompositionEvent->mData = mLastData;
    }
  }

  if (!IsValidStateForComposition(aCompositionEvent->widget)) {
    *aStatus = nsEventStatus_eConsumeNoDefault;
    return;
  }

  
  
  
  
  
  
  if (mRequestedToCommitOrCancel && !aIsSynthesized) {
    *aStatus = nsEventStatus_eConsumeNoDefault;
    return;
  }

  
  
  
  
  
  
  
  
  
  
  
  if (!aIsSynthesized && (mIsRequestingCommit || mIsRequestingCancel)) {
    nsString* committingData = nullptr;
    switch (aCompositionEvent->message) {
      case NS_COMPOSITION_END:
      case NS_COMPOSITION_CHANGE:
      case NS_COMPOSITION_COMMIT_AS_IS:
        committingData = &aCompositionEvent->mData;
        break;
      default:
        NS_WARNING("Unexpected event comes during committing or "
                   "canceling composition");
        break;
    }
    if (committingData) {
      if (mIsRequestingCommit && committingData->IsEmpty() &&
          mLastData != IDEOGRAPHIC_SPACE) {
        committingData->Assign(mLastData);
      } else if (mIsRequestingCancel && !committingData->IsEmpty()) {
        committingData->Truncate();
      }
    }
  }

  bool dispatchEvent = true;
  bool dispatchDOMTextEvent = aCompositionEvent->CausesDOMTextEvent();

  
  
  
  
  if (dispatchDOMTextEvent &&
      aCompositionEvent->message != NS_COMPOSITION_CHANGE &&
      !mIsComposing && mLastData == aCompositionEvent->mData) {
    dispatchEvent = dispatchDOMTextEvent = false;
  }

  if (dispatchDOMTextEvent) {
    if (!MaybeDispatchCompositionUpdate(aCompositionEvent)) {
      return;
    }
  }

  if (dispatchEvent) {
    
    
    
    
    if (dispatchDOMTextEvent &&
        aCompositionEvent->message != NS_COMPOSITION_CHANGE) {
      aCompositionEvent->mFlags =
        CloneAndDispatchAs(aCompositionEvent, NS_COMPOSITION_CHANGE,
                           aStatus, aCallBack);
    } else {
      EventDispatcher::Dispatch(mNode, mPresContext,
                                aCompositionEvent, nullptr, aStatus, aCallBack);
    }
  } else {
    *aStatus = nsEventStatus_eConsumeNoDefault;
  }

  if (!IsValidStateForComposition(aCompositionEvent->widget)) {
    return;
  }

  
  
  if (dispatchDOMTextEvent && !HasEditor()) {
    EditorWillHandleCompositionChangeEvent(aCompositionEvent);
    EditorDidHandleCompositionChangeEvent();
  }

  if (aCompositionEvent->CausesDOMCompositionEndEvent()) {
    
    if (aCompositionEvent->message != NS_COMPOSITION_END) {
      CloneAndDispatchAs(aCompositionEvent, NS_COMPOSITION_END);
    }
    MOZ_ASSERT(!mIsComposing, "Why is the editor still composing?");
    MOZ_ASSERT(!HasEditor(), "Why does the editor still keep to hold this?");
  }

  
  NotityUpdateComposition(aCompositionEvent);
}

void
TextComposition::NotityUpdateComposition(
                   const WidgetCompositionEvent* aCompositionEvent)
{
  nsEventStatus status;

  
  
  
  if (aCompositionEvent->message == NS_COMPOSITION_START) {
    nsCOMPtr<nsIWidget> widget = mPresContext->GetRootWidget();
    
    WidgetQueryContentEvent selectedTextEvent(true,
                                              NS_QUERY_SELECTED_TEXT,
                                              widget);
    widget->DispatchEvent(&selectedTextEvent, status);
    if (selectedTextEvent.mSucceeded) {
      mCompositionStartOffset = selectedTextEvent.mReply.mOffset;
    } else {
      
      NS_WARNING("Cannot get start offset of IME composition");
      mCompositionStartOffset = 0;
    }
    mCompositionTargetOffset = mCompositionStartOffset;
  } else if (aCompositionEvent->CausesDOMTextEvent()) {
    mCompositionTargetOffset =
      mCompositionStartOffset + aCompositionEvent->TargetClauseOffset();
  } else {
    return;
  }

  NotifyIME(NOTIFY_IME_OF_COMPOSITION_UPDATE);
}

void
TextComposition::DispatchCompositionEventRunnable(uint32_t aEventMessage,
                                                  const nsAString& aData,
                                                  bool aIsSynthesizingCommit)
{
  nsContentUtils::AddScriptRunner(
    new CompositionEventDispatcher(this, mNode, aEventMessage, aData,
                                   aIsSynthesizingCommit));
}

nsresult
TextComposition::RequestToCommit(nsIWidget* aWidget, bool aDiscard)
{
  
  
  
  
  if (mRequestedToCommitOrCancel) {
    return NS_OK;
  }

  nsRefPtr<TextComposition> kungFuDeathGrip(this);
  const nsAutoString lastData(mLastData);

  {
    AutoRestore<bool> saveRequestingCancel(mIsRequestingCancel);
    AutoRestore<bool> saveRequestingCommit(mIsRequestingCommit);
    if (aDiscard) {
      mIsRequestingCancel = true;
      mIsRequestingCommit = false;
    } else {
      mIsRequestingCancel = false;
      mIsRequestingCommit = true;
    }
    if (!mIsSynthesizedForTests) {
      
      
      nsresult rv =
        aWidget->NotifyIME(IMENotification(aDiscard ?
                                             REQUEST_TO_CANCEL_COMPOSITION :
                                             REQUEST_TO_COMMIT_COMPOSITION));
      if (rv == NS_ERROR_NOT_IMPLEMENTED) {
        return rv;
      }
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    } else {
      
      
      
      nsCOMPtr<nsIWidget> widget(aWidget);
      nsAutoString commitData(aDiscard ? EmptyString() : lastData);
      if (commitData == mLastData) {
        WidgetCompositionEvent commitEvent(true, NS_COMPOSITION_COMMIT_AS_IS,
                                           widget);
        commitEvent.mFlags.mIsSynthesizedForTests = true;
        nsEventStatus status = nsEventStatus_eIgnore;
        widget->DispatchEvent(&commitEvent, status);
      } else {
        WidgetCompositionEvent changeEvent(true, NS_COMPOSITION_CHANGE, widget);
        changeEvent.mData = commitData;
        changeEvent.mFlags.mIsSynthesizedForTests = true;

        MaybeDispatchCompositionUpdate(&changeEvent);

        
        
        
        if (IsValidStateForComposition(widget)) {
          nsEventStatus status = nsEventStatus_eIgnore;
          widget->DispatchEvent(&changeEvent, status);
        }

        if (IsValidStateForComposition(widget)) {
          nsEventStatus status = nsEventStatus_eIgnore;
          WidgetCompositionEvent endEvent(true, NS_COMPOSITION_END, widget);
          endEvent.mData = commitData;
          endEvent.mFlags.mIsSynthesizedForTests = true;
          widget->DispatchEvent(&endEvent, status);
        }
      }
    }
  }

  mRequestedToCommitOrCancel = true;

  
  if (Destroyed()) {
    return NS_OK;
  }

  
  nsAutoString data(aDiscard ? EmptyString() : lastData);
  if (data == mLastData) {
    DispatchCompositionEventRunnable(NS_COMPOSITION_COMMIT_AS_IS, EmptyString(),
                                     true);
    return NS_OK;
  }
  
  
  
  
  if (lastData != data || !data.IsEmpty()) {
    DispatchCompositionEventRunnable(NS_COMPOSITION_CHANGE, data, true);
  }
  DispatchCompositionEventRunnable(NS_COMPOSITION_END, data, true);

  return NS_OK;
}

nsresult
TextComposition::NotifyIME(IMEMessage aMessage)
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_AVAILABLE);
  return IMEStateManager::NotifyIME(aMessage, mPresContext);
}

void
TextComposition::EditorWillHandleCompositionChangeEvent(
                   const WidgetCompositionEvent* aCompositionChangeEvent)
{
  mIsComposing = aCompositionChangeEvent->IsComposing();
  mRanges = aCompositionChangeEvent->mRanges;
  mIsEditorHandlingEvent = true;

  MOZ_ASSERT(mLastData == aCompositionChangeEvent->mData,
    "The text of a compositionchange event must be same as previous data "
    "attribute value of the latest compositionupdate event");
}

void
TextComposition::EditorDidHandleCompositionChangeEvent()
{
  mString = mLastData;
  mIsEditorHandlingEvent = false;
}

void
TextComposition::StartHandlingComposition(nsIEditor* aEditor)
{
  MOZ_ASSERT(!HasEditor(), "There is a handling editor already");
  mEditorWeak = do_GetWeakReference(aEditor);
}

void
TextComposition::EndHandlingComposition(nsIEditor* aEditor)
{
#ifdef DEBUG
  nsCOMPtr<nsIEditor> editor = GetEditor();
  MOZ_ASSERT(editor == aEditor, "Another editor handled the composition?");
#endif 
  mEditorWeak = nullptr;
}

already_AddRefed<nsIEditor>
TextComposition::GetEditor() const
{
  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mEditorWeak);
  return editor.forget();
}

bool
TextComposition::HasEditor() const
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  return !!editor;
}





TextComposition::CompositionEventDispatcher::CompositionEventDispatcher(
                                               TextComposition* aComposition,
                                               nsINode* aEventTarget,
                                               uint32_t aEventMessage,
                                               const nsAString& aData,
                                               bool aIsSynthesizedEvent)
  : mTextComposition(aComposition)
  , mEventTarget(aEventTarget)
  , mEventMessage(aEventMessage)
  , mData(aData)
  , mIsSynthesizedEvent(aIsSynthesizedEvent)
{
}

NS_IMETHODIMP
TextComposition::CompositionEventDispatcher::Run()
{
  
  
  
  
  
  nsCOMPtr<nsIWidget> widget(mTextComposition->GetWidget());
  if (!mTextComposition->IsValidStateForComposition(widget)) {
    return NS_OK; 
  }

  nsRefPtr<nsPresContext> presContext = mTextComposition->mPresContext;
  nsEventStatus status = nsEventStatus_eIgnore;
  switch (mEventMessage) {
    case NS_COMPOSITION_START: {
      WidgetCompositionEvent compStart(true, NS_COMPOSITION_START, widget);
      WidgetQueryContentEvent selectedText(true, NS_QUERY_SELECTED_TEXT,
                                           widget);
      ContentEventHandler handler(presContext);
      handler.OnQuerySelectedText(&selectedText);
      NS_ASSERTION(selectedText.mSucceeded, "Failed to get selected text");
      compStart.mData = selectedText.mReply.mString;
      compStart.mFlags.mIsSynthesizedForTests =
        mTextComposition->IsSynthesizedForTests();
      IMEStateManager::DispatchCompositionEvent(mEventTarget, presContext,
                                                &compStart, &status, nullptr,
                                                mIsSynthesizedEvent);
      break;
    }
    case NS_COMPOSITION_END:
    case NS_COMPOSITION_CHANGE:
    case NS_COMPOSITION_COMMIT_AS_IS: {
      WidgetCompositionEvent compEvent(true, mEventMessage, widget);
      if (mEventMessage != NS_COMPOSITION_COMMIT_AS_IS) {
        compEvent.mData = mData;
      }
      compEvent.mFlags.mIsSynthesizedForTests =
        mTextComposition->IsSynthesizedForTests();
      IMEStateManager::DispatchCompositionEvent(mEventTarget, presContext,
                                                &compEvent, &status, nullptr,
                                                mIsSynthesizedEvent);
      break;
    }
    default:
      MOZ_CRASH("Unsupported event");
  }
  return NS_OK;
}





TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsIWidget* aWidget)
{
  for (index_type i = Length(); i > 0; --i) {
    if (ElementAt(i - 1)->MatchesNativeContext(aWidget)) {
      return i - 1;
    }
  }
  return NoIndex;
}

TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsPresContext* aPresContext)
{
  for (index_type i = Length(); i > 0; --i) {
    if (ElementAt(i - 1)->GetPresContext() == aPresContext) {
      return i - 1;
    }
  }
  return NoIndex;
}

TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsPresContext* aPresContext,
                              nsINode* aNode)
{
  index_type index = IndexOf(aPresContext);
  if (index == NoIndex) {
    return NoIndex;
  }
  nsINode* node = ElementAt(index)->GetEventTargetNode();
  return node == aNode ? index : NoIndex;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsIWidget* aWidget)
{
  index_type i = IndexOf(aWidget);
  return i != NoIndex ? ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsPresContext* aPresContext,
                                           nsINode* aNode)
{
  index_type i = IndexOf(aPresContext, aNode);
  return i != NoIndex ? ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionInContent(nsPresContext* aPresContext,
                                              nsIContent* aContent)
{
  
  for (index_type i = Length(); i > 0; --i) {
    nsINode* node = ElementAt(i - 1)->GetEventTargetNode();
    if (node && nsContentUtils::ContentIsDescendantOf(node, aContent)) {
      return ElementAt(i - 1);
    }
  }
  return nullptr;
}

} 
