






































#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include <io.h>
#include "resource.h"
#include "progressui.h"
#include "readstrings.h"
#include "errors.h"

#define TIMER_ID 1
#define TIMER_INTERVAL 100

#define RESIZE_WINDOW(hwnd, extrax, extray) \
  { \
    RECT windowSize; \
    GetWindowRect(hwnd, &windowSize); \
    SetWindowPos(hwnd, 0, 0, 0, windowSize.right - windowSize.left + extrax, \
                 windowSize.bottom - windowSize.top + extray, \
                 SWP_NOMOVE | SWP_NOZORDER); \
  }

#define MOVE_WINDOW(hwnd, dx, dy) \
  { \
    WINDOWPLACEMENT windowPos; \
    windowPos.length = sizeof(windowPos); \
    GetWindowPlacement(hwnd, &windowPos); \
    SetWindowPos(hwnd, 0, windowPos.rcNormalPosition.left + dx, windowPos.rcNormalPosition.top + dy, 0, 0, \
                 SWP_NOSIZE | SWP_NOZORDER); \
  }

static float sProgress;  
static BOOL  sQuit = FALSE;

static BOOL
GetStringsFile(WCHAR filename[MAX_PATH])
{
  if (!GetModuleFileNameW(NULL, filename, MAX_PATH))
    return FALSE;
 
  WCHAR *dot = wcsrchr(filename, '.');
  if (!dot || wcsicmp(dot + 1, L"exe"))
    return FALSE;

  wcscpy(dot + 1, L"ini");
  return TRUE;
}

static void
UpdateDialog(HWND hDlg)
{
  int pos = int(sProgress + 0.5f);
  SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, pos, 0L);
}

static void
ResizeDialogToFit(HWND hDlg)
{
  WCHAR text[MAX_TEXT_LEN];
  RECT infoSize, textSize;
  HFONT hInfoFont, hOldFont;

  HWND hWndInfo = GetDlgItem(hDlg, IDC_INFO);
  HWND hWndPro  = GetDlgItem(hDlg, IDC_PROGRESS);

  
  if (!GetWindowText(hWndInfo, text, sizeof(text)))
    return;

  
  GetClientRect(hWndInfo, &infoSize);
  HDC hDCInfo = GetDC(hWndInfo);
  hInfoFont = (HFONT)SendMessage(hWndInfo, WM_GETFONT, 0, 0);
  if (hInfoFont)
    hOldFont = (HFONT)SelectObject(hDCInfo, hInfoFont);

  
  if (DrawText(hDCInfo, text, -1, &textSize,
               DT_CALCRECT | DT_NOCLIP | DT_SINGLELINE)) {
    SIZE extra;
    extra.cx = (textSize.right - textSize.left) - (infoSize.right - infoSize.left);
    extra.cy = (textSize.bottom - textSize.top) - (infoSize.bottom - infoSize.top);
    if (extra.cx < 0)
      extra.cx = 0;
    if (extra.cy < 0)
      extra.cy = 0;

    if ((extra.cx > 0) || (extra.cy > 0)) {
      RESIZE_WINDOW(hDlg, extra.cx, extra.cy);
      RESIZE_WINDOW(hWndInfo, extra.cx, extra.cy);
      RESIZE_WINDOW(hWndPro, extra.cx, 0);
      MOVE_WINDOW(hWndPro, 0, extra.cy);
    }
  }

  if (hOldFont)
    SelectObject(hDCInfo, hOldFont);

  ReleaseDC(hWndInfo, hDCInfo);
}



static void
CenterDialog(HWND hDlg)
{
  RECT rc, rcOwner, rcDlg;

  
  HWND desktop = GetDesktopWindow();

  GetWindowRect(desktop, &rcOwner); 
  GetWindowRect(hDlg, &rcDlg); 
  CopyRect(&rc, &rcOwner); 

  
  
  
  

  OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
  OffsetRect(&rc, -rc.left, -rc.top); 
  OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

  
  

  SetWindowPos(hDlg, 
               HWND_TOP, 
               rcOwner.left + (rc.right / 2), 
               rcOwner.top + (rc.bottom / 2), 
               0, 0,          
               SWP_NOSIZE); 
}

static void
InitDialog(HWND hDlg)
{
  WCHAR filename[MAX_PATH];
  if (!GetStringsFile(filename))
    return;

  StringTable uiStrings;
  if (ReadStrings(filename, &uiStrings) != OK)
    return;

  WCHAR szwTitle[MAX_TEXT_LEN];
  WCHAR szwInfo[MAX_TEXT_LEN];

  MultiByteToWideChar(CP_UTF8, 0, uiStrings.title, strlen(uiStrings.title) + 1,
                      szwTitle, sizeof(szwTitle)/sizeof(szwTitle[0]));
  MultiByteToWideChar(CP_UTF8, 0, uiStrings.info, strlen(uiStrings.info) + 1,
                      szwInfo, sizeof(szwInfo)/sizeof(szwInfo[0]));

  SetWindowTextW(hDlg, szwTitle);
  SetWindowTextW(GetDlgItem(hDlg, IDC_INFO), szwInfo);

  
  HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DIALOG));
  if (hIcon)
    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

  SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

  
  ResizeDialogToFit(hDlg);

  CenterDialog(hDlg);  

  SetTimer(hDlg, TIMER_ID, TIMER_INTERVAL, NULL);
}


static LRESULT CALLBACK
DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    InitDialog(hDlg);
    return TRUE;

  case WM_TIMER:
    if (sQuit) {
      EndDialog(hDlg, 0);
    } else {
      UpdateDialog(hDlg);
    }
    return TRUE;

  case WM_COMMAND:
    return TRUE;
  }
  return FALSE;
}

int
InitProgressUI(int *argc, NS_tchar ***argv)
{
  return 0;
}

int
ShowProgressUI()
{
  
  

  Sleep(500);

  if (sQuit || sProgress > 50.0f)
    return 0;

  
  WCHAR filename[MAX_PATH];
  if (!GetStringsFile(filename))
    return -1;
  if (_waccess(filename, 04))
    return -1;

  INITCOMMONCONTROLSEX icc = {
    sizeof(INITCOMMONCONTROLSEX),
    ICC_PROGRESS_CLASS
  };
  InitCommonControlsEx(&icc);

  DialogBox(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_DIALOG), NULL,
            (DLGPROC) DialogProc);

  return 0;
}

void
QuitProgressUI()
{
  sQuit = TRUE;
}

void
UpdateProgressUI(float progress)
{
  sProgress = progress;  
}
