





#include "nsXULAppAPI.h"
#include "application.ini.h"
#include "nsXPCOMGlue.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "BinaryPath.h"
#include "nsAutoPtr.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <dlfcn.h>

#include "nsXPCOMPrivate.h" 

#define ASSERT(x) if (!(x)) { MOZ_CRASH(); }


XRE_ProcLoaderServiceRunType XRE_ProcLoaderServiceRun;
XRE_ProcLoaderClientInitType XRE_ProcLoaderClientInit;
XRE_ProcLoaderPreloadType XRE_ProcLoaderPreload;
extern XRE_CreateAppDataType XRE_CreateAppData;
extern XRE_GetFileFromPathType XRE_GetFileFromPath;

static const nsDynamicFunctionLoad kXULFuncs[] = {
  { "XRE_ProcLoaderServiceRun", (NSFuncPtr*) &XRE_ProcLoaderServiceRun },
  { "XRE_ProcLoaderClientInit", (NSFuncPtr*) &XRE_ProcLoaderClientInit },
  { "XRE_ProcLoaderPreload", (NSFuncPtr*) &XRE_ProcLoaderPreload },
  { "XRE_CreateAppData", (NSFuncPtr*) &XRE_CreateAppData },
  { "XRE_GetFileFromPath", (NSFuncPtr*) &XRE_GetFileFromPath },
  { nullptr, nullptr }
};

typedef mozilla::Vector<int> FdArray;
static const int kReservedFileDescriptors = 5;
static const int kBeginReserveFileDescriptor = STDERR_FILENO + 1;

static int
GetDirnameSlash(const char *aPath, char *aOutDir, int aMaxLen)
{
  char *lastSlash = strrchr(aPath, XPCOM_FILE_PATH_SEPARATOR[0]);
  if (lastSlash == nullptr) {
    return 0;
  }
  int cpsz = lastSlash - aPath + 1; 
  if (aMaxLen <= cpsz) {
    return 0;
  }
  strncpy(aOutDir, aPath, cpsz);
  aOutDir[cpsz] = 0;
  return cpsz;
}

static bool
GetXPCOMPath(const char *aProgram, char *aOutPath, int aMaxLen)
{
  nsAutoArrayPtr<char> progBuf(new char[aMaxLen]);
  nsresult rv = mozilla::BinaryPath::Get(aProgram, progBuf);
  NS_ENSURE_SUCCESS(rv, false);

  int len = GetDirnameSlash(progBuf, aOutPath, aMaxLen);
  NS_ENSURE_TRUE(!!len, false);

  NS_ENSURE_TRUE((len + sizeof(XPCOM_DLL)) < (unsigned)aMaxLen, false);
  char *afterSlash = aOutPath + len;
  strcpy(afterSlash, XPCOM_DLL);
  return true;
}

static bool
LoadLibxul(const char *aXPCOMPath)
{
  nsresult rv;

  XPCOMGlueEnablePreload();
  rv = XPCOMGlueStartup(aXPCOMPath);
  NS_ENSURE_SUCCESS(rv, false);

  rv = XPCOMGlueLoadXULFunctions(kXULFuncs);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}




static bool
IsArg(const char* arg, const char* s)
{
  if (*arg == '-') {
    if (*++arg == '-') {
      ++arg;
    }
    return !strcasecmp(arg, s);
  }

#if defined(XP_WIN)
  if (*arg == '/') {
    return !strcasecmp(++arg, s);
  }
#endif

  return false;
}

static already_AddRefed<nsIFile>
GetAppIni(int argc, const char *argv[])
{
  nsCOMPtr<nsIFile> appini;
  nsresult rv;

  
  
  const char *appDataFile = getenv("XUL_APP_FILE");
  if (appDataFile && *appDataFile) {
    rv = XRE_GetFileFromPath(appDataFile, getter_AddRefs(appini));
    NS_ENSURE_SUCCESS(rv, nullptr);
  } else if (argc > 1 && IsArg(argv[1], "app")) {
    if (argc == 2) {
      return nullptr;
    }

    rv = XRE_GetFileFromPath(argv[2], getter_AddRefs(appini));
    NS_ENSURE_SUCCESS(rv, nullptr);

    char appEnv[MAXPATHLEN];
    snprintf(appEnv, MAXPATHLEN, "XUL_APP_FILE=%s", argv[2]);
    if (putenv(appEnv)) {
      return nullptr;
    }
  }

  return appini.forget();
}

static bool
LoadStaticData(int argc, const char *argv[])
{
  char xpcomPath[MAXPATHLEN];
  bool ok = GetXPCOMPath(argv[0], xpcomPath, MAXPATHLEN);
  NS_ENSURE_TRUE(ok, false);

  ok = LoadLibxul(xpcomPath);
  NS_ENSURE_TRUE(ok, false);

  char progDir[MAXPATHLEN];
  ok = GetDirnameSlash(xpcomPath, progDir, MAXPATHLEN);
  NS_ENSURE_TRUE(ok, false);

  nsCOMPtr<nsIFile> appini = GetAppIni(argc, argv);
  const nsXREAppData *appData;
  if (appini) {
    nsresult rv =
      XRE_CreateAppData(appini, const_cast<nsXREAppData**>(&appData));
    NS_ENSURE_SUCCESS(rv, false);
  } else {
    appData = &sAppData;
  }

  XRE_ProcLoaderPreload(progDir, appData);

  if (appini) {
    XRE_FreeAppData(const_cast<nsXREAppData*>(appData));
  }

  return true;
}






static int
RunProcesses(int argc, const char *argv[], FdArray& aReservedFds)
{
  



  int b2g_main(int argc, const char *argv[]);

  int ipcSockets[2] = {-1, -1};
  int r = socketpair(AF_LOCAL, SOCK_STREAM, 0, ipcSockets);
  ASSERT(r == 0);
  int parentSock = ipcSockets[0];
  int childSock = ipcSockets[1];

  r = fcntl(parentSock, F_SETFL, O_NONBLOCK);
  ASSERT(r != -1);
  r = fcntl(childSock, F_SETFL, O_NONBLOCK);
  ASSERT(r != -1);

  pid_t pid = fork();
  ASSERT(pid >= 0);
  bool isChildProcess = pid == 0;

  close(isChildProcess ? parentSock : childSock);

  if (isChildProcess) {
    
    



    return XRE_ProcLoaderServiceRun(getppid(), childSock, argc, argv,
                                    aReservedFds);
  }

  
  int childPid = pid;
  XRE_ProcLoaderClientInit(childPid, parentSock, aReservedFds);
  return b2g_main(argc, argv);
}





static void
ReserveFileDescriptors(FdArray& aReservedFds)
{
  for (int i = 0; i < kReservedFileDescriptors; i++) {
    struct stat fileState;
    int target = kBeginReserveFileDescriptor + i;
    if (fstat(target, &fileState) == 0) {
      MOZ_CRASH("ProcLoader error: a magic file descriptor is occupied.");
    }

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
      MOZ_CRASH("ProcLoader error: failed to reserve a magic file descriptor.");
    }

    aReservedFds.append(target);

    if (fd == target) {
      
      continue;
    }

    if (dup2(fd, target)) {
      MOZ_CRASH("ProcLoader error: failed to reserve a magic file descriptor.");
    }

    close(fd);
  }
}











int
main(int argc, const char* argv[])
{
  


  FdArray reservedFds;
  ReserveFileDescriptors(reservedFds);

  



  bool ok = LoadStaticData(argc, argv);
  if (!ok) {
    return 255;
  }

  return RunProcesses(argc, argv, reservedFds);
}
