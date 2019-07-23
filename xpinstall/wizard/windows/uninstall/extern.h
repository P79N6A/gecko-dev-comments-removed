






































#ifndef _EXTERN_H_
#define _EXTERN_H_

#include "uninstall.h"


extern HINSTANCE        hInst;
extern HANDLE           hAccelTable;

extern HWND             hDlgUninstall;
extern HWND             hDlgMessage;
extern HWND             hWndMain;

extern LPSTR            szEGlobalAlloc;
extern LPSTR            szEStringLoad;
extern LPSTR            szEDllLoad;
extern LPSTR            szEStringNull;
extern LPSTR            szTempSetupPath;

extern LPSTR            szClassName;
extern LPSTR            szUninstallDir;
extern LPSTR            szTempDir;
extern LPSTR            szOSTempDir;
extern LPSTR            szFileIniUninstall;
extern LPSTR            szFileIniDefaultsInfo;
extern LPSTR            gszSharedFilename;

extern ULONG            ulOSType;
extern DWORD            dwScreenX;
extern DWORD            dwScreenY;

extern DWORD            gdwWhatToDo;

extern BOOL             gbAllowMultipleInstalls;

extern uninstallGen     ugUninstall;
extern diU              diUninstall;

#endif
