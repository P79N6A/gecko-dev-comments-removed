




#ifndef WEBGL_CONTEXT_LOSS_HANDLER_H_
#define WEBGL_CONTEXT_LOSS_HANDLER_H_

#include "mozilla/DebugOnly.h"
#include "mozilla/RefPtr.h"
#include "mozilla/WeakPtr.h"
#include "nsCOMPtr.h"

class nsIThread;
class nsITimer;

namespace mozilla {
class WebGLContext;

class WebGLContextLossHandler
    : public RefCounted<WebGLContextLossHandler>
{
    WeakPtr<WebGLContext> mWeakWebGL;
    nsCOMPtr<nsITimer> mTimer;
    bool mIsTimerRunning;
    bool mShouldRunTimerAgain;
    bool mIsDisabled;
    DebugOnly<nsIThread*> mThread;

public:
    MOZ_DECLARE_REFCOUNTED_TYPENAME(WebGLContextLossHandler)

    explicit WebGLContextLossHandler(WebGLContext* aWebgl);
    ~WebGLContextLossHandler();

    void RunTimer();
    void DisableTimer();

protected:
    void StartTimer(unsigned long delayMS);
    static void StaticTimerCallback(nsITimer*, void* tempRefForTimer);
    void TimerCallback();
};

} 

#endif
