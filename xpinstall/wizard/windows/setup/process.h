




































#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "windows.h"

#define WM_CLOSE_TIMEOUT_VALUE      10000 /* 3 secs */


#define KP_KILL_PROCESS             TRUE
#define KP_DO_NOT_KILL_PROCESS      FALSE

BOOL FindAndKillProcess(LPSTR aProcessName, BOOL aKillProcess);
BOOL CloseAllWindowsOfWindowHandle(HWND hwndWindow, char *aMsgWait);

#endif 
