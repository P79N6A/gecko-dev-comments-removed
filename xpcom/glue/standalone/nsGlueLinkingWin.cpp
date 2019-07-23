





































#include "nsGlueLinking.h"
#include "nsXPCOMGlue.h"

#include "nsStringAPI.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#ifdef WINCE
#define MOZ_LOADLIBRARY_FLAGS 0
#else
#define MOZ_LOADLIBRARY_FLAGS LOAD_WITH_ALTERED_SEARCH_PATH
#endif

struct DependentLib
{
    HINSTANCE     libHandle;
    DependentLib *next;
};

static DependentLib *sTop;
HINSTANCE sXULLibrary;

static void
AppendDependentLib(HINSTANCE libHandle)
{
    DependentLib *d = new DependentLib;
    if (!d)
        return;

    d->next = sTop;
    d->libHandle = libHandle;

    sTop = d;
}

static void
ReadDependentCB(const char *aDependentLib)
{
    wchar_t wideDependentLib[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, aDependentLib, -1, wideDependentLib, MAX_PATH);

    HINSTANCE h =
        LoadLibraryExW(wideDependentLib, NULL, MOZ_LOADLIBRARY_FLAGS);

    if (!h) {
        wprintf(L"Error loading %s\n", wideDependentLib);
        return;
    }
    AppendDependentLib(h);
}


static char*
ns_strrpbrk(char *string, const char *strCharSet)
{
    char *found = NULL;
    for (; *string; ++string) {
        for (const char *search = strCharSet; *search; ++search) {
            if (*search == *string) {
                found = string;
                
                
            }
        }
    }

    return found;
}

static wchar_t*
ns_wcspbrk(wchar_t *string, const wchar_t *strCharSet)
{
    wchar_t *found = NULL;
    for (; *string; ++string) {
        for (const wchar_t *search = strCharSet; *search; ++search) {
            if (*search == *string) {
                found = string;
                
                
            }
        }
    }

    return found;
}

bool ns_isRelPath(wchar_t* path)
{
#ifdef WINCE
    if (path[0] == '\\')
        return false;
#else
    if (path[1] == ':')
        return false;
#endif
    return true;
    
}

GetFrozenFunctionsFunc
XPCOMGlueLoad(const char *aXpcomFile)
{
    wchar_t xpcomFile[MAXPATHLEN];
    MultiByteToWideChar(CP_UTF8, 0, aXpcomFile,-1,
                        xpcomFile, MAXPATHLEN);
   
    
    if (xpcomFile[0] == '.' && xpcomFile[1] == '\0') {
        wcscpy(xpcomFile, LXPCOM_DLL);
    }
    else {
        wchar_t xpcomDir[MAXPATHLEN];
        
        if (ns_isRelPath(xpcomFile))
        {
            _wfullpath(xpcomDir, xpcomFile, sizeof(xpcomDir)/sizeof(wchar_t));
        } 
        else 
        {
            wcscpy(xpcomDir, xpcomFile);
        }
        wchar_t *lastSlash = ns_wcspbrk(xpcomDir, L"/\\");
        if (lastSlash) {
            *lastSlash = '\0';
            char xpcomDir_narrow[MAXPATHLEN];
            WideCharToMultiByte(CP_UTF8, 0, xpcomDir,-1,
                                xpcomDir_narrow, MAX_PATH, NULL, NULL);

            XPCOMGlueLoadDependentLibs(xpcomDir_narrow, ReadDependentCB);
            
            _snwprintf(lastSlash, MAXPATHLEN - wcslen(xpcomDir), L"\\" LXUL_DLL);
            sXULLibrary =
                LoadLibraryExW(xpcomDir, NULL, MOZ_LOADLIBRARY_FLAGS);

#ifdef DEBUG
            if (!sXULLibrary) 
            {
                LPVOID lpMsgBuf;
                FormatMessage(
                              FORMAT_MESSAGE_ALLOCATE_BUFFER |
                              FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL,
                              GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPTSTR) &lpMsgBuf,
                              0,
                              NULL
                              );
                wprintf(L"Error loading %s: %s\n", xpcomDir, lpMsgBuf);
            }
#endif 
        }
    }
    HINSTANCE h =
        LoadLibraryExW(xpcomFile, NULL, MOZ_LOADLIBRARY_FLAGS);

    if (!h) 
    {
#ifdef DEBUG
        LPVOID lpMsgBuf;
        FormatMessage(
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL
                      );
        wprintf(L"Error loading %s: %s\n", xpcomFile, lpMsgBuf);
#endif        
        return nsnull;
    }

    AppendDependentLib(h);

    GetFrozenFunctionsFunc sym =
        (GetFrozenFunctionsFunc) GetProcAddress(h, "NS_GetFrozenFunctions");

    if (!sym)
        XPCOMGlueUnload();

    return sym;
}

void
XPCOMGlueUnload()
{
    while (sTop) {
        FreeLibrary(sTop->libHandle);

        DependentLib *temp = sTop;
        sTop = sTop->next;

        delete temp;
    }

    if (sXULLibrary) {
        FreeLibrary(sXULLibrary);
        sXULLibrary = nsnull;
    }
}

nsresult
XPCOMGlueLoadXULFunctions(const nsDynamicFunctionLoad *symbols)
{
    if (!sXULLibrary)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv = NS_OK;
    while (symbols->functionName) {
        *symbols->function = 
            (NSFuncPtr) GetProcAddress(sXULLibrary, symbols->functionName);
        if (!*symbols->function)
            rv = NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

        ++symbols;
    }

    return rv;
}
