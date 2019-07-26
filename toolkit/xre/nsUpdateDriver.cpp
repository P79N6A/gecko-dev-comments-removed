





#include <stdlib.h>
#include <stdio.h>
#include "nsUpdateDriver.h"
#include "nsXULAppAPI.h"
#include "nsAppRunner.h"
#include "nsIFile.h"
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
# define getcwd(path, size) _getcwd(path, size)
# define getpid() GetCurrentProcessId()
#elif defined(XP_OS2)
# include <unistd.h>
# define INCL_DOSFILEMGR
# include <os2.h>
#elif defined(XP_UNIX)
# include <unistd.h>
#endif





















#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define USE_EXECV
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo *sUpdateLog = PR_NewLogModule("updatedriver");
#endif
#define LOG(args) PR_LOG(sUpdateLog, PR_LOG_DEBUG, args)

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

static nsresult
GetCurrentWorkingDir(char *buf, size_t size)
{
  
  

#if defined(XP_OS2)
  if (DosQueryPathInfo( ".", FIL_QUERYFULLNAME, buf, size))
    return NS_ERROR_FAILURE;
#elif defined(XP_WIN)
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

  NS_ADDREF(*aResult = static_cast<nsIFile*>(lfm.get()));
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

  PRFileDesc *fd = nsnull;
  nsresult rv = statusFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return false;

  const PRInt32 n = PR_Read(fd, buf, Size);
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
  PRFileDesc *fd = nsnull;
  nsresult rv = versionFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return true;

  char buf[32];
  const PRInt32 n = PR_Read(fd, buf, sizeof(buf));
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
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterApp, updateDir))
    return false;
#else
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterBin, updateDir))
    return false;
#endif
  CopyFileIntoUpdateDir(appDir, kUpdaterINI, updateDir);
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
  rv |= updater->AppendNative(NS_LITERAL_CSTRING("Contents"));
  rv |= updater->AppendNative(NS_LITERAL_CSTRING("MacOS"));
  if (NS_FAILED(rv))
    return false;
#endif
  rv = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterBin));
  return NS_SUCCEEDED(rv); 
}












static void
SwitchToUpdatedApp(nsIFile *greDir, nsIFile *updateDir, nsIFile *statusFile,
                   nsIFile *appDir, int appArgc, char **appArgv)
{
  nsresult rv;

  
  
  

  nsCOMPtr<nsIFile> tmpDir;
  GetSpecialSystemDirectory(OS_TemporaryDirectory,
                            getter_AddRefs(tmpDir));
  if (!tmpDir) {
    LOG(("failed getting a temp dir\n"));
    return;
  }

  
  
  
  
  tmpDir->Append(NS_LITERAL_STRING("MozUpdater"));
  tmpDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0755);

  nsCOMPtr<nsIFile> updater;
  if (!CopyUpdaterIntoUpdateDir(greDir, appDir, tmpDir, updater)) {
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
  nsCAutoString appFilePath;
  rv = appFile->GetNativePath(appFilePath);
  if (NS_FAILED(rv))
    return;

  nsCAutoString updaterPath;
  rv = updater->GetNativePath(updaterPath);
  if (NS_FAILED(rv))
    return;

#endif

  
  
  
  
  nsCOMPtr<nsIFile> updatedDir;
#if defined(XP_MACOSX)
  nsCAutoString applyToDir;
  {
    nsCOMPtr<nsIFile> parentDir1, parentDir2;
    rv = appDir->GetParent(getter_AddRefs(parentDir1));
    if (NS_FAILED(rv))
      return;
    rv = parentDir1->GetParent(getter_AddRefs(parentDir2));
    if (NS_FAILED(rv))
      return;
    if (!GetFile(parentDir2, NS_LITERAL_CSTRING("Updated.app"), updatedDir))
      return;
    rv = updatedDir->GetNativePath(applyToDir);
  }
#else
  if (!GetFile(appDir, NS_LITERAL_CSTRING("updated"), updatedDir))
    return;
#if defined(XP_WIN)
  nsAutoString applyToDirW;
  rv = updatedDir->GetPath(applyToDirW);

  NS_ConvertUTF16toUTF8 applyToDir(applyToDirW);
#else
  nsCAutoString applyToDir;
  rv = updatedDir->GetNativePath(applyToDir);
#endif
#endif
  if (NS_FAILED(rv))
    return;

  
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
  nsCAutoString updateDirPath;
  rv = updateDir->GetNativePath(updateDirPath);
#endif

  if (NS_FAILED(rv))
    return;

  
  char workingDirPath[MAXPATHLEN];
  rv = GetCurrentWorkingDir(workingDirPath, sizeof(workingDirPath));
  if (NS_FAILED(rv))
    return;

  
  
#if defined(USE_EXECV)
  nsCAutoString pid("0");
#else
  nsCAutoString pid;
  pid.AppendInt((PRInt32) getpid());
#endif

  
  
  pid.AppendASCII("/replace");

  int argc = appArgc + 5;
  char **argv = new char*[argc + 1];
  if (!argv)
    return;
  argv[0] = (char*) updaterPath.get();
  argv[1] = (char*) updateDirPath.get();
  argv[2] = (char*) applyToDir.get();
  argv[3] = (char*) pid.get();
  if (appArgc) {
    argv[4] = workingDirPath;
    argv[5] = (char*) appFilePath.get();
    for (int i = 1; i < appArgc; ++i)
      argv[5 + i] = appArgv[i];
    argc = 5 + appArgc;
    argv[argc] = NULL;
  } else {
    argc = 4;
    argv[4] = NULL;
  }

  if (gSafeMode) {
    PR_SetEnv("MOZ_SAFE_MODE_RESTART=1");
  }

  LOG(("spawning updater process for replacing [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
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
  PR_CreateProcessDetached(updaterPath.get(), argv, NULL, NULL);
  exit(0);
#endif
}
















static void
ApplyUpdate(nsIFile *greDir, nsIFile *updateDir, nsIFile *statusFile,
            nsIFile *appDir, int appArgc, char **appArgv, bool restart,
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
  nsCAutoString appFilePath;
  rv = appFile->GetNativePath(appFilePath);
  if (NS_FAILED(rv))
    return;
  
  nsCAutoString updaterPath;
  rv = updater->GetNativePath(updaterPath);
  if (NS_FAILED(rv))
    return;

#endif

  
  
  
  
  nsCOMPtr<nsIFile> updatedDir;
#if defined(XP_MACOSX)
  nsCAutoString applyToDir;
  {
    nsCOMPtr<nsIFile> parentDir1, parentDir2;
    rv = appDir->GetParent(getter_AddRefs(parentDir1));
    if (NS_FAILED(rv))
      return;
    rv = parentDir1->GetParent(getter_AddRefs(parentDir2));
    if (NS_FAILED(rv))
      return;
    if (restart) {
      
      
      rv = parentDir2->GetNativePath(applyToDir);
    } else {
      if (!GetFile(parentDir2, NS_LITERAL_CSTRING("Updated.app"), updatedDir))
        return;
      rv = updatedDir->GetNativePath(applyToDir);
    }
  }
#else
  if (restart) {
    
    
    updatedDir = do_QueryInterface(appDir);
  } else if (!GetFile(appDir, NS_LITERAL_CSTRING("updated"), updatedDir)) {
    return;
  }
#if defined(XP_WIN)
  nsAutoString applyToDirW;
  rv = updatedDir->GetPath(applyToDirW);

  NS_ConvertUTF16toUTF8 applyToDir(applyToDirW);
#else
  nsCAutoString applyToDir;
  rv = updatedDir->GetNativePath(applyToDir);
#endif
#endif
  if (NS_FAILED(rv))
    return;

#if defined(XP_WIN)
  nsAutoString updateDirPathW;
  rv = updateDir->GetPath(updateDirPathW);

  NS_ConvertUTF16toUTF8 updateDirPath(updateDirPathW);
#else
  nsCAutoString updateDirPath;
  rv = updateDir->GetNativePath(updateDirPath);
#endif

  if (NS_FAILED(rv))
    return;

  
  char workingDirPath[MAXPATHLEN];
  rv = GetCurrentWorkingDir(workingDirPath, sizeof(workingDirPath));
  if (NS_FAILED(rv))
    return;

  
  
  
  
  

  
  
  nsCAutoString pid;
  if (!restart) {
    
    
    pid.AssignASCII("-1");
  } else {
#if defined(USE_EXECV)
    pid.AssignASCII("0");
#else
    pid.AppendInt((PRInt32) getpid());
#endif
  }

  int argc = appArgc + 5;
  char **argv = new char*[argc + 1];
  if (!argv)
    return;
  argv[0] = (char*) updaterPath.get();
  argv[1] = (char*) updateDirPath.get();
  argv[2] = (char*) applyToDir.get();
  argv[3] = (char*) pid.get();
  if (restart && appArgc) {
    argv[4] = workingDirPath;
    argv[5] = (char*) appFilePath.get();
    for (int i = 1; i < appArgc; ++i)
      argv[5 + i] = appArgv[i];
    argc = 5 + appArgc;
    argv[argc] = NULL;
  } else {
    argc = 4;
    argv[4] = NULL;
  }

  if (gSafeMode) {
    PR_SetEnv("MOZ_SAFE_MODE_RESTART=1");
  }

  LOG(("spawning updater process [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
  
  if (restart) {
    execv(updaterPath.get(), argv);
  } else {
    *outpid = PR_CreateProcess(updaterPath.get(), argv, NULL, NULL);
  }
#elif defined(XP_WIN)
  
  if (!WinLaunchChild(updaterPathW.get(), argc, argv, NULL, outpid)) {
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
  *outpid = PR_CreateProcess(updaterPath.get(), argv, NULL, NULL);
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
  PRInt32 exitCode;
  PR_WaitProcess(pt, &exitCode);
#endif
}

nsresult
ProcessUpdates(nsIFile *greDir, nsIFile *appDir, nsIFile *updRootDir,
               int argc, char **argv, const char *appVersion,
               bool restart, ProcessType *pid)
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
    
    const char *updRootOverride = PR_GetEnv("MOZ_UPDATE_ROOT_OVERRIDE");
    if (updRootOverride && *updRootOverride) {
      nsCOMPtr<nsIFile> overrideDir;
      nsCAutoString path(updRootOverride);
      rv = NS_NewNativeLocalFile(path, false, getter_AddRefs(overrideDir));
      if (NS_FAILED(rv)) {
        return rv;
      }
      updatesDir = do_QueryInterface(overrideDir);
    }
    
    const char *appDirOverride = PR_GetEnv("MOZ_UPDATE_APPDIR_OVERRIDE");
    if (appDirOverride && *appDirOverride) {
      nsCOMPtr<nsIFile> overrideDir;
      nsCAutoString path(appDirOverride);
      rv = NS_NewNativeLocalFile(path, false, getter_AddRefs(overrideDir));
      if (NS_FAILED(rv)) {
        return rv;
      }
      NS_ADDREF(appDir = overrideDir);
    }
    
    const char *backgroundUpdate = PR_GetEnv("MOZ_UPDATE_BACKGROUND");
    if (backgroundUpdate && *backgroundUpdate) {
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
                  appDir, argc, argv, restart, pid);
    }
    break;
  }
  case eAppliedUpdate:
  case eAppliedService:
    
    
    SwitchToUpdatedApp(greDir, updatesDir, statusFile,
                       appDir, argc, argv);
    break;
  case eNoUpdateAction:
    
    
    break;
  }

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUpdateProcessor, nsIUpdateProcessor)

nsUpdateProcessor::nsUpdateProcessor()
  : mUpdaterPID(0)
{
}

NS_IMETHODIMP
nsUpdateProcessor::ProcessUpdate(nsIUpdate* aUpdate)
{
  nsCOMPtr<nsIFile> greDir, appDir, updRoot;
  nsCAutoString appVersion;
  int argc;
  char **argv;

  NS_ENSURE_ARG_POINTER(aUpdate);

  nsCAutoString binPath;
  nsXREDirProvider* dirProvider = nsXREDirProvider::GetSingleton();
  if (dirProvider) { 
    
    bool persistent;
    nsresult rv = dirProvider->GetFile(XRE_UPDATE_ROOT_DIR, &persistent,
                                       getter_AddRefs(updRoot));
    
    if (NS_FAILED(rv))
      updRoot = dirProvider->GetAppDir();

    greDir = dirProvider->GetGREDir();
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
    appDir = greDir;

    rv = ds->Get(XRE_UPDATE_ROOT_DIR, NS_GET_IID(nsIFile),
                 getter_AddRefs(updRoot));
    if (NS_FAILED(rv))
      updRoot = appDir;

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

  mUpdate = aUpdate;

  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
  return NS_NewThread(getter_AddRefs(mProcessWatcher),
                      NS_NewRunnableMethod(this, &nsUpdateProcessor::StartBackgroundUpdate));
}

void
nsUpdateProcessor::StartBackgroundUpdate()
{
  NS_ABORT_IF_FALSE(!NS_IsMainThread(), "main thread");

  nsresult rv = ProcessUpdates(mInfo.mGREDir,
                               mInfo.mAppDir,
                               mInfo.mUpdateRoot,
                               mInfo.mArgc,
                               mInfo.mArgv,
                               mInfo.mAppVersion.get(),
                               false,
                               &mUpdaterPID);
  NS_ENSURE_SUCCESS(rv, );

  if (mUpdaterPID) {
    
    rv = NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, &nsUpdateProcessor::WaitForProcess));
    NS_ENSURE_SUCCESS(rv, );
  } else {
    
    
    
    rv = NS_DispatchToMainThread(NS_NewRunnableMethod(this, &nsUpdateProcessor::ShutdownWatcherThread));
    NS_ENSURE_SUCCESS(rv, );
  }
}

void
nsUpdateProcessor::ShutdownWatcherThread()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");
  mProcessWatcher->Shutdown();
  mProcessWatcher = nsnull;
  mUpdate = nsnull;
}

void
nsUpdateProcessor::WaitForProcess()
{
  NS_ABORT_IF_FALSE(!NS_IsMainThread(), "main thread");
  ::WaitForProcess(mUpdaterPID);
  NS_DispatchToMainThread(NS_NewRunnableMethod(this, &nsUpdateProcessor::UpdateDone));
}

void
nsUpdateProcessor::UpdateDone()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "not main thread");

  nsCOMPtr<nsIUpdateManager> um =
    do_GetService("@mozilla.org/updates/update-manager;1");
  if (um) {
    um->RefreshUpdateStatus(mUpdate);
  }

  ShutdownWatcherThread();
}

