




#ifndef APZCCallbackHandler_h__
#define APZCCallbackHandler_h__

#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/EventForwards.h"  
#include "mozilla/StaticPtr.h"
#include "mozilla/TimeStamp.h"
#include "GeneratedJNIWrappers.h"
#include "nsIDOMWindowUtils.h"
#include "nsTArray.h"

namespace mozilla {
namespace widget {
namespace android {

class APZCCallbackHandler final : public mozilla::layers::GeckoContentController
{
private:
    static StaticRefPtr<APZCCallbackHandler> sInstance;
    NativePanZoomController::GlobalRef mNativePanZoomController;

private:
    APZCCallbackHandler()
      : mNativePanZoomController(nullptr)
    {}

    nsIDOMWindowUtils* GetDOMWindowUtils();

public:
    static APZCCallbackHandler* GetInstance() {
        if (sInstance.get() == nullptr) {
            sInstance = new APZCCallbackHandler();
        }
        return sInstance.get();
    }

    NativePanZoomController::LocalRef SetNativePanZoomController(NativePanZoomController::Param obj);
    void NotifyDefaultPrevented(uint64_t aInputBlockId, bool aDefaultPrevented);

public: 
    void RequestContentRepaint(const mozilla::layers::FrameMetrics& aFrameMetrics) override;
    void RequestFlingSnap(const mozilla::layers::FrameMetrics::ViewID& aScrollId,
                          const mozilla::CSSPoint& aDestination) override;
    void AcknowledgeScrollUpdate(const mozilla::layers::FrameMetrics::ViewID& aScrollId,
                                 const uint32_t& aScrollGeneration) override;
    void HandleDoubleTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                         const mozilla::layers::ScrollableLayerGuid& aGuid) override;
    void HandleSingleTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                         const mozilla::layers::ScrollableLayerGuid& aGuid) override;
    void HandleLongTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                       const mozilla::layers::ScrollableLayerGuid& aGuid,
                       uint64_t aInputBlockId) override;
    void SendAsyncScrollDOMEvent(bool aIsRoot, const mozilla::CSSRect& aContentRect,
                                 const mozilla::CSSSize& aScrollableSize) override;
    void PostDelayedTask(Task* aTask, int aDelayMs) override;
};

} 
} 
} 

#endif
