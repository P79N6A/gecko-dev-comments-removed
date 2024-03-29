






#include <windows.h>
#include <tchar.h>

#include "SkApplication.h"

#define MAX_LOADSTRING 100


HINSTANCE hInst;                            
TCHAR szTitle[] = _T("SampleApp");          
TCHAR szWindowClass[] = _T("SAMPLEAPP");    


ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, LPTSTR);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    MSG msg;

    
    MyRegisterClass(hInstance);

    
    if (!InitInstance (hInstance, nCmdShow, lpCmdLine))
    {
        return FALSE;
    }

    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (true)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    application_term();

    return (int) msg.wParam;
}
















ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = NULL;
    wcex.hCursor        = NULL;
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL;
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = NULL;

    return RegisterClassEx(&wcex);
}

#include "SkOSWindow_Win.h"
extern SkOSWindow* create_sk_window(void* hwnd, int argc, char** argv);

static SkOSWindow* gSkWind;

char* tchar_to_utf8(const TCHAR* str) {
#ifdef _UNICODE
    int size = WideCharToMultiByte(CP_UTF8, 0, str, wcslen(str), NULL, 0, NULL, NULL);
    char* str8 = (char*) malloc(size+1);
    WideCharToMultiByte(CP_UTF8, 0, str, wcslen(str), str8, size, NULL, NULL);
    str8[size] = '\0';
    return str8;
#else
    return _strdup(str);
#endif
}













BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPTSTR lpCmdLine)
{
   application_init();

   hInst = hInstance; 

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   char* argv[4096];
   int argc = 0;
   TCHAR exename[1024], *next;
   int exenameLen = GetModuleFileName(NULL, exename, SK_ARRAY_COUNT(exename));
   
   (void) exenameLen;
   argv[argc++] = tchar_to_utf8(exename);
   TCHAR* arg = _tcstok_s(lpCmdLine, _T(" "), &next);
   while (arg != NULL) {
      argv[argc++] = tchar_to_utf8(arg);
      arg = _tcstok_s(NULL, _T(" "), &next);
   }

   gSkWind = create_sk_window(hWnd, argc, argv);
   for (int i = 0; i < argc; ++i) {
      free(argv[i]);
   }
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}











LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_COMMAND:
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        if (gSkWind->wndProc(hWnd, message, wParam, lParam)) {
            return 0;
        } else {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
