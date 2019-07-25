






































#include <stdarg.h>

#include "WebGLContext.h"

#include "prprf.h"

#include "nsIJSContextStack.h"
#include "jsapi.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"
#include "nsIVariant.h"

#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDataContainerEvent.h"

#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

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
    static bool firstTime = true;

    if (mVerbose)
        LogMessage(fmt, ap);
    else if (firstTime)
        LogMessage("There are WebGL warnings or messages in this page, but they are hidden. To see them, "
                   "go to about:config, set the webgl.verbose preference, and reload this page.");

    firstTime = PR_FALSE;
}

CheckedUint32
WebGLContext::GetImageSize(WebGLsizei height, 
                           WebGLsizei width, 
                           PRUint32 pixelSize,
                           PRUint32 packOrUnpackAlignment)
{
    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * pixelSize;

    
    CheckedUint32 checked_alignedRowSize = RoundedToNextMultipleOf(checked_plainRowSize, packOrUnpackAlignment);

    
    CheckedUint32 checked_neededByteLength
        = height <= 0 ? 0 : (height-1) * checked_alignedRowSize + checked_plainRowSize;

    return checked_neededByteLength;
}

nsresult
WebGLContext::SynthesizeGLError(WebGLenum err)
{
    
    
    
    
    
    MakeContextCurrent();

    UpdateWebGLErrorAndClearGLError();

    if (!mWebGLError)
        mWebGLError = err;

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

const char *
WebGLContext::ErrorName(GLenum error)
{
    switch(error) {
        case LOCAL_GL_INVALID_ENUM:
            return "INVALID_ENUM";
        case LOCAL_GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
        case LOCAL_GL_INVALID_VALUE:
            return "INVALID_VALUE";
        case LOCAL_GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case LOCAL_GL_INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
        case LOCAL_GL_NO_ERROR:
            return "NO_ERROR";
        default:
            NS_ABORT();
            return "[unknown WebGL error!]";
    }
};
