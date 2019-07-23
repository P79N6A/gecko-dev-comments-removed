




































#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include "crashreporter.h"

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "resource.h"
#include "client/windows/sender/crash_report_sender.h"
#include "common/windows/string_utils-inl.h"

#define CRASH_REPORTER_KEY L"Software\\Mozilla\\Crash Reporter"
#define CRASH_REPORTER_VALUE L"Enabled"

#define WM_UPLOADCOMPLETE WM_APP

using std::string;
using std::wstring;
using std::map;

static wstring UTF8ToWide(const string& utf8, bool *success = 0);
static string WideToUTF8(const wstring& wide, bool *success = 0);
static DWORD WINAPI SendThreadProc(LPVOID param);

typedef struct {
  HWND hDlg;
  wstring dumpFile;
  const StringTable* query_parameters;
  wstring send_url;
  wstring* server_response;
} SENDTHREADDATA;

static wstring Str(const char* key)
{
  return UTF8ToWide(gStrings[key]);
}

static void DoInitCommonControls()
{
	INITCOMMONCONTROLSEX ic;
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&ic);
  
  LoadLibrary(L"riched20.dll");
}

static BOOL CALLBACK EnableDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  switch (message) {
  case WM_INITDIALOG:
    SetWindowText(hwndDlg, Str(ST_CRASHREPORTERTITLE).c_str());
    SetDlgItemText(hwndDlg, IDOK, Str(ST_OK).c_str());
    SetDlgItemText(hwndDlg, IDC_RADIOENABLE, Str(ST_RADIOENABLE).c_str());
    SetDlgItemText(hwndDlg, IDC_RADIODISABLE, Str(ST_RADIODISABLE).c_str());
    SetDlgItemText(hwndDlg, IDC_DESCRIPTIONTEXT, Str(ST_CRASHREPORTERDESCRIPTION).c_str());
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

static bool GetRegValue(HKEY hRegKey, LPCTSTR valueName, DWORD* value)
{
	DWORD type, dataSize;
	dataSize = sizeof(DWORD);
	if (RegQueryValueEx(hRegKey, valueName, NULL, &type, (LPBYTE)value, &dataSize) == ERROR_SUCCESS
		&& type == REG_DWORD)
		return true;

	return false;
}

static bool CheckCrashReporterEnabled(bool* enabled)
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

static void SetCrashReporterEnabled(bool enabled)
{
  HKEY hRegKey;
  if (RegCreateKey(HKEY_CURRENT_USER, CRASH_REPORTER_KEY, &hRegKey) == ERROR_SUCCESS) {
    DWORD data = (enabled ? 1 : 0);
    RegSetValueEx(hRegKey, CRASH_REPORTER_VALUE, 0, REG_DWORD, (LPBYTE)&data, sizeof(data));
    RegCloseKey(hRegKey);
  }
}

static BOOL CALLBACK SendDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static bool finishedOk = false;
  static HANDLE hThread = NULL;

  switch (message) {
  case WM_INITDIALOG:
    {
      
      SetWindowText(hwndDlg, Str(ST_SENDTITLE).c_str());
      SetDlgItemText(hwndDlg, IDCANCEL, Str(ST_CANCEL).c_str());
      
      SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
      SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
      
      SENDTHREADDATA* td = (SENDTHREADDATA*)lParam;
      td->hDlg = hwndDlg;
      CreateThread(NULL, 0, SendThreadProc, td, 0, NULL);
    }
    return TRUE;

  case WM_UPLOADCOMPLETE:
    WaitForSingleObject(hThread, INFINITE);
    finishedOk = (wParam == 1);
    if (finishedOk) {
      SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 100, 0);
      MessageBox(hwndDlg,
                 Str(ST_SUBMITSUCCESS).c_str(),
                 Str(ST_CRASHREPORTERTITLE).c_str(),
                 MB_OK | MB_ICONINFORMATION);
    }
    else {
      MessageBox(hwndDlg,
                 Str(ST_SUBMITFAILED).c_str(),
                 Str(ST_CRASHREPORTERTITLE).c_str(),
                 MB_OK | MB_ICONERROR);
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

static bool SendCrashReport(wstring dumpFile,
                            const StringTable* query_parameters,
                            wstring send_url,
                            wstring* server_response)
{
  SENDTHREADDATA td;
  td.hDlg = NULL;
  td.dumpFile = dumpFile;
  td.query_parameters = query_parameters;
  td.send_url = send_url;
  td.server_response = server_response;

  int res = (int)DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_SENDDIALOG), NULL,
                                (DLGPROC)SendDialogProc, (LPARAM)&td);

  return (res >= 0);
}

static DWORD WINAPI SendThreadProc(LPVOID param)
{
  bool finishedOk;
  SENDTHREADDATA* td = (SENDTHREADDATA*)param;

  if (td->send_url.empty()) {
    finishedOk = false;
  }
  else {
    map<wstring,wstring>query_parameters;
    StringTable::const_iterator end = td->query_parameters->end();
    for (StringTable::const_iterator i = td->query_parameters->begin();
         i != end;
         i++) {
      query_parameters[UTF8ToWide(i->first)] = UTF8ToWide(i->second);
    }

    finishedOk = (google_breakpad::CrashReportSender
      ::SendCrashReport(td->send_url,
                        query_parameters,
                        td->dumpFile,
                        td->server_response)
                  == google_breakpad::RESULT_SUCCEEDED);
  }
  PostMessage(td->hDlg, WM_UPLOADCOMPLETE, finishedOk ? 1 : 0, 0);

  return 0;
}

static wstring UTF8ToWide(const string& utf8, bool *success)
{
  wchar_t* buffer = NULL;
  int buffer_size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                        -1, NULL, 0);
  if(buffer_size == 0) {
    if (success)
      *success = false;
    return L"";
  }

  buffer = new wchar_t[buffer_size];
  if(buffer == NULL) {
    if (success)
      *success = false;
    return L"";
  }

  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                      -1, buffer, buffer_size);
  wstring str = buffer;
  delete [] buffer;

  if (success)
    *success = true;

  return str;
}

static string WideToUTF8(const wstring& wide, bool *success)
{
  char* buffer = NULL;
  int buffer_size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                        -1, NULL, 0, NULL, NULL);
  if(buffer_size == 0) {
    if (success)
      *success = false;
    return "";
  }

  buffer = new char[buffer_size];
  if(buffer == NULL) {
    if (success)
      *success = false;
    return "";
  }

  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                      -1, buffer, buffer_size, NULL, NULL);
  string utf8 = buffer;
  delete [] buffer;

  if (success)
    *success = true;

  return utf8;
}



bool UIInit()
{
  DoInitCommonControls();
  return true;
}

void UIShutdown()
{
}

void UIShowDefaultUI()
{
  bool enabled;
  
  if (!CheckCrashReporterEnabled(&enabled))
    enabled = true;

  enabled = (1 == DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_ENABLEDIALOG), NULL, (DLGPROC)EnableDialogProc, (LPARAM)enabled));
  SetCrashReporterEnabled(enabled);
}

void UIShowCrashUI(const string& dumpfile,
                   const StringTable& query_parameters,
                   const string& send_url)
{
  bool enabled;
  if (!CheckCrashReporterEnabled(&enabled)) {
    
    enabled = (1 == DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_ENABLEDIALOG), NULL, (DLGPROC)EnableDialogProc, (LPARAM)true));
    SetCrashReporterEnabled(enabled);
  }
  
  if (enabled) {
    wstring server_response;
    bool success = SendCrashReport(UTF8ToWide(dumpfile),
                                   &query_parameters,
                                   UTF8ToWide(send_url),
                                   &server_response);
    CrashReporterSendCompleted(success, WideToUTF8(server_response));
  }
}

void UIError(const string& message)
{
  wstring title = Str(ST_CRASHREPORTERTITLE);
  if (title.empty())
    title = L"Crash Reporter Error";

  MessageBox(NULL, UTF8ToWide(message).c_str(), title.c_str(),
             MB_OK | MB_ICONSTOP);
}

bool UIGetIniPath(string& path)
{
  wchar_t fileName[MAX_PATH];
  if (GetModuleFileName(NULL, fileName, MAX_PATH)) {
    
    wchar_t* s = wcsrchr(fileName, '.');
    if (s) {
      wcscpy(s, L".ini");
      path = WideToUTF8(fileName);
      return true;
    }
  }

  return false;
}

bool UIGetSettingsPath(const string& vendor,
                       const string& product,
                       string& settings_path)
{
  wchar_t path[MAX_PATH];
  if(SUCCEEDED(SHGetFolderPath(NULL,
                               CSIDL_APPDATA,
                               NULL,
                               0,
                               path)))  {
    if (!vendor.empty()) {
      PathAppend(path, UTF8ToWide(vendor).c_str());
    }
    PathAppend(path, UTF8ToWide(product).c_str());
    PathAppend(path, L"Crash Reports");
    
    CreateDirectory(path, NULL);
    settings_path = WideToUTF8(path);
    return true;
  }
  return false;
}

bool UIEnsurePathExists(const string& path)
{
  if (CreateDirectory(UTF8ToWide(path).c_str(), NULL) == 0) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }

  return true;
}

bool UIMoveFile(const string& oldfile, const string& newfile)
{
  return MoveFile(UTF8ToWide(oldfile).c_str(), UTF8ToWide(newfile).c_str());
}

bool UIDeleteFile(const string& oldfile)
{
  return DeleteFile(UTF8ToWide(oldfile).c_str());
}
