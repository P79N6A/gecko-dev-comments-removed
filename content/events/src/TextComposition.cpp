





#include "TextComposition.h"
#include "nsContentEventHandler.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIMEStateManager.h"
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsPresContext.h"

namespace mozilla {





TextComposition::TextComposition(nsPresContext* aPresContext,
                                 nsINode* aNode,
                                 nsGUIEvent* aEvent) :
  mPresContext(aPresContext), mNode(aNode),
  mNativeContext(aEvent->widget->GetInputContext().mNativeIMEContext),
  mIsSynthesizedForTests(aEvent->mFlags.mIsSynthesizedForTests)
{
}

TextComposition::TextComposition(const TextComposition& aOther)
{
  mNativeContext = aOther.mNativeContext;
  mPresContext = aOther.mPresContext;
  mNode = aOther.mNode;
  mLastData = aOther.mLastData;
  mIsSynthesizedForTests = aOther.mIsSynthesizedForTests;
}

bool
TextComposition::MatchesNativeContext(nsIWidget* aWidget) const
{
  return mNativeContext == aWidget->GetInputContext().mNativeIMEContext;
}

bool
TextComposition::MatchesEventTarget(nsPresContext* aPresContext,
                                    nsINode* aNode) const
{
  return mPresContext == aPresContext && mNode == aNode;
}

void
TextComposition::DispatchEvent(nsGUIEvent* aEvent,
                               nsEventStatus* aStatus,
                               nsDispatchingCallback* aCallBack)
{
  if (aEvent->message == NS_COMPOSITION_UPDATE) {
    mLastData = static_cast<nsCompositionEvent*>(aEvent)->data;
  }

  nsEventDispatcher::Dispatch(mNode, mPresContext,
                              aEvent, nullptr, aStatus, aCallBack);
}

void
TextComposition::DispatchCompsotionEventRunnable(uint32_t aEventMessage,
                                                 const nsAString& aData)
{
  nsContentUtils::AddScriptRunner(
    new CompositionEventDispatcher(mPresContext, mNode,
                                   aEventMessage, aData));
}

void
TextComposition::SynthesizeCommit(bool aDiscard)
{
  
  
  TextComposition composition = *this;
  nsAutoString data(aDiscard ? EmptyString() : composition.mLastData);
  if (composition.mLastData != data) {
    composition.DispatchCompsotionEventRunnable(NS_COMPOSITION_UPDATE, data);
    composition.DispatchCompsotionEventRunnable(NS_TEXT_TEXT, data);
  }
  composition.DispatchCompsotionEventRunnable(NS_COMPOSITION_END, data);
}

nsresult
TextComposition::NotifyIME(widget::NotificationToIME aNotification)
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_AVAILABLE);
  return nsIMEStateManager::NotifyIME(aNotification, mPresContext);
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
      nsCompositionEvent compStart(true, NS_COMPOSITION_START, mWidget);
      nsQueryContentEvent selectedText(true, NS_QUERY_SELECTED_TEXT, mWidget);
      nsContentEventHandler handler(mPresContext);
      handler.OnQuerySelectedText(&selectedText);
      NS_ASSERTION(selectedText.mSucceeded, "Failed to get selected text");
      compStart.data = selectedText.mReply.mString;
      nsIMEStateManager::DispatchCompositionEvent(mEventTarget, mPresContext,
                                                  &compStart, &status, nullptr);
      break;
    }
    case NS_COMPOSITION_UPDATE:
    case NS_COMPOSITION_END: {
      nsCompositionEvent compEvent(true, mEventMessage, mWidget);
      compEvent.data = mData;
      nsIMEStateManager::DispatchCompositionEvent(mEventTarget, mPresContext,
                                                  &compEvent, &status, nullptr);
      break;
    }
    case NS_TEXT_TEXT: {
      nsTextEvent textEvent(true, NS_TEXT_TEXT, mWidget);
      textEvent.theText = mData;
      nsIMEStateManager::DispatchCompositionEvent(mEventTarget, mPresContext,
                                                  &textEvent, &status, nullptr);
      break;
    }
    default:
      MOZ_NOT_REACHED("Unsupported event");
      break;
  }
  return NS_OK;
}





TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsIWidget* aWidget)
{
  for (index_type i = Length(); i > 0; --i) {
    if (ElementAt(i - 1).MatchesNativeContext(aWidget)) {
      return i - 1;
    }
  }
  return NoIndex;
}

TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsPresContext* aPresContext)
{
  for (index_type i = Length(); i > 0; --i) {
    if (ElementAt(i - 1).GetPresContext() == aPresContext) {
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
  nsINode* node = ElementAt(index).GetEventTargetNode();
  return node == aNode ? index : NoIndex;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsIWidget* aWidget)
{
  index_type i = IndexOf(aWidget);
  return i != NoIndex ? &ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsPresContext* aPresContext)
{
  index_type i = IndexOf(aPresContext);
  return i != NoIndex ? &ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsPresContext* aPresContext,
                                           nsINode* aNode)
{
  index_type i = IndexOf(aPresContext, aNode);
  return i != NoIndex ? &ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionInContent(nsPresContext* aPresContext,
                                              nsIContent* aContent)
{
  
  for (index_type i = Length(); i > 0; --i) {
    nsINode* node = ElementAt(i - 1).GetEventTargetNode();
    if (node && nsContentUtils::ContentIsDescendantOf(node, aContent)) {
      return &ElementAt(i - 1);
    }
  }
  return nullptr;
}

} 
