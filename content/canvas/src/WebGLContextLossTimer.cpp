




#include "WebGLContext.h"

using namespace mozilla;

 void
WebGLContext::RobustnessTimerCallbackStatic(nsITimer* timer, void *thisPointer) {
    static_cast<WebGLContext*>(thisPointer)->RobustnessTimerCallback(timer);
}

void
WebGLContext::SetupContextLossTimer() {
    
    
    
    
    if (mContextLossTimerRunning) {
        mDrawSinceContextLossTimerSet = true;
        return;
    }

    mContextRestorer->InitWithFuncCallback(RobustnessTimerCallbackStatic,
                                            static_cast<void*>(this),
                                            1000,
                                            nsITimer::TYPE_ONE_SHOT);
    mContextLossTimerRunning = true;
    mDrawSinceContextLossTimerSet = false;
}

void
WebGLContext::TerminateContextLossTimer() {
    if (mContextLossTimerRunning) {
        mContextRestorer->Cancel();
        mContextLossTimerRunning = false;
    }
}
