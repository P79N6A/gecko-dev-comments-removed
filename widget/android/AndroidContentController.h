




#ifndef AndroidContentController_h__
#define AndroidContentController_h__

#include "mozilla/layers/ChromeProcessController.h"
#include "mozilla/layers/APZEventState.h"
#include "mozilla/EventForwards.h"  
#include "mozilla/StaticPtr.h"
#include "mozilla/TimeStamp.h"
#include "GeneratedJNIWrappers.h"
#include "nsIDOMWindowUtils.h"
#include "nsTArray.h"

namespace mozilla {
namespace widget {
namespace android {

class AndroidContentController final : public mozilla::layers::ChromeProcessController
{
public:
    AndroidContentController(nsIWidget* aWidget, mozilla::layers::APZEventState* aAPZEventState)
      : mozilla::layers::ChromeProcessController(aWidget, aAPZEventState)
    {}

    
    void PostDelayedTask(Task* aTask, int aDelayMs) override;

public:
    static NativePanZoomController::LocalRef SetNativePanZoomController(NativePanZoomController::Param obj);
    static void NotifyDefaultPrevented(uint64_t aInputBlockId, bool aDefaultPrevented);

private:
    static NativePanZoomController::GlobalRef sNativePanZoomController;
};

} 
} 
} 

#endif
