







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif




#define wcharcount(array) (sizeof(array) / sizeof(TCHAR))


MOZCE_SHUNT_API UINT mozce_GetWindowsDirectoryW(LPWSTR inBuffer, UINT inSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetWindowsDirectoryW called\n");
#endif

    UINT retval = 0;

    if(inSize < 9)
    {
        retval = 9;
    }
    else
    {
        wcscpy(inBuffer, L"\\WINDOWS");
        retval = 8;
    }

    return retval;
}


MOZCE_SHUNT_API UINT mozce_GetSystemDirectoryW(LPWSTR inBuffer, UINT inSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetSystemDirectoryW called\n");
#endif

    UINT retval = 0;

    if(inSize < 9)
    {
        retval = 9;
    }
    else
    {
        wcscpy(inBuffer, L"\\WINDOWS");
        retval = 8;
    }

    return retval;
}


MOZCE_SHUNT_API HANDLE mozce_OpenSemaphoreW(DWORD inDesiredAccess, BOOL inInheritHandle, LPCWSTR inName)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_OpenSemaphoreW called\n");
#endif

    HANDLE retval = NULL;
    HANDLE semaphore = NULL;

    semaphore = CreateSemaphoreW(NULL, 0, 0x7fffffff, inName);
    if(NULL != semaphore)
    {
        DWORD lastErr = GetLastError();
        
        if(ERROR_ALREADY_EXISTS != lastErr)
        {
            CloseHandle(semaphore);
        }
        else
        {
            retval = semaphore;
        }
    }

    return retval;
}


MOZCE_SHUNT_API DWORD mozce_GetGlyphOutlineW(HDC inDC, WCHAR inChar, UINT inFormat, void* inGM, DWORD inBufferSize, LPVOID outBuffer, CONST VOID* inMAT2)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetGlyphOutlineW called\n");
#endif

    DWORD retval = GDI_ERROR;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API DWORD mozce_GetCurrentDirectoryW(DWORD inBufferLength, LPTSTR outBuffer)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetCurrentDirectoryW called\n");
#endif

    DWORD retval = 0;

    if(NULL != outBuffer && 0 < inBufferLength)
    {
        outBuffer[0] = _T('\0');
    }

    SetLastError(ERROR_NOT_SUPPORTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_SetCurrentDirectoryW(LPCTSTR inPathName)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_SetCurrentDirectoryW called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_NOT_SUPPORTED);

    return retval;
}


#if 0
{
#endif
} 
