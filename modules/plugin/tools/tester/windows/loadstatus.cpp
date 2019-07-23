




































#include <windows.h>
#include <windowsx.h>

extern HINSTANCE hInst;
static char szClassName[] = "LoadStatusWindowClass";
HBRUSH hBrushBackground = NULL;

HWND ShowLoadStatus(char * aMessage)
{
  if (!aMessage)
    return NULL;

  LOGBRUSH lb;
  lb.lbStyle = BS_SOLID;
  lb.lbColor = RGB(255,255,0);
  lb.lbHatch = NULL;
  hBrushBackground = CreateBrushIndirect(&lb);

  WNDCLASS wc;
  wc.style         = 0; 
  wc.lpfnWndProc   = DefWindowProc; 
  wc.cbClsExtra    = 0; 
  wc.cbWndExtra    = 0; 
  wc.hInstance     = hInst; 
  wc.hIcon         = NULL; 
  wc.hCursor       = NULL; 
  wc.hbrBackground = hBrushBackground ? hBrushBackground : (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL; 
  wc.lpszClassName = szClassName;

  if(!RegisterClass(&wc)) {
    DeleteBrush(hBrushBackground);
    return NULL;
  }
  
  HWND hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szClassName, "", 
                             WS_POPUP | WS_VISIBLE | WS_DISABLED,
                             0,0,0,0, NULL, NULL, hInst, NULL);
  if (!hWnd) {
    UnregisterClass(szClassName, hInst);
    DeleteBrush(hBrushBackground);
    return NULL;
  }

  HDC hDC = GetDC(hWnd);
  if (!hDC) {
    DestroyWindow(hWnd);
    UnregisterClass(szClassName, hInst);
    DeleteBrush(hBrushBackground);
    return NULL;
  }

  HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);
  HFONT hFontOld = SelectFont(hDC, hFont);
  SIZE size;
  GetTextExtentPoint32(hDC, aMessage, strlen(aMessage), &size);
  SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, size.cx + 4, size.cy + 2, SWP_SHOWWINDOW);
  SetBkMode(hDC, TRANSPARENT);
  TextOut(hDC, 2, 1, aMessage, strlen(aMessage));
  ReleaseDC(hWnd, hDC);

  return hWnd;
}

void DestroyLoadStatus(HWND ahWnd)
{
  if (ahWnd)
    DestroyWindow(ahWnd);

  UnregisterClass(szClassName, hInst);
  DeleteBrush(hBrushBackground);
}
