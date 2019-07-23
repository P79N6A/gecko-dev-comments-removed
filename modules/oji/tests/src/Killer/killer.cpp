

















































#include <windows.h>
#include <stdio.h>


#define ERROR_DIALOG_TITLE "Mozilla: mozilla.exe - Application Error"

#define ERROR_DIALOG_KW_1 "mozilla"
#define ERROR_DIALOG_KW_2 "Error"

#define OK_BUTTON_TITLE "OK"

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM out) {
	char title[1024];
	GetWindowText(hwnd, title, 1024);
	
	if (!strcmp(title, OK_BUTTON_TITLE)) {
		*((HWND*)out) = hwnd;
		return FALSE;
	}	
	return TRUE;
}

BOOL CALLBACK EnumWindowsProc(  HWND hwnd, LPARAM lParam) {
	char title[1024];
	GetWindowText(hwnd, title, 1024);
	
	if (strstr(title, ERROR_DIALOG_KW_1) && strstr(title, ERROR_DIALOG_KW_2)) {
		DWORD lp = 0, wp = 0;
		HWND ok;
		
		EnumChildWindows(hwnd, EnumChildProc, (LPARAM)(&ok));
		if (!ok) {
			printf("OK button not found !\n");
			return FALSE;
		}
		







		lp = (unsigned long)ok;
		wp = 1;
		wp = wp | (BN_CLICKED << 16);
		
		SendMessage(hwnd, WM_COMMAND, wp, lp);
		return FALSE;
	}
	return TRUE;
}

void main() {
	EnumWindows(EnumWindowsProc, 0);
}
