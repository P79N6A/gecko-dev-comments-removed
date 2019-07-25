





































#include "nsGlueLinking.h"
#include "nsXPCOMGlue.h"

#include "nsStringAPI.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#define MOZ_LOADLIBRARY_FLAGS LOAD_WITH_ALTERED_SEARCH_PATH

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
preload(LPCWSTR dll)
{
    HANDLE fd = CreateFileW(dll, GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    char buf[64 * 1024];

    if (fd == INVALID_HANDLE_VALUE)
        return;
  
    DWORD dwBytesRead;
    
    
    
    while (ReadFile(fd, buf, sizeof(buf), &dwBytesRead, NULL) && dwBytesRead == sizeof(buf))
        ;
  
    CloseHandle(fd);
}

static void
ReadDependentCB(const char *aDependentLib, PRBool do_preload)
{
    wchar_t wideDependentLib[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, aDependentLib, -1, wideDependentLib, MAX_PATH);

    if (do_preload)
        preload(wideDependentLib);

    HINSTANCE h =
        LoadLibraryExW(wideDependentLib, NULL, MOZ_LOADLIBRARY_FLAGS);

    if (!h)
        return;

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
    return !(path[1] == ':');
}

nsresult
XPCOMGlueLoad(const char *aXpcomFile, GetFrozenFunctionsFunc *func)
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

            if (!sXULLibrary) 
            {
                DWORD err = GetLastError();
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
                wprintf(L"Error loading %s: %s\n", xpcomDir, lpMsgBuf);
                LocalFree(lpMsgBuf);
#endif 
                return (err == ERROR_NOT_ENOUGH_MEMORY || err == ERROR_OUTOFMEMORY)
                    ? NS_ERROR_OUT_OF_MEMORY : NS_ERROR_FAILURE;
            }
        }
    }
    HINSTANCE h =
        LoadLibraryExW(xpcomFile, NULL, MOZ_LOADLIBRARY_FLAGS);

    if (!h) 
    {
        DWORD err = GetLastError();
#ifdef DEBUG
        LPVOID lpMsgBuf;
        FormatMessage(
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      err,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL
                      );
        wprintf(L"Error loading %s: %s\n", xpcomFile, lpMsgBuf);
        LocalFree(lpMsgBuf);
#endif        
        return (err == ERROR_NOT_ENOUGH_MEMORY || err == ERROR_OUTOFMEMORY)
            ? NS_ERROR_OUT_OF_MEMORY : NS_ERROR_FAILURE;
    }

    AppendDependentLib(h);

    GetFrozenFunctionsFunc sym =
        (GetFrozenFunctionsFunc) GetProcAddress(h, "NS_GetFrozenFunctions");

    if (!sym) { 
        XPCOMGlueUnload();
        return NS_ERROR_NOT_AVAILABLE;
    }

    *func = sym;

    return NS_OK;
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
