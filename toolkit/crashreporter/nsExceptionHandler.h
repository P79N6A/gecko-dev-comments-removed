




































#ifndef nsExceptionHandler_h__
#define nsExceptionHandler_h__

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsStringGlue.h"

#include "nsIFile.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace CrashReporter {
nsresult SetExceptionHandler(nsILocalFile* aXREDirectory, bool force=false);
nsresult UnsetExceptionHandler();
bool     GetEnabled();
bool     GetServerURL(nsACString& aServerURL);
nsresult SetServerURL(const nsACString& aServerURL);
bool     GetMinidumpPath(nsAString& aPath);
nsresult SetMinidumpPath(const nsAString& aPath);
nsresult AnnotateCrashReport(const nsACString& key, const nsACString& data);
nsresult AppendAppNotesToCrashReport(const nsACString& data);
nsresult SetRestartArgs(int argc, char** argv);
nsresult SetupExtraData(nsILocalFile* aAppDataDirectory,
                        const nsACString& aBuildID);
#ifdef XP_WIN32
  nsresult WriteMinidumpForException(EXCEPTION_POINTERS* aExceptionInfo);
#endif
#ifdef XP_MACOSX
  nsresult AppendObjCExceptionInfoToAppNotes(void *inException);
#endif
nsresult GetSubmitReports(PRBool* aSubmitReport);
nsresult SetSubmitReports(PRBool aSubmitReport);

#ifdef MOZ_IPC


#if defined(XP_WIN32)
typedef HANDLE ProcessHandle;
#else
typedef int ProcessHandle;
#endif




bool TakeMinidumpForChild(ProcessHandle childPid, nsIFile** dump NS_OUTPARAM);

#  if defined(XP_WIN32)

const char* GetChildNotificationPipe();


bool SetRemoteExceptionHandler(const nsACString& crashPipe);

#  elif defined(XP_LINUX)










bool CreateNotificationPipeForChild(int* childCrashFd, int* childCrashRemapFd);


bool SetRemoteExceptionHandler();
#endif  

bool UnsetRemoteExceptionHandler();
#endif 
}

#endif 
