






































#include "X11Util.h"

namespace mozilla {

ScopedXErrorHandler::ErrorEvent* ScopedXErrorHandler::sXErrorPtr;

int
ScopedXErrorHandler::ErrorHandler(Display *, XErrorEvent *ev)
{
    
    
    if (!sXErrorPtr->mError.error_code)
      sXErrorPtr->mError = *ev;
    return 0;
}

ScopedXErrorHandler::ScopedXErrorHandler()
{
    
    
    mOldXErrorPtr = sXErrorPtr;
    sXErrorPtr = &mXError;
    mOldErrorHandler = XSetErrorHandler(ErrorHandler);
}

ScopedXErrorHandler::~ScopedXErrorHandler()
{
    sXErrorPtr = mOldXErrorPtr;
    XSetErrorHandler(mOldErrorHandler);
}

bool
ScopedXErrorHandler::SyncAndGetError(Display *dpy, XErrorEvent *ev)
{
    XSync(dpy, False);
    return GetError(ev);
}

bool
ScopedXErrorHandler::GetError(XErrorEvent *ev)
{
    bool retval = mXError.mError.error_code != 0;
    if (ev)
        *ev = mXError.mError;
    mXError = ErrorEvent(); 
    return retval;
}

} 
