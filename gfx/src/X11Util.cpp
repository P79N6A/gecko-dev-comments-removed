






































#include "X11Util.h"

namespace mozilla {

ScopedXErrorHandler::ErrorEvent* ScopedXErrorHandler::sXErrorPtr;

int
ScopedXErrorHandler::ErrorHandler(Display *, XErrorEvent *ev)
{
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
    bool retval = mXError.mError.error_code != 0;
    if (ev)
        *ev = mXError.mError;
    mXError = ErrorEvent(); 
    return retval;
}


} 
