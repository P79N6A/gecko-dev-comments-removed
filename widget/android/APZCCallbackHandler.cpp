




#include "APZCCallbackHandler.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "nsAppShell.h"
#include "nsLayoutUtils.h"
#include "nsPrintfCString.h"
#include "nsThreadUtils.h"
#include "base/message_loop.h"
#include "nsWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "AndroidBridge.h"

using mozilla::layers::APZCCallbackHelper;
using mozilla::layers::APZCTreeManager;
using mozilla::layers::FrameMetrics;
using mozilla::layers::ScrollableLayerGuid;

namespace mozilla {
namespace widget {
namespace android {

StaticRefPtr<APZCCallbackHandler> APZCCallbackHandler::sInstance;

NativePanZoomController*
APZCCallbackHandler::SetNativePanZoomController(jobject obj)
{
    NativePanZoomController* old = mNativePanZoomController;
    mNativePanZoomController = NativePanZoomController::Wrap(obj);
    return old;
}

void
APZCCallbackHandler::NotifyDefaultPrevented(const ScrollableLayerGuid& aGuid,
                                            bool aDefaultPrevented)
{
    if (NS_IsMainThread()) {
        
        
        
        AndroidBridge::Bridge()->PostTaskToUiThread(NewRunnableMethod(
            this, &APZCCallbackHandler::NotifyDefaultPrevented,
            aGuid, aDefaultPrevented), 0);
        return;
    }

    
    APZCTreeManager* controller = nsWindow::GetAPZCTreeManager();
    if (controller) {
        controller->ContentReceivedTouch(aGuid, aDefaultPrevented);
    }
}

nsIDOMWindowUtils*
APZCCallbackHandler::GetDOMWindowUtils()
{
    nsIAndroidBrowserApp* browserApp = nullptr;
    if (!nsAppShell::gAppShell) {
        return nullptr;
    }
    nsAppShell::gAppShell->GetBrowserApp(&browserApp);
    if (!browserApp) {
        return nullptr;
    }
    nsIBrowserTab* tab = nullptr;
    if (browserApp->GetSelectedTab(&tab) != NS_OK) {
        return nullptr;
    }
    nsIDOMWindow* window = nullptr;
    if (tab->GetWindow(&window) != NS_OK) {
        return nullptr;
    }
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
    return utils.get();
}

void
APZCCallbackHandler::RequestContentRepaint(const FrameMetrics& aFrameMetrics)
{
    if (!NS_IsMainThread()) {
        NS_DispatchToMainThread(NS_NewRunnableMethodWithArg<FrameMetrics>(
            this, &APZCCallbackHandler::RequestContentRepaint, aFrameMetrics));
        return;
    }

    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aFrameMetrics.GetScrollId() != FrameMetrics::NULL_SCROLL_ID);

    if (aFrameMetrics.mIsRoot) {
        nsIDOMWindowUtils* utils = GetDOMWindowUtils();
        if (utils && APZCCallbackHelper::HasValidPresShellId(utils, aFrameMetrics)) {
            FrameMetrics metrics = aFrameMetrics;
            APZCCallbackHelper::UpdateRootFrame(utils, metrics);
            APZCCallbackHelper::UpdateCallbackTransform(aFrameMetrics, metrics);
        }
    } else {
        
        
        nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aFrameMetrics.GetScrollId());
        if (content) {
            FrameMetrics newSubFrameMetrics(aFrameMetrics);
            APZCCallbackHelper::UpdateSubFrame(content, newSubFrameMetrics);
            APZCCallbackHelper::UpdateCallbackTransform(aFrameMetrics, newSubFrameMetrics);
        }
    }
}

void
APZCCallbackHandler::AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                             const uint32_t& aScrollGeneration)
{
    APZCCallbackHelper::AcknowledgeScrollUpdate(aScrollId, aScrollGeneration);
}

void
APZCCallbackHandler::HandleDoubleTap(const CSSPoint& aPoint,
                                     int32_t aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid)
{
    nsCString data = nsPrintfCString("{ \"x\": %d, \"y\": %d }", aPoint.x, aPoint.y);
    nsAppShell::gAppShell->PostEvent(AndroidGeckoEvent::MakeBroadcastEvent(
            NS_LITERAL_CSTRING("Gesture:DoubleTap"), data));
}

void
APZCCallbackHandler::HandleSingleTap(const CSSPoint& aPoint,
                                     int32_t aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid)
{
    
    nsCString data = nsPrintfCString("{ \"x\": %d, \"y\": %d }", aPoint.x, aPoint.y);
    nsAppShell::gAppShell->PostEvent(AndroidGeckoEvent::MakeBroadcastEvent(
            NS_LITERAL_CSTRING("Gesture:SingleTap"), data));
}

void
APZCCallbackHandler::HandleLongTap(const CSSPoint& aPoint,
                                   int32_t aModifiers,
                                   const mozilla::layers::ScrollableLayerGuid& aGuid)
{
    
    nsCString data = nsPrintfCString("{ \"x\": %d, \"y\": %d }", aPoint.x, aPoint.y);
    nsAppShell::gAppShell->PostEvent(AndroidGeckoEvent::MakeBroadcastEvent(
            NS_LITERAL_CSTRING("Gesture:LongPress"), data));
}

void
APZCCallbackHandler::HandleLongTapUp(const CSSPoint& aPoint,
                                     int32_t aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid)
{
    HandleSingleTap(aPoint, aModifiers, aGuid);
}

void
APZCCallbackHandler::SendAsyncScrollDOMEvent(bool aIsRoot,
                                             const CSSRect& aContentRect,
                                             const CSSSize& aScrollableSize)
{
    
    
}

void
APZCCallbackHandler::PostDelayedTask(Task* aTask, int aDelayMs)
{
    AndroidBridge::Bridge()->PostTaskToUiThread(aTask, aDelayMs);
}

} 
} 
} 
