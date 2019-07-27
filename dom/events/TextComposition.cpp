





#include "ContentEventHandler.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIEditor.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"

using namespace mozilla::widget;

namespace mozilla {





TextComposition::TextComposition(nsPresContext* aPresContext,
                                 nsINode* aNode,
                                 WidgetGUIEvent* aEvent)
  : mPresContext(aPresContext)
  , mNode(aNode)
  , mNativeContext(aEvent->widget->GetInputContext().mNativeIMEContext)
  , mCompositionStartOffset(0)
  , mCompositionTargetOffset(0)
  , mIsSynthesizedForTests(aEvent->mFlags.mIsSynthesizedForTests)
  , mIsComposing(false)
  , mIsEditorHandlingEvent(false)
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

void
TextComposition::DispatchEvent(WidgetGUIEvent* aEvent,
                               nsEventStatus* aStatus,
                               EventDispatchingCallback* aCallBack)
{
  if (aEvent->message == NS_COMPOSITION_UPDATE) {
    mLastData = aEvent->AsCompositionEvent()->data;
  }

  EventDispatcher::Dispatch(mNode, mPresContext,
                            aEvent, nullptr, aStatus, aCallBack);

  if (!mPresContext) {
    return;
  }

  
  
  if (aEvent->message == NS_TEXT_TEXT && !HasEditor()) {
    EditorWillHandleTextEvent(aEvent->AsTextEvent());
    EditorDidHandleTextEvent();
  }

#ifdef DEBUG
  else if (aEvent->message == NS_COMPOSITION_END) {
    MOZ_ASSERT(!mIsComposing, "Why is the editor still composing?");
    MOZ_ASSERT(!HasEditor(), "Why does the editor still keep to hold this?");
  }
#endif 

  
  NotityUpdateComposition(aEvent);
}

void
TextComposition::NotityUpdateComposition(WidgetGUIEvent* aEvent)
{
  nsEventStatus status;

  
  
  
  if (aEvent->message == NS_COMPOSITION_START) {
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
  } else if (aEvent->mClass != eTextEventClass) {
    return;
  } else {
    mCompositionTargetOffset =
      mCompositionStartOffset + aEvent->AsTextEvent()->TargetClauseOffset();
  }

  NotifyIME(NOTIFY_IME_OF_COMPOSITION_UPDATE);
}

void
TextComposition::DispatchCompositionEventRunnable(uint32_t aEventMessage,
                                                  const nsAString& aData)
{
  nsContentUtils::AddScriptRunner(
    new CompositionEventDispatcher(mPresContext, mNode,
                                   aEventMessage, aData));
}

void
TextComposition::SynthesizeCommit(bool aDiscard)
{
  nsRefPtr<TextComposition> kungFuDeathGrip(this);
  nsAutoString data(aDiscard ? EmptyString() : mLastData);
  if (mLastData != data) {
    DispatchCompositionEventRunnable(NS_COMPOSITION_UPDATE, data);
    DispatchCompositionEventRunnable(NS_TEXT_TEXT, data);
  }
  DispatchCompositionEventRunnable(NS_COMPOSITION_END, data);
}

nsresult
TextComposition::NotifyIME(IMEMessage aMessage)
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_AVAILABLE);
  return IMEStateManager::NotifyIME(aMessage, mPresContext);
}

void
TextComposition::EditorWillHandleTextEvent(const WidgetTextEvent* aTextEvent)
{
  mIsComposing = aTextEvent->IsComposing();
  mRanges = aTextEvent->mRanges;
  mIsEditorHandlingEvent = true;

  MOZ_ASSERT(mLastData == aTextEvent->theText,
    "The text of a text event must be same as previous data attribute value "
    "of the latest compositionupdate event");
}

void
TextComposition::EditorDidHandleTextEvent()
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
                                               nsPresContext* aPresContext,
                                               nsINode* aEventTarget,
                                               uint32_t aEventMessage,
                                               const nsAString& aData) :
  mPresContext(aPresContext), mEventTarget(aEventTarget),
  mEventMessage(aEventMessage), mData(aData)
{
  mWidget = mPresContext->GetRootWidget();
}

NS_IMETHODIMP
TextComposition::CompositionEventDispatcher::Run()
{
  if (!mPresContext->GetPresShell() ||
      mPresContext->GetPresShell()->IsDestroying()) {
    return NS_OK; 
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  switch (mEventMessage) {
    case NS_COMPOSITION_START: {
      WidgetCompositionEvent compStart(true, NS_COMPOSITION_START, mWidget);
      WidgetQueryContentEvent selectedText(true, NS_QUERY_SELECTED_TEXT,
                                           mWidget);
      ContentEventHandler handler(mPresContext);
      handler.OnQuerySelectedText(&selectedText);
      NS_ASSERTION(selectedText.mSucceeded, "Failed to get selected text");
      compStart.data = selectedText.mReply.mString;
      IMEStateManager::DispatchCompositionEvent(mEventTarget, mPresContext,
                                                &compStart, &status, nullptr);
      break;
    }
    case NS_COMPOSITION_UPDATE:
    case NS_COMPOSITION_END: {
      WidgetCompositionEvent compEvent(true, mEventMessage, mWidget);
      compEvent.data = mData;
      IMEStateManager::DispatchCompositionEvent(mEventTarget, mPresContext,
                                                &compEvent, &status, nullptr);
      break;
    }
    case NS_TEXT_TEXT: {
      WidgetTextEvent textEvent(true, NS_TEXT_TEXT, mWidget);
      textEvent.theText = mData;
      IMEStateManager::DispatchCompositionEvent(mEventTarget, mPresContext,
                                                &textEvent, &status, nullptr);
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
