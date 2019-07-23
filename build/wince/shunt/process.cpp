








































#include "mozce_internal.h"

#include "map.h"


extern "C" {
#if 0
}
#endif

#include "kfuncs.h"

MOZCE_SHUNT_API void abort(void)
{
#ifdef API_LOGGING
    mozce_printf("abort called\n");
#endif

#if defined(DEBUG)
    DebugBreak();
#endif
    TerminateProcess((HANDLE) GetCurrentProcessId(), 3);
}


MOZCE_SHUNT_API char* getenv(const char* inName)
{
    return map_get(inName);
}

MOZCE_SHUNT_API int putenv(const char *a)
{
#ifdef API_LOGGING
    mozce_printf("putenv called %s\n",a);
#endif
    int len = strlen(a);
    char* key = (char*) malloc(len*sizeof(char));
    strcpy(key,a);
    char* val = strchr(key,'=');
    val[0] = '\0';
    int rv;
    val++;
    rv = map_put(key,val);
    free(key);
    return rv;
}

MOZCE_SHUNT_API int getpid(void)
{
#ifdef API_LOGGING
    mozce_printf("getpid called\n");
#endif
    
    int retval = 0;
    
    retval = (int)GetCurrentProcessId();
    
    return retval;
}


#if 0
{
#endif
} 

