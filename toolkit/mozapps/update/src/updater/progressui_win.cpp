






































#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include <io.h>
#include "resource.h"

#define TIMER_ID 1
#define TIMER_INTERVAL 100

static float sProgress;  
static BOOL  sQuit = FALSE;
static HFONT sSystemFont = 0;

static BOOL
GetStringsFile(char filename[MAX_PATH])
{
  if (!GetModuleFileName(NULL, filename, MAX_PATH))
    return FALSE;

  char *dot = strrchr(filename, '.');
  if (!dot || stricmp(dot + 1, "exe"))
    return FALSE;

  strcpy(dot + 1, "ini");
  return TRUE;
}

static void
UpdateDialog(HWND hDlg)
{
  int pos = int(sProgress + 0.5f);
  SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, pos, 0L);
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
SetItemText(HWND hwnd, const char *key, const char *ini)
{
  char text[512];
  if (!GetPrivateProfileString("Strings", key, NULL, text, sizeof(text), ini))
    return;
  SetWindowText(hwnd, text);
}

static void
InitDialog(HWND hDlg)
{
  char filename[MAX_PATH];
  if (!GetStringsFile(filename))
    return;

  SetItemText(hDlg, "Title", filename);
  SetItemText(GetDlgItem(hDlg, IDC_INFO), "Info", filename);

  
  
  
  
  
  if (!sSystemFont) {
    NONCLIENTMETRICS ncm;
    memset(&ncm, 0, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    sSystemFont = CreateFontIndirect(&ncm.lfMessageFont);
  }
  if (sSystemFont)
    SendDlgItemMessage(hDlg, IDC_INFO, WM_SETFONT, (WPARAM)sSystemFont, 0L);

  
  HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DIALOG));
  if (hIcon)
    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

  SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

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
      if (sSystemFont) {
        DeleteObject(sSystemFont);
        sSystemFont = 0;
      }
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
InitProgressUI(int *argc, char ***argv)
{
  return 0;
}

int
ShowProgressUI()
{
  
  

  Sleep(500);

  if (sQuit || sProgress > 50.0f)
    return 0;

  
  char filename[MAX_PATH];
  if (!GetStringsFile(filename))
    return -1;
  if (_access(filename, 04))
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
