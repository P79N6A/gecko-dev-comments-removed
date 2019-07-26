




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


HANDLE gTestOutputPipe = INVALID_HANDLE_VALUE;

#define PIPE_BUFFER_SIZE 4096
char buffer[PIPE_BUFFER_SIZE + 1];

CString sAppParams;
CString sFirefoxPath;



#define kMetroTestFile "tests.ini"



#define SUCCESS   0
#define WARNINGS  1
#define FAILURE   2
#define EXCEPTION 3
#define RETRY     4

static void Log(const wchar_t *fmt, ...)
{
  va_list a = nullptr;
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

static void Fail(bool aRequestRetry, const wchar_t *fmt, ...)
{
  va_list a = nullptr;
  wchar_t szDebugString[1024];
  if(!lstrlenW(fmt))
    return;
  va_start(a,fmt);
  vswprintf(szDebugString, 1024, fmt, a);
  va_end(a);
  if(!lstrlenW(szDebugString))
    return;
  if (aRequestRetry) {
    wprintf(L"FAIL-SHOULD-RETRY | metrotestharness.exe | %s\n", szDebugString);
  } else {
    wprintf(L"TEST-UNEXPECTED-FAIL | metrotestharness.exe | %s\n", szDebugString);
  }
  fflush(stdout);
}






static bool GetModulePath(CStringW& aPathBuffer)
{
  WCHAR buffer[MAX_PATH];
  memset(buffer, 0, sizeof(buffer));

  if (!GetModuleFileName(nullptr, buffer, MAX_PATH)) {
    Fail(false, L"GetModuleFileName failed.");
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
    return false;
  }
  DWORD len = aCharLength * sizeof(WCHAR);
  memset(aIDBuffer, 0, len);
  if (RegQueryValueExW(key, L"AppUserModelID", nullptr, nullptr,
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

static bool SetupTestOutputPipe()
{
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  gTestOutputPipe =
    CreateNamedPipeW(L"\\\\.\\pipe\\metrotestharness",
                     PIPE_ACCESS_INBOUND,
                     PIPE_TYPE_BYTE|PIPE_WAIT,
                     1,
                     PIPE_BUFFER_SIZE,
                     PIPE_BUFFER_SIZE, 0, nullptr);

  if (gTestOutputPipe == INVALID_HANDLE_VALUE) {
    Log(L"Failed to create named logging pipe.");
    return false;
  }
  return true;
}

static void ReadPipe()
{
  DWORD numBytesRead;
  while (ReadFile(gTestOutputPipe, buffer, PIPE_BUFFER_SIZE,
                  &numBytesRead, nullptr) &&
         numBytesRead) {
    buffer[numBytesRead] = '\0';
    printf("%s", buffer);
    fflush(stdout);
  }
}

static int Launch()
{
  Log(L"Launching browser...");

  DWORD processID;

  
  CComPtr<IApplicationActivationManager> activateMgr;
  if (FAILED(CoCreateInstance(CLSID_ApplicationActivationManager, nullptr,
                              CLSCTX_LOCAL_SERVER,
                              IID_IApplicationActivationManager,
                              (void**)&activateMgr))) {
    Fail(false, L"CoCreateInstance CLSID_ApplicationActivationManager failed.");
    return FAILURE;
  }
  
  HRESULT hr;
  WCHAR appModelID[256];
  
  if (!GetDefaultBrowserAppModelID(appModelID, (sizeof(appModelID)/sizeof(WCHAR)))) {
    Fail(false, L"GetDefaultBrowserAppModelID failed.");
    return FAILURE;
  }
  Log(L"App model id='%s'", appModelID);

  
  
  
  hr = CoAllowSetForegroundWindow(activateMgr, nullptr);
  if (FAILED(hr)) {
    
    
    Log(L"Windows focus rights hand off failed (HRESULT=0x%X). Ignoring.", hr);
  }

  Log(L"Harness process id: %d", GetCurrentProcessId());

  
  int binLen = wcslen(kFirefoxExe);
  if (sFirefoxPath.GetLength() && sFirefoxPath.Right(binLen) != kFirefoxExe) {
    Log(L"firefoxpath is missing a valid bin name! Assuming '%s'.", kFirefoxExe);
    if (sFirefoxPath.Right(1) != L"\\") {
      sFirefoxPath += L"\\";
    }
    sFirefoxPath += kFirefoxExe;
  }

  
  
  CStringA testFilePath;
  if (sFirefoxPath.GetLength()) {
    
    int index = sFirefoxPath.ReverseFind('\\');
    if (index == -1) {
      Fail(false, L"Bad firefoxpath path");
      return FAILURE;
    }
    testFilePath = sFirefoxPath.Mid(0, index);
    testFilePath += "\\";
    testFilePath += kMetroTestFile;
  } else {
    
    char path[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) {
      Fail(false, L"GetModuleFileNameA errorno=%d", GetLastError());
      return FAILURE;
    }
    char* slash = strrchr(path, '\\');
    if (!slash)
      return FAILURE;
    *slash = '\0'; 
    testFilePath = path;
    testFilePath += "\\";
    sFirefoxPath = testFilePath;
    sFirefoxPath += kFirefoxExe;
    testFilePath += kMetroTestFile;
  }

  
  if (GetFileAttributesW(sFirefoxPath) == INVALID_FILE_ATTRIBUTES) {
    Fail(false, L"Invalid bin path: '%s'", sFirefoxPath);
    return FAILURE;
  }

  Log(L"Using bin path: '%s'", sFirefoxPath);

  Log(L"Writing out tests.ini to: '%s'", CStringW(testFilePath));
  HANDLE hTestFile = CreateFileA(testFilePath, GENERIC_WRITE,
                                 0, nullptr, CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 nullptr);
  if (hTestFile == INVALID_HANDLE_VALUE) {
    Fail(false, L"CreateFileA errorno=%d", GetLastError());
    return FAILURE;
  }

  DeleteTestFileHelper dtf(testFilePath);

  
  
  CStringA asciiParams = sFirefoxPath;
  asciiParams += " ";
  asciiParams += sAppParams;
  asciiParams.Trim();
  Log(L"Browser command line args: '%s'", CString(asciiParams));
  if (!WriteFile(hTestFile, asciiParams, asciiParams.GetLength(),
                 nullptr, 0)) {
    CloseHandle(hTestFile);
    Fail(false, L"WriteFile errorno=%d", GetLastError());
    return FAILURE;
  }
  FlushFileBuffers(hTestFile);
  CloseHandle(hTestFile);

  
  if (!SetupTestOutputPipe()) {
    Fail(false, L"SetupTestOutputPipe failed (errno=%d)", GetLastError());
    return FAILURE;
  }

  
  hr = activateMgr->ActivateApplication(appModelID, L"", AO_NOERRORUI, &processID);
  if (FAILED(hr)) {
    Fail(true, L"ActivateApplication result %X", hr);
    return RETRY;
  }

  Log(L"Activation succeeded.");

  
  Log(L"METRO_BROWSER_PROCESS=%d", processID);

  HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, processID);
  if (!child) {
    Fail(false, L"Couldn't find child process. (%d)", GetLastError());
    return FAILURE;
  }

  Log(L"Waiting on child process...");

  MSG msg;
  DWORD waitResult = WAIT_TIMEOUT;
  HANDLE handles[2] = { child, gTestOutputPipe };
  while ((waitResult = MsgWaitForMultipleObjects(2, handles, FALSE, INFINITE, QS_ALLINPUT)) != WAIT_OBJECT_0) {
    if (waitResult == WAIT_FAILED) {
      Log(L"Wait failed (errno=%d)", GetLastError());
      break;
    } else if (waitResult == WAIT_OBJECT_0 + 1) {
      ReadPipe();
    } else if (waitResult == WAIT_OBJECT_0 + 2 &&
               PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  ReadPipe();
  CloseHandle(gTestOutputPipe);
  CloseHandle(child);

  Log(L"Exiting.");
  return SUCCESS;
}

int wmain(int argc, WCHAR* argv[])
{
  CoInitialize(nullptr);

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
  int res = Launch();
  CoUninitialize();
  return res;
}
