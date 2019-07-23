






































#include <stdlib.h>
#include <stdio.h>
#include "nsUpdateDriver.h"
#include "nsXULAppAPI.h"
#include "nsAppRunner.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsIDirectoryEnumerator.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "prproces.h"
#include "prlog.h"

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
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
#elif defined(XP_UNIX) || defined(XP_BEOS)
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

static nsresult
GetCurrentWorkingDir(char *buf, size_t size)
{
  
  

#if defined(XP_OS2)
  if (DosQueryPathInfo( ".", FIL_QUERYFULLNAME, buf, size))
    return NS_ERROR_FAILURE;
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
  nsresult rv;
  nsCOMPtr<nsILocalFile> lf;

  NS_NewNativeLocalFile(EmptyCString(), PR_TRUE, getter_AddRefs(lf));
  nsCOMPtr<nsILocalFileMac> lfm (do_QueryInterface(lf));
  if (!lfm)
    return NS_ERROR_FAILURE;

  
  CFBundleRef appBundle = CFBundleGetMainBundle();
  if (!appBundle)
    return NS_ERROR_FAILURE;

  CFURLRef bundleURL = CFBundleCopyExecutableURL(appBundle);
  if (!bundleURL)
    return NS_ERROR_FAILURE;

  FSRef fileRef;
  if (!CFURLGetFSRef(bundleURL, &fileRef)) {
    CFRelease(bundleURL);
    return NS_ERROR_FAILURE;
  }

  rv = lfm->InitWithFSRef(&fileRef);
  CFRelease(bundleURL);

  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*aResult = lf);
  return NS_OK;
}
#endif 

PR_STATIC_CALLBACK(int)
ScanDirComparator(nsIFile *a, nsIFile *b, void *unused)
{
  
  nsCAutoString a_name, b_name;
  a->GetNativeLeafName(a_name);
  b->GetNativeLeafName(b_name);
  return Compare(a_name, b_name);
}

static nsresult
ScanDir(nsIFile *dir, nsCOMArray<nsIFile> *result)
{
  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> simpEnum;
  rv = dir->GetDirectoryEntries(getter_AddRefs(simpEnum));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDirectoryEnumerator> dirEnum = do_QueryInterface(simpEnum, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> file;
  for (;;) {
    rv = dirEnum->GetNextFile(getter_AddRefs(file));
    if (NS_FAILED(rv))
      return rv;

    
    if (!file)
      break;

    if (!result->AppendObject(file))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  result->Sort(ScanDirComparator, nsnull);
  return NS_OK;
}

static PRBool
GetFile(nsIFile *dir, const nsCSubstring &name, nsCOMPtr<nsILocalFile> &result)
{
  nsresult rv;
  
  nsCOMPtr<nsIFile> statusFile;
  rv = dir->Clone(getter_AddRefs(statusFile));
  if (NS_FAILED(rv))
    return PR_FALSE;

  rv = statusFile->AppendNative(name);
  if (NS_FAILED(rv))
    return PR_FALSE;

  result = do_QueryInterface(statusFile, &rv);
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
  nsresult rv;

  FILE *fp;
  rv = statusFile->OpenANSIFileDesc("r", &fp);
  if (NS_FAILED(rv))
    return PR_FALSE;

  char buf[32];
  char *result = fgets(buf, sizeof(buf), fp);
  fclose(fp);
  if (!result)
    return PR_FALSE;
  
  const char kPending[] = "pending";
  return (strncmp(buf, kPending, sizeof(kPending) - 1) == 0);
}

static PRBool
SetStatus(nsILocalFile *statusFile, const char *status)
{
  FILE *fp;
  nsresult rv = statusFile->OpenANSIFileDesc("w", &fp);
  if (NS_FAILED(rv))
    return PR_FALSE;

  fprintf(fp, "%s\n", status);
  fclose(fp);
  return PR_TRUE;
}

static PRBool
CopyUpdaterIntoUpdateDir(nsIFile *greDir, nsIFile *appDir, nsIFile *updateDir,
                         nsCOMPtr<nsIFile> &updater)
{
  
  const char *filesToMove[] = {
#if defined(XP_MACOSX)
    kUpdaterApp,
#else
    kUpdaterINI,
    kUpdaterBin,
#endif
    nsnull
  };

  nsresult rv;

  for (const char **leafName = filesToMove; *leafName; ++leafName) {
    nsDependentCString leaf(*leafName);
    nsCOMPtr<nsIFile> file;

    
    rv = updateDir->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))
      return PR_FALSE;
    rv = file->AppendNative(leaf);
    if (NS_FAILED(rv))
      return PR_FALSE;
    file->Remove(PR_FALSE);

    
    rv = greDir->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))
      return PR_FALSE;
    rv = file->AppendNative(leaf);
    if (NS_FAILED(rv))
      return PR_FALSE;
    rv = file->CopyToNative(updateDir, EmptyCString());
    if (*leafName != kUpdaterINI && NS_FAILED(rv))
      return PR_FALSE;
  }
  
  
  rv = updateDir->Clone(getter_AddRefs(updater));
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
  nsCAutoString appFilePath;
  nsresult rv = appFile->GetNativePath(appFilePath);
  if (NS_FAILED(rv))
    return;
  
  nsCAutoString updaterPath;
  rv = updater->GetNativePath(updaterPath);
  if (NS_FAILED(rv))
    return;

  
  
  
  nsCAutoString applyToDir;
#if defined(XP_MACOSX)
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
#else
  rv = appDir->GetNativePath(applyToDir);
#endif
  if (NS_FAILED(rv))
    return;

  nsCAutoString updateDirPath;
  rv = updateDir->GetNativePath(updateDirPath);
  if (NS_FAILED(rv))
    return;

  
  char workingDirPath[MAXPATHLEN];
  rv = GetCurrentWorkingDir(workingDirPath, sizeof(workingDirPath));
  if (NS_FAILED(rv))
    return;

  if (!SetStatus(statusFile, "applying")) {
    LOG(("failed setting status to 'applying'\n"));
    return;
  }

  
  
#if defined(USE_EXECV)
  NS_NAMED_LITERAL_CSTRING(pid, "0");
#else
  nsCAutoString pid;
  pid.AppendInt((PRInt32) getpid());
#endif

  int argc = appArgc + 4;
  char **argv = new char*[argc + 1];
  if (!argv)
    return;
  argv[0] = (char*) updaterPath.get();
  argv[1] = (char*) updateDirPath.get();
  argv[2] = (char*) pid.get();
  if (appArgc) {
    argv[3] = workingDirPath;
    argv[4] = (char*) appFilePath.get();
    for (int i = 1; i < appArgc; ++i)
      argv[4 + i] = appArgv[i];
    argv[4 + appArgc] = nsnull;
  } else {
    argv[3] = nsnull;
    argc = 3;
  }

  LOG(("spawning updater process [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
  chdir(applyToDir.get());
  execv(updaterPath.get(), argv);
#elif defined(XP_WIN)
  _chdir(applyToDir.get());

  if (!WinLaunchChild(updaterPath.get(), appArgc + 4, argv, 1))
    return;
  _exit(0);
#else
  PRStatus status;
  PRProcessAttr *attr;
  
  attr = PR_NewProcessAttr();
  if (!attr)
    goto end;

  status = PR_ProcessAttrSetCurrentDirectory(attr, applyToDir.get());
  if (status != PR_SUCCESS)
    goto end;

#ifdef XP_MACOSX
  SetupMacCommandLine(argc, argv);
#endif

  PR_CreateProcessDetached(updaterPath.get(), argv, nsnull, attr);
  exit(0);

end:
  PR_DestroyProcessAttr(attr); 
  delete[] argv;
#endif
}

nsresult
ProcessUpdates(nsIFile *greDir, nsIFile *appDir, nsIFile *updRootDir,
               int argc, char **argv)
{
  nsresult rv;

  nsCOMPtr<nsIFile> updatesDir;
  rv = updRootDir->Clone(getter_AddRefs(updatesDir));
  if (NS_FAILED(rv))
    return rv;
  rv = updatesDir->AppendNative(NS_LITERAL_CSTRING("updates"));
  if (NS_FAILED(rv))
    return rv;

  PRBool exists;
  rv = updatesDir->Exists(&exists);
  if (NS_FAILED(rv) || !exists)
    return rv;

  nsCOMArray<nsIFile> dirEntries;
  rv = ScanDir(updatesDir, &dirEntries);
  if (NS_FAILED(rv))
    return rv;
  if (dirEntries.Count() == 0)
    return NS_OK;

  
  for (int i = 0; i < dirEntries.Count(); ++i) {
    nsCOMPtr<nsILocalFile> statusFile;
    if (GetStatusFile(dirEntries[i], statusFile) && IsPending(statusFile)) {
      ApplyUpdate(greDir, dirEntries[i], statusFile, appDir, argc, argv);
      break;
    }
  }

  return NS_OK;
}
