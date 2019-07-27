





#include <stdlib.h>
#include <stdio.h>
#include "nsUpdateDriver.h"
#include "nsXULAppAPI.h"
#include "nsAppRunner.h"
#include "nsIWritablePropertyBag.h"
#include "nsIFile.h"
#include "nsIVariant.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "prproces.h"
#include "prlog.h"
#include "prenv.h"
#include "nsVersionComparator.h"
#include "nsXREDirProvider.h"
#include "SpecialSystemDirectory.h"
#include "nsDirectoryServiceDefs.h"
#include "nsThreadUtils.h"
#include "nsIXULAppInfo.h"
#include "mozilla/Preferences.h"
#include "nsPrintfCString.h"
#include "mozilla/DebugOnly.h"

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
#include "MacLaunchHelper.h"
#endif

#if defined(XP_WIN)
# include <direct.h>
# include <process.h>
# include <windows.h>
# include <shlwapi.h>
# include "nsWindowsHelpers.h"
# define getcwd(path, size) _getcwd(path, size)
# define getpid() GetCurrentProcessId()
#elif defined(XP_UNIX)
# include <unistd.h>
#endif

using namespace mozilla;





















#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define USE_EXECV
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo *
GetUpdateLog()
{
  static PRLogModuleInfo *sUpdateLog;
  if (!sUpdateLog)
    sUpdateLog = PR_NewLogModule("updatedriver");
  return sUpdateLog;
}
#endif
#define LOG(args) PR_LOG(GetUpdateLog(), PR_LOG_DEBUG, args)

#ifdef XP_WIN
static const char kUpdaterBin[] = "updater.exe";
#else
static const char kUpdaterBin[] = "updater";
#endif
static const char kUpdaterINI[] = "updater.ini";
#ifdef XP_MACOSX
static const char kUpdaterApp[] = "updater.app";
#endif
#if defined(XP_UNIX) && !defined(XP_MACOSX)
static const char kUpdaterPNG[] = "updater.png";
#endif

#if defined(MOZ_WIDGET_GONK)
#include <linux/ioprio.h>

static const int kB2GServiceArgc = 2;
static const char *kB2GServiceArgv[] = { "/system/bin/start", "b2g" };

static const char kAppUpdaterPrio[]        = "app.update.updater.prio";
static const char kAppUpdaterOomScoreAdj[] = "app.update.updater.oom_score_adj";
static const char kAppUpdaterIOPrioClass[] = "app.update.updater.ioprio.class";
static const char kAppUpdaterIOPrioLevel[] = "app.update.updater.ioprio.level";

static const int  kAppUpdaterPrioDefault        = 19;     
static const int  kAppUpdaterOomScoreAdjDefault = -1000;  
static const int  kAppUpdaterIOPrioClassDefault = IOPRIO_CLASS_IDLE;
static const int  kAppUpdaterIOPrioLevelDefault = 0;      
#endif

static nsresult
GetCurrentWorkingDir(char *buf, size_t size)
{
  
  

#if defined(XP_WIN)
  wchar_t wpath[MAX_PATH];
  if (!_wgetcwd(wpath, size))
    return NS_ERROR_FAILURE;
  NS_ConvertUTF16toUTF8 path(wpath);
  strncpy(buf, path.get(), size);
#else
  if(!getcwd(buf, size))
    return NS_ERROR_FAILURE;
#endif
  return NS_OK;
}








static nsresult
GetInstallDirPath(nsIFile *appDir, nsACString& installDirPath)
{
  nsresult rv;
#ifdef XP_MACOSX
  nsCOMPtr<nsIFile> parentDir1, parentDir2;
  rv = appDir->GetParent(getter_AddRefs(parentDir1));
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = parentDir1->GetParent(getter_AddRefs(parentDir2));
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = parentDir2->GetNativePath(installDirPath);
#elif XP_WIN
  nsAutoString installDirPathW;
  rv = appDir->GetPath(installDirPathW);
  if (NS_FAILED(rv)) {
    return rv;
  }
  installDirPath = NS_ConvertUTF16toUTF8(installDirPathW);
#else
  rv = appDir->GetNativePath(installDirPath);
#endif
  if (NS_FAILED(rv)) {
    return rv;
  }
  return NS_OK;
}

#if defined(XP_MACOSX)



static nsresult
GetXULRunnerStubPath(const char* argv0, nsIFile* *aResult)
{
  
  CFBundleRef appBundle = ::CFBundleGetMainBundle();
  if (!appBundle)
    return NS_ERROR_FAILURE;

  CFURLRef bundleURL = ::CFBundleCopyExecutableURL(appBundle);
  if (!bundleURL)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFileMac> lfm;
  nsresult rv = NS_NewLocalFileWithCFURL(bundleURL, true, getter_AddRefs(lfm));

  ::CFRelease(bundleURL);

  if (NS_FAILED(rv))
    return rv;

  lfm.forget(aResult);
  return NS_OK;
}
#endif 

static bool
GetFile(nsIFile *dir, const nsCSubstring &name, nsCOMPtr<nsIFile> &result)
{
  nsresult rv;
  
  nsCOMPtr<nsIFile> file;
  rv = dir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return false;

  rv = file->AppendNative(name);
  if (NS_FAILED(rv))
    return false;

  result = do_QueryInterface(file, &rv);
  return NS_SUCCEEDED(rv);
}

static bool
GetStatusFile(nsIFile *dir, nsCOMPtr<nsIFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("update.status"), result);
}









template <size_t Size>
static bool
GetStatusFileContents(nsIFile *statusFile, char (&buf)[Size])
{
  
  PR_STATIC_ASSERT(Size > 16);

  PRFileDesc *fd = nullptr;
  nsresult rv = statusFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return false;

  const int32_t n = PR_Read(fd, buf, Size);
  PR_Close(fd);

  return (n >= 0);
}

typedef enum {
  eNoUpdateAction,
  ePendingUpdate,
  ePendingService,
  eAppliedUpdate,
  eAppliedService
} UpdateStatus;









static UpdateStatus
GetUpdateStatus(nsIFile* dir, nsCOMPtr<nsIFile> &statusFile)
{
  if (GetStatusFile(dir, statusFile)) {
    char buf[32];
    if (GetStatusFileContents(statusFile, buf)) {
      const char kPending[] = "pending";
      const char kPendingService[] = "pending-service";
      const char kApplied[] = "applied";
      const char kAppliedService[] = "applied-service";
      if (!strncmp(buf, kPendingService, sizeof(kPendingService) - 1)) {
        return ePendingService;
      }
      if (!strncmp(buf, kPending, sizeof(kPending) - 1)) {
        return ePendingUpdate;
      }
      if (!strncmp(buf, kAppliedService, sizeof(kAppliedService) - 1)) {
        return eAppliedService;
      }
      if (!strncmp(buf, kApplied, sizeof(kApplied) - 1)) {
        return eAppliedUpdate;
      }
    }
  }
  return eNoUpdateAction;
}

static bool
GetVersionFile(nsIFile *dir, nsCOMPtr<nsIFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("update.version"), result);
}



static bool
IsOlderVersion(nsIFile *versionFile, const char *appVersion)
{
  PRFileDesc *fd = nullptr;
  nsresult rv = versionFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return true;

  char buf[32];
  const int32_t n = PR_Read(fd, buf, sizeof(buf));
  PR_Close(fd);

  if (n < 0)
    return false;

  
  if (buf[n - 1] == '\n')
    buf[n - 1] = '\0';

  
  
  const char kNull[] = "null";
  if (strncmp(buf, kNull, sizeof(kNull) - 1) == 0)
    return false;

  if (mozilla::Version(appVersion) > buf)
    return true;

  return false;
}

#if defined(XP_WIN) && defined(MOZ_METRO)
static bool
IsWindowsMetroUpdateRequest(int appArgc, char **appArgv)
{
  for (int index = 0; index < appArgc; index++) {
    if (!strcmp(appArgv[index], "--metro-update")) {
      return true;
    }
  }
  return false;
}
#endif

static bool
CopyFileIntoUpdateDir(nsIFile *parentDir, const char *leafName, nsIFile *updateDir)
{
  nsDependentCString leaf(leafName);
  nsCOMPtr<nsIFile> file;

  
  nsresult rv = updateDir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return false;
  rv = file->AppendNative(leaf);
  if (NS_FAILED(rv))
    return false;
  file->Remove(true);

  
  rv = parentDir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return false;
  rv = file->AppendNative(leaf);
  if (NS_FAILED(rv))
    return false;
  rv = file->CopyToNative(updateDir, EmptyCString());
  if (NS_FAILED(rv))
    return false;

  return true;
}

static bool
CopyUpdaterIntoUpdateDir(nsIFile *greDir, nsIFile *appDir, nsIFile *updateDir,
                         nsCOMPtr<nsIFile> &updater)
{
  
#if defined(XP_MACOSX)
  if (!CopyFileIntoUpdateDir(appDir, kUpdaterApp, updateDir))
    return false;
  CopyFileIntoUpdateDir(greDir, kUpdaterINI, updateDir);
#else
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterBin, updateDir))
    return false;
  CopyFileIntoUpdateDir(appDir, kUpdaterINI, updateDir);
#endif
#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(ANDROID)
  nsCOMPtr<nsIFile> iconDir;
  appDir->Clone(getter_AddRefs(iconDir));
  iconDir->AppendNative(NS_LITERAL_CSTRING("icons"));
  if (!CopyFileIntoUpdateDir(iconDir, kUpdaterPNG, updateDir))
    return false;
#endif
  
  nsresult rv = updateDir->Clone(getter_AddRefs(updater));
  if (NS_FAILED(rv))
    return false;
#if defined(XP_MACOSX)
  rv  = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterApp));
  nsresult tmp = updater->AppendNative(NS_LITERAL_CSTRING("Contents"));
  if (NS_FAILED(tmp)) {
    rv = tmp;
  }
  tmp = updater->AppendNative(NS_LITERAL_CSTRING("MacOS"));
  if (NS_FAILED(tmp) || NS_FAILED(rv))
    return false;
#endif
  rv = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterBin));
  return NS_SUCCEEDED(rv); 
}











static void
SwitchToUpdatedApp(nsIFile *greDir, nsIFile *updateDir,
                   nsIFile *appDir, int appArgc, char **appArgv)
{
  nsresult rv;

  
  
  

  nsCOMPtr<nsIFile> mozUpdaterDir;
  rv = updateDir->Clone(getter_AddRefs(mozUpdaterDir));
  if (NS_FAILED(rv)) {
    LOG(("failed cloning update dir\n"));
    return;
  }

  
  
  
  
  
  
  mozUpdaterDir->Append(NS_LITERAL_STRING("MozUpdater"));
  mozUpdaterDir->Append(NS_LITERAL_STRING("bgupdate"));
  mozUpdaterDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0755);

  nsCOMPtr<nsIFile> updater;
  if (!CopyUpdaterIntoUpdateDir(greDir, appDir, mozUpdaterDir, updater)) {
    LOG(("failed copying updater\n"));
    return;
  }

  
  
  nsCOMPtr<nsIFile> appFile;

#if defined(XP_MACOSX)
  
  
  GetXULRunnerStubPath(appArgv[0], getter_AddRefs(appFile));
#else
  XRE_GetBinaryPath(appArgv[0], getter_AddRefs(appFile));
#endif

  if (!appFile)
    return;

#ifdef XP_WIN
  nsAutoString appFilePathW;
  rv = appFile->GetPath(appFilePathW);
  if (NS_FAILED(rv))
    return;
  NS_ConvertUTF16toUTF8 appFilePath(appFilePathW);

  nsAutoString updaterPathW;
  rv = updater->GetPath(updaterPathW);
  if (NS_FAILED(rv))
    return;

  NS_ConvertUTF16toUTF8 updaterPath(updaterPathW);
#else

  nsAutoCString appFilePath;
#if defined(MOZ_WIDGET_GONK)
  appFilePath.Assign(kB2GServiceArgv[0]);
  appArgc = kB2GServiceArgc;
  appArgv = const_cast<char**>(kB2GServiceArgv);
#else
  rv = appFile->GetNativePath(appFilePath);
  if (NS_FAILED(rv))
    return;
#endif

  nsAutoCString updaterPath;
  rv = updater->GetNativePath(updaterPath);
  if (NS_FAILED(rv))
    return;
#endif

  nsAutoCString installDirPath;
  rv = GetInstallDirPath(appDir, installDirPath);
  if (NS_FAILED(rv)) {
    return;
  }

  
  nsAutoCString applyToDir;
  nsCOMPtr<nsIFile> updatedDir;
#ifdef XP_MACOSX
  if (!GetFile(updateDir, NS_LITERAL_CSTRING("Updated.app"), updatedDir)) {
#else
  if (!GetFile(appDir, NS_LITERAL_CSTRING("updated"), updatedDir)) {
#endif
    return;
  }
#ifdef XP_WIN
  nsAutoString applyToDirW;
  rv = updatedDir->GetPath(applyToDirW);
  if (NS_FAILED(rv)) {
    return;
  }
  applyToDir = NS_ConvertUTF16toUTF8(applyToDirW);
#else
  rv = updatedDir->GetNativePath(applyToDir);
#endif
  if (NS_FAILED(rv)) {
    return;
  }

  
  bool updatedDirExists = false;
  updatedDir->Exists(&updatedDirExists);
  if (!updatedDirExists) {
    return;
  }

#if defined(XP_WIN)
  nsAutoString updateDirPathW;
  rv = updateDir->GetPath(updateDirPathW);
  NS_ConvertUTF16toUTF8 updateDirPath(updateDirPathW);
#else
  nsAutoCString updateDirPath;
  rv = updateDir->GetNativePath(updateDirPath);
#endif
  if (NS_FAILED(rv))
    return;

  
  char workingDirPath[MAXPATHLEN];
  rv = GetCurrentWorkingDir(workingDirPath, sizeof(workingDirPath));
  if (NS_FAILED(rv))
    return;

  
  
#if defined(USE_EXECV)
  nsAutoCString pid("0");
#else
  nsAutoCString pid;
  pid.AppendInt((int32_t) getpid());
#endif

  
  
  pid.AppendLiteral("/replace");

  int immersiveArgc = 0;
#if defined(XP_WIN) && defined(MOZ_METRO)
  
  
  if (IsWindowsMetroUpdateRequest(appArgc, appArgv) || IsRunningInWindowsMetro()) {
    immersiveArgc = 1;
  }
#endif
  int argc = appArgc + 6 + immersiveArgc;
  char **argv = new char*[argc + 1];
  if (!argv)
    return;
  argv[0] = (char*) updaterPath.get();
  argv[1] = (char*) updateDirPath.get();
  argv[2] = (char*) installDirPath.get();
  argv[3] = (char*) applyToDir.get();
  argv[4] = (char*) pid.get();
  if (appArgc) {
    argv[5] = workingDirPath;
    argv[6] = (char*) appFilePath.get();
    for (int i = 1; i < appArgc; ++i)
      argv[6 + i] = appArgv[i];
#ifdef XP_WIN
    if (immersiveArgc) {
      argv[argc - 1] = "-ServerName:DefaultBrowserServer";
    }
#endif
    argv[argc] = nullptr;
  } else {
    argc = 5;
    argv[5] = nullptr;
  }

  if (gSafeMode) {
    PR_SetEnv("MOZ_SAFE_MODE_RESTART=1");
  }

  LOG(("spawning updater process for replacing [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
# if defined(MOZ_WIDGET_GONK)
  
  
  
  unsetenv("LD_PRELOAD");
# endif
  execv(updaterPath.get(), argv);
#elif defined(XP_WIN)
  
  if (!WinLaunchChild(updaterPathW.get(), argc, argv)) {
    return;
  }
  _exit(0);
#elif defined(XP_MACOSX)
  CommandLineServiceMac::SetupMacCommandLine(argc, argv, true);
  
  
  
  LaunchChildMac(argc, argv);
  exit(0);
#else
  PR_CreateProcessDetached(updaterPath.get(), argv, nullptr, nullptr);
  exit(0);
#endif
}

#if defined(MOZ_WIDGET_GONK)
static nsresult
GetOSApplyToDir(nsACString& applyToDir)
{
  nsCOMPtr<nsIProperties> ds =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
  NS_ASSERTION(ds, "Can't get directory service");

  nsCOMPtr<nsIFile> osApplyToDir;
  nsresult rv = ds->Get(XRE_OS_UPDATE_APPLY_TO_DIR, NS_GET_IID(nsIFile),
                                   getter_AddRefs(osApplyToDir));
  if (NS_FAILED(rv)) {
    LOG(("Can't get the OS applyTo dir"));
    return rv;
  }

  return osApplyToDir->GetNativePath(applyToDir);
}

static void
SetOSApplyToDir(nsIUpdate* update, const nsACString& osApplyToDir)
{
  nsresult rv;
  nsCOMPtr<nsIWritablePropertyBag> updateProperties =
    do_QueryInterface(update, &rv);

  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIWritableVariant> variant =
    do_CreateInstance("@mozilla.org/variant;1", &rv);
  if (NS_FAILED(rv)) {
    return;
  }

  rv = variant->SetAsACString(osApplyToDir);
  if (NS_FAILED(rv)) {
    return;
  }

  updateProperties->SetProperty(NS_LITERAL_STRING("osApplyToDir"), variant);
}
#endif
















static void
ApplyUpdate(nsIFile *greDir, nsIFile *updateDir, nsIFile *statusFile,
            nsIFile *appDir, int appArgc, char **appArgv,
            bool restart, bool isOSUpdate, nsIFile *osApplyToDir,
            ProcessType *outpid)
{
  nsresult rv;

  
  
  
  

  nsCOMPtr<nsIFile> updater;
  if (!CopyUpdaterIntoUpdateDir(greDir, appDir, updateDir, updater)) {
    LOG(("failed copying updater\n"));
    return;
  }

  
  
  nsCOMPtr<nsIFile> appFile;

#if defined(XP_MACOSX)
  
  
  GetXULRunnerStubPath(appArgv[0], getter_AddRefs(appFile));
#else
  XRE_GetBinaryPath(appArgv[0], getter_AddRefs(appFile));
#endif

  if (!appFile)
    return;

#ifdef XP_WIN
  nsAutoString appFilePathW;
  rv = appFile->GetPath(appFilePathW);
  if (NS_FAILED(rv))
    return;
  NS_ConvertUTF16toUTF8 appFilePath(appFilePathW);

  nsAutoString updaterPathW;
  rv = updater->GetPath(updaterPathW);
  if (NS_FAILED(rv))
    return;

  NS_ConvertUTF16toUTF8 updaterPath(updaterPathW);

#else
  nsAutoCString appFilePath;
  rv = appFile->GetNativePath(appFilePath);
  if (NS_FAILED(rv))
    return;
  
  nsAutoCString updaterPath;
  rv = updater->GetNativePath(updaterPath);
  if (NS_FAILED(rv))
    return;

#endif

  nsAutoCString installDirPath;
  rv = GetInstallDirPath(appDir, installDirPath);
  if (NS_FAILED(rv))
    return;

  
  
#ifndef MOZ_WIDGET_GONK
  
  
  isOSUpdate = false;
#endif
  nsAutoCString applyToDir;
  nsCOMPtr<nsIFile> updatedDir;
  if (restart && !isOSUpdate) {
    applyToDir.Assign(installDirPath);
  } else {
#ifdef XP_MACOSX
    if (!GetFile(updateDir, NS_LITERAL_CSTRING("Updated.app"), updatedDir)) {
#else
    if (!GetFile(appDir, NS_LITERAL_CSTRING("updated"), updatedDir)) {
#endif
      return;
    }
#ifdef XP_WIN
    nsAutoString applyToDirW;
    rv = updatedDir->GetPath(applyToDirW);
    if (NS_FAILED(rv)) {
      return;
    }
    applyToDir = NS_ConvertUTF16toUTF8(applyToDirW);
#elif MOZ_WIDGET_GONK
    if (isOSUpdate) {
      if (!osApplyToDir) {
        return;
      }
      rv = osApplyToDir->GetNativePath(applyToDir);
    } else {
      rv = updatedDir->GetNativePath(applyToDir);
    }
#else
    rv = updatedDir->GetNativePath(applyToDir);
#endif
  }
  if (NS_FAILED(rv))
    return;

#if defined(XP_WIN)
  nsAutoString updateDirPathW;
  rv = updateDir->GetPath(updateDirPathW);
  NS_ConvertUTF16toUTF8 updateDirPath(updateDirPathW);
#else
  nsAutoCString updateDirPath;
  rv = updateDir->GetNativePath(updateDirPath);
#endif
  if (NS_FAILED(rv)) {
    return;
  }

  
  char workingDirPath[MAXPATHLEN];
  rv = GetCurrentWorkingDir(workingDirPath, sizeof(workingDirPath));
  if (NS_FAILED(rv))
    return;

  
  
  
  
  

  
  
  nsAutoCString pid;
  if (!restart) {
    
    pid.AssignASCII("-1");
  } else {
#if defined(USE_EXECV)
    pid.AssignASCII("0");
#else
    pid.AppendInt((int32_t) getpid());
#endif
  }

  int immersiveArgc = 0;
#if defined(XP_WIN) && defined(MOZ_METRO)
  
  
  if (IsWindowsMetroUpdateRequest(appArgc, appArgv) || IsRunningInWindowsMetro()) {
    immersiveArgc = 1;
  }
#endif
  int argc = appArgc + 6 + immersiveArgc;
  char **argv = new char*[argc + 1 ];
  if (!argv)
    return;
  argv[0] = (char*) updaterPath.get();
  argv[1] = (char*) updateDirPath.get();
  argv[2] = (char*) installDirPath.get();
  argv[3] = (char*) applyToDir.get();
  argv[4] = (char*) pid.get();
  if (restart && appArgc) {
    argv[5] = workingDirPath;
    argv[6] = (char*) appFilePath.get();
    for (int i = 1; i < appArgc; ++i)
      argv[6 + i] = appArgv[i];
#ifdef XP_WIN
    if (immersiveArgc) {
      argv[argc - 1] = "-ServerName:DefaultBrowserServer";
    }
#endif
    argv[argc] = nullptr;
  } else {
    argc = 5;
    argv[5] = nullptr;
  }

  if (gSafeMode) {
    PR_SetEnv("MOZ_SAFE_MODE_RESTART=1");
  }

  if (isOSUpdate) {
    PR_SetEnv("MOZ_OS_UPDATE=1");
  }
#if defined(MOZ_WIDGET_GONK)
  
  
  

  int32_t prioVal = Preferences::GetInt(kAppUpdaterPrio,
                                        kAppUpdaterPrioDefault);
  int32_t oomScoreAdj = Preferences::GetInt(kAppUpdaterOomScoreAdj,
                                            kAppUpdaterOomScoreAdjDefault);
  int32_t ioprioClass = Preferences::GetInt(kAppUpdaterIOPrioClass,
                                            kAppUpdaterIOPrioClassDefault);
  int32_t ioprioLevel = Preferences::GetInt(kAppUpdaterIOPrioLevel,
                                            kAppUpdaterIOPrioLevelDefault);
  nsPrintfCString prioEnv("MOZ_UPDATER_PRIO=%d/%d/%d/%d",
                          prioVal, oomScoreAdj, ioprioClass, ioprioLevel);
  PR_SetEnv(prioEnv.get());
#endif

  LOG(("spawning updater process [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
  
  if (restart) {
    execv(updaterPath.get(), argv);
  } else {
    *outpid = PR_CreateProcess(updaterPath.get(), argv, nullptr, nullptr);
  }
#elif defined(XP_WIN)
  
  if (!WinLaunchChild(updaterPathW.get(), argc, argv, nullptr, outpid)) {
    return;
  }

  if (restart) {
    
    _exit(0);
  }
#elif defined(XP_MACOSX)
  CommandLineServiceMac::SetupMacCommandLine(argc, argv, true);
  
  
  
  LaunchChildMac(argc, argv, 0, outpid);
  if (restart) {
    exit(0);
  }
#else
  *outpid = PR_CreateProcess(updaterPath.get(), argv, nullptr, nullptr);
  if (restart) {
    exit(0);
  }
#endif
}




static void
WaitForProcess(ProcessType pt)
{
#if defined(XP_WIN)
  WaitForSingleObject(pt, INFINITE);
  CloseHandle(pt);
#elif defined(XP_MACOSX)
  waitpid(pt, 0, 0);
#else
  int32_t exitCode;
  PR_WaitProcess(pt, &exitCode);
  if (exitCode != 0) {
    LOG(("Error while running the updater process, check update.log"));
  }
#endif
}

nsresult
ProcessUpdates(nsIFile *greDir, nsIFile *appDir, nsIFile *updRootDir,
               int argc, char **argv, const char *appVersion,
               bool restart, bool isOSUpdate, nsIFile *osApplyToDir,
               ProcessType *pid)
{
  nsresult rv;

  nsCOMPtr<nsIFile> updatesDir;
  rv = updRootDir->Clone(getter_AddRefs(updatesDir));
  if (NS_FAILED(rv))
    return rv;

  rv = updatesDir->AppendNative(NS_LITERAL_CSTRING("updates"));
  if (NS_FAILED(rv))
    return rv;

  rv = updatesDir->AppendNative(NS_LITERAL_CSTRING("0"));
  if (NS_FAILED(rv))
    return rv;
 
  ProcessType dummyPID; 
  const char *processingUpdates = PR_GetEnv("MOZ_PROCESS_UPDATES");
  if (processingUpdates && *processingUpdates) {
    
    const char *stagingUpdate = PR_GetEnv("MOZ_UPDATE_STAGING");
    if (stagingUpdate && *stagingUpdate) {
      restart = false;
      pid = &dummyPID;
    }
  }

  nsCOMPtr<nsIFile> statusFile;
  UpdateStatus status = GetUpdateStatus(updatesDir, statusFile);
  switch (status) {
  case ePendingUpdate:
  case ePendingService: {
    nsCOMPtr<nsIFile> versionFile;
    
    
    
    if (!GetVersionFile(updatesDir, versionFile) ||
        IsOlderVersion(versionFile, appVersion)) {
      updatesDir->Remove(true);
    } else {
      ApplyUpdate(greDir, updatesDir, statusFile,
                  appDir, argc, argv, restart, isOSUpdate, osApplyToDir, pid);
    }
    break;
  }
  case eAppliedUpdate:
  case eAppliedService:
    
    
    SwitchToUpdatedApp(greDir, updatesDir, appDir, argc, argv);
    break;
  case eNoUpdateAction:
    
    
    break;
  }

  return NS_OK;
}



NS_IMPL_ISUPPORTS(nsUpdateProcessor, nsIUpdateProcessor)

nsUpdateProcessor::nsUpdateProcessor()
  : mUpdaterPID(0)
{
}

nsUpdateProcessor::~nsUpdateProcessor()
{
}

NS_IMETHODIMP
nsUpdateProcessor::ProcessUpdate(nsIUpdate* aUpdate)
{
  nsCOMPtr<nsIFile> greDir, appDir, updRoot;
  nsAutoCString appVersion;
  int argc;
  char **argv;

  nsAutoCString binPath;
  nsXREDirProvider* dirProvider = nsXREDirProvider::GetSingleton();
  if (dirProvider) { 
    
    bool persistent;
    nsresult rv = NS_ERROR_FAILURE; 
#ifdef MOZ_WIDGET_GONK
    
    
    rv = dirProvider->GetFile(XRE_UPDATE_ARCHIVE_DIR, &persistent,
                              getter_AddRefs(updRoot));
#endif
    if (NS_FAILED(rv)) {
      rv = dirProvider->GetFile(XRE_UPDATE_ROOT_DIR, &persistent,
                                getter_AddRefs(updRoot));
    }
    
    if (NS_FAILED(rv))
      updRoot = dirProvider->GetAppDir();

    greDir = dirProvider->GetGREDir();
    nsCOMPtr<nsIFile> exeFile;
    rv = dirProvider->GetFile(XRE_EXECUTABLE_FILE, &persistent,
                              getter_AddRefs(exeFile));
    if (NS_SUCCEEDED(rv))
      rv = exeFile->GetParent(getter_AddRefs(appDir));

    if (NS_FAILED(rv))
      appDir = dirProvider->GetAppDir();

    appVersion = gAppData->version;
    argc = gRestartArgc;
    argv = gRestartArgv;
  } else {
    
    
    
    
    nsCOMPtr<nsIProperties> ds =
      do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
    if (!ds) {
      NS_ABORT(); 
    }

    nsresult rv = ds->Get(NS_GRE_DIR, NS_GET_IID(nsIFile),
                          getter_AddRefs(greDir));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Can't get the GRE dir");

    nsCOMPtr<nsIFile> exeFile;
    rv = ds->Get(XRE_EXECUTABLE_FILE, NS_GET_IID(nsIFile),
                 getter_AddRefs(exeFile));
    if (NS_SUCCEEDED(rv))
      rv = exeFile->GetParent(getter_AddRefs(appDir));

    NS_ASSERTION(NS_SUCCEEDED(rv), "Can't get the XREExeF parent dir");

    rv = ds->Get(XRE_UPDATE_ROOT_DIR, NS_GET_IID(nsIFile),
                 getter_AddRefs(updRoot));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Can't get the UpdRootD dir");

    nsCOMPtr<nsIXULAppInfo> appInfo =
      do_GetService("@mozilla.org/xre/app-info;1");
    if (appInfo) {
      rv = appInfo->GetVersion(appVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      appVersion = MOZ_APP_VERSION;
    }

    
    
    
    argc = 1;
    nsCOMPtr<nsIFile> binary;
    rv = ds->Get(XRE_EXECUTABLE_FILE, NS_GET_IID(nsIFile),
                 getter_AddRefs(binary));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Can't get the binary path");
    binary->GetNativePath(binPath);
  }

  
  
  mInfo.mGREDir = greDir;
  mInfo.mAppDir = appDir;
  mInfo.mUpdateRoot = updRoot;
  mInfo.mArgc = argc;
  mInfo.mArgv = new char*[argc];
  if (dirProvider) {
    for (int i = 0; i < argc; ++i) {
      const size_t length = strlen(argv[i]);
      mInfo.mArgv[i] = new char[length + 1];
      strcpy(mInfo.mArgv[i], argv[i]);
    }
  } else {
    MOZ_ASSERT(argc == 1); 
    const size_t length = binPath.Length();
    mInfo.mArgv[0] = new char[length + 1];
    strcpy(mInfo.mArgv[0], binPath.get());
  }
  mInfo.mAppVersion = appVersion;

#if defined(MOZ_WIDGET_GONK)
  NS_ENSURE_ARG_POINTER(aUpdate);

  bool isOSUpdate;
  if (NS_SUCCEEDED(aUpdate->GetIsOSUpdate(&isOSUpdate)) &&
      isOSUpdate) {
    nsAutoCString osApplyToDir;

    
    
    nsresult rv = GetOSApplyToDir(osApplyToDir);
    if (NS_FAILED(rv)) {
      LOG(("Can't get the OS apply to dir"));
      return rv;
    }

    SetOSApplyToDir(aUpdate, osApplyToDir);

    mInfo.mIsOSUpdate = true;
    rv = NS_NewNativeLocalFile(osApplyToDir, false,
                               getter_AddRefs(mInfo.mOSApplyToDir));
    if (NS_FAILED(rv)) {
      LOG(("Can't create nsIFile for OS apply to dir"));
      return rv;
    }
  }
#endif

  MOZ_ASSERT(NS_IsMainThread(), "not main thread");
  return NS_NewThread(getter_AddRefs(mProcessWatcher),
                      NS_NewRunnableMethod(this, &nsUpdateProcessor::StartStagedUpdate));
}



void
nsUpdateProcessor::StartStagedUpdate()
{
  MOZ_ASSERT(!NS_IsMainThread(), "main thread");

  nsresult rv = ProcessUpdates(mInfo.mGREDir,
                               mInfo.mAppDir,
                               mInfo.mUpdateRoot,
                               mInfo.mArgc,
                               mInfo.mArgv,
                               mInfo.mAppVersion.get(),
                               false,
                               mInfo.mIsOSUpdate,
                               mInfo.mOSApplyToDir,
                               &mUpdaterPID);
  NS_ENSURE_SUCCESS_VOID(rv);

  if (mUpdaterPID) {
    
    rv = NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, &nsUpdateProcessor::WaitForProcess));
    NS_ENSURE_SUCCESS_VOID(rv);
  } else {
    
    
    
    rv = NS_DispatchToMainThread(NS_NewRunnableMethod(this, &nsUpdateProcessor::ShutdownWatcherThread));
    NS_ENSURE_SUCCESS_VOID(rv);
  }
}

void
nsUpdateProcessor::ShutdownWatcherThread()
{
  MOZ_ASSERT(NS_IsMainThread(), "not main thread");
  mProcessWatcher->Shutdown();
  mProcessWatcher = nullptr;
}

void
nsUpdateProcessor::WaitForProcess()
{
  MOZ_ASSERT(!NS_IsMainThread(), "main thread");
  ::WaitForProcess(mUpdaterPID);
  NS_DispatchToMainThread(NS_NewRunnableMethod(this, &nsUpdateProcessor::UpdateDone));
}

void
nsUpdateProcessor::UpdateDone()
{
  MOZ_ASSERT(NS_IsMainThread(), "not main thread");

  nsCOMPtr<nsIUpdateManager> um =
    do_GetService("@mozilla.org/updates/update-manager;1");
  if (um) {
    um->RefreshUpdateStatus();
  }

  ShutdownWatcherThread();
}

