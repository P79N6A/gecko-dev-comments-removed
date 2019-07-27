




#include "ParentProcessController.h"
#include "nsIContent.h"
#include "nsLayoutUtils.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "base/message_loop.h"

namespace mozilla {
namespace widget {

void
ParentProcessController::RequestContentRepaint(const FrameMetrics& aFrameMetrics)
{
    MOZ_ASSERT(NS_IsMainThread());

    if (aFrameMetrics.GetScrollId() == FrameMetrics::NULL_SCROLL_ID) {
        return;
    }

    nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aFrameMetrics.GetScrollId());
    if (content) {
        FrameMetrics metrics = aFrameMetrics;
        mozilla::layers::APZCCallbackHelper::UpdateSubFrame(content, metrics);
    }
}

void
ParentProcessController::AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                                 const uint32_t& aScrollGeneration)
{
    mozilla::layers::APZCCallbackHelper::AcknowledgeScrollUpdate(aScrollId, aScrollGeneration);
}

void
ParentProcessController::PostDelayedTask(Task* aTask, int aDelayMs)
{
    MessageLoop::current()->PostDelayedTask(FROM_HERE, aTask, aDelayMs);
}

}
}
