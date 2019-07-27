




#include "WebGLContextLossHandler.h"

#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "WebGLContext.h"

namespace mozilla {

WebGLContextLossHandler::WebGLContextLossHandler(WebGLContext* webgl)
    : mWeakWebGL(webgl)
    , mTimer(do_CreateInstance(NS_TIMER_CONTRACTID))
    , mIsTimerRunning(false)
    , mShouldRunTimerAgain(false)
    , mIsDisabled(false)
#ifdef DEBUG
    , mThread(NS_GetCurrentThread())
#endif
{
}

WebGLContextLossHandler::~WebGLContextLossHandler()
{
    MOZ_ASSERT(!mIsTimerRunning);
}

void
WebGLContextLossHandler::StartTimer(unsigned long delayMS)
{
    
    
    this->AddRef();

    mTimer->InitWithFuncCallback(StaticTimerCallback,
                                 static_cast<void*>(this),
                                 delayMS,
                                 nsITimer::TYPE_ONE_SHOT);
}

 void
WebGLContextLossHandler::StaticTimerCallback(nsITimer*,
                                             void* voidHandler)
{
    typedef WebGLContextLossHandler T;
    T* handler = static_cast<T*>(voidHandler);

    handler->TimerCallback();

    
    handler->Release();
}

void
WebGLContextLossHandler::TimerCallback()
{
    MOZ_ASSERT(NS_GetCurrentThread() == mThread);

    if (mIsDisabled)
        return;

    MOZ_ASSERT(mIsTimerRunning);
    mIsTimerRunning = false;

    
    
    
    if (mShouldRunTimerAgain) {
        RunTimer();
        MOZ_ASSERT(mIsTimerRunning);
    }

    if (mWeakWebGL) {
        mWeakWebGL->UpdateContextLossStatus();
    }
}

void
WebGLContextLossHandler::RunTimer()
{
    MOZ_ASSERT(!mIsDisabled);

    
    
    
    
    if (mIsTimerRunning) {
        mShouldRunTimerAgain = true;
        return;
    }

    StartTimer(1000);

    mIsTimerRunning = true;
    mShouldRunTimerAgain = false;
}

void
WebGLContextLossHandler::DisableTimer()
{
    if (!mIsDisabled)
        return;

    mIsDisabled = true;

    
    
    

    

    if (!mIsTimerRunning)
        return;

    mTimer->SetDelay(0);
}

} 
