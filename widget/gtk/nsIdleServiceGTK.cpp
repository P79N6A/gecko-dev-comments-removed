






#include <gtk/gtk.h>

#include "nsIdleServiceGTK.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "prlink.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* sIdleLog = nullptr;
#endif

typedef bool (*_XScreenSaverQueryExtension_fn)(Display* dpy, int* event_base,
                                                 int* error_base);

typedef XScreenSaverInfo* (*_XScreenSaverAllocInfo_fn)(void);

typedef void (*_XScreenSaverQueryInfo_fn)(Display* dpy, Drawable drw,
                                          XScreenSaverInfo *info);

static bool sInitialized = false;
static _XScreenSaverQueryExtension_fn _XSSQueryExtension = nullptr;
static _XScreenSaverAllocInfo_fn _XSSAllocInfo = nullptr;
static _XScreenSaverQueryInfo_fn _XSSQueryInfo = nullptr;

NS_IMPL_ISUPPORTS_INHERITED0(nsIdleServiceGTK, nsIdleService)

static void Initialize()
{
    if (!GDK_IS_X11_DISPLAY(gdk_display_get_default()))
        return;

    
    PRLibrary* xsslib = PR_LoadLibrary("libXss.so.1");
    if (!xsslib) 
    {
#ifdef PR_LOGGING
        PR_LOG(sIdleLog, PR_LOG_WARNING, ("Failed to find libXss.so!\n"));
#endif
        return;
    }

    _XSSQueryExtension = (_XScreenSaverQueryExtension_fn)
        PR_FindFunctionSymbol(xsslib, "XScreenSaverQueryExtension");
    _XSSAllocInfo = (_XScreenSaverAllocInfo_fn)
        PR_FindFunctionSymbol(xsslib, "XScreenSaverAllocInfo");
    _XSSQueryInfo = (_XScreenSaverQueryInfo_fn)
        PR_FindFunctionSymbol(xsslib, "XScreenSaverQueryInfo");
#ifdef PR_LOGGING
    if (!_XSSQueryExtension)
        PR_LOG(sIdleLog, PR_LOG_WARNING, ("Failed to get XSSQueryExtension!\n"));
    if (!_XSSAllocInfo)
        PR_LOG(sIdleLog, PR_LOG_WARNING, ("Failed to get XSSAllocInfo!\n"));
    if (!_XSSQueryInfo)
        PR_LOG(sIdleLog, PR_LOG_WARNING, ("Failed to get XSSQueryInfo!\n"));
#endif

    sInitialized = true;
}

nsIdleServiceGTK::nsIdleServiceGTK()
    : mXssInfo(nullptr)
{
#ifdef PR_LOGGING
    if (!sIdleLog)
        sIdleLog = PR_NewLogModule("nsIIdleService");
#endif

    Initialize();
}

nsIdleServiceGTK::~nsIdleServiceGTK()
{
    if (mXssInfo)
        XFree(mXssInfo);




#if 0
    if (xsslib) {
        PR_UnloadLibrary(xsslib);
        xsslib = nullptr;
    }
#endif
}

bool
nsIdleServiceGTK::PollIdleTime(uint32_t *aIdleTime)
{
    if (!sInitialized) {
        
        return false;
    }

    
    *aIdleTime = 0;

    
    Display *dplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (!dplay) {
#ifdef PR_LOGGING
        PR_LOG(sIdleLog, PR_LOG_WARNING, ("No display found!\n"));
#endif
        return false;
    }

    if (!_XSSQueryExtension || !_XSSAllocInfo || !_XSSQueryInfo) {
        return false;
    }

    int event_base, error_base;
    if (_XSSQueryExtension(dplay, &event_base, &error_base))
    {
        if (!mXssInfo)
            mXssInfo = _XSSAllocInfo();
        if (!mXssInfo)
            return false;
        _XSSQueryInfo(dplay, GDK_ROOT_WINDOW(), mXssInfo);
        *aIdleTime = mXssInfo->idle;
        return true;
    }
    
#ifdef PR_LOGGING
    PR_LOG(sIdleLog, PR_LOG_WARNING, ("XSSQueryExtension returned false!\n"));
#endif
    return false;
}

bool
nsIdleServiceGTK::UsePollMode()
{
    return sInitialized;
}
