




































#ifndef WindowAPI_h__
#define WindowAPI_h__

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

typedef LRESULT (WINAPI *NS_DefWindowProc) (HWND, UINT, WPARAM, LPARAM);
typedef LRESULT (WINAPI *NS_CallWindowProc) (WNDPROC, HWND, UINT, WPARAM, LPARAM);
typedef LONG (WINAPI *NS_SetWindowLong) (HWND, int, LONG);
typedef LONG (WINAPI *NS_GetWindowLong) (HWND, int);
typedef LRESULT (WINAPI *NS_SendMessage) (HWND, UINT, WPARAM, LPARAM )  ;
typedef LONG (WINAPI *NS_DispatchMessage) (CONST MSG *);
typedef BOOL (WINAPI *NS_GetMessage) (LPMSG, HWND, UINT, UINT);
typedef BOOL (WINAPI *NS_PeekMessage) (LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL (WINAPI *NS_GetOpenFileName) (LPOPENFILENAMEW);
typedef BOOL (WINAPI *NS_GetSaveFileName) (LPOPENFILENAMEW);
typedef int (WINAPI *NS_GetClassName) (HWND, LPWSTR, int);
typedef HWND (WINAPI *NS_CreateWindowEx) 
          (DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef ATOM (WINAPI *NS_RegisterClass) (CONST WNDCLASSW *); 
typedef BOOL (WINAPI *NS_UnregisterClass) (LPCWSTR, HINSTANCE); 
typedef BOOL (WINAPI *NS_SHGetPathFromIDList) (LPCITEMIDLIST, LPWSTR);

#ifndef WINCE
typedef LPITEMIDLIST (WINAPI *NS_SHBrowseForFolder) (LPBROWSEINFOW);
#endif


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(pt) (short(LOWORD(pt)))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(pt) (short(HIWORD(pt)))
#endif

#endif 
