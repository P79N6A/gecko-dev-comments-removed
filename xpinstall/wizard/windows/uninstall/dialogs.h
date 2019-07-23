






































#ifndef _DIALOGS_H_
#define _DIALOGS_H_

LRESULT CALLBACK  DlgProcUninstall(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcWhatToDo(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcMessage(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);

void              ParseAllUninstallLogs();
void              ParseDefaultsInfo();
void              SetDefault();
HWND              InstantiateDialog(HWND hParent, DWORD dwDlgID, LPSTR szTitle, WNDPROC wpDlgProc);
void              ShowMessage(LPSTR szMessage, BOOL bShow);
void              ProcessWindowsMessages(void);

#endif

