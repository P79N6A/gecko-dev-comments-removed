






































#ifndef _DIALOGS_H_
#define _DIALOGS_H_

LRESULT CALLBACK  DlgProcMain(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK  DlgProcWelcome(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcLicense(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcSetupType(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcSelectComponents(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcSelectAdditionalComponents(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcWindowsIntegration(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcProgramFolder(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcAdditionalOptions(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcAdvancedSettings(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcQuickLaunch(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcSiteSelector(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcStartInstall(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcReboot(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  DlgProcMessage(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);
LRESULT CALLBACK  NewListBoxWndProc( HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK  DlgProcUpgrade(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam);

void              ToggleCheck(HWND hwndListBox, DWORD dwIndex, DWORD dwACFlag);
BOOL              AskCancelDlg(HWND hDlg);
void              lbAddItem(HWND hList, siC *siCComponent);
HWND              InstantiateDialog(HWND hParent, DWORD dwDlgID, LPSTR szTitle, WNDPROC wpDlgProc);
void              DlgSequence(int iDirection);
void              PaintGradientShade(HWND hWnd, HDC hdc);
BOOL              BrowseForDirectory(HWND hDlg, char *szCurrDir);
LRESULT CALLBACK  BrowseHookProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void              ShowMessage(LPSTR szMessage, BOOL bShow);
void              DrawLBText(LPDRAWITEMSTRUCT lpdis, DWORD dwACFlag);
void              DrawCheck(LPDRAWITEMSTRUCT lpdis, DWORD dwACFlag);
void              InvalidateLBCheckbox(HWND hwndListBox);
void              ProcessWindowsMessages(void);
LPSTR             GetStartInstallMessage(void);
void              AppendStringWOAmpersand(LPSTR szInputString, DWORD dwInputStringSize, LPSTR szString);
void              TruncateString(HWND hWnd, LPSTR szInPath, LPSTR szOutPath, DWORD dwOutPathBufSize);
void              SaveAdditionalOptions(HWND hDlg, HWND hwndCBSiteSelector);
WNDPROC           SubclassWindow( HWND hWnd, WNDPROC NewWndProc);
LRESULT CALLBACK  ListBoxBrowseWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void              DisableSystemMenuItems(HWND hWnd, BOOL bDisableClose);
void              CommitInstall(void);
void              RepositionWindow(HWND aHwndDlg, DWORD aBannerImage);
void              SaveWindowPosition(HWND aDlg);
void              ClosePreviousDialog(void);

#endif 

