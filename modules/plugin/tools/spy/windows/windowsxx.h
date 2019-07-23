




































#ifndef __WINDOWSXX_H__
#define __WINDOWSXX_H__


#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) \
    (fn)((hwnd), (int)(wParam), (LPNMHDR)lParam)

#endif
