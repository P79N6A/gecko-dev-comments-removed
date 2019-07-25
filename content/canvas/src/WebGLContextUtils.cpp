






































#include <stdarg.h>

#include "WebGLContext.h"

#include "prprf.h"

#include "nsIJSContextStack.h"
#include "jsapi.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrefBranch.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIVariant.h"

#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDataContainerEvent.h"

#include "nsContentUtils.h"

#if 0
#include "nsIContentURIGrouper.h"
#include "nsIContentPrefService.h"
#endif

using namespace mozilla;

void
WebGLContext::LogMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    LogMessage(fmt, ap);

    va_end(ap);
}

void
WebGLContext::LogMessage(const char *fmt, va_list ap)
{
    if (!fmt) return;

    char buf[1024];
    PR_vsnprintf(buf, 1024, fmt, ap);

    

    nsCOMPtr<nsIJSContextStack> stack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
    JSContext* ccx = nsnull;
    if (stack && NS_SUCCEEDED(stack->Peek(&ccx)) && ccx)
        JS_ReportWarning(ccx, "WebGL: %s", buf);
}

void
WebGLContext::LogMessageIfVerbose(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    LogMessageIfVerbose(fmt, ap);

    va_end(ap);
}

void
WebGLContext::LogMessageIfVerbose(const char *fmt, va_list ap)
{
    static PRBool firstTime = PR_TRUE;

    if (mVerbose)
        LogMessage(fmt, ap);
    else if (firstTime)
        LogMessage("There are WebGL warnings or messages in this page, but they are hidden. To see them, "
                   "go to about:config, set the webgl.verbose preference, and reload this page.");

    firstTime = PR_FALSE;
}

nsresult
WebGLContext::SynthesizeGLError(WebGLenum err)
{
    
    
    
    

    if (mSynthesizedGLError == LOCAL_GL_NO_ERROR) {
        MakeContextCurrent();

        mSynthesizedGLError = gl->fGetError();

        if (mSynthesizedGLError == LOCAL_GL_NO_ERROR)
            mSynthesizedGLError = err;
    }

    return NS_OK;
}

nsresult
WebGLContext::SynthesizeGLError(WebGLenum err, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    LogMessageIfVerbose(fmt, va);
    va_end(va);

    return SynthesizeGLError(err);
}

nsresult
WebGLContext::ErrorInvalidEnum(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    LogMessageIfVerbose(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_ENUM);
}

nsresult
WebGLContext::ErrorInvalidOperation(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    LogMessageIfVerbose(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_OPERATION);
}

nsresult
WebGLContext::ErrorInvalidValue(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    LogMessageIfVerbose(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_VALUE);
}

nsresult
WebGLContext::ErrorOutOfMemory(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    LogMessageIfVerbose(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_OUT_OF_MEMORY);
}

