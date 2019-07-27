




#include "ChromeProcessController.h"

#include "MainThreadUtils.h"    
#include "base/message_loop.h"  
#include "mozilla/dom/Element.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "mozilla/layers/APZEventState.h"
#include "nsIDocument.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPresShell.h"
#include "nsLayoutUtils.h"
#include "nsView.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::widget;

ChromeProcessController::ChromeProcessController(nsIWidget* aWidget,
                                                 APZEventState* aAPZEventState)
  : mWidget(aWidget)
  , mAPZEventState(aAPZEventState)
  , mUILoop(MessageLoop::current())
{
  
  MOZ_ASSERT(NS_IsMainThread());

  mUILoop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this, &ChromeProcessController::InitializeRoot));
}

void
ChromeProcessController::InitializeRoot()
{
  
  
  
  
  
  
  
  nsIPresShell* presShell = GetPresShell();
  MOZ_ASSERT(presShell);
  MOZ_ASSERT(presShell->GetDocument());
  nsIContent* content = presShell->GetDocument()->GetDocumentElement();
  MOZ_ASSERT(content);
  uint32_t presShellId;
  FrameMetrics::ViewID viewId;
  if (APZCCallbackHelper::GetOrCreateScrollIdentifiers(content, &presShellId, &viewId)) {
    nsLayoutUtils::SetDisplayPortMargins(content, presShell, ScreenMargin(), 0,
        nsLayoutUtils::RepaintMode::DoNotRepaint);
  }
}

void
ChromeProcessController::RequestContentRepaint(const FrameMetrics& aFrameMetrics)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aFrameMetrics.GetScrollId() == FrameMetrics::NULL_SCROLL_ID) {
    return;
  }

  nsCOMPtr<nsIContent> targetContent = nsLayoutUtils::FindContentFor(aFrameMetrics.GetScrollId());
  if (targetContent) {
    FrameMetrics metrics = aFrameMetrics;
    APZCCallbackHelper::UpdateSubFrame(targetContent, metrics);
  }
}

void
ChromeProcessController::PostDelayedTask(Task* aTask, int aDelayMs)
{
  MessageLoop::current()->PostDelayedTask(FROM_HERE, aTask, aDelayMs);
}

void
ChromeProcessController::AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                                 const uint32_t& aScrollGeneration)
{
  APZCCallbackHelper::AcknowledgeScrollUpdate(aScrollId, aScrollGeneration);
}

void
ChromeProcessController::Destroy()
{
  if (MessageLoop::current() != mUILoop) {
    mUILoop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this, &ChromeProcessController::Destroy));
    return;
  }

  MOZ_ASSERT(MessageLoop::current() == mUILoop);
  mWidget = nullptr;
}

float
ChromeProcessController::GetPresShellResolution() const
{
  
  
  return 1.0f;
}

nsIPresShell*
ChromeProcessController::GetPresShell() const
{
  nsView* view = nsView::GetViewFor(mWidget);
  MOZ_ASSERT(view);
  return view->GetPresShell();
}

already_AddRefed<nsIDOMWindowUtils>
ChromeProcessController::GetDOMWindowUtils() const
{
  if (nsIDocument* doc = GetPresShell()->GetDocument()) {
    nsCOMPtr<nsIDOMWindowUtils> result = do_GetInterface(doc->GetWindow());
    return result.forget();
  }
  return nullptr;
}

void
ChromeProcessController::HandleSingleTap(const CSSPoint& aPoint,
                                         int32_t aModifiers,
                                         const ScrollableLayerGuid& aGuid)
{
  if (MessageLoop::current() != mUILoop) {
    mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromeProcessController::HandleSingleTap,
                          aPoint, aModifiers, aGuid));
    return;
  }

  mAPZEventState->ProcessSingleTap(aPoint, aGuid, GetPresShellResolution());
}

void
ChromeProcessController::HandleLongTap(const mozilla::CSSPoint& aPoint, int32_t aModifiers,
                                       const ScrollableLayerGuid& aGuid,
                                       uint64_t aInputBlockId)
{
  if (MessageLoop::current() != mUILoop) {
    mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromeProcessController::HandleLongTap,
                          aPoint, aModifiers, aGuid, aInputBlockId));
    return;
  }

  mAPZEventState->ProcessLongTap(GetDOMWindowUtils(), aPoint, aGuid,
      aInputBlockId, GetPresShellResolution());
}

void
ChromeProcessController::HandleLongTapUp(const CSSPoint& aPoint, int32_t aModifiers,
                                         const ScrollableLayerGuid& aGuid)
{
  if (MessageLoop::current() != mUILoop) {
    mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromeProcessController::HandleLongTapUp,
                          aPoint, aModifiers, aGuid));
    return;
  }

  mAPZEventState->ProcessLongTapUp(aPoint, aGuid, GetPresShellResolution());
}

void
ChromeProcessController::NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                              APZStateChange aChange,
                                              int aArg)
{
  if (MessageLoop::current() != mUILoop) {
    mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ChromeProcessController::NotifyAPZStateChange,
                          aGuid, aChange, aArg));
    return;
  }

  mAPZEventState->ProcessAPZStateChange(GetPresShell()->GetDocument(), aGuid.mScrollId, aChange, aArg);
}


