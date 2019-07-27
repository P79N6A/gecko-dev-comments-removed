




#ifndef nsExceptionHandler_h__
#define nsExceptionHandler_h__

#include <stddef.h>
#include <stdint.h>
#include "nsError.h"
#include "nsStringGlue.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if defined(XP_MACOSX)
#include <mach/mach.h>
#endif

#if defined(XP_LINUX)
#include <signal.h>
#endif

class nsIFile;
template<class KeyClass, class DataType> class nsDataHashtable;
class nsCStringHashKey;

namespace CrashReporter {
nsresult SetExceptionHandler(nsIFile* aXREDirectory, bool force=false);
nsresult UnsetExceptionHandler();














void SetUserAppDataDirectory(nsIFile* aDir);
void SetProfileDirectory(nsIFile* aDir);
void UpdateCrashEventsDir();
void SetMemoryReportFile(nsIFile* aFile);




bool     GetCrashEventsDir(nsAString& aPath);

bool     GetEnabled();
bool     GetServerURL(nsACString& aServerURL);
nsresult SetServerURL(const nsACString& aServerURL);
bool     GetMinidumpPath(nsAString& aPath);
nsresult SetMinidumpPath(const nsAString& aPath);





nsresult AnnotateCrashReport(const nsACString& key, const nsACString& data);
nsresult AppendAppNotesToCrashReport(const nsACString& data);

void AnnotateOOMAllocationSize(size_t size);
nsresult SetGarbageCollecting(bool collecting);
void SetEventloopNestingLevel(uint32_t level);

nsresult SetRestartArgs(int argc, char** argv);
nsresult SetupExtraData(nsIFile* aAppDataDirectory,
                        const nsACString& aBuildID);
bool GetLastRunCrashID(nsAString& id);


nsresult RegisterAppMemory(void* ptr, size_t length);
nsresult UnregisterAppMemory(void* ptr);


typedef nsDataHashtable<nsCStringHashKey, nsCString> AnnotationTable;

bool GetMinidumpForID(const nsAString& id, nsIFile** minidump);
bool GetIDFromMinidump(nsIFile* minidump, nsAString& id);
bool GetExtraFileForID(const nsAString& id, nsIFile** extraFile);
bool GetExtraFileForMinidump(nsIFile* minidump, nsIFile** extraFile);
bool AppendExtraData(const nsAString& id, const AnnotationTable& data);
bool AppendExtraData(nsIFile* extraFile, const AnnotationTable& data);
void RenameAdditionalHangMinidump(nsIFile* minidump, nsIFile* childMinidump,
                                  const nsACString& name);

#ifdef XP_WIN32
  nsresult WriteMinidumpForException(EXCEPTION_POINTERS* aExceptionInfo);
#endif
#ifdef XP_LINUX
  bool WriteMinidumpForSigInfo(int signo, siginfo_t* info, void* uc);
#endif
#ifdef XP_MACOSX
  nsresult AppendObjCExceptionInfoToAppNotes(void *inException);
#endif
nsresult GetSubmitReports(bool* aSubmitReport);
nsresult SetSubmitReports(bool aSubmitReport);







void OOPInit();





bool TakeMinidumpForChild(uint32_t childPid,
                          nsIFile** dump,
                          uint32_t* aSequence = nullptr);

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
                           nsIFile** childDump);





bool CreateAdditionalChildMinidump(ProcessHandle childPid,
                                   ThreadId childBlamedThread,
                                   nsIFile* parentMinidump,
                                   const nsACString& name);

#  if defined(XP_WIN32) || defined(XP_MACOSX)

const char* GetChildNotificationPipe();

#ifdef MOZ_CRASHREPORTER_INJECTOR



class InjectorCrashCallback
{
public:
  InjectorCrashCallback() { }

  






  virtual void OnCrash(DWORD processID) = 0;
};


void InjectCrashReporterIntoProcess(DWORD processID, InjectorCrashCallback* cb);
void UnregisterInjectorCallback(DWORD processID);
#endif


bool SetRemoteExceptionHandler(const nsACString& crashPipe);

#  elif defined(XP_LINUX)










bool CreateNotificationPipeForChild(int* childCrashFd, int* childCrashRemapFd);


bool SetRemoteExceptionHandler();

#endif  

bool UnsetRemoteExceptionHandler();

#if defined(MOZ_WIDGET_ANDROID)





void AddLibraryMapping(const char* library_name,
                       uintptr_t   start_address,
                       size_t      mapping_length,
                       size_t      file_offset);

#endif
}

#endif 
