







































#include "nsIdleServiceGTK.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "prlink.h"
#include "prlog.h"


#ifdef PR_LOGGING
static PRLogModuleInfo* sIdleLog = nsnull;
#endif

typedef PRBool (*_XScreenSaverQueryExtension_fn)(Display* dpy, int* event_base,
                                                 int* error_base);

typedef XScreenSaverInfo* (*_XScreenSaverAllocInfo_fn)(void);

typedef void (*_XScreenSaverQueryInfo_fn)(Display* dpy, Drawable drw,
                                          XScreenSaverInfo *info);

static PRBool sInitialized = PR_FALSE;
static _XScreenSaverQueryExtension_fn _XSSQueryExtension = nsnull;
static _XScreenSaverAllocInfo_fn _XSSAllocInfo = nsnull;
static _XScreenSaverQueryInfo_fn _XSSQueryInfo = nsnull;


NS_IMPL_ISUPPORTS1(nsIdleServiceGTK, nsIIdleService)

nsIdleServiceGTK::nsIdleServiceGTK()
    : mXssInfo(nsnull)
{
}

static void Initialize()
{
    sInitialized = PR_TRUE;
#ifdef PR_LOGGING
    sIdleLog = PR_NewLogModule("nsIIdleService");
#endif

    
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
}

nsIdleServiceGTK::~nsIdleServiceGTK()
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
nsIdleServiceGTK::GetIdleTime(PRUint32 *aTimeDiff)
{
    
    *aTimeDiff = 0;

    
    Display *dplay = GDK_DISPLAY();
    if (!dplay) {
#ifdef PR_LOGGING
        PR_LOG(sIdleLog, PR_LOG_WARNING, ("No display found!\n"));
#endif
        return NS_ERROR_FAILURE;
    }

    if (!sInitialized) {
        Initialize();
    }
    if (!_XSSQueryExtension || !_XSSAllocInfo || !_XSSQueryInfo) {
        return NS_ERROR_FAILURE;
    }

    int event_base, error_base;
    if (_XSSQueryExtension(dplay, &event_base, &error_base))
    {
        if (!mXssInfo)
            mXssInfo = _XSSAllocInfo();
        if (!mXssInfo)
            return NS_ERROR_OUT_OF_MEMORY;
        _XSSQueryInfo(dplay, GDK_ROOT_WINDOW(), mXssInfo);
        *aTimeDiff = mXssInfo->idle;
        return NS_OK;
    }
    
#ifdef PR_LOGGING
    PR_LOG(sIdleLog, PR_LOG_WARNING, ("XSSQueryExtension returned false!\n"));
#endif
    return NS_ERROR_FAILURE;
}

