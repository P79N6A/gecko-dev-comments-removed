






































#include "nsIdleServiceQt.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "prlink.h"


#ifdef Q_WS_X11
#include <QX11Info>

typedef PRBool (*_XScreenSaverQueryExtension_fn)(Display* dpy, int* event_base,
                                                 int* error_base);

typedef XScreenSaverInfo* (*_XScreenSaverAllocInfo_fn)(void);

typedef void (*_XScreenSaverQueryInfo_fn)(Display* dpy, Drawable drw,
                                          XScreenSaverInfo *info);

static PRBool sInitialized = PR_FALSE;
static _XScreenSaverQueryExtension_fn _XSSQueryExtension = nsnull;
static _XScreenSaverAllocInfo_fn _XSSAllocInfo = nsnull;
static _XScreenSaverQueryInfo_fn _XSSQueryInfo = nsnull;


NS_IMPL_ISUPPORTS1(nsIdleServiceQt, nsIIdleService)

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

NS_IMETHODIMP
nsIdleServiceQt::GetIdleTime(PRUint32 *aTimeDiff)
{
    
    *aTimeDiff = 0;

    
    Display *dplay = QX11Info::display();
    if (!dplay) {
        return NS_ERROR_FAILURE;
    }

    if (!sInitialized) {
        Initialize();
    }
    if (!_XSSQueryExtension || !_XSSAllocInfo || !_XSSQueryInfo) {
        return NS_ERROR_FAILURE;
    }

    int event_base, error_base;
    if (_XSSQueryExtension(dplay, &event_base, &error_base)) {
        if (!mXssInfo)
            mXssInfo = _XSSAllocInfo();
        if (!mXssInfo)
            return NS_ERROR_OUT_OF_MEMORY;

        _XSSQueryInfo(dplay, QX11Info::appRootWindow(), mXssInfo);
        *aTimeDiff = mXssInfo->idle;
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}

#else

NS_IMPL_ISUPPORTS1(nsIdleServiceQt, nsIIdleService)

nsIdleServiceQt::nsIdleServiceQt()
{
}

static void Initialize()
{
}

nsIdleServiceQt::~nsIdleServiceQt()
{
}

NS_IMETHODIMP
nsIdleServiceQt::GetIdleTime(PRUint32 *aTimeDiff)
{
    return NS_ERROR_FAILURE;
}

#endif
