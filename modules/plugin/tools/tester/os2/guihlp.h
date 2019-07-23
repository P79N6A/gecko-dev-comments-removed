




































#ifndef __GUIHLP_H__
#define __GUIHLP_H__

void EnableWindowNow(HWND hWnd, BOOL bEnable);
void ShowWindowNow(HWND hWnd, BOOL iShow);
void fillAPIComboBoxAndSetSel(HWND hWndCombo, int iSel);
void updateUI(HWND hWnd);
void onGo(HWND hWnd);
void onPaste(HWND hWndToPasteTo);

#endif 
