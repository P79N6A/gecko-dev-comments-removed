




#include "AndroidContentController.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "base/message_loop.h"
#include "nsWindow.h"
#include "AndroidBridge.h"

using mozilla::layers::APZCTreeManager;

namespace mozilla {
namespace widget {
namespace android {

NativePanZoomController::GlobalRef AndroidContentController::sNativePanZoomController = nullptr;

NativePanZoomController::LocalRef
AndroidContentController::SetNativePanZoomController(NativePanZoomController::Param obj)
{
    NativePanZoomController::LocalRef old = sNativePanZoomController;
    sNativePanZoomController = obj;
    return old;
}

void
AndroidContentController::NotifyDefaultPrevented(uint64_t aInputBlockId,
                                                 bool aDefaultPrevented)
{
    if (!AndroidBridge::IsJavaUiThread()) {
        
        
        
        AndroidBridge::Bridge()->PostTaskToUiThread(NewRunnableFunction(
            &AndroidContentController::NotifyDefaultPrevented,
            aInputBlockId, aDefaultPrevented), 0);
        return;
    }

    MOZ_ASSERT(AndroidBridge::IsJavaUiThread());
    APZCTreeManager* controller = nsWindow::GetAPZCTreeManager();
    if (controller) {
        controller->ContentReceivedInputBlock(aInputBlockId, aDefaultPrevented);
    }
}

void
AndroidContentController::PostDelayedTask(Task* aTask, int aDelayMs)
{
    AndroidBridge::Bridge()->PostTaskToUiThread(aTask, aDelayMs);
}

} 
} 
} 
