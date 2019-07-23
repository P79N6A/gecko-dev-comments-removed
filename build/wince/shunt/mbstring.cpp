







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif







MOZCE_SHUNT_API unsigned char* _mbsinc(const unsigned char* inCurrent)
{
    WINCE_LOG_API_CALL("mbsinc called\n");
    
    return (unsigned char*)(inCurrent + 1);
}


MOZCE_SHUNT_API unsigned char* _mbspbrk(const unsigned char* inString, const unsigned char* inStrCharSet)
{
    WINCE_LOG_API_CALL("mbspbrk called\n");

    LPWSTR wstring = a2w_malloc((const char *)inString, -1, NULL);
    LPWSTR wset    = a2w_malloc((const char *)inStrCharSet, -1, NULL);
    LPWSTR result  = wcspbrk(wstring, wset);
    free(wstring);
    free(wset);
    return (unsigned char *)result;
}


MOZCE_SHUNT_API unsigned char* mbsrchr(const unsigned char* inString, unsigned int inC)
{
    WINCE_LOG_API_CALL("mbsrchr called\n");

    return (unsigned char*) strrchr((char*)inString, inC);
}


MOZCE_SHUNT_API unsigned char* mbschr(const unsigned char* inString, unsigned int inC)
{
    WINCE_LOG_API_CALL("mbschr called\n");
    return (unsigned char*)strchr((const char*)inString, (int)inC);
}


MOZCE_SHUNT_API int mbsicmp(const unsigned char *string1, const unsigned char *string2)
{
    WINCE_LOG_API_CALL("mbsicmp called\n");
    return _stricmp((const char*)string1, (const char*)string2);
}

MOZCE_SHUNT_API unsigned char* mbsdec(const unsigned char *string1, const unsigned char *string2)
{
    WINCE_LOG_API_CALL("mbsdec called\n");
    
    if (string1 == string2)
        return 0;
    
    return (unsigned char *)string2 - 1;
}

#if 0
{
#endif
} 

