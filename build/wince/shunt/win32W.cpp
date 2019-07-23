







































#include "mozce_internal.h"
#include "map.h"

extern "C" {
#if 0
}
#endif




#define wcharcount(array) (sizeof(array) / sizeof(TCHAR))

#define MOZCE_NOT_IMPLEMENTED_RV(fname, rv) \
  SetLastError(0); \
  mozce_printf("-- fname called\n"); \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  return rv;

MOZCE_SHUNT_API UINT GetWindowsDirectoryW(LPWSTR inBuffer, UINT inSize)
{
    SetLastError(0);
#ifdef API_LOGGING
    mozce_printf("GetWindowsDirectoryW called\n");
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


MOZCE_SHUNT_API UINT GetSystemDirectoryW(LPWSTR inBuffer, UINT inSize)
{
    SetLastError(0);
#ifdef API_LOGGING
    mozce_printf("GetSystemDirectoryW called\n");
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


MOZCE_SHUNT_API HANDLE OpenSemaphoreW(DWORD inDesiredAccess, BOOL inInheritHandle, LPCWSTR inName)
{
#ifdef API_LOGGING
    mozce_printf("OpenSemaphoreW called\n");
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


MOZCE_SHUNT_API DWORD GetGlyphOutlineW(HDC inDC, WCHAR inChar, UINT inFormat, void* inGM, DWORD inBufferSize, LPVOID outBuffer, CONST VOID* inMAT2)
{
   MOZCE_NOT_IMPLEMENTED_RV(__FUNCTION__, GDI_ERROR); 
}


MOZCE_SHUNT_API DWORD GetCurrentDirectoryW(DWORD inBufferLength, LPTSTR outBuffer)
{
    if(NULL != outBuffer && 0 < inBufferLength)
    {
        outBuffer[0] = _T('\0');
    }
   MOZCE_NOT_IMPLEMENTED_RV(__FUNCTION__, 0); 
}


MOZCE_SHUNT_API BOOL SetCurrentDirectoryW(LPCTSTR inPathName)
{
    MOZCE_NOT_IMPLEMENTED_RV(__FUNCTION__, FALSE); 
}

MOZCE_SHUNT_API BOOL MoveFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName,DWORD dwFlags)
{
#ifdef API_LOGGING
    mozce_printf("MoveFileExW called\n");
#endif
    BOOL retval = ::MoveFileW(lpExistingFileName, lpNewFileName);
    return retval;
}

MOZCE_SHUNT_API BOOL SetEnvironmentVariableW( LPCWSTR name, LPCWSTR value )
{
    char key[256];
    char val[256];
    int rv =WideCharToMultiByte( CP_ACP, 0, name, -1, key, 256, NULL, NULL );
    if(rv<0){
        return rv;
    }
    rv =WideCharToMultiByte( CP_ACP, 0, value, -1, val, 256, NULL, NULL );
    if(rv<0){
        return rv;
    }
    return map_put(key,val);
}

#if 0
{
#endif
} 
