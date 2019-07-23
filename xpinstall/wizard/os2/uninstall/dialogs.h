






































#ifndef _DIALOGS_H_
#define _DIALOGS_H_

MRESULT APIENTRY  DlgProcUninstall(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT APIENTRY  DlgProcWhatToDo(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT APIENTRY  DlgProcMessage(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2);

void              ParseAllUninstallLogs();
HWND              InstantiateDialog(HWND hParent, ULONG ulDlgID, PSZ szTitle, PFNWP pfnwpDlgProc);
void              ShowMessage(PSZ szMessage, BOOL bShow);
void              ProcessWindowsMessages(void);

#endif

