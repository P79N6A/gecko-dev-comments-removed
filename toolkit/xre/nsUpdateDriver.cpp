








































#include <stdlib.h>
#include <stdio.h>
#include "nsUpdateDriver.h"
#include "nsXULAppAPI.h"
#include "nsAppRunner.h"
#include "nsILocalFile.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "prproces.h"
#include "prlog.h"
#include "prenv.h"
#include "nsVersionComparator.h"

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
#include "MacLaunchHelper.h"
#endif

#if defined(XP_WIN)
# include <direct.h>
# include <process.h>
# include <windows.h>
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
GetXULRunnerStubPath(const char* argv0, nsILocalFile* *aResult)
{
  
  CFBundleRef appBundle = ::CFBundleGetMainBundle();
  if (!appBundle)
    return NS_ERROR_FAILURE;

  CFURLRef bundleURL = ::CFBundleCopyExecutableURL(appBundle);
  if (!bundleURL)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFileMac> lfm;
  nsresult rv = NS_NewLocalFileWithCFURL(bundleURL, PR_TRUE, getter_AddRefs(lfm));

  ::CFRelease(bundleURL);

  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*aResult = static_cast<nsILocalFile*>(lfm.get()));
  return NS_OK;
}
#endif 

static PRBool
GetFile(nsIFile *dir, const nsCSubstring &name, nsCOMPtr<nsILocalFile> &result)
{
  nsresult rv;
  
  nsCOMPtr<nsIFile> file;
  rv = dir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return PR_FALSE;

  rv = file->AppendNative(name);
  if (NS_FAILED(rv))
    return PR_FALSE;

  result = do_QueryInterface(file, &rv);
  return NS_SUCCEEDED(rv);
}

static PRBool
GetStatusFile(nsIFile *dir, nsCOMPtr<nsILocalFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("update.status"), result);
}

static PRBool
IsPending(nsILocalFile *statusFile)
{
  PRFileDesc *fd = nsnull;
  nsresult rv = statusFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return PR_FALSE;

  char buf[32];
  const PRInt32 n = PR_Read(fd, buf, sizeof(buf));
  PR_Close(fd);

  if (n < 0)
    return PR_FALSE;
  
  const char kPending[] = "pending";
  return (strncmp(buf, kPending, sizeof(kPending) - 1) == 0);
}

static PRBool
SetStatusApplying(nsILocalFile *statusFile)
{
  PRFileDesc *fd = nsnull;
  nsresult rv = statusFile->OpenNSPRFileDesc(PR_WRONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return PR_FALSE;

  static const char kApplying[] = "Applying\n";
  PR_Write(fd, kApplying, sizeof(kApplying) - 1);
  PR_Close(fd);

  return PR_TRUE;
}

static PRBool
GetVersionFile(nsIFile *dir, nsCOMPtr<nsILocalFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("update.version"), result);
}

static PRBool
GetChannelChangeFile(nsIFile *dir, nsCOMPtr<nsILocalFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("channelchange"), result);
}



static PRBool
IsOlderVersion(nsILocalFile *versionFile, const char *&appVersion)
{
  PRFileDesc *fd = nsnull;
  nsresult rv = versionFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return PR_TRUE;

  char buf[32];
  const PRInt32 n = PR_Read(fd, buf, sizeof(buf));
  PR_Close(fd);

  if (n < 0)
    return PR_FALSE;

  
  if (buf[n - 1] == '\n')
    buf[n - 1] = '\0';

  
  
  const char kNull[] = "null";
  if (strncmp(buf, kNull, sizeof(kNull) - 1) == 0)
    return PR_FALSE;

  if (NS_CompareVersions(appVersion, buf) > 0)
    return PR_TRUE;

  return PR_FALSE;
}

static PRBool
CopyFileIntoUpdateDir(nsIFile *parentDir, const char *leafName, nsIFile *updateDir)
{
  nsDependentCString leaf(leafName);
  nsCOMPtr<nsIFile> file;

  
  nsresult rv = updateDir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return PR_FALSE;
  rv = file->AppendNative(leaf);
  if (NS_FAILED(rv))
    return PR_FALSE;
  file->Remove(PR_FALSE);

  
  rv = parentDir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return PR_FALSE;
  rv = file->AppendNative(leaf);
  if (NS_FAILED(rv))
    return PR_FALSE;
  rv = file->CopyToNative(updateDir, EmptyCString());
  if (NS_FAILED(rv))
    return PR_FALSE;

  return PR_TRUE;
}

static PRBool
CopyUpdaterIntoUpdateDir(nsIFile *greDir, nsIFile *appDir, nsIFile *updateDir,
                         nsCOMPtr<nsIFile> &updater)
{
  
#if defined(XP_MACOSX)
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterApp, updateDir))
    return PR_FALSE;
#else
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterBin, updateDir))
    return PR_FALSE;
#endif
  CopyFileIntoUpdateDir(appDir, kUpdaterINI, updateDir);
#if defined(XP_UNIX) && !defined(XP_MACOSX)
  nsCOMPtr<nsIFile> iconDir;
  appDir->Clone(getter_AddRefs(iconDir));
  iconDir->AppendNative(NS_LITERAL_CSTRING("icons"));
  if (!CopyFileIntoUpdateDir(iconDir, kUpdaterPNG, updateDir))
    return PR_FALSE;
#endif
  
  nsresult rv = updateDir->Clone(getter_AddRefs(updater));
  if (NS_FAILED(rv))
    return PR_FALSE;
#if defined(XP_MACOSX)
  rv  = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterApp));
  rv |= updater->AppendNative(NS_LITERAL_CSTRING("Contents"));
  rv |= updater->AppendNative(NS_LITERAL_CSTRING("MacOS"));
  if (NS_FAILED(rv))
    return PR_FALSE;
#endif
  rv = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterBin));
  return NS_SUCCEEDED(rv); 
}

static void
ApplyUpdate(nsIFile *greDir, nsIFile *updateDir, nsILocalFile *statusFile,
            nsIFile *appDir, int appArgc, char **appArgv)
{
  nsresult rv;

  
  
  
  

  nsCOMPtr<nsIFile> updater;
  if (!CopyUpdaterIntoUpdateDir(greDir, appDir, updateDir, updater)) {
    LOG(("failed copying updater\n"));
    return;
  }

  
  
  nsCOMPtr<nsILocalFile> appFile;

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
    rv = parentDir2->GetNativePath(applyToDir);
  }
#elif defined(XP_WIN)
  nsAutoString applyToDirW;
  rv = appDir->GetPath(applyToDirW);

  NS_ConvertUTF16toUTF8 applyToDir(applyToDirW);
#else
  nsCAutoString applyToDir;
  rv = appDir->GetNativePath(applyToDir);
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

  if (!SetStatusApplying(statusFile)) {
    LOG(("failed setting status to 'applying'\n"));
    return;
  }

  
  
#if defined(USE_EXECV)
  NS_NAMED_LITERAL_CSTRING(pid, "0");
#else
  nsCAutoString pid;
  pid.AppendInt((PRInt32) getpid());
#endif

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

  LOG(("spawning updater process [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
  execv(updaterPath.get(), argv);
#elif defined(XP_WIN)
  if (!WinLaunchChild(updaterPathW.get(), argc, argv))
    return;
  _exit(0);
#elif defined(XP_MACOSX)
  CommandLineServiceMac::SetupMacCommandLine(argc, argv, PR_TRUE);
  
  
  
  LaunchChildMac(argc, argv);
  exit(0);
#else
  PR_CreateProcessDetached(updaterPath.get(), argv, NULL, NULL);
  exit(0);
#endif
}

nsresult
ProcessUpdates(nsIFile *greDir, nsIFile *appDir, nsIFile *updRootDir,
               int argc, char **argv, const char *&appVersion)
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

  nsCOMPtr<nsILocalFile> statusFile;
  if (GetStatusFile(updatesDir, statusFile) && IsPending(statusFile)) {
    nsCOMPtr<nsILocalFile> versionFile;
    nsCOMPtr<nsILocalFile> channelChangeFile;
    
    
    
    if (!GetChannelChangeFile(updatesDir, channelChangeFile) &&
        (!GetVersionFile(updatesDir, versionFile) ||
         IsOlderVersion(versionFile, appVersion))) {
      updatesDir->Remove(PR_TRUE);
    } else {
      ApplyUpdate(greDir, updatesDir, statusFile, appDir, argc, argv);
    }
  }

  return NS_OK;
}
