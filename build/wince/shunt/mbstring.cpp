







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif







#define LOG_CALLS

MOZCE_SHUNT_API unsigned char* mozce_mbsinc(const unsigned char* inCurrent)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_mbsinc called\n");
#endif
#endif
    
    return (unsigned char*)(inCurrent + 1);
}


MOZCE_SHUNT_API unsigned char* mozce_mbspbrk(const unsigned char* inString, const unsigned char* inStrCharSet)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_mbspbrk called\n");
#endif
#endif

    LPWSTR wstring = a2w_malloc((const char *)inString, -1, NULL);
    LPWSTR wset    = a2w_malloc((const char *)inStrCharSet, -1, NULL);
    LPWSTR result  = wcspbrk(wstring, wset);
    free(wstring);
    free(wset);
    return (unsigned char *)result;
}


MOZCE_SHUNT_API unsigned char* mozce_mbsrchr(const unsigned char* inString, unsigned int inC)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_mbsrchr called\n");
#endif
#endif

    return (unsigned char*) strrchr((char*)inString, inC);
}


MOZCE_SHUNT_API unsigned char* mozce_mbschr(const unsigned char* inString, unsigned int inC)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_mbschr called\n");
#endif
#endif
    return (unsigned char*)strchr((const char*)inString, (int)inC);
}


MOZCE_SHUNT_API int mozce_mbsicmp(const unsigned char *string1, const unsigned char *string2)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_mbsicmp called\n");
#endif
#endif
    return _stricmp((const char*)string1, (const char*)string2);
}

MOZCE_SHUNT_API unsigned char* mozce_mbsdec(const unsigned char *string1, const unsigned char *string2)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_mbsdec called\n");
#endif
#endif
    
    if (string1 == string2)
        return 0;
    
    return (unsigned char *)string2 - 1;
}

#if 0
{
#endif
} 

