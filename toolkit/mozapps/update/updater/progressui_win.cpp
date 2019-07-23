






































#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include <io.h>

#ifdef WINCE_WINDOWS_MOBILE
#include <aygshell.h>
#endif

#include "resource.h"
#include "progressui.h"
#include "readstrings.h"
#include "errors.h"

#ifdef WINCE
#include "updater_wince.h"
#endif

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
    RECT rc; \
    POINT pt; \
    GetWindowRect(hwnd, &rc); \
    pt.x = rc.left; \
    pt.y = rc.top; \
    ScreenToClient(GetParent(hwnd), &pt); \
    SetWindowPos(hwnd, 0, pt.x + dx, pt.y + dy, 0, 0, \
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
  HWND hWndPro = GetDlgItem(hDlg, IDC_PROGRESS);
  SendMessage(hWndPro, PBM_SETPOS, pos, 0L);
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

  HWND hWndPro = GetDlgItem(hDlg, IDC_PROGRESS);
  SendMessage(hWndPro, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

  
  RECT infoSize, textSize;
  HWND hWndInfo = GetDlgItem(hDlg, IDC_INFO);

  
  HDC hDCInfo = GetDC(hWndInfo);
  HFONT hInfoFont, hOldFont;
  hInfoFont = (HFONT)SendMessage(hWndInfo, WM_GETFONT, 0, 0);

  if (hInfoFont)
    hOldFont = (HFONT)SelectObject(hDCInfo, hInfoFont);

  
  
  
  
  
  
#ifdef WINCE
#ifdef WINCE_WINDOWS_MOBILE
  RECT rcDlgInner1, rcDlgInner2, rcInfoOuter1, rcInfoOuter2;
  
  
  
  GetClientRect(hDlg, &rcDlgInner1);
  GetWindowRect(hWndInfo, &rcInfoOuter1);

  
  SHINITDLGINFO shidi;
  shidi.dwMask = SHIDIM_FLAGS;
  shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
  shidi.hDlg = hDlg;
  SHInitDialog(&shidi);
  if (!SHInitDialog(&shidi))
    return;

  
  SHDoneButton(hDlg, SHDB_HIDE);

  GetClientRect(hDlg, &rcDlgInner2);
  GetWindowRect(hWndInfo, &rcInfoOuter2);
  textSize.left = 0;
  
  
  
  
  textSize.right = (rcInfoOuter2.right - rcInfoOuter2.left) + \
                   (rcDlgInner2.right - rcDlgInner1.right) + \
                   (rcInfoOuter1.left - rcInfoOuter2.left);
#else
  RECT rcWorkArea, rcInfoOuter1;
  GetWindowRect(hWndInfo, &rcInfoOuter1);
  SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcWorkArea, NULL);
  textSize.left = 0;
  
  
  textSize.right = (rcWorkArea.right - rcWorkArea.left) - \
                   (rcInfoOuter1.left + rcInfoOuter1.right);
#endif
  
  
  if (DrawText(hDCInfo, szwInfo, -1, &textSize,
               DT_CALCRECT | DT_NOCLIP | DT_WORDBREAK)) {
    GetClientRect(hWndInfo, &infoSize);
    SIZE extra;
    
    
    
    extra.cx = (textSize.right - textSize.left) - \
               (infoSize.right - infoSize.left);
    extra.cy = (textSize.bottom - textSize.top) - \
               (infoSize.bottom - infoSize.top);
    
    
    
    
    extra.cx += 2;

    RESIZE_WINDOW(hWndInfo, extra.cx, extra.cy);
    RESIZE_WINDOW(hWndPro, extra.cx, 0);

#ifdef WINCE_WINDOWS_MOBILE
    
    
    
    MOVE_WINDOW(hWndInfo, -1, 0);
    MOVE_WINDOW(hWndPro, -1, extra.cy);
#else
    RESIZE_WINDOW(hDlg, extra.cx, extra.cy);
    MOVE_WINDOW(hWndPro, 0, extra.cy);
#endif
  }

#else
  
  
  if (DrawText(hDCInfo, szwInfo, -1, &textSize,
               DT_CALCRECT | DT_NOCLIP | DT_SINGLELINE)) {
    GetClientRect(hWndInfo, &infoSize);
    SIZE extra;
    
    
    
    extra.cx = (textSize.right - textSize.left) - \
               (infoSize.right - infoSize.left);
    extra.cy = (textSize.bottom - textSize.top) - \
               (infoSize.bottom - infoSize.top);
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
#endif

  if (hOldFont)
    SelectObject(hDCInfo, hOldFont);

  ReleaseDC(hWndInfo, hDCInfo);

  
#ifndef WINCE_WINDOWS_MOBILE
  CenterDialog(hDlg);  
#endif

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
