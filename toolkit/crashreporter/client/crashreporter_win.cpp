




































#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <shellapi.h>
#include "resource.h"
#include "client/windows/sender/crash_report_sender.h"

#define CRASH_REPORTER_KEY L"Software\\Mozilla\\Crash Reporter"
#define CRASH_REPORTER_VALUE L"Enabled"

#define WM_UPLOADCOMPLETE WM_APP

using std::wstring;
using std::map;

bool ReadConfig();
BOOL CALLBACK EnableDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SendDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
HANDLE CreateSendThread(HWND hDlg, LPCTSTR dumpFile);
bool CheckCrashReporterEnabled(bool* enabled);
void SetCrashReporterEnabled(bool enabled);
bool SendCrashReport(HINSTANCE hInstance, LPCTSTR dumpFile);
DWORD WINAPI SendThreadProc(LPVOID param);

typedef struct {
  HWND hDlg;
  LPCTSTR dumpFile;
} SENDTHREADDATA;

TCHAR sendURL[2048] = L"\0";
bool  deleteDump = true;


enum {
  ST_OK,
  ST_CANCEL,
  ST_CRASHREPORTERTITLE,
  ST_CRASHREPORTERDESCRIPTION,
  ST_RADIOENABLE,
  ST_RADIODISABLE,
  ST_SENDTITLE,
  ST_SUBMITSUCCESS,
  ST_SUBMITFAILED,
  NUM_STRINGS
};

LPCTSTR stringNames[] = {
  L"Ok",
  L"Cancel",
  L"CrashReporterTitle",
  L"CrashReporterDescription",
  L"RadioEnable",
  L"RadioDisable",
  L"SendTitle",
  L"SubmitSuccess",
  L"SubmitFailed"
};

LPTSTR strings[NUM_STRINGS];

void DoInitCommonControls()
{
	INITCOMMONCONTROLSEX ic;
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&ic);
  
  LoadLibrary(L"riched20.dll");
}

bool LoadStrings(LPCTSTR fileName)
{
  for (int i=ST_OK; i<NUM_STRINGS; i++) {
    strings[i] = new TCHAR[1024];
    GetPrivateProfileString(L"Strings", stringNames[i], L"", strings[i], 1024, fileName);
    if (stringNames[i][0] == '\0')
      return false;
    
  }
  return true;
}

bool ReadConfig()
{
  TCHAR fileName[MAX_PATH];

  if (GetModuleFileName(NULL, fileName, MAX_PATH)) {
    
    LPTSTR s = wcsrchr(fileName, '.');
    if (s) {
      wcscpy(s, L".ini");

      GetPrivateProfileString(L"Settings", L"URL", L"", sendURL, 2048, fileName);
      if (sendURL[0] == '\0')
        return false;

      TCHAR tmp[16];
      GetPrivateProfileString(L"Settings", L"Delete", L"1", tmp, 16, fileName);
      deleteDump = _wtoi(tmp) > 0;

      return LoadStrings(fileName);
    }
  }
  return false;
}

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)

{
	bool enabled;
  LPTSTR* argv = NULL;
  int argc = 0;

	DoInitCommonControls();
  if (!ReadConfig()) {
    MessageBox(NULL, L"Missing crashreporter.ini file", L"Crash Reporter Error", MB_OK | MB_ICONSTOP);
    return 0;
  }

  argv = CommandLineToArgvW(GetCommandLine(), &argc);

  if (argc == 1) {
    
    if (!CheckCrashReporterEnabled(&enabled))
      enabled = true;

    enabled = (1 == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ENABLEDIALOG), NULL, (DLGPROC)EnableDialogProc, (LPARAM)enabled));
    SetCrashReporterEnabled(enabled);
  }
  else {
    if (!CheckCrashReporterEnabled(&enabled)) {
      
      enabled = (1 == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ENABLEDIALOG), NULL, (DLGPROC)EnableDialogProc, (LPARAM)true));
      SetCrashReporterEnabled(enabled);
    }
    
    if (enabled) {
      if (SendCrashReport(hInstance, argv[1]) && deleteDump)
        DeleteFile(argv[1]);
      
    }
  }

  if (argv)
    LocalFree(argv);
	
	return 0;
}

BOOL CALLBACK EnableDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  switch (message) {
  case WM_INITDIALOG:
    SetWindowText(hwndDlg, strings[ST_CRASHREPORTERTITLE]);
    SetDlgItemText(hwndDlg, IDOK, strings[ST_OK]);
    SetDlgItemText(hwndDlg, IDC_RADIOENABLE, strings[ST_RADIOENABLE]);
    SetDlgItemText(hwndDlg, IDC_RADIODISABLE, strings[ST_RADIODISABLE]);
    SetDlgItemText(hwndDlg, IDC_DESCRIPTIONTEXT, strings[ST_CRASHREPORTERDESCRIPTION]);
    SendDlgItemMessage(hwndDlg, IDC_DESCRIPTIONTEXT, EM_SETTARGETDEVICE, (WPARAM)NULL, 0);
    SetFocus(GetDlgItem(hwndDlg, IDC_RADIOENABLE));
    CheckRadioButton(hwndDlg, IDC_RADIOENABLE, IDC_RADIODISABLE, lParam ? IDC_RADIOENABLE : IDC_RADIODISABLE);
    return FALSE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED)
      {
        UINT enableChecked = IsDlgButtonChecked(hwndDlg, IDC_RADIOENABLE);
        EndDialog(hwndDlg, (enableChecked > 0) ? 1 : 0);
      }
    return FALSE;

  default: 
    return FALSE; 
  } 
}

bool GetRegValue(HKEY hRegKey, LPCTSTR valueName, DWORD* value)
{
	DWORD type, dataSize;
	dataSize = sizeof(DWORD);
	if (RegQueryValueEx(hRegKey, valueName, NULL, &type, (LPBYTE)value, &dataSize) == ERROR_SUCCESS
		&& type == REG_DWORD)
		return true;

	return false;
}

bool CheckCrashReporterEnabled(bool* enabled)
{
  *enabled = false;
  bool found = false;
  HKEY hRegKey;
  DWORD val;
  
  if (RegOpenKey(HKEY_LOCAL_MACHINE, CRASH_REPORTER_KEY, &hRegKey) == ERROR_SUCCESS)
    {
      if (GetRegValue(hRegKey, CRASH_REPORTER_VALUE, &val))
        {
          *enabled = (val == 1);
          found = true;
        }
      RegCloseKey(hRegKey);
    }
  else
    {
      
      if (RegOpenKey(HKEY_CURRENT_USER, CRASH_REPORTER_KEY, &hRegKey) == ERROR_SUCCESS)	
        {
          if (GetRegValue(hRegKey, CRASH_REPORTER_VALUE, &val))
            {
              *enabled = (val == 1);
              found = true;
            }
          RegCloseKey(hRegKey);
        }
    }

  
  return found;
}

void SetCrashReporterEnabled(bool enabled)
{
  HKEY hRegKey;
  if (RegCreateKey(HKEY_CURRENT_USER, CRASH_REPORTER_KEY, &hRegKey) == ERROR_SUCCESS) {
    DWORD data = (enabled ? 1 : 0);
    RegSetValueEx(hRegKey, CRASH_REPORTER_VALUE, 0, REG_DWORD, (LPBYTE)&data, sizeof(data));
    RegCloseKey(hRegKey);
  }
}

BOOL CALLBACK SendDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static bool finishedOk = false;
  static HANDLE hThread = NULL;

  switch (message) {
  case WM_INITDIALOG:
    {
      
      SetWindowText(hwndDlg, strings[ST_SENDTITLE]);
      SetDlgItemText(hwndDlg, IDCANCEL, strings[ST_CANCEL]);
      
      SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
      SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
      
      LPCTSTR dumpFile = (LPCTSTR)lParam;
      hThread = CreateSendThread(hwndDlg, dumpFile);
    }
    return TRUE;

  case WM_UPLOADCOMPLETE:
    WaitForSingleObject(hThread, INFINITE);
    finishedOk = (wParam == 1);
    if (finishedOk) {
      SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 100, 0);
      MessageBox(hwndDlg, strings[ST_SUBMITSUCCESS], strings[ST_CRASHREPORTERTITLE], MB_OK | MB_ICONINFORMATION);
    }
    else {
      MessageBox(hwndDlg, strings[ST_SUBMITFAILED], strings[ST_CRASHREPORTERTITLE], MB_OK | MB_ICONERROR);
    }
    EndDialog(hwndDlg, finishedOk ? 1 : 0);
    return TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDCANCEL && HIWORD(wParam) == BN_CLICKED) {
      EndDialog(hwndDlg, finishedOk ? 1 : 0);
    }
    return TRUE;

  default:
    return FALSE; 
  } 
}

bool SendCrashReport(HINSTANCE hInstance, LPCTSTR dumpFile)
{
  int res = (int)DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SENDDIALOG), NULL, (DLGPROC)SendDialogProc, (LPARAM)dumpFile);
  return (res >= 0);
}

DWORD WINAPI SendThreadProc(LPVOID param)
{
  SENDTHREADDATA* td = (SENDTHREADDATA*)param;
  
  map<wstring, wstring> params;
  wstring url(sendURL);
  bool finishedOk = google_airbag::CrashReportSender
    ::SendCrashReport(url,
                      params,
                      wstring(td->dumpFile));
  PostMessage(td->hDlg, WM_UPLOADCOMPLETE, finishedOk ? 1 : 0, 0);
  delete td;
  return 0;
}

HANDLE CreateSendThread(HWND hDlg, LPCTSTR dumpFile)
{
  SENDTHREADDATA* td = new SENDTHREADDATA;
  td->hDlg = hDlg;
  td->dumpFile = dumpFile;
  return CreateThread(NULL, 0, SendThreadProc, td, 0, NULL);
}
