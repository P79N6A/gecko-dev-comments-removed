




#ifndef WEBGL_CONTEXT_LOSS_HANDLER_H_
#define WEBGL_CONTEXT_LOSS_HANDLER_H_

#include "mozilla/DebugOnly.h"
#include "mozilla/WeakPtr.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"

class nsIThread;
class nsITimer;

namespace mozilla {
class WebGLContext;

class WebGLContextLossHandler
{
    WeakPtr<WebGLContext> mWeakWebGL;
    nsCOMPtr<nsITimer> mTimer;
    bool mIsTimerRunning;
    bool mShouldRunTimerAgain;
    bool mIsDisabled;
    DebugOnly<nsIThread*> mThread;

public:
    NS_INLINE_DECL_REFCOUNTING(WebGLContextLossHandler)

    explicit WebGLContextLossHandler(WebGLContext* aWebgl);

    void RunTimer();
    void DisableTimer();

protected:
    ~WebGLContextLossHandler();

    void StartTimer(unsigned long delayMS);
    static void StaticTimerCallback(nsITimer*, void* tempRefForTimer);
    void TimerCallback();
};

} 

#endif
