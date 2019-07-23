







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif







MOZCE_SHUNT_API unsigned char* _mbsinc(const unsigned char* inCurrent)
{
#ifdef API_LOGGING
    mozce_printf("mbsinc called\n");
#endif
    
    return (unsigned char*)(inCurrent + 1);
}


MOZCE_SHUNT_API unsigned char* _mbspbrk(const unsigned char* inString, const unsigned char* inStrCharSet)
{
#ifdef API_LOGGING
    mozce_printf("mbspbrk called\n");
#endif

    LPWSTR wstring = a2w_malloc((const char *)inString, -1, NULL);
    LPWSTR wset    = a2w_malloc((const char *)inStrCharSet, -1, NULL);
    LPWSTR result  = wcspbrk(wstring, wset);
    free(wstring);
    free(wset);
    return (unsigned char *)result;
}


MOZCE_SHUNT_API unsigned char* mbsrchr(const unsigned char* inString, unsigned int inC)
{
#ifdef API_LOGGING
    mozce_printf("mbsrchr called\n");
#endif

    return (unsigned char*) strrchr((char*)inString, inC);
}


MOZCE_SHUNT_API unsigned char* mbschr(const unsigned char* inString, unsigned int inC)
{
#ifdef API_LOGGING
    mozce_printf("mbschr called\n");
#endif
    return (unsigned char*)strchr((const char*)inString, (int)inC);
}


MOZCE_SHUNT_API int mbsicmp(const unsigned char *string1, const unsigned char *string2)
{
#ifdef API_LOGGING
    mozce_printf("mbsicmp called\n");
#endif
    return _stricmp((const char*)string1, (const char*)string2);
}

MOZCE_SHUNT_API unsigned char* mbsdec(const unsigned char *string1, const unsigned char *string2)
{
#ifdef API_LOGGING
    mozce_printf("mbsdec called\n");
#endif
    
    if (string1 == string2)
        return 0;
    
    return (unsigned char *)string2 - 1;
}

#if 0
{
#endif
} 

