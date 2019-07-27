




#ifndef APZCCallbackHandler_h__
#define APZCCallbackHandler_h__

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

class APZCCallbackHandler final : public mozilla::layers::ChromeProcessController
{
private:
    static StaticRefPtr<APZCCallbackHandler> sInstance;

    static NativePanZoomController::GlobalRef sNativePanZoomController;

private:
    APZCCallbackHandler(nsIWidget* aWidget, mozilla::layers::APZEventState* aAPZEventState)
      : mozilla::layers::ChromeProcessController(aWidget, aAPZEventState)
    {}

public:
    static void Initialize(nsIWidget* aWidget, mozilla::layers::APZEventState* aAPZEventState) {

        MOZ_ASSERT(!sInstance.get(), "APZCCallbackHandler.Initialize() got called twice");
        sInstance = new APZCCallbackHandler(aWidget, aAPZEventState);
    }

    static APZCCallbackHandler* GetInstance() {
        MOZ_ASSERT(sInstance.get(), "Calling APZCCallbackHandler.GetInstance() before it's initialization");
        return sInstance.get();
    }

    static NativePanZoomController::LocalRef SetNativePanZoomController(NativePanZoomController::Param obj);
    void NotifyDefaultPrevented(uint64_t aInputBlockId, bool aDefaultPrevented);

public: 

    void PostDelayedTask(Task* aTask, int aDelayMs) override;
};

} 
} 
} 

#endif
