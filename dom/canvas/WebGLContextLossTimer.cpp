




#include "WebGLContext.h"

using namespace mozilla;

 void
WebGLContext::ContextLossCallbackStatic(nsITimer* timer, void* thisPointer)
{
    (void)timer;
    WebGLContext* context = static_cast<WebGLContext*>(thisPointer);

    context->TerminateContextLossTimer();

    context->UpdateContextLossStatus();
}

void
WebGLContext::RunContextLossTimer()
{
    
    
    
    
    if (mContextLossTimerRunning) {
        mRunContextLossTimerAgain = true;
        return;
    }
    mContextRestorer->InitWithFuncCallback(ContextLossCallbackStatic,
                                           static_cast<void*>(this),
                                           1000,
                                           nsITimer::TYPE_ONE_SHOT);
    mContextLossTimerRunning = true;
    mRunContextLossTimerAgain = false;
}

void
WebGLContext::TerminateContextLossTimer()
{
    if (!mContextLossTimerRunning)
        return;

    mContextRestorer->Cancel();
    mContextLossTimerRunning = false;

    if (mRunContextLossTimerAgain) {
        RunContextLossTimer();
    }
}
