







































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

static PRLibrary* xsslib = nsnull;
static _XScreenSaverQueryExtension_fn _XSSQueryExtension = nsnull;
static _XScreenSaverAllocInfo_fn _XSSAllocInfo = nsnull;
static _XScreenSaverQueryInfo_fn _XSSQueryInfo = nsnull;


NS_IMPL_ISUPPORTS1(nsIdleServiceGTK, nsIIdleService)

nsIdleServiceGTK::nsIdleServiceGTK()
    : mXssInfo(nsnull)
{
    NS_ASSERTION(!xsslib, "created two instances of the idle service");
#ifdef PR_LOGGING
    if (!sIdleLog)
        sIdleLog = PR_NewLogModule("nsIIdleService");
#endif

    xsslib = PR_LoadLibrary("libXss.so.1");
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
    if (xsslib) {
        PR_UnloadLibrary(xsslib);
        xsslib = nsnull;
    }
}

NS_IMETHODIMP
nsIdleServiceGTK::GetIdleTime(PRUint32 *aTimeDiff)
{
    
    int event_base, error_base;
    *aTimeDiff = 0;

    
    Display *dplay = GDK_DISPLAY();
    if (!dplay || !_XSSQueryExtension || !_XSSAllocInfo || !_XSSQueryInfo)
    {
#ifdef PR_LOGGING
        if (!dplay)
            PR_LOG(sIdleLog, PR_LOG_WARNING, ("No display found!\n"));
        else
            PR_LOG(sIdleLog, PR_LOG_WARNING, ("One of the Xss functions is missing!\n"));
#endif
        return NS_ERROR_FAILURE;
    }

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

