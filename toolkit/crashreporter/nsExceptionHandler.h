




































#ifndef nsExceptionHandler_h__
#define nsExceptionHandler_h__

#include "nscore.h"
#include "nsDataHashtable.h"
#include "nsXPCOM.h"
#include "nsStringGlue.h"

#include "nsIFile.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if defined(XP_MACOSX)
#include <mach/mach.h>
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


typedef nsDataHashtable<nsCStringHashKey, nsCString> AnnotationTable;

bool GetMinidumpForID(const nsAString& id, nsILocalFile** minidump);
bool GetIDFromMinidump(nsILocalFile* minidump, nsAString& id);
bool GetExtraFileForID(const nsAString& id, nsILocalFile** extraFile);
bool GetExtraFileForMinidump(nsILocalFile* minidump, nsILocalFile** extraFile);
bool AppendExtraData(const nsAString& id, const AnnotationTable& data);
bool AppendExtraData(nsILocalFile* extraFile, const AnnotationTable& data);

#ifdef XP_WIN32
  nsresult WriteMinidumpForException(EXCEPTION_POINTERS* aExceptionInfo);
#endif
#ifdef XP_MACOSX
  nsresult AppendObjCExceptionInfoToAppNotes(void *inException);
#endif
nsresult GetSubmitReports(PRBool* aSubmitReport);
nsresult SetSubmitReports(PRBool aSubmitReport);






bool TakeMinidumpForChild(PRUint32 childPid,
                          nsILocalFile** dump NS_OUTPARAM);

#if defined(XP_WIN)
typedef HANDLE ProcessHandle;
typedef DWORD ThreadId;
#elif defined(XP_MACOSX)
typedef task_t ProcessHandle;
typedef mach_port_t ThreadId;
#else
typedef int ProcessHandle;
typedef int ThreadId;
#endif







ThreadId CurrentThreadId();








bool CreatePairedMinidumps(ProcessHandle childPid,
                           ThreadId childBlamedThread,
                           nsAString* pairGUID NS_OUTPARAM,
                           nsILocalFile** childDump NS_OUTPARAM,
                           nsILocalFile** parentDump NS_OUTPARAM);

#  if defined(XP_WIN32) || defined(XP_MACOSX)

const char* GetChildNotificationPipe();


bool SetRemoteExceptionHandler(const nsACString& crashPipe);

#  elif defined(XP_LINUX)










bool CreateNotificationPipeForChild(int* childCrashFd, int* childCrashRemapFd);


bool SetRemoteExceptionHandler();

#endif  

bool UnsetRemoteExceptionHandler();

#if defined(__ANDROID__)





void AddLibraryMapping(const char* library_name,
                       const char* file_id,
                       uintptr_t   start_address,
                       size_t      mapping_length,
                       size_t      file_offset);

void AddLibraryMappingForChild(PRUint32    childPid,
                               const char* library_name,
                               const char* file_id,
                               uintptr_t   start_address,
                               size_t      mapping_length,
                               size_t      file_offset);
void RemoveLibraryMappingsForChild(PRUint32 childPid);
#endif
}

#endif 
