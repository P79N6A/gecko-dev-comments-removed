




#include "nsXULAppAPI.h"
#include "mozilla/AppData.h"
#include "application.ini.h"
#include "nsXPCOMGlue.h"
#if defined(XP_WIN)
#include <windows.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#elif defined(XP_UNIX)
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#endif

#ifdef XP_MACOSX
#include <mach/mach_time.h>
#include "MacQuirks.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsStringGlue.h"





#ifdef XP_WIN

#include "nsWindowsWMain.cpp"
#define snprintf _snprintf
#define strcasecmp _stricmp
#endif
#include "BinaryPath.h"

#include "nsXPCOMPrivate.h" 

#include "mozilla/Telemetry.h"
#include "mozilla/WindowsDllBlocklist.h"

using namespace mozilla;

#ifdef XP_MACOSX
#define kOSXResourcesFolder "Resources"
#endif
#define kDesktopFolder "browser"
#define kMetroFolder "metro"
#define kMetroAppIniFilename "metroapp.ini"
#ifdef XP_WIN
#define kMetroTestFile "tests.ini"
const char* kMetroConsoleIdParam = "testconsoleid=";
#endif

static void Output(const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#ifndef XP_WIN
  vfprintf(stderr, fmt, ap);
#else
  char msg[2048];
  vsnprintf_s(msg, _countof(msg), _TRUNCATE, fmt, ap);

  wchar_t wide_msg[2048];
  MultiByteToWideChar(CP_UTF8,
                      0,
                      msg,
                      -1,
                      wide_msg,
                      _countof(wide_msg));
#if MOZ_WINCONSOLE
  fwprintf_s(stderr, wide_msg);
#else
  
  
  HMODULE user32 = LoadLibraryW(L"user32.dll");
  if (user32) {
    decltype(MessageBoxW)* messageBoxW =
      (decltype(MessageBoxW)*) GetProcAddress(user32, "MessageBoxW");
    if (messageBoxW) {
      messageBoxW(nullptr, wide_msg, L"Firefox", MB_OK
                                               | MB_ICONERROR
                                               | MB_SETFOREGROUND);
    }
    FreeLibrary(user32);
  }
#endif
#endif

  va_end(ap);
}




static bool IsArg(const char* arg, const char* s)
{
  if (*arg == '-')
  {
    if (*++arg == '-')
      ++arg;
    return !strcasecmp(arg, s);
  }

#if defined(XP_WIN)
  if (*arg == '/')
    return !strcasecmp(++arg, s);
#endif

  return false;
}

#ifdef XP_WIN









static void AttachToTestHarness()
{
  
  HANDLE winOut = CreateFileA("\\\\.\\pipe\\metrotestharness",
                              GENERIC_WRITE,
                              FILE_SHARE_WRITE, 0,
                              OPEN_EXISTING, 0, 0);
  
  if (winOut == INVALID_HANDLE_VALUE) {
    OutputDebugStringW(L"Could not create named logging pipe.\n");
    return;
  }

  
  int stdOut = _open_osfhandle((intptr_t)winOut, _O_APPEND);
  if (stdOut == -1) {
    OutputDebugStringW(L"Could not open c-runtime handle.\n");
    return;
  }
  FILE *fp = _fdopen(stdOut, "a");
  *stdout = *fp;
}
#endif

XRE_GetFileFromPathType XRE_GetFileFromPath;
XRE_CreateAppDataType XRE_CreateAppData;
XRE_FreeAppDataType XRE_FreeAppData;
XRE_TelemetryAccumulateType XRE_TelemetryAccumulate;
XRE_StartupTimelineRecordType XRE_StartupTimelineRecord;
XRE_mainType XRE_main;
XRE_StopLateWriteChecksType XRE_StopLateWriteChecks;

static const nsDynamicFunctionLoad kXULFuncs[] = {
    { "XRE_GetFileFromPath", (NSFuncPtr*) &XRE_GetFileFromPath },
    { "XRE_CreateAppData", (NSFuncPtr*) &XRE_CreateAppData },
    { "XRE_FreeAppData", (NSFuncPtr*) &XRE_FreeAppData },
    { "XRE_TelemetryAccumulate", (NSFuncPtr*) &XRE_TelemetryAccumulate },
    { "XRE_StartupTimelineRecord", (NSFuncPtr*) &XRE_StartupTimelineRecord },
    { "XRE_main", (NSFuncPtr*) &XRE_main },
    { "XRE_StopLateWriteChecks", (NSFuncPtr*) &XRE_StopLateWriteChecks },
    { nullptr, nullptr }
};

static int do_main(int argc, char* argv[], nsIFile *xreDirectory)
{
  nsCOMPtr<nsIFile> appini;
  nsresult rv;
  uint32_t mainFlags = 0;

  
  
  const char *appDataFile = getenv("XUL_APP_FILE");
  if (appDataFile && *appDataFile) {
    rv = XRE_GetFileFromPath(appDataFile, getter_AddRefs(appini));
    if (NS_FAILED(rv)) {
      Output("Invalid path found: '%s'", appDataFile);
      return 255;
    }
  }
  else if (argc > 1 && IsArg(argv[1], "app")) {
    if (argc == 2) {
      Output("Incorrect number of arguments passed to -app");
      return 255;
    }

    rv = XRE_GetFileFromPath(argv[2], getter_AddRefs(appini));
    if (NS_FAILED(rv)) {
      Output("application.ini path not recognized: '%s'", argv[2]);
      return 255;
    }

    char appEnv[MAXPATHLEN];
    snprintf(appEnv, MAXPATHLEN, "XUL_APP_FILE=%s", argv[2]);
    if (putenv(appEnv)) {
      Output("Couldn't set %s.\n", appEnv);
      return 255;
    }
    argv[2] = argv[0];
    argv += 2;
    argc -= 2;
  }

  if (appini) {
    nsXREAppData *appData;
    rv = XRE_CreateAppData(appini, &appData);
    if (NS_FAILED(rv)) {
      Output("Couldn't read application.ini");
      return 255;
    }
    
    appData->xreDirectory = xreDirectory;
    int result = XRE_main(argc, argv, appData, mainFlags);
    XRE_FreeAppData(appData);
    return result;
  }

  bool metroOnDesktop = false;

#ifdef MOZ_METRO
  if (argc > 1) {
    
    
    
    if (IsArg(argv[1], "ServerName:DefaultBrowserServer")) {
      mainFlags = XRE_MAIN_FLAG_USE_METRO;
      argv[1] = argv[0];
      argv++;
      argc--;
    } else if (IsArg(argv[1], "BackgroundSessionClosed")) {
      
      
      mainFlags = XRE_MAIN_FLAG_USE_METRO;
    } else {
#ifndef RELEASE_BUILD
      
      
      for (int idx = 1; idx < argc; idx++) {
        if (IsArg(argv[idx], "metrodesktop")) {
          metroOnDesktop = true;
          
          char crashSwitch[] = "MOZ_CRASHREPORTER_DISABLE=1";
          putenv(crashSwitch);
          break;
        } 
      }
#endif
    }
  }
#endif

  
  if (mainFlags != XRE_MAIN_FLAG_USE_METRO && !metroOnDesktop) {
    ScopedAppData appData(&sAppData);
    nsCOMPtr<nsIFile> exeFile;
    rv = mozilla::BinaryPath::GetFile(argv[0], getter_AddRefs(exeFile));
    if (NS_FAILED(rv)) {
      Output("Couldn't find the application directory.\n");
      return 255;
    }

    nsCOMPtr<nsIFile> greDir;
    exeFile->GetParent(getter_AddRefs(greDir));
#ifdef XP_MACOSX
    greDir->SetNativeLeafName(NS_LITERAL_CSTRING(kOSXResourcesFolder));
#endif
    nsCOMPtr<nsIFile> appSubdir;
    greDir->Clone(getter_AddRefs(appSubdir));
    appSubdir->Append(NS_LITERAL_STRING(kDesktopFolder));

    SetStrongPtr(appData.directory, static_cast<nsIFile*>(appSubdir.get()));
    
    appData.xreDirectory = xreDirectory;

    return XRE_main(argc, argv, &appData, mainFlags);
  }

  
#ifdef MOZ_METRO
  nsCOMPtr<nsIFile> iniFile, appSubdir;

  xreDirectory->Clone(getter_AddRefs(iniFile));
  xreDirectory->Clone(getter_AddRefs(appSubdir));

  iniFile->Append(NS_LITERAL_STRING(kMetroFolder));
  iniFile->Append(NS_LITERAL_STRING(kMetroAppIniFilename));

  appSubdir->Append(NS_LITERAL_STRING(kMetroFolder));

  nsAutoCString path;
  if (NS_FAILED(iniFile->GetNativePath(path))) {
    Output("Couldn't get ini file path.\n");
    return 255;
  }

  nsXREAppData *appData;
  rv = XRE_CreateAppData(iniFile, &appData);
  if (NS_FAILED(rv) || !appData) {
    Output("Couldn't read application.ini");
    return 255;
  }

  SetStrongPtr(appData->directory, static_cast<nsIFile*>(appSubdir.get()));
  
  appData->xreDirectory = xreDirectory;

#ifdef XP_WIN
  if (!metroOnDesktop) {
    nsCOMPtr<nsIFile> testFile;

    xreDirectory->Clone(getter_AddRefs(testFile));
    testFile->Append(NS_LITERAL_STRING(kMetroTestFile));

    nsAutoCString path;
    if (NS_FAILED(testFile->GetNativePath(path))) {
      Output("Couldn't get test file path.\n");
      return 255;
    }

    
    HANDLE hTestFile = CreateFileA(path.get(),
                                   GENERIC_READ,
                                   0, nullptr, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
    if (hTestFile != INVALID_HANDLE_VALUE) {
      
      char buffer[1024];
      memset(buffer, 0, sizeof(buffer));
      DWORD bytesRead = 0;
      if (!ReadFile(hTestFile, (VOID*)buffer, sizeof(buffer)-1,
                    &bytesRead, nullptr) || !bytesRead) {
        CloseHandle(hTestFile);
        printf("failed to read test file '%s'", testFile);
        return -1;
      }
      CloseHandle(hTestFile);

      
      char* newArgv[20];
      int newArgc = 1;

      memset(newArgv, 0, sizeof(newArgv));

      char* ptr = buffer;
      newArgv[0] = ptr;
      while (*ptr != '\0' &&
             (ptr - buffer) < sizeof(buffer) &&
             newArgc < ARRAYSIZE(newArgv)) {
        if (isspace(*ptr)) {
          *ptr = '\0';
          ptr++;
          newArgv[newArgc] = ptr;
          newArgc++;
          continue;
        }
        ptr++;
      }
      if (ptr == newArgv[newArgc-1])
        newArgc--;

      
      AttachToTestHarness();

      int result = XRE_main(newArgc, newArgv, appData, mainFlags);
      XRE_FreeAppData(appData);
      return result;
    }
  }
#endif

  int result = XRE_main(argc, argv, appData, mainFlags);
  XRE_FreeAppData(appData);
  return result;
#endif

  NS_NOTREACHED("browser do_main failed to pickup proper initialization");
  return 255;
}

#ifdef XP_WIN





static DWORD sLastGTCResult = 0;





static DWORD sLastGTCRollover = 0;








static ULONGLONG WINAPI
MozGetTickCount64()
{
  DWORD GTC = ::GetTickCount();

  

  if ((sLastGTCResult > GTC) && ((sLastGTCResult - GTC) > (1UL << 30)))
    ++sLastGTCRollover;

  sLastGTCResult = GTC;
  return (ULONGLONG)sLastGTCRollover << 32 | sLastGTCResult;
}

typedef ULONGLONG (WINAPI* GetTickCount64_t)();
static GetTickCount64_t sGetTickCount64 = nullptr;

#endif





static uint64_t
TimeStamp_Now()
{
#ifdef XP_WIN
  LARGE_INTEGER freq;
  ::QueryPerformanceFrequency(&freq);

  HMODULE kernelDLL = GetModuleHandleW(L"kernel32.dll");
  sGetTickCount64 = reinterpret_cast<GetTickCount64_t>
    (GetProcAddress(kernelDLL, "GetTickCount64"));

  if (!sGetTickCount64) {
    

    sGetTickCount64 = MozGetTickCount64;
  }

  return sGetTickCount64() * freq.QuadPart;
#elif defined(XP_MACOSX)
  return mach_absolute_time();
#elif defined(HAVE_CLOCK_MONOTONIC)
  struct timespec ts;
  int rv = clock_gettime(CLOCK_MONOTONIC, &ts);

  if (rv != 0) {
    return 0;
  }

  uint64_t baseNs = (uint64_t)ts.tv_sec * 1000000000;
  return baseNs + (uint64_t)ts.tv_nsec;
#endif
}

static bool
FileExists(const char *path)
{
#ifdef XP_WIN
  wchar_t wideDir[MAX_PATH];
  MultiByteToWideChar(CP_UTF8, 0, path, -1, wideDir, MAX_PATH);
  DWORD fileAttrs = GetFileAttributesW(wideDir);
  return fileAttrs != INVALID_FILE_ATTRIBUTES;
#else
  return access(path, R_OK) == 0;
#endif
}

#ifdef LIBXUL_SDK
#  define XPCOM_PATH "xulrunner" XPCOM_FILE_PATH_SEPARATOR XPCOM_DLL
#else
#  define XPCOM_PATH XPCOM_DLL
#endif
static nsresult
InitXPCOMGlue(const char *argv0, nsIFile **xreDirectory)
{
  char exePath[MAXPATHLEN];

  nsresult rv = mozilla::BinaryPath::Get(argv0, exePath);
  if (NS_FAILED(rv)) {
    Output("Couldn't find the application directory.\n");
    return rv;
  }

  char *lastSlash = strrchr(exePath, XPCOM_FILE_PATH_SEPARATOR[0]);
  if (!lastSlash || (size_t(lastSlash - exePath) > MAXPATHLEN - sizeof(XPCOM_PATH) - 1))
    return NS_ERROR_FAILURE;

  strcpy(lastSlash + 1, XPCOM_PATH);
  lastSlash += sizeof(XPCOM_PATH) - sizeof(XPCOM_DLL);

  if (!FileExists(exePath)) {
#if defined(LIBXUL_SDK) && defined(XP_MACOSX)
    
    bool greFound = false;
    CFBundleRef appBundle = CFBundleGetMainBundle();
    if (!appBundle)
      return NS_ERROR_FAILURE;
    CFURLRef fwurl = CFBundleCopyPrivateFrameworksURL(appBundle);
    CFURLRef absfwurl = nullptr;
    if (fwurl) {
      absfwurl = CFURLCopyAbsoluteURL(fwurl);
      CFRelease(fwurl);
    }
    if (absfwurl) {
      CFURLRef xulurl =
        CFURLCreateCopyAppendingPathComponent(nullptr, absfwurl,
                                              CFSTR("XUL.framework"),
                                              true);

      if (xulurl) {
        CFURLRef xpcomurl =
          CFURLCreateCopyAppendingPathComponent(nullptr, xulurl,
                                                CFSTR("libxpcom.dylib"),
                                                false);

        if (xpcomurl) {
          if (CFURLGetFileSystemRepresentation(xpcomurl, true,
                                               (UInt8*) exePath,
                                               sizeof(exePath)) &&
              access(tbuffer, R_OK | X_OK) == 0) {
            if (realpath(tbuffer, exePath)) {
              greFound = true;
            }
          }
          CFRelease(xpcomurl);
        }
        CFRelease(xulurl);
      }
      CFRelease(absfwurl);
    }
  }
  if (!greFound) {
#endif
    Output("Could not find the Mozilla runtime.\n");
    return NS_ERROR_FAILURE;
  }

  
  XPCOMGlueEnablePreload();

  rv = XPCOMGlueStartup(exePath);
  if (NS_FAILED(rv)) {
    Output("Couldn't load XPCOM.\n");
    return rv;
  }

  rv = XPCOMGlueLoadXULFunctions(kXULFuncs);
  if (NS_FAILED(rv)) {
    Output("Couldn't load XRE functions.\n");
    return rv;
  }

  NS_LogInit();

  
  *lastSlash = '\0';
#ifdef XP_MACOSX
  lastSlash = strrchr(exePath, XPCOM_FILE_PATH_SEPARATOR[0]);
  strcpy(lastSlash + 1, kOSXResourcesFolder);
#endif
#ifdef XP_WIN
  rv = NS_NewLocalFile(NS_ConvertUTF8toUTF16(exePath), false,
                       xreDirectory);
#else
  rv = NS_NewNativeLocalFile(nsDependentCString(exePath), false,
                             xreDirectory);
#endif

  return rv;
}

int main(int argc, char* argv[])
{
#ifdef DEBUG_delay_start_metro
  Sleep(5000);
#endif
  uint64_t start = TimeStamp_Now();

#ifdef XP_MACOSX
  TriggerQuirks();
#endif

  int gotCounters;
#if defined(XP_UNIX)
  struct rusage initialRUsage;
  gotCounters = !getrusage(RUSAGE_SELF, &initialRUsage);
#elif defined(XP_WIN)
  IO_COUNTERS ioCounters;
  gotCounters = GetProcessIoCounters(GetCurrentProcess(), &ioCounters);
#endif

  nsIFile *xreDirectory;

#ifdef HAS_DLL_BLOCKLIST
  DllBlocklist_Initialize();

#ifdef DEBUG
  
  
  if (GetModuleHandleA("user32.dll")) {
    fprintf(stderr, "DLL blocklist was unable to intercept AppInit DLLs.\n");
  }
#endif
#endif

  nsresult rv = InitXPCOMGlue(argv[0], &xreDirectory);
  if (NS_FAILED(rv)) {
    return 255;
  }

  XRE_StartupTimelineRecord(mozilla::StartupTimeline::START, start);

  if (gotCounters) {
#if defined(XP_WIN)
    XRE_TelemetryAccumulate(mozilla::Telemetry::EARLY_GLUESTARTUP_READ_OPS,
                            int(ioCounters.ReadOperationCount));
    XRE_TelemetryAccumulate(mozilla::Telemetry::EARLY_GLUESTARTUP_READ_TRANSFER,
                            int(ioCounters.ReadTransferCount / 1024));
    IO_COUNTERS newIoCounters;
    if (GetProcessIoCounters(GetCurrentProcess(), &newIoCounters)) {
      XRE_TelemetryAccumulate(mozilla::Telemetry::GLUESTARTUP_READ_OPS,
                              int(newIoCounters.ReadOperationCount - ioCounters.ReadOperationCount));
      XRE_TelemetryAccumulate(mozilla::Telemetry::GLUESTARTUP_READ_TRANSFER,
                              int((newIoCounters.ReadTransferCount - ioCounters.ReadTransferCount) / 1024));
    }
#elif defined(XP_UNIX)
    XRE_TelemetryAccumulate(mozilla::Telemetry::EARLY_GLUESTARTUP_HARD_FAULTS,
                            int(initialRUsage.ru_majflt));
    struct rusage newRUsage;
    if (!getrusage(RUSAGE_SELF, &newRUsage)) {
      XRE_TelemetryAccumulate(mozilla::Telemetry::GLUESTARTUP_HARD_FAULTS,
                              int(newRUsage.ru_majflt - initialRUsage.ru_majflt));
    }
#endif
  }

  int result = do_main(argc, argv, xreDirectory);

  NS_LogTerm();

#ifdef XP_MACOSX
  
  
  
  
  
  XRE_StopLateWriteChecks();
#endif

  return result;
}
