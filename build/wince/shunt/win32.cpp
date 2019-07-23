







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

#include "kfuncs.h"
#include "wingdi.h"
#include "Windows.h"
#include "locale.h"

#define wcharcount(array) (sizeof(array) / sizeof(TCHAR))


MOZCE_SHUNT_API DWORD mozce_CommDlgExtendedError()
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_CommDlgExtendedError called\n");
#endif

    return -1 ;
}


MOZCE_SHUNT_API int mozce_MulDiv(int inNumber, int inNumerator, int inDenominator)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_MulDiv called\n");
#endif
    
    if (inDenominator == 0)
        return 0;

    return (int)(((INT64)inNumber * (INT64)inNumerator) / (INT64)inDenominator);
}


MOZCE_SHUNT_API int mozce_GetDIBits(HDC inDC, HBITMAP inBMP, UINT inStartScan, UINT inScanLines, LPVOID inBits, LPBITMAPINFO inInfo, UINT inUsage)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetDIBits called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API int mozce_SetDIBits(HDC inDC, HBITMAP inBMP, UINT inStartScan, UINT inScanLines, CONST LPVOID inBits, CONST LPBITMAPINFO inInfo, UINT inUsage)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetDIBits called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API HBITMAP mozce_CreateDIBitmap(HDC inDC, CONST BITMAPINFOHEADER *inBMIH, DWORD inInit, CONST VOID *inBInit, CONST BITMAPINFO *inBMI, UINT inUsage)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_CreateDIBitmap called\n");
#endif

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}


MOZCE_SHUNT_API int mozce_SetPolyFillMode(HDC inDC, int inPolyFillMode)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetPolyFillMode called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API int mozce_SetStretchBltMode(HDC inDC, int inStretchMode)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetStretchBltMode called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API int mozce_ExtSelectClipRgn(HDC inDC, HRGN inRGN, int inMode)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_ExtSelectClipRgn called\n");
#endif
    
    
    
    
    
    
    
    

    if (inMode == RGN_COPY)
    {
        return SelectClipRgn(inDC, inRGN);
    }

    HRGN cRGN = NULL;
    int result = GetClipRgn(inDC, cRGN);
    
    
    
    

    if (result != 1)
    {
        RECT cRect;
        GetClipBox(inDC,&cRect);
        cRGN = CreateRectRgn(cRect.left,cRect.top,cRect.right,cRect.bottom);
    }

    
    result = SelectClipRgn(inDC,cRGN);		
    
    if (result == NULLREGION) 
    {
        if (inMode == RGN_DIFF || inMode == RGN_AND)
            result = SelectClipRgn(inDC,NULL);
        else
            result = SelectClipRgn(inDC,inRGN);
        
        DeleteObject(cRGN);
        return result;
    } 
    
    if (result == SIMPLEREGION || result == COMPLEXREGION)
    {
        if (inMode == RGN_DIFF)
            CombineRgn(cRGN, cRGN, inRGN, inMode);
        else
            CombineRgn(cRGN, inRGN, cRGN, inMode);
        result = SelectClipRgn(inDC,cRGN);
        DeleteObject(cRGN);
        return result;
    }
    
    HRGN rgn = CreateRectRgn(0, 0, 32000, 32000);
    result = SelectClipRgn(inDC, rgn);
    DeleteObject(rgn);

    return result;
}

typedef VOID CALLBACK LINEDDAPROC(
  int X,          
  int Y,          
  LPARAM lpData   
);

MOZCE_SHUNT_API BOOL mozce_LineDDA(int inXStart, int inYStart, int inXEnd, int inYEnd, LINEDDAPROC inLineFunc, LPARAM inData)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_LineDDA called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API int mozce_FrameRect(HDC inDC, CONST RECT *inRect, HBRUSH inBrush)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_FrameRect called\n");
#endif

    HBRUSH oldBrush = (HBRUSH)SelectObject(inDC, inBrush);
    RECT myRect = *inRect;
    InflateRect(&myRect, 1, 1); 
                                
                                
    
    
    
    
    
    
    

    MoveToEx(inDC, myRect.left, myRect.top, (LPPOINT) NULL); 

    
    LineTo(inDC, myRect.right, myRect.top); 
    
    
    LineTo(inDC, myRect.right, myRect.bottom); 

    
    LineTo(inDC, myRect.left, myRect.bottom); 

    SelectObject(inDC, oldBrush);

    return 1;
}


MOZCE_SHUNT_API int mozce_SetArcDirection(HDC inDC, int inArcDirection)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetArcDirection called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_Arc(HDC inDC, int inLeftRect, int inTopRect, int inRightRect, int inBottomRect, int inXStartArc, int inYStartArc, int inXEndArc, int inYEndArc)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_Arc called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_Pie(HDC inDC, int inLeftRect, int inTopRect, int inRightRect, int inBottomRect, int inXRadial1, int inYRadial1, int inXRadial2, int inYRadial2)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_Pie called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API DWORD mozce_GetFontData(HDC inDC, DWORD inTable, DWORD inOffset, LPVOID outBuffer, DWORD inData)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetFontData called\n");
#endif

    DWORD retval = GDI_ERROR;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API UINT mozce_GetTextCharset(HDC inDC)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetTextCharset called\n");
#endif

    UINT retval = DEFAULT_CHARSET;

    TEXTMETRIC tm;
    if(GetTextMetrics(inDC, &tm))
    {
        retval = tm.tmCharSet;
    }

    return retval;
}


MOZCE_SHUNT_API UINT mozce_GetTextCharsetInfo(HDC inDC, LPFONTSIGNATURE outSig, DWORD inFlags)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetTextCharsetInfo called\n");
#endif

    
    if(NULL != outSig)
    {
        memset(outSig, 0, sizeof(FONTSIGNATURE));
    }

    return mozce_GetTextCharset(inDC);
}


MOZCE_SHUNT_API UINT mozce_GetOutlineTextMetrics(HDC inDC, UINT inData, void* outOTM)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetOutlineTextMetrics called.\n");
#endif

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#define FACENAME_MAX 128
typedef struct __struct_CollectFaces
{
    UINT    mCount;
    LPTSTR  mNames[FACENAME_MAX];
}
CollectFaces;

static int CALLBACK collectProc(CONST LOGFONT* inLF, CONST TEXTMETRIC* inTM, DWORD inFontType, LPARAM inParam)
{
    int retval = 0;
    CollectFaces* collection = (CollectFaces*)inParam;

    if(FACENAME_MAX > collection->mCount)
    {
        retval = 1;

        collection->mNames[collection->mCount] = _tcsdup(inLF->lfFaceName);
        if(NULL != collection->mNames[collection->mCount])
        {
            collection->mCount++;
        }
    }

    return retval;
}

MOZCE_SHUNT_API int mozce_EnumFontFamiliesEx(HDC inDC, const LOGFONTA* inLogfont, FONTENUMPROC inFunc, LPARAM inParam, DWORD inFlags)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_EnumFontFamiliesEx called\n");
#endif

    int retval = 0;

    
    
    if(DEFAULT_CHARSET != inLogfont->lfCharSet)
    {
#ifdef DEBUG
        mozce_printf("mozce_EnumFontFamiliesEx failed\n");
#endif
        SetLastError(ERROR_NOT_SUPPORTED);
        return retval;
    }
     
    CollectFaces collection;
    collection.mCount = 0;
    
    EnumFonts(inDC, NULL, collectProc, (LPARAM)&collection);
    
    UINT loop;
    for(loop = 0; loop < collection.mCount; loop++)
    {
        retval = EnumFonts(inDC, collection.mNames[loop], inFunc, inParam);
    }
    
    for(loop = 0; loop < collection.mCount; loop++)
    {
        free(collection.mNames[loop]);
    }

    return retval;
}

MOZCE_SHUNT_API int mozce_GetMapMode(HDC inDC)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetMapMode called\n");
#endif

    int retval = MM_TEXT;
    return retval;
}


MOZCE_SHUNT_API BOOL mozce_GetIconInfo(HICON inIcon, PICONINFO outIconinfo)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetIconInfo called\n");
#endif

    BOOL retval = FALSE;

    if(NULL != outIconinfo)
    {
        memset(outIconinfo, 0, sizeof(ICONINFO));
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_LPtoDP(HDC inDC, LPPOINT inoutPoints, int inCount)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_LPtoDP called\n");
#endif

    BOOL retval = TRUE;

    return retval;
}


MOZCE_SHUNT_API LONG mozce_RegCreateKey(HKEY inKey, LPCTSTR inSubKey, PHKEY outResult)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_RegCreateKey called\n");
#endif

    LONG retval = ERROR_SUCCESS;
    DWORD disp = 0;

    retval = RegCreateKeyEx(inKey, inSubKey, 0, NULL, 0, 0, NULL, outResult, &disp);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_WaitMessage(VOID)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_WaitMessage called\n");
#endif

    BOOL retval = TRUE;

    HANDLE hThread = GetCurrentThread();
    DWORD waitRes = MsgWaitForMultipleObjectsEx(1, &hThread, INFINITE, QS_ALLEVENTS, 0);
    if((DWORD)-1 == waitRes)
    {
        retval = FALSE;
    }

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_FlashWindow(HWND inWnd, BOOL inInvert)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_FlashWindow called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


typedef struct ECWWindows
{
    LPARAM      params;
    WNDENUMPROC func;
    HWND        parent;
} ECWWindows;

static BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    ECWWindows *myParams = (ECWWindows*) lParam;

    if (IsChild(myParams->parent, hwnd))
    {
        return myParams->func(hwnd, myParams->params);
    }

    return TRUE;
}

MOZCE_SHUNT_API BOOL mozce_EnumChildWindows(HWND inParent, WNDENUMPROC inFunc, LPARAM inParam)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_EnumChildWindows called\n");
#endif

    ECWWindows myParams;
    myParams.params = inParam;
    myParams.func   = inFunc;
    myParams.parent = inParent;

    return EnumWindows(MyEnumWindowsProc, (LPARAM) &myParams);
}


MOZCE_SHUNT_API BOOL mozce_EnumThreadWindows(DWORD inThreadID, WNDENUMPROC inFunc, LPARAM inParam)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_EnumThreadWindows called\n");
#endif
    return FALSE; 
}


MOZCE_SHUNT_API BOOL mozce_IsIconic(HWND inWnd)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_IsIconic called\n");
#endif

    BOOL retval = FALSE;
    return retval;
}


MOZCE_SHUNT_API BOOL mozce_OpenIcon(HWND inWnd)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_OpenIcon called\n");
#endif
    return SetActiveWindow(inWnd) ? 1:0;
}


MOZCE_SHUNT_API HHOOK mozce_SetWindowsHookEx(int inType, void* inFunc, HINSTANCE inMod, DWORD inThreadId)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetWindowsHookEx called\n");
#endif

    HHOOK retval = NULL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_UnhookWindowsHookEx(HHOOK inHook)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_UnhookWindowsHookEx called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API LRESULT mozce_CallNextHookEx(HHOOK inHook, int inCode, WPARAM wParam, LPARAM lParam)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_CallNextHookEx called\n");
#endif

    LRESULT retval = NULL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_InvertRgn(HDC inDC, HRGN inRGN)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_InvertRgn called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API int mozce_GetScrollPos(HWND inWnd, int inBar)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetScrollPos called\n");
#endif

    int retval = 0;
    SCROLLINFO info;

    if(GetScrollInfo(inWnd, inBar, &info))
    {
        return info.nPos;
    }

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_GetScrollRange(HWND inWnd, int inBar, LPINT outMinPos, LPINT outMaxPos)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetScrollRange called\n");
#endif

    BOOL retval = FALSE;
    SCROLLINFO info;

    if((retval = GetScrollInfo(inWnd, inBar, &info)))
    {
        if(NULL != outMinPos)
        {
            *outMinPos = info.nMin;
        }
        if(NULL != outMaxPos)
        {
            *outMaxPos = info.nMax;
        }
    }

    return retval;
}


MOZCE_SHUNT_API HRESULT mozce_CoLockObjectExternal(IUnknown* inUnk, BOOL inLock, BOOL inLastUnlockReleases)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_CoLockObjectExternal called\n");
#endif

    HRESULT retval = S_OK;

    if(NULL != inUnk)
    {
        if(FALSE == inLock)
        {
            inUnk->Release();
        }
        else
        {
            inUnk->AddRef();
        }
    }
    else
    {
        retval = E_INVALIDARG;
    }

    return retval;
}

MOZCE_SHUNT_API HRESULT mozce_CoInitialize(LPVOID pvReserved)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_Conitialize called\n");
#endif

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    return S_OK;

}

MOZCE_SHUNT_API LRESULT mozce_OleInitialize(LPVOID pvReserved)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_OleInitialize called\n");
#endif

    return S_OK;
}

MOZCE_SHUNT_API void mozce_OleUninitialize()
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_OleUninitialize called\n");
#endif
}

MOZCE_SHUNT_API HRESULT mozce_OleQueryLinkFromData(IDataObject* inSrcDataObject)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_OleQueryLinkFromData called\n");
#endif

    HRESULT retval = E_NOTIMPL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API void* mozce_SHBrowseForFolder(void*  inBI)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SHBrowseForFolder called\n");
#endif

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


MOZCE_SHUNT_API BOOL mozce_SetMenu(HWND inWnd, HMENU inMenu)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetMenu called\n");
#endif
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


MOZCE_SHUNT_API BOOL mozce_GetUserName(LPTSTR inBuffer, LPDWORD inoutSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetUserName called\n");
#endif

    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    *inoutSize = 0;

    return retval;
}


MOZCE_SHUNT_API DWORD mozce_GetShortPathName(LPCSTR inLongPath, LPSTR outShortPath, DWORD inBufferSize)
{
    MOZCE_PRECHECK
        
#ifdef DEBUG
   mozce_printf("mozce_GetShortPathName called\n");
#endif
    
    DWORD retval = strlen(inLongPath);
    strncpy(outShortPath, inLongPath, inBufferSize);
    return ((retval > inBufferSize) ? inBufferSize : retval);
}

MOZCE_SHUNT_API DWORD mozce_GetShortPathNameW(LPCWSTR inLongPath, LPWSTR outShortPath, DWORD inBufferSize)
{
    MOZCE_PRECHECK
        
#ifdef DEBUG
   mozce_printf("mozce_GetShortPathNameW called\n");
#endif
    
    DWORD retval = wcslen(inLongPath);
    wcsncpy(outShortPath, inLongPath, inBufferSize);
    return ((retval > inBufferSize) ? inBufferSize : retval);
}

MOZCE_SHUNT_API DWORD mozce_GetEnvironmentVariable(LPCSTR lpName, LPCSTR lpBuffer, DWORD nSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetEnvironmentVariable called\n");
#endif

    DWORD retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}

MOZCE_SHUNT_API DWORD mozce_GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetEnvironmentVariableW called\n");
#endif

    DWORD retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}

MOZCE_SHUNT_API void mozce_GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetSystemTimeAsFileTime called\n");
#endif

    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,lpSystemTimeAsFileTime);
}

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif


#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif

MOZCE_SHUNT_API DWORD mozce_GetFullPathName(const char* lpFileName, 
                                            DWORD nBufferLength, 
                                            const char* lpBuffer, 
                                            const char** lpFilePart)
{
#ifdef DEBUG
    mozce_printf("mozce_GetFullPathName called\n");
#endif

    DWORD len = strlen(lpFileName);
    if (len > nBufferLength)
        return len;
    
    strncpy((char*)lpBuffer, lpFileName, len);
    ((char*)lpBuffer)[len] = '\0';
    
    if(lpFilePart)
    {
        char* sep = strrchr(lpBuffer, '\\');
        if (sep) {
            sep++; 
            *lpFilePart = sep;
        }
        else
            *lpFilePart = lpBuffer;
    }
    
#ifdef DEBUG
    mozce_printf("mozce_GetFullPathName called %s (%s)\n", lpBuffer, *lpFilePart);
#endif
    return len;	
}

static LONG gGetMessageTime = 0;

MOZCE_SHUNT_API BOOL mozce_GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax )
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetMessage called\n");
#endif

    BOOL b = GetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMin);

    if (b)
        gGetMessageTime = lpMsg->time;

    return b;
}


MOZCE_SHUNT_API BOOL mozce_PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    MOZCE_PRECHECK

#ifdef LOUD_PEEKMESSAGE
#ifdef DEBUG
    mozce_printf("mozce_PeekMessageA called\n");
#endif
#endif

    BOOL b = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    
    if (b && wRemoveMsg == PM_REMOVE)
        gGetMessageTime = lpMsg->time; 
    
    return b;
}


MOZCE_SHUNT_API LONG mozce_GetMessageTime(void)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetMessageTime called\n");
#endif

  return gGetMessageTime;
}

MOZCE_SHUNT_API UINT mozce_GetACP(void)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetACP called\n");
#endif

    return GetACP();
}



MOZCE_SHUNT_API DWORD mozce_ExpandEnvironmentStrings(LPCTSTR lpSrc, LPTSTR lpDst, DWORD nSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_ExpandEnvironmentStrings called\n");
#endif

    return 0;
}

MOZCE_SHUNT_API DWORD mozce_ExpandEnvironmentStringsW(const unsigned short* lpSrc, const unsigned short* lpDst, DWORD nSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_ExpandEnvironmentStrings called\n");
#endif

    return 0;
}

MOZCE_SHUNT_API BOOL mozce_GdiFlush(void)
{
    MOZCE_PRECHECK

    return TRUE;
}

MOZCE_SHUNT_API BOOL mozce_GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
   MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_GetWindowPlacement called\n");
#endif

   memset(lpwndpl, 0, sizeof(WINDOWPLACEMENT));
   
   
   lpwndpl->showCmd = SW_SHOWNORMAL;
   GetWindowRect(hWnd, &lpwndpl->rcNormalPosition);
   
   return TRUE;
}

MOZCE_SHUNT_API HINSTANCE mozce_ShellExecute(HWND hwnd, 
                                             LPCSTR lpOperation, 
                                             LPCSTR lpFile, 
                                             LPCSTR lpParameters, 
                                             LPCSTR lpDirectory, 
                                             INT nShowCmd)
{
    
    LPTSTR op   = a2w_malloc(lpOperation, -1, NULL);
    LPTSTR file = a2w_malloc(lpFile, -1, NULL);
    LPTSTR parm = a2w_malloc(lpParameters, -1, NULL);
    LPTSTR dir  = a2w_malloc(lpDirectory, -1, NULL);
    
    SHELLEXECUTEINFO info;
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.fMask  = SEE_MASK_NOCLOSEPROCESS;
    info.hwnd   = hwnd;
    info.lpVerb = op;
    info.lpFile = file;
    info.lpParameters = parm;
    info.lpDirectory  = dir;
    info.nShow  = nShowCmd;
    
    BOOL b = ShellExecuteEx(&info);
    
    if (op)
        free(op);
    if (file)
        free(file);
    if (parm)
        free(parm);
    if (dir)
        free(dir);
    
    return (HINSTANCE) info.hProcess;
}

MOZCE_SHUNT_API HINSTANCE mozce_ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
   MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_ShellExecuteW called\n");
#endif

    SHELLEXECUTEINFO info;
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.fMask  = SEE_MASK_NOCLOSEPROCESS;
    info.hwnd   = hwnd;
    info.lpVerb = lpOperation;
    info.lpFile = lpFile;
    info.lpParameters = lpParameters;
    info.lpDirectory  = lpDirectory;
    info.nShow  = nShowCmd;
    
    BOOL b = ShellExecuteEx(&info);

    return (HINSTANCE) info.hProcess;
}

struct lconv s_locale_conv =
{
    ".",   
    ",",   
    "333", 
    "$",   
    "$",   
    "",    
    "",    
    "",    
    "+",   
    "-",   
    '2',   
    '2',   
    1,     
    1,     
    1,     
    1,     
    1,     
    1,     
};



MOZCE_SHUNT_API struct lconv * mozce_localeconv(void)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_localeconv called\n");
#endif
    return &s_locale_conv;
}

MOZCE_SHUNT_API BOOL mozce_CreatePipe(PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_CreatePipe called\n");
#endif
    return FALSE;
}

MOZCE_SHUNT_API DWORD_PTR mozce_SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetThreadAffinityMask called\n");
#endif
    return 0;
}

MOZCE_SHUNT_API BOOL mozce_GetProcessAffinityMask(HANDLE hProcess, PDWORD_PTR lpProcessAffinityMask, PDWORD_PTR lpSystemAffinityMask)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetProcessAffinityMask called\n");
#endif
    return FALSE;
}


MOZCE_SHUNT_API HANDLE mozce_OpenFileMapping(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_OpenFileMapping called\n");
#endif
    return 0;
}

MOZCE_SHUNT_API UINT mozce_GetDriveType(const char* lpRootPathName)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetDriveType called\n");
#endif

    return 0;
}



MOZCE_SHUNT_API BOOL mozce_AlphaBlend(
                                      HDC hdcDest,                 
                                      int nXOriginDest,            
                                      int nYOriginDest,            
                                      int nWidthDest,              
                                      int nHeightDest,             
                                      HDC hdcSrc,                  
                                      int nXOriginSrc,             
                                      int nYOriginSrc,             
                                      int nWidthSrc,               
                                      int nHeightSrc,              
                                      BLENDFUNCTION blendFunction  
                                      ){
    DWORD SCA = blendFunction.SourceConstantAlpha;
    int w = MIN(nWidthSrc,nWidthDest);
    int h = MIN(nHeightSrc, nHeightDest);
    for ( int x = 0; x<= w; x++){
        for( int y = 0; y<=h; y++){
            COLORREF dc = GetPixel(hdcDest, nXOriginDest+x, nYOriginDest+y);
            COLORREF sc = GetPixel(hdcSrc,  nXOriginSrc+x,  nYOriginSrc+y);
            color Src,Dst;
            Src.Red = GetRValue(sc);
            Dst.Red = GetRValue(dc);
            Src.Green = GetGValue(sc);
            Dst.Green = GetGValue(dc);
            Src.Blue = GetBValue(sc);
            Dst.Blue = GetBValue(dc);
            
            Src.Alpha = 1.0 - (double)((sc >> 24)/255.0);
            Dst.Alpha = 1.0 - (double)((dc >> 24)/255.0);
            
            
            
            
            
            if(blendFunction.AlphaFormat & AC_SRC_ALPHA){
                Dst.Red 	= (unsigned char)(Src.Red * (SCA/255.0) 	+ Dst.Red * (1.0 - (SCA/255.0)));
                Dst.Green 	= (unsigned char)(Src.Green * (SCA/255.0) 	+ Dst.Green * (1.0 - (SCA/255.0)));
                Dst.Blue 	= (unsigned char)(Src.Blue * (SCA/255.0) 	+ Dst.Blue * (1.0 - (SCA/255.0)));
                Dst.Alpha 	= MAX(0,MIN(1,Src.Alpha * (SCA/255.0) 	+ Dst.Alpha * (1.0 - (SCA/255.0))));
            }else if(SCA == 0xff){
                Dst.Red 	= (unsigned char)(Src.Alpha*Src.Red 	+ (1 - Src.Alpha) * Dst.Red);
                Dst.Green 	= (unsigned char)(Src.Alpha*Src.Green 	+ (1 - Src.Alpha) * Dst.Green);
                Dst.Blue 	= (unsigned char)(Src.Alpha*Src.Blue 	+ (1 - Src.Alpha) * Dst.Blue);
                Dst.Alpha 	= MAX(0,MIN(1,Src.Alpha 	+ (1 - Src.Alpha) * Dst.Alpha));
            }else{
                Src.Red 	= (unsigned char)(Src.Red 	* SCA / 255.0);
                Src.Green 	= (unsigned char)(Src.Green 	* SCA / 255.0);
                Src.Blue 	= (unsigned char)(Src.Blue 	* SCA / 255.0);
                Src.Alpha 	= MAX(0,MIN(1,Src.Alpha 	* SCA / 255.0));
                double t = (Src.Red 	+ (1 - Src.Alpha) * Dst.Red);
                Dst.Red 	= (unsigned char)(t>255?255:t);
                t = (Src.Green 	+ (1 - Src.Alpha) * Dst.Green);
                Dst.Green 	= (unsigned char)(t>255?255:t);
                t = (Src.Blue 	+ (1 - Src.Alpha) * Dst.Blue);
                Dst.Blue 	= (unsigned char)(t>255?255:t);
                Dst.Alpha 	= MAX(0,MIN(1,Src.Alpha 	+ (1 - Src.Alpha) * Dst.Alpha));
            }
            SetPixel(hdcDest,nXOriginDest+x, nYOriginDest+y, RGB(Dst.Red,Dst.Green,Dst.Blue));
            
            
        }
    }
    
    
    return true;
    
}


MOZCE_SHUNT_API BOOL mozce_SetHandleInformation(HANDLE hObject, DWORD dwMask, DWORD dwFlags)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_SetHandleInformation called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}

MOZCE_SHUNT_API BOOL mozce_GetHandleInformation(HANDLE hObject, LPDWORD lpdwFlags)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetHandleInformation called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZCE_SHUNT_API BOOL mozce_LockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                                    DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_LockFile called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}

MOZCE_SHUNT_API BOOL mozce_UnlockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                                    DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_UnlockFile called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}

MOZCE_SHUNT_API BOOL mozce_GetDiskFreeSpaceA(LPCTSTR lpRootPathName, LPDWORD lpSectorsPerCluster, 
                                            LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_GetDiskFreeSpace called\n");
#endif

    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}

#if 0
{
#endif
} 

void dumpMemoryInfo()
{
    MEMORYSTATUS ms;
    ms.dwLength = sizeof(MEMORYSTATUS);
    
    
    GlobalMemoryStatus(&ms);
    
    wprintf(L"-> %d %d %d %d %d %d %d\n", 
            ms.dwMemoryLoad, 
            ms.dwTotalPhys, 
            ms.dwAvailPhys, 
            ms.dwTotalPageFile, 
            ms.dwAvailPageFile, 
            ms.dwTotalVirtual, 
            ms.dwAvailVirtual);
}
