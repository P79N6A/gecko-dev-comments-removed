






#include "X11Util.h"
#include "nsDebug.h"                    
#include "MainThreadUtils.h"            

namespace mozilla {

void
FindVisualAndDepth(Display* aDisplay, VisualID aVisualID,
                   Visual** aVisual, int* aDepth)
{
    const Screen* screen = DefaultScreenOfDisplay(aDisplay);

    for (int d = 0; d < screen->ndepths; d++) {
        Depth *d_info = &screen->depths[d];
        for (int v = 0; v < d_info->nvisuals; v++) {
            Visual* visual = &d_info->visuals[v];
            if (visual->visualid == aVisualID) {
                *aVisual = visual;
                *aDepth = d_info->depth;
                return;
            }
        }
    }

    NS_ASSERTION(aVisualID == None, "VisualID not on Screen.");
    *aVisual = nullptr;
    *aDepth = 0;
    return;
}

void
FinishX(Display* aDisplay)
{
  unsigned long lastRequest = NextRequest(aDisplay) - 1;
  if (lastRequest == LastKnownRequestProcessed(aDisplay))
    return;

  XSync(aDisplay, False);
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
    
    
    NS_WARN_IF_FALSE(NS_IsMainThread(), "ScopedXErrorHandler being called off main thread, may cause issues");
    
    
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
    FinishX(dpy);

    bool retval = mXError.mError.error_code != 0;
    if (ev)
        *ev = mXError.mError;
    mXError = ErrorEvent(); 
    return retval;
}

} 
