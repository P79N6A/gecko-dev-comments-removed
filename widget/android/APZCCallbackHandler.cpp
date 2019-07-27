




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
#include "nsIContent.h"

using mozilla::layers::APZCCallbackHelper;
using mozilla::layers::APZCTreeManager;
using mozilla::layers::FrameMetrics;
using mozilla::layers::ScrollableLayerGuid;

namespace mozilla {
namespace widget {
namespace android {

StaticRefPtr<APZCCallbackHandler> APZCCallbackHandler::sInstance;

NativePanZoomController::LocalRef
APZCCallbackHandler::SetNativePanZoomController(NativePanZoomController::Param obj)
{
    NativePanZoomController::LocalRef old = mNativePanZoomController;
    mNativePanZoomController = obj;
    return old;
}

void
APZCCallbackHandler::NotifyDefaultPrevented(uint64_t aInputBlockId,
                                            bool aDefaultPrevented)
{
    if (!AndroidBridge::IsJavaUiThread()) {
        
        
        
        AndroidBridge::Bridge()->PostTaskToUiThread(NewRunnableMethod(
            this, &APZCCallbackHandler::NotifyDefaultPrevented,
            aInputBlockId, aDefaultPrevented), 0);
        return;
    }

    MOZ_ASSERT(AndroidBridge::IsJavaUiThread());
    APZCTreeManager* controller = nsWindow::GetAPZCTreeManager();
    if (controller) {
        controller->ContentReceivedInputBlock(aInputBlockId, aDefaultPrevented);
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
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aFrameMetrics.GetScrollId() != FrameMetrics::NULL_SCROLL_ID);

    if (aFrameMetrics.GetIsRoot()) {
        nsIDOMWindowUtils* utils = GetDOMWindowUtils();
        if (utils && APZCCallbackHelper::HasValidPresShellId(utils, aFrameMetrics)) {
            FrameMetrics metrics = aFrameMetrics;
            APZCCallbackHelper::UpdateRootFrame(utils, metrics);
        }
    } else {
        
        
        nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aFrameMetrics.GetScrollId());
        if (content) {
            FrameMetrics newSubFrameMetrics(aFrameMetrics);
            APZCCallbackHelper::UpdateSubFrame(content, newSubFrameMetrics);
        }
    }
}

void
APZCCallbackHandler::RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                      const mozilla::CSSPoint& aDestination)
{
    APZCCallbackHelper::RequestFlingSnap(aScrollId, aDestination);
}

void
APZCCallbackHandler::AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                             const uint32_t& aScrollGeneration)
{
    APZCCallbackHelper::AcknowledgeScrollUpdate(aScrollId, aScrollGeneration);
}

void
APZCCallbackHandler::HandleDoubleTap(const CSSPoint& aPoint,
                                     Modifiers aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid)
{
    CSSIntPoint point = RoundedToInt(aPoint);
    nsCString data = nsPrintfCString("{ \"x\": %d, \"y\": %d }", point.x, point.y);
    nsAppShell::gAppShell->PostEvent(AndroidGeckoEvent::MakeBroadcastEvent(
            NS_LITERAL_CSTRING("Gesture:DoubleTap"), data));
}

void
APZCCallbackHandler::HandleSingleTap(const CSSPoint& aPoint,
                                     Modifiers aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid)
{
    
    CSSIntPoint point = RoundedToInt(aPoint);
    nsCString data = nsPrintfCString("{ \"x\": %d, \"y\": %d }", point.x, point.y);
    nsAppShell::gAppShell->PostEvent(AndroidGeckoEvent::MakeBroadcastEvent(
            NS_LITERAL_CSTRING("Gesture:SingleTap"), data));
}

void
APZCCallbackHandler::HandleLongTap(const CSSPoint& aPoint,
                                   Modifiers aModifiers,
                                   const mozilla::layers::ScrollableLayerGuid& aGuid,
                                   uint64_t aInputBlockId)
{
    
    CSSIntPoint point = RoundedToInt(aPoint);
    nsCString data = nsPrintfCString("{ \"x\": %d, \"y\": %d }", point.x, point.y);
    nsAppShell::gAppShell->PostEvent(AndroidGeckoEvent::MakeBroadcastEvent(
            NS_LITERAL_CSTRING("Gesture:LongPress"), data));
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
