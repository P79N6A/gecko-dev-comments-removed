




#include "APZCCallbackHandler.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "base/message_loop.h"
#include "nsWindow.h"
#include "AndroidBridge.h"

using mozilla::layers::APZCTreeManager;

namespace mozilla {
namespace widget {
namespace android {

NativePanZoomController::GlobalRef APZCCallbackHandler::sNativePanZoomController = nullptr;

NativePanZoomController::LocalRef
APZCCallbackHandler::SetNativePanZoomController(NativePanZoomController::Param obj)
{
    NativePanZoomController::LocalRef old = sNativePanZoomController;
    sNativePanZoomController = obj;
    return old;
}

void
APZCCallbackHandler::NotifyDefaultPrevented(uint64_t aInputBlockId,
                                            bool aDefaultPrevented)
{
    if (!AndroidBridge::IsJavaUiThread()) {
        
        
        
        AndroidBridge::Bridge()->PostTaskToUiThread(NewRunnableFunction(
            &APZCCallbackHandler::NotifyDefaultPrevented,
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
APZCCallbackHandler::PostDelayedTask(Task* aTask, int aDelayMs)
{
    AndroidBridge::Bridge()->PostTaskToUiThread(aTask, aDelayMs);
}

} 
} 
} 
