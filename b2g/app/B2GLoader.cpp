





#include "nsXULAppAPI.h"
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

static const nsDynamicFunctionLoad kXULFuncs[] = {
  { "XRE_ProcLoaderServiceRun", (NSFuncPtr*) &XRE_ProcLoaderServiceRun },
  { "XRE_ProcLoaderClientInit", (NSFuncPtr*) &XRE_ProcLoaderClientInit },
  { nullptr, nullptr }
};

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

  NS_ENSURE_TRUE((len + sizeof(XPCOM_DLL)) < aMaxLen, false);
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
LoadStaticData(const char *aProgram)
{
  char xpcomPath[MAXPATHLEN];
  bool ok = GetXPCOMPath(aProgram, xpcomPath, MAXPATHLEN);
  NS_ENSURE_TRUE(ok, false);

  ok = LoadLibxul(xpcomPath);
  return ok;
}






static int
RunProcesses(int argc, const char *argv[])
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
    
    



    return XRE_ProcLoaderServiceRun(getppid(), childSock, argc, argv);
  }

  
  int childPid = pid;
  XRE_ProcLoaderClientInit(childPid, parentSock);
  return b2g_main(argc, argv);
}











int
main(int argc, const char* argv[])
{
  const char *program = argv[0];
  



  bool ok = LoadStaticData(program);
  if (!ok) {
    return 255;
  }

  return RunProcesses(argc, argv);
}
