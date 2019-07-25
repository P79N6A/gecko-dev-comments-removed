






































#include <QX11Info>

#include "nsIdleServiceQt.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "prlink.h"

typedef PRBool (*_XScreenSaverQueryExtension_fn)(Display* dpy, int* event_base,
                                                 int* error_base);

typedef XScreenSaverInfo* (*_XScreenSaverAllocInfo_fn)(void);

typedef void (*_XScreenSaverQueryInfo_fn)(Display* dpy, Drawable drw,
                                          XScreenSaverInfo *info);

static PRBool sInitialized = PR_FALSE;
static _XScreenSaverQueryExtension_fn _XSSQueryExtension = nsnull;
static _XScreenSaverAllocInfo_fn _XSSAllocInfo = nsnull;
static _XScreenSaverQueryInfo_fn _XSSQueryInfo = nsnull;


NS_IMPL_ISUPPORTS2(nsIdleServiceQt, nsIIdleService, nsIdleService)

nsIdleServiceQt::nsIdleServiceQt()
    : mXssInfo(nsnull)
{
}

static void Initialize()
{
    sInitialized = PR_TRUE;

    
    PRLibrary* xsslib = PR_LoadLibrary("libXss.so.1");
    if (!xsslib) {
        return;
    }

    _XSSQueryExtension = (_XScreenSaverQueryExtension_fn)
        PR_FindFunctionSymbol(xsslib, "XScreenSaverQueryExtension");
    _XSSAllocInfo = (_XScreenSaverAllocInfo_fn)
        PR_FindFunctionSymbol(xsslib, "XScreenSaverAllocInfo");
    _XSSQueryInfo = (_XScreenSaverQueryInfo_fn)
        PR_FindFunctionSymbol(xsslib, "XScreenSaverQueryInfo");
}

nsIdleServiceQt::~nsIdleServiceQt()
{
    if (mXssInfo)
        XFree(mXssInfo);




#if 0
    if (xsslib) {
        PR_UnloadLibrary(xsslib);
        xsslib = nsnull;
    }
#endif
}

bool
nsIdleServiceQt::PollIdleTime(PRUint32 *aIdleTime)
{
    
    *aIdleTime = 0;

    
    Display *dplay = QX11Info::display();
    if (!dplay) {
        return false;
    }

    if (!sInitialized) {
        Initialize();
    }
    if (!_XSSQueryExtension || !_XSSAllocInfo || !_XSSQueryInfo) {
        return false;
    }

    int event_base, error_base;
    if (_XSSQueryExtension(dplay, &event_base, &error_base)) {
        if (!mXssInfo)
            mXssInfo = _XSSAllocInfo();
        if (!mXssInfo)
            return false;

        _XSSQueryInfo(dplay, QX11Info::appRootWindow(), mXssInfo);
        *aIdleTime = mXssInfo->idle;
        return true;
    }

    return false;
}

bool
nsIdleServiceQt::UsePollMode()
{
    return true;
}

