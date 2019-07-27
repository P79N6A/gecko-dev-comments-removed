




#ifndef __mozilla_widget_DynamicToolbarController_h__
#define __mozilla_widget_DynamicToolbarController_h__

#include "mozilla/layers/GeckoContentController.h"

namespace mozilla {
namespace widget {

class ParentProcessController : public mozilla::layers::GeckoContentController
{
    typedef mozilla::layers::FrameMetrics FrameMetrics;
    typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;

public:
    virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
    virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aViewId,
                                         const uint32_t& aScrollGeneration) MOZ_OVERRIDE;
    virtual void PostDelayedTask(Task* aTask, int aDelayMs) MOZ_OVERRIDE;

    
    virtual void HandleDoubleTap(const CSSPoint& aPoint,
                                 int32_t aModifiers,
                                 const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE {}
    virtual void HandleSingleTap(const CSSPoint& aPoint,
                                 int32_t aModifiers,
                                 const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE {}
    virtual void HandleLongTap(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid,
                               uint64_t aInputBlockId) MOZ_OVERRIDE {}
    virtual void HandleLongTapUp(const CSSPoint& aPoint,
                                 int32_t aModifiers,
                                 const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE {}

    virtual void SendAsyncScrollDOMEvent(bool aIsRoot,
                                         const CSSRect &aContentRect,
                                         const CSSSize &aScrollableSize) MOZ_OVERRIDE {}
};

}
}

#endif 
