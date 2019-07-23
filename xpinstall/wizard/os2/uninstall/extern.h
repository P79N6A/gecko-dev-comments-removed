






































#ifndef _EXTERN_H_
#define _EXTERN_H_

#include "uninstall.h"


extern HWND             hDlgUninstall;
extern HWND             hDlgMessage;
extern HWND             hWndMain;

extern PSZ              szEGlobalAlloc;
extern PSZ              szEStringLoad;
extern PSZ              szEDllLoad;
extern PSZ              szEStringNull;
extern PSZ              szTempSetupPath;

extern PSZ              szClassName;
extern PSZ              szUninstallDir;
extern PSZ              szTempDir;
extern PSZ              szOSTempDir;
extern PSZ              szFileIniUninstall;
extern PSZ              szFileIniDefaultsInfo;
extern PSZ              gszSharedFilename;

extern ULONG            ulOSType;
extern ULONG            ulScreenX;
extern ULONG            ulScreenY;
extern ULONG            ulDlgFrameX;
extern ULONG            ulDlgFrameY;
extern ULONG            ulTitleBarY;

extern ULONG            gulWhatToDo;

extern uninstallGen     ugUninstall;
extern diU              diUninstall;

#endif
