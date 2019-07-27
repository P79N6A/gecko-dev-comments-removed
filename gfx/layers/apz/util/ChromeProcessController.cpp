




#include "ChromeProcessController.h"

#include "MainThreadUtils.h"    
#include "base/message_loop.h"  
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsLayoutUtils.h"
#include "nsView.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::widget;

ChromeProcessController::ChromeProcessController(nsIWidget* aWidget)
  : mWidget(aWidget)
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
  
  
  
  
  
  
  
  nsView* view = nsView::GetViewFor(mWidget);
  MOZ_ASSERT(view);
  nsIPresShell* presShell = view->GetPresShell();
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

  LayoutDevicePoint point =
      APZCCallbackHelper::ApplyCallbackTransform(aPoint, aGuid, GetPresShellResolution())
    * mWidget->GetDefaultScale();

  APZCCallbackHelper::FireSingleTapEvent(point, mWidget);
}
