








































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

#include "kfuncs.h"

MOZCE_SHUNT_API void mozce_abort(void)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_abort called\n");
#endif

#if defined(DEBUG)
    DebugBreak();
#endif
    TerminateProcess((HANDLE) GetCurrentProcessId(), 3);
}


MOZCE_SHUNT_API char* mozce_getenv(const char* inName)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_getenv called (%s)\n", inName);
#endif

    char* retval = NULL;

#ifdef DEBUG_NSPR_ALL
    if (!strcmp(inName, "NSPR_LOG_MODULES"))
        return "all:5";

    if (!strcmp(inName, "NSPR_LOG_FILE"))
        return "nspr.log";
#endif  

#ifdef TIMELINE
    if (!strcmp(inName, "NS_TIMELINE_LOG_FILE"))
        return "\\bin\\timeline.log";
    
    if (!strcmp(inName, "NS_TIMELINE_ENABLE"))
        return "1";
#endif

	if (!_stricmp(inName, "tmp"))
        return "/Temp";
    return retval;
}

MOZCE_SHUNT_API int mozce_putenv(const char *a) 
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_putenv called\n");
#endif

    return 0;
}

MOZCE_SHUNT_API int mozce_getpid(void)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_getpid called\n");
#endif

    int retval = 0;

    retval = (int)GetCurrentProcessId();

    return retval;
}


#if 0
{
#endif
} 

