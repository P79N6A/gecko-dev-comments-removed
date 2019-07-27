




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

class APZCCallbackHandler MOZ_FINAL : public mozilla::layers::GeckoContentController
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
    void RequestContentRepaint(const mozilla::layers::FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
    void RequestFlingSnap(const mozilla::layers::FrameMetrics::ViewID& aScrollId,
                          const mozilla::CSSPoint& aDestination) MOZ_OVERRIDE;
    void AcknowledgeScrollUpdate(const mozilla::layers::FrameMetrics::ViewID& aScrollId,
                                 const uint32_t& aScrollGeneration) MOZ_OVERRIDE;
    void HandleDoubleTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                         const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    void HandleSingleTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                         const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    void HandleLongTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                       const mozilla::layers::ScrollableLayerGuid& aGuid,
                       uint64_t aInputBlockId) MOZ_OVERRIDE;
    void SendAsyncScrollDOMEvent(bool aIsRoot, const mozilla::CSSRect& aContentRect,
                                 const mozilla::CSSSize& aScrollableSize) MOZ_OVERRIDE;
    void PostDelayedTask(Task* aTask, int aDelayMs) MOZ_OVERRIDE;
};

} 
} 
} 

#endif
