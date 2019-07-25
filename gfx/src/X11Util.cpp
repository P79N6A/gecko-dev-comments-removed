






































#include "X11Util.h"

namespace mozilla {

bool
XVisualIDToInfo(Display* aDisplay, VisualID aVisualID,
                Visual** aVisual, unsigned int* aDepth)
{
    if (aVisualID == None) {
        *aVisual = NULL;
        *aDepth = 0;
        return true;
    }

    const Screen* screen = DefaultScreenOfDisplay(aDisplay);

    for (int d = 0; d < screen->ndepths; d++) {
        Depth *d_info = &screen->depths[d];
        for (int v = 0; v < d_info->nvisuals; v++) {
            Visual* visual = &d_info->visuals[v];
            if (visual->visualid == aVisualID) {
                *aVisual = visual;
                *aDepth = d_info->depth;
                return true;
            }
        }
    }

    NS_ERROR("VisualID not on Screen.");
    return false;
}

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
