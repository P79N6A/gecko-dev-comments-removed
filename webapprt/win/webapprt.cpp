






#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#include <windows.h>


#include "nsIFile.h"
#include "nsINIParser.h"
#include "nsWindowsWMain.cpp"   
#include "nsXPCOMGlue.h"
#include "nsXPCOMPrivate.h"     
#include "nsXULAppAPI.h"
#include "mozilla/AppData.h"

using namespace mozilla;

XRE_GetFileFromPathType XRE_GetFileFromPath;
XRE_CreateAppDataType XRE_CreateAppData;
XRE_FreeAppDataType XRE_FreeAppData;
XRE_mainType XRE_main;

namespace {
  const char kAPP_INI[] = "application.ini";
  const char kWEBAPP_INI[] = "webapp.ini";
  const char kWEBAPPRT_INI[] = "webapprt.ini";
  const char kWEBAPPRT_PATH[] = "webapprt";
  const char kAPP_ENV_PREFIX[] = "XUL_APP_FILE=";
  const char kAPP_RT[] = "webapprt-stub.exe";

  const wchar_t kAPP_RT_BACKUP[] = L"webapprt.old";

  wchar_t curExePath[MAXPATHLEN];
  wchar_t backupFilePath[MAXPATHLEN];
  wchar_t iconPath[MAXPATHLEN];
  char profile[MAXPATHLEN];
  bool isProfileOverridden = false;
  int* pargc;
  char*** pargv;

  nsresult
  joinPath(char* const dest,
           char const* const dir,
           char const* const leaf,
           size_t bufferSize)
  {
    size_t dirLen = strlen(dir);
    size_t leafLen = strlen(leaf);
    bool needsSeparator = (dirLen != 0
                        && dir[dirLen-1] != '\\'
                        && leafLen != 0
                        && leaf[0] != '\\');

    if (dirLen + (needsSeparator? 1 : 0) + leafLen >= bufferSize) {
      return NS_ERROR_FAILURE;
    }

    strncpy(dest, dir, bufferSize);
    char* destEnd = dest + dirLen;
    if (needsSeparator) {
      *(destEnd++) = '\\';
    }

    strncpy(destEnd, leaf, leafLen);
    return NS_OK;
  }

  


  class ScopedLogging
  {
    public:
      ScopedLogging() { NS_LogInit(); }
      ~ScopedLogging() { NS_LogTerm(); }
  };

  


  class ScopedXREAppData
  {
    public:
      ScopedXREAppData()
        : mAppData(nullptr) { }

      nsresult
      create(nsIFile* aINIFile)
      {
        return XRE_CreateAppData(aINIFile, &mAppData);
      }

      ~ScopedXREAppData()
      {
        if (nullptr != mAppData) {
          XRE_FreeAppData(mAppData);
        }
      }

      nsXREAppData* const
      operator->()
      {
        return get();
      }

      nsXREAppData
      operator*()
      {
        return *get();
      }

      operator
      nsXREAppData*()
      {
        return get();
      }
    private:
      nsXREAppData* mAppData;
      nsXREAppData* const get() { return mAppData; }
  };

  void
  Output(const wchar_t *fmt, ... )
  {
    va_list ap;
    va_start(ap, fmt);

    wchar_t msg[1024];
    _vsnwprintf_s(msg, _countof(msg), _countof(msg), fmt, ap);

    MessageBoxW(nullptr, msg, L"Web Runtime", MB_OK);

    va_end(ap);
  }

  void
  Output(const char *fmt, ... )
  {
    va_list ap;
    va_start(ap, fmt);

    char msg[1024];
    vsnprintf(msg, sizeof(msg), fmt, ap);

    wchar_t wide_msg[1024];
    MultiByteToWideChar(CP_UTF8,
                        0,
                        msg,
                        -1,
                        wide_msg,
                        _countof(wide_msg));
    Output(wide_msg);

    va_end(ap);
  }

  const nsDynamicFunctionLoad kXULFuncs[] = {
      { "XRE_GetFileFromPath", (NSFuncPtr*) &XRE_GetFileFromPath },
      { "XRE_CreateAppData", (NSFuncPtr*) &XRE_CreateAppData },
      { "XRE_FreeAppData", (NSFuncPtr*) &XRE_FreeAppData },
      { "XRE_main", (NSFuncPtr*) &XRE_main },
      { nullptr, nullptr }
  };

  bool
  AttemptCopyAndLaunch(wchar_t* src)
  {
    
    if (FALSE == ::MoveFileExW(curExePath,
                               backupFilePath,
                               MOVEFILE_REPLACE_EXISTING)) {
      return false;
    }

    
    if (FALSE == ::CopyFileW(src,
                             curExePath,
                             TRUE)) {
      
      ::MoveFileW(backupFilePath,
                  curExePath);
      return false;
    }

    

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ::ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(curExePath, 
                        nullptr,    
                        nullptr,    
                        nullptr,    
                        FALSE,      
                        0,          
                        nullptr,    
                        nullptr,    
                        &si,
                        &pi)) {
      return false;
    }

    
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return true;
  }

  bool
  AttemptCopyAndLaunch(char* srcUtf8)
  {
    wchar_t src[MAXPATHLEN];
    if (0 == MultiByteToWideChar(CP_UTF8,
                                 0,
                                 srcUtf8,
                                 -1,
                                 src,
                                 MAXPATHLEN)) {
      return false;
    }

    return AttemptCopyAndLaunch(src);
  }

  bool
  AttemptGRELoadAndLaunch(char* greDir)
  {
    nsresult rv;

    char xpcomDllPath[MAXPATHLEN];
    rv = joinPath(xpcomDllPath, greDir, XPCOM_DLL, MAXPATHLEN);
    NS_ENSURE_SUCCESS(rv, false);

    rv = XPCOMGlueStartup(xpcomDllPath);
    NS_ENSURE_SUCCESS(rv, false);

    rv = XPCOMGlueLoadXULFunctions(kXULFuncs);
    NS_ENSURE_SUCCESS(rv, false);

    
    { 

      ScopedLogging log;

      
      char rtPath[MAXPATHLEN];
      rv = joinPath(rtPath, greDir, kWEBAPPRT_PATH, MAXPATHLEN);
      NS_ENSURE_SUCCESS(rv, false);

      
      char rtIniPath[MAXPATHLEN];
      rv = joinPath(rtIniPath, rtPath, kWEBAPPRT_INI, MAXPATHLEN);
      NS_ENSURE_SUCCESS(rv, false);

      
      nsCOMPtr<nsIFile> rtINI;
      rv = XRE_GetFileFromPath(rtIniPath, getter_AddRefs(rtINI));
      NS_ENSURE_SUCCESS(rv, false);

      bool exists;
      rv = rtINI->Exists(&exists);
      if (NS_FAILED(rv) || !exists)
        return false;

      ScopedXREAppData webShellAppData;
      rv = webShellAppData.create(rtINI);
      NS_ENSURE_SUCCESS(rv, false);

      if (!isProfileOverridden) {
        SetAllocatedString(webShellAppData->profile, profile);
        SetAllocatedString(webShellAppData->name, profile);
      }

      nsCOMPtr<nsIFile> directory;
      rv = XRE_GetFileFromPath(rtPath, getter_AddRefs(directory));
      NS_ENSURE_SUCCESS(rv, false);

      nsCOMPtr<nsIFile> xreDir;
      rv = XRE_GetFileFromPath(greDir, getter_AddRefs(xreDir));
      NS_ENSURE_SUCCESS(rv, false);

      xreDir.forget(&webShellAppData->xreDirectory);
      NS_IF_RELEASE(webShellAppData->directory);
      directory.forget(&webShellAppData->directory);

      
      XRE_main(*pargc, *pargv, webShellAppData, 0);
    }

    return true;
  }

  bool
  AttemptLoadFromDir(char* firefoxDir)
  {
    nsresult rv;

    
    char appIniPath[MAXPATHLEN];
    rv = joinPath(appIniPath, firefoxDir, kAPP_INI, MAXPATHLEN);
    NS_ENSURE_SUCCESS(rv, false);

    nsINIParser parser;
    rv = parser.Init(appIniPath);
    NS_ENSURE_SUCCESS(rv, false);

    
    char buildid[MAXPATHLEN]; 
                              
                              
    rv = parser.GetString("App",
                          "BuildID",
                          buildid,
                          MAXPATHLEN);
    NS_ENSURE_SUCCESS(rv, false);

    if (0 == strcmp(buildid, NS_STRINGIFY(GRE_BUILDID))) {
      return AttemptGRELoadAndLaunch(firefoxDir);
    }

    char webAppRTExe[MAXPATHLEN];
    rv = joinPath(webAppRTExe, firefoxDir, kAPP_RT, MAXPATHLEN);
    NS_ENSURE_SUCCESS(rv, false);

    return AttemptCopyAndLaunch(webAppRTExe);
  }

  bool
  GetFirefoxDirFromRegistry(char* firefoxDir)
  {
    HKEY key;
    wchar_t wideGreDir[MAXPATHLEN];

    if (ERROR_SUCCESS !=
                RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              L"SOFTWARE\\Microsoft\\Windows"
                              L"\\CurrentVersion\\App paths\\firefox.exe",
                              0,
                              KEY_READ,
                              &key)) {
      return false;
    }

    DWORD length = MAXPATHLEN * sizeof(wchar_t);
    
    
    if (ERROR_SUCCESS != RegQueryValueExW(key,
                                          L"Path",
                                          nullptr,
                                          nullptr,
                                          reinterpret_cast<BYTE*>(wideGreDir),
                                          &length)) {
      RegCloseKey(key);
      return false;
    };
    RegCloseKey(key);

    
    
    length = length / sizeof(wchar_t);
    if (wideGreDir[length] != L'\0') {
      if (length >= MAXPATHLEN) {
        return false;
      }
      wideGreDir[length] = L'\0';
    }

    if (0 == WideCharToMultiByte(CP_UTF8,
                                 0,
                                 wideGreDir,
                                 -1,
                                 firefoxDir,
                                 MAXPATHLEN,
                                 nullptr,
                                 nullptr)) {
      return false;
    }

    return true;
  }
};







int
main(int argc, char* argv[])
{
  pargc = &argc;
  pargv = &argv;
  nsresult rv;
  char buffer[MAXPATHLEN];
  wchar_t wbuffer[MAXPATHLEN];

  
  if (!GetModuleFileNameW(0, wbuffer, MAXPATHLEN)) {
    Output("Couldn't calculate the application directory.");
    return 255;
  }
  wcsncpy(curExePath, wbuffer, MAXPATHLEN);

  
  wchar_t* lastSlash = wcsrchr(wbuffer, L'\\');
  if (!lastSlash) {
    Output("Application directory format not understood.");
    return 255;
  }
  *(++lastSlash) = L'\0';

  
  if (wcslen(wbuffer) + _countof(kAPP_RT_BACKUP) >= MAXPATHLEN) {
    Output("Application directory path is too long (couldn't set up backup file path).");
  }
  wcsncpy(lastSlash, kAPP_RT_BACKUP, _countof(kAPP_RT_BACKUP));
  wcsncpy(backupFilePath, wbuffer, MAXPATHLEN);

  *lastSlash = L'\0';

  
  if (0 == WideCharToMultiByte(CP_UTF8,
                               0,
                               wbuffer,
                               -1,
                               buffer,
                               MAXPATHLEN,
                               nullptr,
                               nullptr)) {
    Output("Application directory could not be processed.");
    return 255;
  }

  
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-profile")) {
      isProfileOverridden = true;
      break;
    }
  }

  
  
  
  if (AttemptLoadFromDir(buffer)) {
    return 0;
  }

  
  
  char appIniPath[MAXPATHLEN];
  if (NS_FAILED(joinPath(appIniPath, buffer, kWEBAPP_INI, MAXPATHLEN))) {
    Output("Path to webapp.ini could not be processed.");
    return 255;
  }

  
  
  nsINIParser parser;
  if (NS_FAILED(parser.Init(appIniPath))) {
    Output("Could not open webapp.ini");
    return 255;
  }

  
  char appEnv[MAXPATHLEN + _countof(kAPP_ENV_PREFIX)];
  strcpy(appEnv, kAPP_ENV_PREFIX);
  strcpy(appEnv + _countof(kAPP_ENV_PREFIX) - 1, appIniPath);
  if (putenv(appEnv)) {
    Output("Couldn't set up app environment");
    return 255;
  }

  if (!isProfileOverridden) {
    
    if (NS_FAILED(parser.GetString("Webapp",
                                   "Profile",
                                   profile,
                                   MAXPATHLEN))) {
      Output("Unable to retrieve profile from web app INI file");
      return 255;
    }
  }

  char firefoxDir[MAXPATHLEN];

  
  

  
  rv = parser.GetString("WebappRT",
                        "InstallDir",
                        firefoxDir,
                        MAXPATHLEN);
  if (NS_SUCCEEDED(rv)) {
    if (AttemptLoadFromDir(firefoxDir)) {
      return 0;
    }
  }

  
  
  if (GetFirefoxDirFromRegistry(firefoxDir)) {
    if (AttemptLoadFromDir(firefoxDir)) {
      
      return 0;
    }
  }

  
  Output("This app requires that Firefox version 16 or above is installed."
         " Firefox 16+ has not been detected.");
  return 255;
}
