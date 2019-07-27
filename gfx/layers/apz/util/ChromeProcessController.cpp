




#include "ChromeProcessController.h"

#include "MainThreadUtils.h"    
#include "base/message_loop.h"  
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "nsLayoutUtils.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::widget;

ChromeProcessController::ChromeProcessController(nsIWidget* aWidget)
  : mWidget(aWidget)
  , mUILoop(MessageLoop::current())
{
  
  MOZ_ASSERT(NS_IsMainThread());
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
