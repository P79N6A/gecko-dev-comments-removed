




#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x602
#define _WIN32_WINNT 0x602

#include <windows.h>
#include <objbase.h>
#include <combaseapi.h>
#include <atlcore.h>
#include <atlstr.h>
#include <wininet.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <propkey.h>
#include <propvarutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <io.h>
#include <shellapi.h>

static const WCHAR* kFirefoxExe = L"firefox.exe";
static const WCHAR* kDefaultMetroBrowserIDPathKey = L"FirefoxURL";
static const WCHAR* kDemoMetroBrowserIDPathKey = L"Mozilla.Firefox.URL";

CString sAppParams;
CString sFirefoxPath;



#define kMetroTestFile "tests.ini"

static void Log(const wchar_t *fmt, ...)
{
  va_list a = NULL;
  wchar_t szDebugString[1024];
  if(!lstrlenW(fmt))
    return;
  va_start(a,fmt);
  vswprintf(szDebugString, 1024, fmt, a);
  va_end(a);
  if(!lstrlenW(szDebugString))
    return;

  wprintf(L"INFO | metrotestharness.exe | %s\n", szDebugString);
  fflush(stdout);
}

static void Fail(const wchar_t *fmt, ...)
{
  va_list a = NULL;
  wchar_t szDebugString[1024];
  if(!lstrlenW(fmt))
    return;
  va_start(a,fmt);
  vswprintf(szDebugString, 1024, fmt, a);
  va_end(a);
  if(!lstrlenW(szDebugString))
    return;

  wprintf(L"TEST-UNEXPECTED-FAIL | metrotestharness.exe | %s\n", szDebugString);
  fflush(stdout);
}






static bool GetModulePath(CStringW& aPathBuffer)
{
  WCHAR buffer[MAX_PATH];
  memset(buffer, 0, sizeof(buffer));

  if (!GetModuleFileName(NULL, buffer, MAX_PATH)) {
    Fail(L"GetModuleFileName failed.");
    return false;
  }

  WCHAR* slash = wcsrchr(buffer, '\\');
  if (!slash)
    return false;
  *slash = '\0';

  aPathBuffer = buffer;
  return true;
}






static bool GetDesktopBrowserPath(CStringW& aPathBuffer)
{
  if (!GetModulePath(aPathBuffer))
    return false;

  
  
  aPathBuffer.Append(L"\\");
  aPathBuffer.Append(kFirefoxExe);
  return true;
}







static bool GetDefaultBrowserAppModelID(WCHAR* aIDBuffer,
                                        long aCharLength)
{
  if (!aIDBuffer || aCharLength <= 0)
    return false;

  memset(aIDBuffer, 0, (sizeof(WCHAR)*aCharLength));

  HKEY key;
  if (RegOpenKeyExW(HKEY_CLASSES_ROOT, kDefaultMetroBrowserIDPathKey,
                    0, KEY_READ, &key) != ERROR_SUCCESS) {
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, kDemoMetroBrowserIDPathKey,
                      0, KEY_READ, &key) != ERROR_SUCCESS) {
      return false;
    }
  }
  DWORD len = aCharLength * sizeof(WCHAR);
  memset(aIDBuffer, 0, len);
  if (RegQueryValueExW(key, L"AppUserModelID", NULL, NULL,
                       (LPBYTE)aIDBuffer, &len) != ERROR_SUCCESS || !len) {
    RegCloseKey(key);
    return false;
  }
  RegCloseKey(key);
  return true;
}


class DeleteTestFileHelper
{
  CStringA mTestFile;
public:
  DeleteTestFileHelper(CStringA& aTestFile) :
    mTestFile(aTestFile) {}
  ~DeleteTestFileHelper() {
    if (mTestFile.GetLength()) {
      Log(L"Deleting %s", CStringW(mTestFile));
      DeleteFileA(mTestFile);
    }
  }
};

static bool Launch()
{
  Log(L"Launching browser...");

  DWORD processID;

  
  CComPtr<IApplicationActivationManager> activateMgr;
  if (FAILED(CoCreateInstance(CLSID_ApplicationActivationManager, NULL,
                              CLSCTX_LOCAL_SERVER,
                              IID_IApplicationActivationManager,
                              (void**)&activateMgr))) {
    Fail(L"CoCreateInstance CLSID_ApplicationActivationManager failed.");
    return false;
  }
  
  HRESULT hr;
  WCHAR appModelID[256];
  
  if (!GetDefaultBrowserAppModelID(appModelID, (sizeof(appModelID)/sizeof(WCHAR)))) {
    Fail(L"GetDefaultBrowserAppModelID failed.");
    return false;
  }
  Log(L"App model id='%s'", appModelID);

  
  
  hr = CoAllowSetForegroundWindow(activateMgr, NULL);
  if (FAILED(hr)) {
    Fail(L"CoAllowSetForegroundWindow result %X", hr);
    return false;
  }

  Log(L"Harness process id: %d", GetCurrentProcessId());

  
  
  CStringA testFilePath;
  if (sFirefoxPath.GetLength()) {
    
    int index = sFirefoxPath.ReverseFind('\\');
    if (index == -1) {
      Fail(L"Bad firefoxpath path");
      return false;
    }
    testFilePath = sFirefoxPath.Mid(0, index);
    testFilePath += "\\";
    testFilePath += kMetroTestFile;
  } else {
    
    char path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, path, MAX_PATH)) {
      Fail(L"GetModuleFileNameA errorno=%d", GetLastError());
      return false;
    }
    char* slash = strrchr(path, '\\');
    if (!slash)
      return false;
    *slash = '\0'; 
    testFilePath = path;
    testFilePath += "\\";
    testFilePath += kMetroTestFile;
  }

  Log(L"Writing out tests.ini to: '%s'", CStringW(testFilePath));
  HANDLE hTestFile = CreateFileA(testFilePath, GENERIC_WRITE,
                                 0, NULL, CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
  if (hTestFile == INVALID_HANDLE_VALUE) {
    Fail(L"CreateFileA errorno=%d", GetLastError());
    return false;
  }

  DeleteTestFileHelper dtf(testFilePath);

  CStringA asciiParams = sAppParams;
  if (!WriteFile(hTestFile, asciiParams, asciiParams.GetLength(), NULL, 0)) {
    CloseHandle(hTestFile);
    Fail(L"WriteFile errorno=%d", GetLastError());
    return false;
  }
  FlushFileBuffers(hTestFile);
  CloseHandle(hTestFile);

  
  hr = activateMgr->ActivateApplication(appModelID, L"", AO_NOERRORUI, &processID);
  if (FAILED(hr)) {
    Fail(L"ActivateApplication result %X", hr);
    return false;
  }

  Log(L"Activation succeeded. processid=%d", processID);

  HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, processID);
  if (!child) {
    Fail(L"Couldn't find child process. (%d)", GetLastError());
    return false;
  }

  Log(L"Waiting on child process...");

  MSG msg;
  DWORD waitResult = WAIT_TIMEOUT;
  while ((waitResult = WaitForSingleObject(child, 10)) != WAIT_OBJECT_0) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  Log(L"Exiting.");
  return true;
}

int wmain(int argc, WCHAR* argv[])
{
  CoInitialize(NULL);

  int idx;
  bool firefoxParam = false;
  for (idx = 1; idx < argc; idx++) {
    CString param = argv[idx];
    param.Trim();

    
    
    if (param == "-firefoxpath") {
      firefoxParam = true;
      continue;
    } else if (firefoxParam) {
      firefoxParam = false;
      sFirefoxPath = param;
      continue;
    }

    sAppParams.Append(argv[idx]);
    sAppParams.Append(L" ");
  }
  sAppParams.Trim();
  if (sFirefoxPath.GetLength()) {
    Log(L"firefoxpath: '%s'", sFirefoxPath);
  }
  Log(L"args: '%s'", sAppParams);
  Launch();

  CoUninitialize();
  return 0;
}
