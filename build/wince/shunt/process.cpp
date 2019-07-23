








































#include "mozce_internal.h"

#include "map.h"


extern "C" {
#if 0
}
#endif

#include "kfuncs.h"

MOZCE_SHUNT_API void abort(void)
{
    WINCE_LOG_API_CALL("abort called\n");

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
    WINCE_LOG_API_CALL_1("mozce_PutEnv called %s\n",a);

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
    WINCE_LOG_API_CALL("getpid called\n");
    
    int retval = 0;
    
    retval = (int)GetCurrentProcessId();
    
    return retval;
}


#if 0
{
#endif
} 

