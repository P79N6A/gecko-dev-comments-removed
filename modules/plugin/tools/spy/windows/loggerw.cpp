




































#include "xp.h"
#include "windowsx.h"

#include "resource.h"
#include "loggerw.h"
#include "profilew.h"
#include "actionnames.h"

extern HINSTANCE hInst;
static char szClassName[] = "NPSpyWindowClass";

BOOL CALLBACK MainDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PauseDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LoggerWin::LoggerWin() : Logger(),
  hWnd(NULL),
  width(0),
  height(0),
  x(0),
  y(0),
  bSaveSettings(FALSE)
{
}

LoggerWin::~LoggerWin()
{
}

BOOL LoggerWin::platformInit()
{
  WNDCLASS wc;
  wc.style         = 0; 
  wc.lpfnWndProc   = DefDlgProc; 
  wc.cbClsExtra    = 0; 
  wc.cbWndExtra    = DLGWINDOWEXTRA; 
  wc.hInstance     = hInst; 
  wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_APP)); 
  wc.hCursor       = LoadCursor(0, IDC_ARROW); 
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL; 
  wc.lpszClassName = szClassName;

  if(!RegisterClass(&wc))
    return FALSE;

  
  ProfileWin profile;

  profile.getBool(NPSPY_REG_KEY_ONTOP, &bOnTop);
  profile.getBool(NPSPY_REG_KEY_LOGTOWINDOW, &bToWindow);
  profile.getBool(NPSPY_REG_KEY_LOGTOCONSOLE, &bToConsole);
  profile.getBool(NPSPY_REG_KEY_LOGTOFILE, &bToFile);
  profile.getBool(NPSPY_REG_KEY_SPALID, &bSPALID);
  profile.getString(NPSPY_REG_KEY_LOGFILENAME, szFile, strlen(szFile));

  for(int i = 1; i < TOTAL_NUMBER_OF_API_CALLS; i++)
  {
    BOOL selected = TRUE;
    if(profile.getBool(ActionName[i], &selected))
      bMutedCalls[i] = !selected;
  }
  
  if(!profile.getSizeAndPosition(&width, &height, &x, &y))
  {
    width = 0;
    height = 0;
    x = 0;
    y = 0;
  }

  hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_MAIN), GetDesktopWindow(), (DLGPROC)MainDlgProc, (LPARAM)this);
  if(hWnd == NULL)
  {
    UnregisterClass(szClassName, hInst);
    return FALSE;
  }

  if(bOnTop)
    SetWindowPos(hWnd, bOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);

  return TRUE;
}

void LoggerWin::platformShut()
{
  if(hWnd != NULL)
  {
    char szLog[] = "--- GOING AWAY... PRESS SPACE BAR TO CONTINUE ---";
    HWND hWndOutput = GetDlgItem(hWnd, IDC_MAIN_OUTPUT);
    ListBox_AddString(hWndOutput, "");
    ListBox_AddString(hWndOutput, szLog);
    int count = ListBox_GetCount(hWndOutput);
    ListBox_SetCaretIndex(hWndOutput, count - 1);
    UpdateWindow(hWndOutput);

    DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PAUSE), hWnd, (DLGPROC)PauseDlgProc);

    ProfileWin profile;

    RECT rc;
    if(GetWindowRect(hWnd, &rc))
      profile.setSizeAndPosition(rc.right - rc.left, rc.bottom - rc.top, rc.left, rc.top);

    DestroyWindow(hWnd);
    hWnd = NULL;
  }

  UnregisterClass(szClassName, hInst);
}

void LoggerWin::onDestroyWindow()
{
  hWnd = NULL;
}

void LoggerWin::dumpStringToMainWindow(char * string)
{
  
  char * p = strrchr(string, '\n');
  if(p)
    *p = '\0';

  p = strrchr(string, '\r');
  if(p)
    *p = '\0';

  HWND hWndOutput = GetDlgItem(hWnd, IDC_MAIN_OUTPUT);
  ListBox_AddString(hWndOutput, string);
  int count = ListBox_GetCount(hWndOutput);
  if(count == 32767)
    ListBox_ResetContent(hWndOutput);
  ListBox_SetCaretIndex(hWndOutput, count - 1);
  UpdateWindow(hWndOutput);
}

void LoggerWin::onClear()
{
  HWND hWndOutput = GetDlgItem(hWnd, IDC_MAIN_OUTPUT);
  ListBox_ResetContent(hWndOutput);
  UpdateWindow(hWndOutput);
}

Logger * NewLogger()
{
  LoggerWin * res = new LoggerWin();
  return res;
}

void DeleteLogger(Logger * logger)
{
  if(logger)
    delete logger;
}