




#ifndef __mozilla_widget_DynamicToolbarController_h__
#define __mozilla_widget_DynamicToolbarController_h__

#include "mozilla/layers/GeckoContentController.h"

namespace mozilla {
namespace widget {

class ParentProcessController : public mozilla::layers::GeckoContentController
{
    typedef mozilla::layers::FrameMetrics FrameMetrics;

public:
    virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
    virtual void PostDelayedTask(Task* aTask, int aDelayMs) MOZ_OVERRIDE;

    
    virtual void HandleDoubleTap(const CSSIntPoint& aPoint, int32_t aModifiers) MOZ_OVERRIDE {}
    virtual void HandleSingleTap(const CSSIntPoint& aPoint, int32_t aModifiers) MOZ_OVERRIDE {}
    virtual void HandleLongTap(const CSSIntPoint& aPoint, int32_t aModifiers) MOZ_OVERRIDE {}
    virtual void SendAsyncScrollDOMEvent(bool aIsRoot,
                                         const CSSRect &aContentRect,
                                         const CSSSize &aScrollableSize) MOZ_OVERRIDE {}
};

}
}

#endif 
