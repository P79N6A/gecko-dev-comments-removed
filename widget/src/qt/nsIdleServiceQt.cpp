






































#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
#include <QX11Info>
#endif

#include "nsIdleServiceQt.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "prlink.h"

#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
typedef bool (*_XScreenSaverQueryExtension_fn)(Display* dpy, int* event_base,
                                                 int* error_base);

typedef XScreenSaverInfo* (*_XScreenSaverAllocInfo_fn)(void);

typedef void (*_XScreenSaverQueryInfo_fn)(Display* dpy, Drawable drw,
                                          XScreenSaverInfo *info);

static _XScreenSaverQueryExtension_fn _XSSQueryExtension = nsnull;
static _XScreenSaverAllocInfo_fn _XSSAllocInfo = nsnull;
static _XScreenSaverQueryInfo_fn _XSSQueryInfo = nsnull;
#endif

static bool sInitialized = false;

NS_IMPL_ISUPPORTS2(nsIdleServiceQt, nsIIdleService, nsIdleService)

nsIdleServiceQt::nsIdleServiceQt()
#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
    : mXssInfo(nsnull)
#endif
{
}

static void Initialize()
{
    sInitialized = true;

#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
    
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
#endif
}

nsIdleServiceQt::~nsIdleServiceQt()
{
#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
    if (mXssInfo)
        XFree(mXssInfo);




#if 0
    if (xsslib) {
        PR_UnloadLibrary(xsslib);
        xsslib = nsnull;
    }
#endif
#endif
}

bool
nsIdleServiceQt::PollIdleTime(PRUint32 *aIdleTime)
{
#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
    
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
#endif

    return false;
}

bool
nsIdleServiceQt::UsePollMode()
{
#if !defined(MOZ_PLATFORM_MAEMO) && defined(MOZ_X11)
    return false;
#endif
    return true;
}

