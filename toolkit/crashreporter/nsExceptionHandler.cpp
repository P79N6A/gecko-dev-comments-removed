




#include "nsExceptionHandler.h"
#include "nsDataHashtable.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/dom/CrashReporterChild.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "mozilla/unused.h"
#include "mozilla/Snprintf.h"
#include "mozilla/SyncRunnable.h"

#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "jsfriendapi.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include "nsXULAppAPI.h"
#include "nsIXULAppInfo.h"
#include "nsIWindowsRegKey.h"
#include "client/windows/crash_generation/client_info.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#include <dbghelp.h>
#include <string.h>
#include "nsDirectoryServiceUtils.h"

#include "nsWindowsDllInterceptor.h"
#elif defined(XP_MACOSX)
#include "client/mac/crash_generation/client_info.h"
#include "client/mac/crash_generation/crash_generation_server.h"
#include "client/mac/handler/exception_handler.h"
#include <string>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <crt_externs.h>
#include <fcntl.h>
#include <mach/mach.h>
#include <sys/types.h>
#include <spawn.h>
#include <unistd.h>
#include "mac_utils.h"
#elif defined(XP_LINUX)
#include "nsIINIParser.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"
#include "client/linux/crash_generation/client_info.h"
#include "client/linux/crash_generation/crash_generation_server.h"
#include "client/linux/handler/exception_handler.h"
#include "common/linux/eintr_wrapper.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(XP_SOLARIS)
#include "client/solaris/handler/exception_handler.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "Not yet implemented for this platform"
#endif 

#ifdef MOZ_CRASHREPORTER_INJECTOR
#include "InjectCrashReporter.h"
using mozilla::InjectCrashRunnable;
#endif

#include <stdlib.h>
#include <time.h>
#include <prenv.h>
#include <prio.h>
#include <prmem.h>
#include "mozilla/Mutex.h"
#include "nsDebug.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "prprf.h"
#include <map>
#include <vector>

#include "mozilla/IOInterposer.h"
#include "mozilla/mozalloc_oom.h"
#include "mozilla/WindowsDllBlocklist.h"

#if defined(XP_MACOSX)
CFStringRef reporterClientAppID = CFSTR("org.mozilla.crashreporter");
#endif
#if defined(MOZ_WIDGET_ANDROID)
#include "common/linux/file_id.h"
#endif

using google_breakpad::CrashGenerationServer;
using google_breakpad::ClientInfo;
#ifdef XP_LINUX
using google_breakpad::MinidumpDescriptor;
#endif
using namespace mozilla;
using mozilla::dom::CrashReporterChild;
using mozilla::dom::PCrashReporterChild;

namespace CrashReporter {

#ifdef XP_WIN32
typedef wchar_t XP_CHAR;
typedef std::wstring xpstring;
#define XP_TEXT(x) L##x
#define CONVERT_XP_CHAR_TO_UTF16(x) x
#define XP_STRLEN(x) wcslen(x)
#define my_strlen strlen
#define CRASH_REPORTER_FILENAME "crashreporter.exe"
#define PATH_SEPARATOR "\\"
#define XP_PATH_SEPARATOR L"\\"

#define XP_PATH_MAX 4096

#define CMDLINE_SIZE ((XP_PATH_MAX * 2) + 6)
#ifdef _USE_32BIT_TIME_T
#define XP_TTOA(time, buffer, base) ltoa(time, buffer, base)
#else
#define XP_TTOA(time, buffer, base) _i64toa(time, buffer, base)
#endif
#define XP_STOA(size, buffer, base) _ui64toa(size, buffer, base)
#else
typedef char XP_CHAR;
typedef std::string xpstring;
#define XP_TEXT(x) x
#define CONVERT_XP_CHAR_TO_UTF16(x) NS_ConvertUTF8toUTF16(x)
#define CRASH_REPORTER_FILENAME "crashreporter"
#define PATH_SEPARATOR "/"
#define XP_PATH_SEPARATOR "/"
#define XP_PATH_MAX PATH_MAX
#ifdef XP_LINUX
#define XP_STRLEN(x) my_strlen(x)
#define XP_TTOA(time, buffer, base) my_inttostring(time, buffer, sizeof(buffer))
#define XP_STOA(size, buffer, base) my_inttostring(size, buffer, sizeof(buffer))
#else
#define XP_STRLEN(x) strlen(x)
#define XP_TTOA(time, buffer, base) sprintf(buffer, "%ld", time)
#define XP_STOA(size, buffer, base) sprintf(buffer, "%zu", (size_t) size)
#define my_strlen strlen
#define sys_close close
#define sys_fork fork
#define sys_open open
#define sys_read read
#define sys_write write
#endif
#endif 

#ifndef XP_LINUX
static const XP_CHAR dumpFileExtension[] = XP_TEXT(".dmp");
#endif

static const XP_CHAR extraFileExtension[] = XP_TEXT(".extra");
static const XP_CHAR memoryReportExtension[] = XP_TEXT(".memory.json.gz");



static char const * const kCrashEventAnnotations[] = {
  "AsyncShutdownTimeout",
  "BuildID",
  "TelemetryEnvironment",
  "ProductID",
  "ProductName",
  "ReleaseChannel",
  "SecondsSinceLastCrash",
  "ShutdownProgress",
  "Version"
  
  
  
  
  
  
  
  
  
  
  
  
};

static const char kCrashMainID[] = "crash.main.2\n";

static google_breakpad::ExceptionHandler* gExceptionHandler = nullptr;

static XP_CHAR* pendingDirectory;
static XP_CHAR* crashReporterPath;
static XP_CHAR* memoryReportPath;


static XP_CHAR* eventsDirectory;
static char* eventsEnv = nullptr;


static bool doReport = true;


static bool headlessClient = false;


static bool showOSCrashReporter = false;


static time_t lastCrashTime = 0;

static XP_CHAR lastCrashTimeFilename[XP_PATH_MAX] = {0};



static XP_CHAR crashMarkerFilename[XP_PATH_MAX] = {0};


static bool lastRunCrashID_checked = false;

static nsString* lastRunCrashID = nullptr;

#if defined(MOZ_WIDGET_ANDROID)



static char* androidUserSerial = nullptr;
#endif


static const char kTimeSinceLastCrashParameter[] = "SecondsSinceLastCrash=";
static const int kTimeSinceLastCrashParameterLen =
                                     sizeof(kTimeSinceLastCrashParameter)-1;


static Mutex* crashReporterAPILock;
static Mutex* notesFieldLock;
static AnnotationTable* crashReporterAPIData_Hash;
static nsCString* crashReporterAPIData = nullptr;
static nsCString* crashEventAPIData = nullptr;
static nsCString* notesField = nullptr;
static bool isGarbageCollecting;
static uint32_t eventloopNestingLevel = 0;


static Mutex* dumpSafetyLock;
static bool isSafeToDump = false;


static CrashGenerationServer* crashServer; 

#  if defined(XP_WIN) || defined(XP_MACOSX)


static const char kNullNotifyPipe[] = "-";
static char* childCrashNotifyPipe;

#  elif defined(XP_LINUX)
static int serverSocketFd = -1;
static int clientSocketFd = -1;
static const int kMagicChildCrashReportFd = 4;

#  endif


static Mutex* dumpMapLock;
struct ChildProcessData : public nsUint32HashKey
{
  explicit ChildProcessData(KeyTypePointer aKey)
    : nsUint32HashKey(aKey)
    , sequence(0)
#ifdef MOZ_CRASHREPORTER_INJECTOR
    , callback(nullptr)
#endif
  { }

  nsCOMPtr<nsIFile> minidump;
  
  
  uint32_t sequence;
#ifdef MOZ_CRASHREPORTER_INJECTOR
  InjectorCrashCallback* callback;
#endif
};

typedef nsTHashtable<ChildProcessData> ChildMinidumpMap;
static ChildMinidumpMap* pidToMinidump;
static uint32_t crashSequence;
static bool OOPInitialized();

#ifdef MOZ_CRASHREPORTER_INJECTOR
static nsIThread* sInjectorThread;

class ReportInjectedCrash : public nsRunnable
{
public:
  explicit ReportInjectedCrash(uint32_t pid) : mPID(pid) { }

  NS_IMETHOD Run();

private:
  uint32_t mPID;
};
#endif 



static const char* kSubprocessBlacklist[] = {
  "FramePoisonBase",
  "FramePoisonSize",
  "StartupTime",
  "URL"
};



class DelayedNote;
nsTArray<nsAutoPtr<DelayedNote> >* gDelayedAnnotations;

#if defined(XP_WIN)




typedef LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *SetUnhandledExceptionFilter_func)
  (LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
static SetUnhandledExceptionFilter_func stub_SetUnhandledExceptionFilter = 0;
static LPTOP_LEVEL_EXCEPTION_FILTER previousUnhandledExceptionFilter = nullptr;
static WindowsDllInterceptor gKernel32Intercept;
static bool gBlockUnhandledExceptionFilter = true;

static LPTOP_LEVEL_EXCEPTION_FILTER GetUnhandledExceptionFilter()
{
  
  LPTOP_LEVEL_EXCEPTION_FILTER current = SetUnhandledExceptionFilter(nullptr);
  SetUnhandledExceptionFilter(current);
  return current;
}

static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI
patched_SetUnhandledExceptionFilter (LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
  if (!gBlockUnhandledExceptionFilter) {
    
    return stub_SetUnhandledExceptionFilter(lpTopLevelExceptionFilter);
  }

  if (lpTopLevelExceptionFilter == previousUnhandledExceptionFilter) {
    
    previousUnhandledExceptionFilter =
      stub_SetUnhandledExceptionFilter(lpTopLevelExceptionFilter);
    return previousUnhandledExceptionFilter;
  }

  
  return nullptr;
}

static LPTOP_LEVEL_EXCEPTION_FILTER sUnhandledExceptionFilter = nullptr;

static long
JitExceptionHandler(void *exceptionRecord, void *context)
{
    EXCEPTION_POINTERS pointers = {
        (PEXCEPTION_RECORD)exceptionRecord,
        (PCONTEXT)context
    };
    return sUnhandledExceptionFilter(&pointers);
}









static const SIZE_T kReserveSize = 0x4000000; 
static void* gBreakpadReservedVM;
#endif

#ifdef XP_MACOSX
static cpu_type_t pref_cpu_types[2] = {
#if defined(__i386__)
                                 CPU_TYPE_X86,
#elif defined(__x86_64__)
                                 CPU_TYPE_X86_64,
#elif defined(__ppc__)
                                 CPU_TYPE_POWERPC,
#endif
                                 CPU_TYPE_ANY };

static posix_spawnattr_t spawnattr;
#endif

#if defined(MOZ_WIDGET_ANDROID)



typedef struct {
  std::string name;
  uintptr_t   start_address;
  size_t      length;
  size_t      file_offset;
} mapping_info;
static std::vector<mapping_info> library_mappings;
typedef std::map<uint32_t,google_breakpad::MappingList> MappingMap;
#endif

#ifdef XP_LINUX
inline void
my_inttostring(intmax_t t, char* buffer, size_t buffer_length)
{
  my_memset(buffer, 0, buffer_length);
  my_uitos(buffer, t, my_uint_len(t));
}
#endif

#ifdef XP_WIN
static void
CreateFileFromPath(const xpstring& path, nsIFile** file)
{
  NS_NewLocalFile(nsDependentString(path.c_str()), false, file);
}
#else
static void
CreateFileFromPath(const xpstring& path, nsIFile** file)
{
  NS_NewNativeLocalFile(nsDependentCString(path.c_str()), false, file);
}
#endif

static XP_CHAR*
Concat(XP_CHAR* str, const XP_CHAR* toAppend, int* size)
{
  int appendLen = XP_STRLEN(toAppend);
  if (appendLen >= *size) appendLen = *size - 1;

  memcpy(str, toAppend, appendLen * sizeof(XP_CHAR));
  str += appendLen;
  *str = '\0';
  *size -= appendLen;

  return str;
}

static size_t gOOMAllocationSize = 0;

void AnnotateOOMAllocationSize(size_t size)
{
  gOOMAllocationSize = size;
}

#ifndef XP_WIN

bool copy_file(const char* from, const char* to)
{
  const int kBufSize = 4096;
  int fdfrom = sys_open(from, O_RDONLY, 0);
  if (fdfrom < 0) {
    return false;
  }

  bool ok = false;

  int fdto = sys_open(to, O_WRONLY | O_CREAT, 0666);
  if (fdto < 0) {
    sys_close(fdfrom);
    return false;
  }

  char buf[kBufSize];
  while (true) {
    int r = sys_read(fdfrom, buf, kBufSize);
    if (r == 0) {
      ok = true;
      break;
    }
    if (r < 0) {
      break;
    }
    char* wbuf = buf;
    while (r) {
      int w = sys_write(fdto, wbuf, r);
      if (w > 0) {
        r -= w;
        wbuf += w;
      } else if (errno != EINTR) {
        break;
      }
    }
    if (r) {
      break;
    }
  }

  sys_close(fdfrom);
  sys_close(fdto);

  return ok;
}
#endif

#ifdef XP_WIN
class PlatformWriter
{
public:
  PlatformWriter()
    : mHandle(INVALID_HANDLE_VALUE)
  { }

  explicit PlatformWriter(const wchar_t* path)
    : PlatformWriter()
  {
    Open(path);
  }

  ~PlatformWriter() {
    if (Valid()) {
      CloseHandle(mHandle);
    }
  }

  void Open(const wchar_t* path) {
    mHandle = CreateFile(path, GENERIC_WRITE, 0,
                         nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                         nullptr);
  }

  bool Valid() {
    return mHandle != INVALID_HANDLE_VALUE;
  }

  void WriteBuffer(const char* buffer, size_t len)
  {
    if (!Valid()) {
      return;
    }
    DWORD nBytes;
    WriteFile(mHandle, buffer, len, &nBytes, nullptr);
  }

  HANDLE Handle() {
    return mHandle;
  }

private:
  HANDLE mHandle;
};

#elif defined(XP_UNIX)

class PlatformWriter
{
public:
  PlatformWriter()
    : mFD(-1)
  { }

  explicit PlatformWriter(const char* path)
    : PlatformWriter()
  {
    Open(path);
  }

  ~PlatformWriter() {
    if (Valid()) {
      sys_close(mFD);
    }
  }

  void Open(const char* path) {
    mFD = sys_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  }

  bool Valid() {
    return mFD != -1;
  }

  void WriteBuffer(const char* buffer, size_t len) {
    if (!Valid()) {
      return;
    }
    unused << sys_write(mFD, buffer, len);
  }

private:
  int mFD;
};

#else
#error "Need implementation of PlatformWrite for this platform"
#endif

template<int N>
void
WriteLiteral(PlatformWriter& pw, const char (&str)[N])
{
  pw.WriteBuffer(str, N - 1);
}

static void
WriteString(PlatformWriter& pw, const char* str) {
#ifdef XP_LINUX
  size_t len = my_strlen(str);
#else
  size_t len = strlen(str);
#endif

  pw.WriteBuffer(str, len);
}

template<int N>
static void
WriteAnnotation(PlatformWriter& pw, const char (&name)[N],
                const char* value) {
  WriteLiteral(pw, name);
  WriteLiteral(pw, "=");
  WriteString(pw, value);
  WriteLiteral(pw, "\n");
};

bool MinidumpCallback(
#ifdef XP_LINUX
                      const MinidumpDescriptor& descriptor,
#else
                      const XP_CHAR* dump_path,
                      const XP_CHAR* minidump_id,
#endif
                      void* context,
#ifdef XP_WIN32
                      EXCEPTION_POINTERS* exinfo,
                      MDRawAssertionInfo* assertion,
#endif
                      bool succeeded)
{
  bool returnValue = showOSCrashReporter ? false : succeeded;

  static XP_CHAR minidumpPath[XP_PATH_MAX];
  int size = XP_PATH_MAX;
  XP_CHAR* p;
#ifndef XP_LINUX
  p = Concat(minidumpPath, dump_path, &size);
  p = Concat(p, XP_PATH_SEPARATOR, &size);
  p = Concat(p, minidump_id, &size);
  Concat(p, dumpFileExtension, &size);
#else
  Concat(minidumpPath, descriptor.path(), &size);
#endif

  static XP_CHAR extraDataPath[XP_PATH_MAX];
  size = XP_PATH_MAX;
#ifndef XP_LINUX
  p = Concat(extraDataPath, dump_path, &size);
  p = Concat(p, XP_PATH_SEPARATOR, &size);
  p = Concat(p, minidump_id, &size);
#else
  p = Concat(extraDataPath, descriptor.path(), &size);
  
  p -= 4;
#endif
  Concat(p, extraFileExtension, &size);

  static XP_CHAR memoryReportLocalPath[XP_PATH_MAX];
  size = XP_PATH_MAX;
#ifndef XP_LINUX
  p = Concat(memoryReportLocalPath, dump_path, &size);
  p = Concat(p, XP_PATH_SEPARATOR, &size);
  p = Concat(p, minidump_id, &size);
#else
  p = Concat(memoryReportLocalPath, descriptor.path(), &size);
  
  p -= 4;
#endif
  Concat(p, memoryReportExtension, &size);

  if (memoryReportPath) {
#ifdef XP_WIN
    CopyFile(memoryReportPath, memoryReportLocalPath, false);
#else
    copy_file(memoryReportPath, memoryReportLocalPath);
#endif
  }

  if (headlessClient) {
    
    PlatformWriter markerFile(crashMarkerFilename);
#if defined(XP_WIN)
    markerFile.WriteBuffer(reinterpret_cast<const char*>(minidumpPath),
                           2*wcslen(minidumpPath));
#elif defined(XP_UNIX)
    markerFile.WriteBuffer(minidumpPath, my_strlen(minidumpPath));
#endif
  }

  char oomAllocationSizeBuffer[32] = "";
  if (gOOMAllocationSize) {
    XP_STOA(gOOMAllocationSize, oomAllocationSizeBuffer, 10);
  }

  
  
  time_t crashTime;
#ifdef XP_LINUX
  struct kernel_timeval tv;
  sys_gettimeofday(&tv, nullptr);
  crashTime = tv.tv_sec;
#else
  crashTime = time(nullptr);
#endif
  time_t timeSinceLastCrash = 0;
  
  char crashTimeString[32];
  char timeSinceLastCrashString[32];

  XP_TTOA(crashTime, crashTimeString, 10);
  if (lastCrashTime != 0) {
    timeSinceLastCrash = crashTime - lastCrashTime;
    XP_TTOA(timeSinceLastCrash, timeSinceLastCrashString, 10);
  }
  
  if (lastCrashTimeFilename[0] != 0) {
    PlatformWriter lastCrashFile(lastCrashTimeFilename);
    WriteString(lastCrashFile, crashTimeString);
  }

  

  
  static char id_ascii[37];
#ifdef XP_LINUX
  const char * index = strrchr(descriptor.path(), '/');
  MOZ_ASSERT(index);
  MOZ_ASSERT(strlen(index) == 1 + 36 + 4); 
  for (uint32_t i = 0; i < 36; i++) {
    id_ascii[i] = *(index + 1 + i);
  }
#else
  MOZ_ASSERT(XP_STRLEN(minidump_id) == 36);
  for (uint32_t i = 0; i < 36; i++) {
    id_ascii[i] = *((char *)(minidump_id + i));
  }
#endif

  {
    PlatformWriter apiData;
    PlatformWriter eventFile;

    if (eventsDirectory) {
      static XP_CHAR crashEventPath[XP_PATH_MAX];
      int size = XP_PATH_MAX;
      XP_CHAR* p;
      p = Concat(crashEventPath, eventsDirectory, &size);
      p = Concat(p, XP_PATH_SEPARATOR, &size);
#ifdef XP_LINUX
      p = Concat(p, id_ascii, &size);
#else
      p = Concat(p, minidump_id, &size);
#endif

      eventFile.Open(crashEventPath);
      WriteLiteral(eventFile, kCrashMainID);
      WriteString(eventFile, crashTimeString);
      WriteLiteral(eventFile, "\n");
      WriteString(eventFile, id_ascii);
      WriteLiteral(eventFile, "\n");
      if (crashEventAPIData) {
        eventFile.WriteBuffer(crashEventAPIData->get(), crashEventAPIData->Length());
      }
    }

    if (!crashReporterAPIData->IsEmpty()) {
      
      apiData.Open(extraDataPath);
      apiData.WriteBuffer(crashReporterAPIData->get(), crashReporterAPIData->Length());
    }
    WriteAnnotation(apiData, "CrashTime", crashTimeString);
    if (timeSinceLastCrash != 0) {
      WriteAnnotation(apiData, "SecondsSinceLastCrash",
                      timeSinceLastCrashString);
      WriteAnnotation(eventFile, "SecondsSinceLastCrash",
                      timeSinceLastCrashString);
    }
    if (isGarbageCollecting) {
      WriteAnnotation(apiData, "IsGarbageCollecting",
                      isGarbageCollecting ? "1" : "0");
      WriteAnnotation(eventFile, "IsGarbageCollecting",
                      isGarbageCollecting ? "1" : "0");
    }

    char buffer[128];

    if (eventloopNestingLevel > 0) {
      XP_STOA(eventloopNestingLevel, buffer, 10);
      WriteAnnotation(apiData, "EventLoopNestingLevel", buffer);
      WriteAnnotation(eventFile, "EventLoopNestingLevel", buffer);
    }

#ifdef XP_WIN
    if (gBreakpadReservedVM) {
      _ui64toa(uintptr_t(gBreakpadReservedVM), buffer, 10);
      WriteAnnotation(apiData, "BreakpadReserveAddress", buffer);
      _ui64toa(kReserveSize, buffer, 10);
      WriteAnnotation(apiData, "BreakpadReserveSize", buffer);
    }

#ifdef HAS_DLL_BLOCKLIST
    if (apiData.Valid()) {
      DllBlocklist_WriteNotes(apiData.Handle());
      DllBlocklist_WriteNotes(eventFile.Handle());
    }
#endif

    
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {

#define WRITE_STATEX_FIELD(field, name, conversionFunc)          \
      conversionFunc(statex.field, buffer, 10);                  \
      WriteAnnotation(apiData, name, buffer);                    \
      WriteAnnotation(eventFile, name, buffer);

      WRITE_STATEX_FIELD(dwMemoryLoad, "SystemMemoryUsePercentage", ltoa);
      WRITE_STATEX_FIELD(ullTotalVirtual, "TotalVirtualMemory", _ui64toa);
      WRITE_STATEX_FIELD(ullAvailVirtual, "AvailableVirtualMemory", _ui64toa);
      WRITE_STATEX_FIELD(ullTotalPageFile, "TotalPageFile", _ui64toa);
      WRITE_STATEX_FIELD(ullAvailPageFile, "AvailablePageFile", _ui64toa);
      WRITE_STATEX_FIELD(ullTotalPhys, "TotalPhysicalMemory", _ui64toa);
      WRITE_STATEX_FIELD(ullAvailPhys, "AvailablePhysicalMemory", _ui64toa);

#undef WRITE_STATEX_FIELD
    }
#endif 
    if (oomAllocationSizeBuffer[0]) {
      WriteAnnotation(apiData, "OOMAllocationSize", oomAllocationSizeBuffer);
      WriteAnnotation(eventFile, "OOMAllocationSize", oomAllocationSizeBuffer);
    }

    if (memoryReportPath) {
      WriteLiteral(apiData, "ContainsMemoryReport=1\n");
      WriteLiteral(eventFile, "ContainsMemoryReport=1\n");
    }
  }

#ifdef XP_WIN
  if (!doReport) {
    TerminateProcess(GetCurrentProcess(), 1);
    return returnValue;
  }

  XP_CHAR cmdLine[CMDLINE_SIZE];
  size = CMDLINE_SIZE;
  p = Concat(cmdLine, L"\"", &size);
  p = Concat(p, crashReporterPath, &size);
  p = Concat(p, L"\" \"", &size);
  p = Concat(p, minidumpPath, &size);
  Concat(p, L"\"", &size);

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOWNORMAL;
  ZeroMemory(&pi, sizeof(pi));

  if (CreateProcess(nullptr, (LPWSTR)cmdLine, nullptr, nullptr, FALSE, 0,
                    nullptr, nullptr, &si, &pi)) {
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
  }
  
  TerminateProcess(GetCurrentProcess(), 1);
#elif defined(XP_UNIX)
  if (!doReport) {
    return returnValue;
  }

#ifdef XP_MACOSX
  char* const my_argv[] = {
    crashReporterPath,
    minidumpPath,
    nullptr
  };

  char **env = nullptr;
  char ***nsEnv = _NSGetEnviron();
  if (nsEnv)
    env = *nsEnv;
  int result = posix_spawnp(nullptr,
                            my_argv[0],
                            nullptr,
                            &spawnattr,
                            my_argv,
                            env);

  if (result != 0)
    return false;

#else 
  pid_t pid = sys_fork();

  if (pid == -1)
    return false;
  else if (pid == 0) {
#if !defined(MOZ_WIDGET_ANDROID)
    
    
    unsetenv("LD_LIBRARY_PATH");
    unused << execl(crashReporterPath,
                    crashReporterPath, minidumpPath, (char*)0);
#else
    
    if (androidUserSerial) {
      unused << execlp("/system/bin/am",
                       "/system/bin/am",
                       "start",
                       "--user", androidUserSerial,
                       "-a", "org.mozilla.gecko.reportCrash",
                       "-n", crashReporterPath,
                       "--es", "minidumpPath", minidumpPath,
                       (char*)0);
    } else {
      unused << execlp("/system/bin/am",
                       "/system/bin/am",
                       "start",
                       "-a", "org.mozilla.gecko.reportCrash",
                       "-n", crashReporterPath,
                       "--es", "minidumpPath", minidumpPath,
                       (char*)0);
    }
#endif
    _exit(1);
#ifdef MOZ_WIDGET_ANDROID
  } else {
    
    
    int status;
    unused << HANDLE_EINTR(sys_waitpid(pid, &status, __WALL));
#endif
  }
#endif
#endif

  return returnValue;
}

#ifdef XP_WIN
static void
ReserveBreakpadVM()
{
  if (!gBreakpadReservedVM) {
    gBreakpadReservedVM = VirtualAlloc(nullptr, kReserveSize, MEM_RESERVE,
                                       PAGE_NOACCESS);
  }
}

static void
FreeBreakpadVM()
{
  if (gBreakpadReservedVM) {
    VirtualFree(gBreakpadReservedVM, 0, MEM_RELEASE);
  }
}







static bool FPEFilter(void* context, EXCEPTION_POINTERS* exinfo,
                      MDRawAssertionInfo* assertion)
{
  if (!exinfo) {
    mozilla::IOInterposer::Disable();
    FreeBreakpadVM();
    return true;
  }

  PEXCEPTION_RECORD e = (PEXCEPTION_RECORD)exinfo->ExceptionRecord;
  switch (e->ExceptionCode) {
    case STATUS_FLOAT_DENORMAL_OPERAND:
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
    case STATUS_FLOAT_INEXACT_RESULT:
    case STATUS_FLOAT_INVALID_OPERATION:
    case STATUS_FLOAT_OVERFLOW:
    case STATUS_FLOAT_STACK_CHECK:
    case STATUS_FLOAT_UNDERFLOW:
    case STATUS_FLOAT_MULTIPLE_FAULTS:
    case STATUS_FLOAT_MULTIPLE_TRAPS:
      return false; 
  }
  mozilla::IOInterposer::Disable();
  FreeBreakpadVM();
  return true;
}
#endif 

static bool ShouldReport()
{
  
  
  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER_NO_REPORT");
  if (envvar && *envvar) {
    return false;
  }

  envvar = PR_GetEnv("MOZ_CRASHREPORTER_FULLDUMP");
  if (envvar && *envvar) {
    return false;
  }

  return true;
}

namespace {
  bool Filter(void* context) {
    mozilla::IOInterposer::Disable();
    return true;
  }
}


nsresult SetExceptionHandler(nsIFile* aXREDirectory,
                             bool force)
{
  if (gExceptionHandler)
    return NS_ERROR_ALREADY_INITIALIZED;

#if !defined(DEBUG) || defined(MOZ_WIDGET_GONK)
  
  
  
  
  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER_DISABLE");
  if (envvar && *envvar && !force)
    return NS_OK;
#else
  
  
  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER");
  if ((!envvar || !*envvar) && !force)
    return NS_OK;
#endif

#if defined(MOZ_WIDGET_GONK)
  doReport = false;
  headlessClient = true;
#elif defined(XP_WIN)
  doReport = ShouldReport();
#else
  
  
  doReport = ShouldReport();
#endif

  
  crashReporterAPIData = new nsCString();
  crashEventAPIData = new nsCString();

  NS_ASSERTION(!crashReporterAPILock, "Shouldn't have a lock yet");
  crashReporterAPILock = new Mutex("crashReporterAPILock");
  NS_ASSERTION(!notesFieldLock, "Shouldn't have a lock yet");
  notesFieldLock = new Mutex("notesFieldLock");

  crashReporterAPIData_Hash =
    new nsDataHashtable<nsCStringHashKey,nsCString>();
  NS_ENSURE_TRUE(crashReporterAPIData_Hash, NS_ERROR_OUT_OF_MEMORY);

  notesField = new nsCString();
  NS_ENSURE_TRUE(notesField, NS_ERROR_OUT_OF_MEMORY);

  if (!headlessClient) {
    
    nsCOMPtr<nsIFile> exePath;
    nsresult rv = aXREDirectory->Clone(getter_AddRefs(exePath));
    NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_MACOSX)
    exePath->SetNativeLeafName(NS_LITERAL_CSTRING("MacOS"));
    exePath->Append(NS_LITERAL_STRING("crashreporter.app"));
    exePath->Append(NS_LITERAL_STRING("Contents"));
    exePath->Append(NS_LITERAL_STRING("MacOS"));
#endif

    exePath->AppendNative(NS_LITERAL_CSTRING(CRASH_REPORTER_FILENAME));

#ifdef XP_WIN32
    nsString crashReporterPath_temp;

    exePath->GetPath(crashReporterPath_temp);
    crashReporterPath = reinterpret_cast<wchar_t*>(ToNewUnicode(crashReporterPath_temp));
#elif !defined(__ANDROID__)
    nsCString crashReporterPath_temp;

    exePath->GetNativePath(crashReporterPath_temp);
    crashReporterPath = ToNewCString(crashReporterPath_temp);
#else
    
    
    nsCString package(ANDROID_PACKAGE_NAME "/org.mozilla.gecko.CrashReporter");
    crashReporterPath = ToNewCString(package);
#endif
  }

  
#if defined(XP_WIN32)
  nsString tempPath;

  
  int pathLen = GetTempPath(0, nullptr);
  if (pathLen == 0)
    return NS_ERROR_FAILURE;

  tempPath.SetLength(pathLen);
  GetTempPath(pathLen, (LPWSTR)tempPath.BeginWriting());
#elif defined(XP_MACOSX)
  nsCString tempPath;
  FSRef fsRef;
  OSErr err = FSFindFolder(kUserDomain, kTemporaryFolderType,
                           kCreateFolder, &fsRef);
  if (err != noErr)
    return NS_ERROR_FAILURE;

  char path[PATH_MAX];
  OSStatus status = FSRefMakePath(&fsRef, (UInt8*)path, PATH_MAX);
  if (status != noErr)
    return NS_ERROR_FAILURE;

  tempPath = path;

#elif defined(__ANDROID__)
  
  const char *tempenv = PR_GetEnv("TMPDIR");
  if (!tempenv)
    return NS_ERROR_FAILURE;
  nsCString tempPath(tempenv);
#elif defined(XP_UNIX)
  
  nsCString tempPath = NS_LITERAL_CSTRING("/tmp/");
#else
#error "Implement this for your platform"
#endif

#ifdef XP_MACOSX
  
  if (posix_spawnattr_init(&spawnattr) != 0) {
    return NS_ERROR_FAILURE;
  }

  
  size_t attr_count = ArrayLength(pref_cpu_types);
  size_t attr_ocount = 0;
  if (posix_spawnattr_setbinpref_np(&spawnattr,
                                    attr_count,
                                    pref_cpu_types,
                                    &attr_ocount) != 0 ||
      attr_ocount != attr_count) {
    posix_spawnattr_destroy(&spawnattr);
    return NS_ERROR_FAILURE;
  }
#endif

#ifdef XP_WIN32
  ReserveBreakpadVM();

  MINIDUMP_TYPE minidump_type = MiniDumpNormal;

  
  

  DWORD version_size = GetFileVersionInfoSizeW(L"dbghelp.dll", nullptr);
  if (version_size > 0) {
    std::vector<BYTE> buffer(version_size);
    if (GetFileVersionInfoW(L"dbghelp.dll",
                            0,
                            version_size,
                            &buffer[0])) {
      UINT len;
      VS_FIXEDFILEINFO* file_info;
      VerQueryValue(&buffer[0], L"\\", (void**)&file_info, &len);
      WORD major = HIWORD(file_info->dwFileVersionMS),
           minor = LOWORD(file_info->dwFileVersionMS),
           revision = HIWORD(file_info->dwFileVersionLS);
      if (major > 6 || (major == 6 && minor > 1) ||
          (major == 6 && minor == 1 && revision >= 7600)) {
        minidump_type = MiniDumpWithFullMemoryInfo;
      }
    }
  }

  const char* e = PR_GetEnv("MOZ_CRASHREPORTER_FULLDUMP");
  if (e && *e) {
    minidump_type = MiniDumpWithFullMemory;
  }
#endif 

#ifdef MOZ_WIDGET_ANDROID
  androidUserSerial = getenv("MOZ_ANDROID_USER_SERIAL_NUMBER");
#endif

  
  
  NS_ASSERTION(!dumpSafetyLock, "Shouldn't have a lock yet");
  
  
  dumpSafetyLock = new Mutex("dumpSafetyLock");
  MutexAutoLock lock(*dumpSafetyLock);
  isSafeToDump = true;

  
#ifdef XP_LINUX
  MinidumpDescriptor descriptor(tempPath.get());
#endif

#ifdef XP_WIN
  previousUnhandledExceptionFilter = GetUnhandledExceptionFilter();
#endif

  gExceptionHandler = new google_breakpad::
    ExceptionHandler(
#ifdef XP_LINUX
                     descriptor,
#else
                     tempPath.get(),
#endif

#ifdef XP_WIN
                     FPEFilter,
#else
                     Filter,
#endif
                     MinidumpCallback,
                     nullptr,
#ifdef XP_WIN32
                     google_breakpad::ExceptionHandler::HANDLER_ALL,
                     minidump_type,
                     (const wchar_t*) nullptr,
                     nullptr);
#else
                     true
#ifdef XP_MACOSX
                       , nullptr
#endif
#ifdef XP_LINUX
                       , -1
#endif
                      );
#endif

  if (!gExceptionHandler)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef XP_WIN
  gExceptionHandler->set_handle_debug_exceptions(true);
  
#ifdef _WIN64
  
  sUnhandledExceptionFilter = GetUnhandledExceptionFilter();
  if (sUnhandledExceptionFilter)
      js::SetJitExceptionHandler(JitExceptionHandler);
#endif

  
  gBlockUnhandledExceptionFilter = true;
  gKernel32Intercept.Init("kernel32.dll");
  bool ok = gKernel32Intercept.AddHook("SetUnhandledExceptionFilter",
          reinterpret_cast<intptr_t>(patched_SetUnhandledExceptionFilter),
          (void**) &stub_SetUnhandledExceptionFilter);

#ifdef DEBUG
  if (!ok)
    printf_stderr ("SetUnhandledExceptionFilter hook failed; crash reporter is vulnerable.\n");
#endif
#endif

  
  char timeString[32];
  time_t startupTime = time(nullptr);
  XP_TTOA(startupTime, timeString, 10);
  AnnotateCrashReport(NS_LITERAL_CSTRING("StartupTime"),
                      nsDependentCString(timeString));

#if defined(XP_MACOSX)
  
  
  
  Boolean keyExistsAndHasValidFormat = false;
  Boolean prefValue = ::CFPreferencesGetAppBooleanValue(CFSTR("OSCrashReporter"),
                                                        kCFPreferencesCurrentApplication,
                                                        &keyExistsAndHasValidFormat);
  if (keyExistsAndHasValidFormat)
    showOSCrashReporter = prefValue;
#endif

#if defined(MOZ_WIDGET_ANDROID)
  for (unsigned int i = 0; i < library_mappings.size(); i++) {
    u_int8_t guid[sizeof(MDGUID)];
    google_breakpad::FileID::ElfFileIdentifierFromMappedFile((void const *)library_mappings[i].start_address, guid);
    gExceptionHandler->AddMappingInfo(library_mappings[i].name,
                                      guid,
                                      library_mappings[i].start_address,
                                      library_mappings[i].length,
                                      library_mappings[i].file_offset);
  }
#endif

  mozalloc_set_oom_abort_handler(AnnotateOOMAllocationSize);

  return NS_OK;
}

bool GetEnabled()
{
  return gExceptionHandler != nullptr;
}

bool GetMinidumpPath(nsAString& aPath)
{
  if (!gExceptionHandler)
    return false;

#ifndef XP_LINUX
  aPath = CONVERT_XP_CHAR_TO_UTF16(gExceptionHandler->dump_path().c_str());
#else
  aPath = CONVERT_XP_CHAR_TO_UTF16(
      gExceptionHandler->minidump_descriptor().directory().c_str());
#endif
  return true;
}

nsresult SetMinidumpPath(const nsAString& aPath)
{
  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

#ifdef XP_WIN32
  gExceptionHandler->set_dump_path(char16ptr_t(aPath.BeginReading()));
#elif defined(XP_LINUX)
  gExceptionHandler->set_minidump_descriptor(
      MinidumpDescriptor(NS_ConvertUTF16toUTF8(aPath).BeginReading()));
#else
  gExceptionHandler->set_dump_path(NS_ConvertUTF16toUTF8(aPath).BeginReading());
#endif
  return NS_OK;
}

static nsresult
WriteDataToFile(nsIFile* aFile, const nsACString& data)
{
  PRFileDesc* fd;
  nsresult rv = aFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE, 00600, &fd);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_OK;
  if (PR_Write(fd, data.Data(), data.Length()) == -1) {
    rv = NS_ERROR_FAILURE;
  }
  PR_Close(fd);
  return rv;
}

static nsresult
GetFileContents(nsIFile* aFile, nsACString& data)
{
  PRFileDesc* fd;
  nsresult rv = aFile->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_OK;
  int32_t filesize = PR_Available(fd);
  if (filesize <= 0) {
    rv = NS_ERROR_FILE_NOT_FOUND;
  }
  else {
    data.SetLength(filesize);
    if (PR_Read(fd, data.BeginWriting(), filesize) == -1) {
      rv = NS_ERROR_FAILURE;
    }
  }
  PR_Close(fd);
  return rv;
}



typedef nsresult (*InitDataFunc)(nsACString&);




static nsresult
GetOrInit(nsIFile* aDir, const nsACString& filename,
          nsACString& aContents, InitDataFunc aInitFunc)
{
  bool exists;

  nsCOMPtr<nsIFile> dataFile;
  nsresult rv = aDir->Clone(getter_AddRefs(dataFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataFile->AppendNative(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    if (aInitFunc) {
      
      rv = aInitFunc(aContents);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = WriteDataToFile(dataFile, aContents);
    }
    else {
      
      rv = NS_ERROR_FAILURE;
    }
  }
  else {
    
    rv = GetFileContents(dataFile, aContents);
  }

  return rv;
}



static nsresult
InitInstallTime(nsACString& aInstallTime)
{
  time_t t = time(nullptr);
  char buf[16];
  snprintf_literal(buf, "%ld", t);
  aInstallTime = buf;

  return NS_OK;
}


static nsresult
EnsureDirectoryExists(nsIFile* dir)
{
  nsresult rv = dir->Create(nsIFile::DIRECTORY_TYPE, 0700);

  if (NS_WARN_IF(NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS)) {
    return rv;
  }

  return NS_OK;
}






nsresult SetupExtraData(nsIFile* aAppDataDirectory,
                        const nsACString& aBuildID)
{
  nsCOMPtr<nsIFile> dataDirectory;
  nsresult rv = aAppDataDirectory->Clone(getter_AddRefs(dataDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataDirectory->AppendNative(NS_LITERAL_CSTRING("Crash Reports"));
  NS_ENSURE_SUCCESS(rv, rv);

  EnsureDirectoryExists(dataDirectory);

#if defined(XP_WIN32)
  nsAutoString dataDirEnv(NS_LITERAL_STRING("MOZ_CRASHREPORTER_DATA_DIRECTORY="));

  nsAutoString dataDirectoryPath;
  rv = dataDirectory->GetPath(dataDirectoryPath);
  NS_ENSURE_SUCCESS(rv, rv);

  dataDirEnv.Append(dataDirectoryPath);

  _wputenv(dataDirEnv.get());
#else
  
  nsAutoCString dataDirEnv("MOZ_CRASHREPORTER_DATA_DIRECTORY=");

  nsAutoCString dataDirectoryPath;
  rv = dataDirectory->GetNativePath(dataDirectoryPath);
  NS_ENSURE_SUCCESS(rv, rv);

  dataDirEnv.Append(dataDirectoryPath);

  char* env = ToNewCString(dataDirEnv);
  NS_ENSURE_TRUE(env, NS_ERROR_OUT_OF_MEMORY);

  PR_SetEnv(env);
#endif

  nsAutoCString data;
  if(NS_SUCCEEDED(GetOrInit(dataDirectory,
                            NS_LITERAL_CSTRING("InstallTime") + aBuildID,
                            data, InitInstallTime)))
    AnnotateCrashReport(NS_LITERAL_CSTRING("InstallTime"), data);

  
  
  
  
  
  if(NS_SUCCEEDED(GetOrInit(dataDirectory, NS_LITERAL_CSTRING("LastCrash"),
                            data, nullptr))) {
    lastCrashTime = (time_t)atol(data.get());
  }

  
  nsCOMPtr<nsIFile> lastCrashFile;
  rv = dataDirectory->Clone(getter_AddRefs(lastCrashFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = lastCrashFile->AppendNative(NS_LITERAL_CSTRING("LastCrash"));
  NS_ENSURE_SUCCESS(rv, rv);
  memset(lastCrashTimeFilename, 0, sizeof(lastCrashTimeFilename));

#if defined(XP_WIN32)
  nsAutoString filename;
  rv = lastCrashFile->GetPath(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  if (filename.Length() < XP_PATH_MAX)
    wcsncpy(lastCrashTimeFilename, filename.get(), filename.Length());
#else
  nsAutoCString filename;
  rv = lastCrashFile->GetNativePath(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  if (filename.Length() < XP_PATH_MAX)
    strncpy(lastCrashTimeFilename, filename.get(), filename.Length());
#endif

  if (headlessClient) {
    nsCOMPtr<nsIFile> markerFile;
    rv = dataDirectory->Clone(getter_AddRefs(markerFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = markerFile->AppendNative(NS_LITERAL_CSTRING("LastCrashFilename"));
    NS_ENSURE_SUCCESS(rv, rv);
    memset(crashMarkerFilename, 0, sizeof(crashMarkerFilename));

#if defined(XP_WIN32)
    nsAutoString markerFilename;
    rv = markerFile->GetPath(markerFilename);
    NS_ENSURE_SUCCESS(rv, rv);

    if (markerFilename.Length() < XP_PATH_MAX)
      wcsncpy(crashMarkerFilename, markerFilename.get(),
              markerFilename.Length());
#else
    nsAutoCString markerFilename;
    rv = markerFile->GetNativePath(markerFilename);
    NS_ENSURE_SUCCESS(rv, rv);

    if (markerFilename.Length() < XP_PATH_MAX)
      strncpy(crashMarkerFilename, markerFilename.get(),
              markerFilename.Length());
#endif
  }

  return NS_OK;
}

static void OOPDeinit();

nsresult UnsetExceptionHandler()
{
  if (isSafeToDump) {
    MutexAutoLock lock(*dumpSafetyLock);
    isSafeToDump = false;
  }

#ifdef XP_WIN
  
  gBlockUnhandledExceptionFilter = false;
#endif

  delete gExceptionHandler;

  
  
  delete crashReporterAPIData_Hash;
  crashReporterAPIData_Hash = nullptr;

  delete crashReporterAPILock;
  crashReporterAPILock = nullptr;

  delete notesFieldLock;
  notesFieldLock = nullptr;

  delete crashReporterAPIData;
  crashReporterAPIData = nullptr;

  delete crashEventAPIData;
  crashEventAPIData = nullptr;

  delete notesField;
  notesField = nullptr;

  delete lastRunCrashID;
  lastRunCrashID = nullptr;

  if (pendingDirectory) {
    free(pendingDirectory);
    pendingDirectory = nullptr;
  }

  if (crashReporterPath) {
    free(crashReporterPath);
    crashReporterPath = nullptr;
  }

  if (eventsDirectory) {
    free(eventsDirectory);
    eventsDirectory = nullptr;
  }

  if (memoryReportPath) {
    free(memoryReportPath);
    memoryReportPath = nullptr;
  }

#ifdef XP_MACOSX
  posix_spawnattr_destroy(&spawnattr);
#endif

  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  gExceptionHandler = nullptr;

  OOPDeinit();

  delete dumpSafetyLock;
  dumpSafetyLock = nullptr;

  return NS_OK;
}

static void ReplaceChar(nsCString& str, const nsACString& character,
                        const nsACString& replacement)
{
  nsCString::const_iterator start, end;

  str.BeginReading(start);
  str.EndReading(end);

  while (FindInReadable(character, start, end)) {
    int32_t pos = end.size_backward();
    str.Replace(pos - 1, 1, replacement);

    str.BeginReading(start);
    start.advance(pos + replacement.Length() - 1);
    str.EndReading(end);
  }
}

static bool DoFindInReadable(const nsACString& str, const nsACString& value)
{
  nsACString::const_iterator start, end;
  str.BeginReading(start);
  str.EndReading(end);

  return FindInReadable(value, start, end);
}

static bool
IsInWhitelist(const nsACString& key)
{
  for (size_t i = 0; i < ArrayLength(kCrashEventAnnotations); ++i) {
    if (key.EqualsASCII(kCrashEventAnnotations[i])) {
      return true;
    }
  }
  return false;
}

static PLDHashOperator EnumerateEntries(const nsACString& key,
                                        nsCString entry,
                                        void* userData)
{
  if (!entry.IsEmpty()) {
    NS_NAMED_LITERAL_CSTRING(kEquals, "=");
    NS_NAMED_LITERAL_CSTRING(kNewline, "\n");
    nsAutoCString line = key + kEquals + entry + kNewline;

    crashReporterAPIData->Append(line);
    if (IsInWhitelist(key)) {
      crashEventAPIData->Append(line);
    }
  }
  return PL_DHASH_NEXT;
}


#ifdef _MSC_VER
#pragma optimize("", off)
#endif
static nsresult
EscapeAnnotation(const nsACString& key, const nsACString& data, nsCString& escapedData)
{
  if (DoFindInReadable(key, NS_LITERAL_CSTRING("=")) ||
      DoFindInReadable(key, NS_LITERAL_CSTRING("\n")))
    return NS_ERROR_INVALID_ARG;

  if (DoFindInReadable(data, NS_LITERAL_CSTRING("\0")))
    return NS_ERROR_INVALID_ARG;

  escapedData = data;

  
  ReplaceChar(escapedData, NS_LITERAL_CSTRING("\\"),
              NS_LITERAL_CSTRING("\\\\"));
  
  ReplaceChar(escapedData, NS_LITERAL_CSTRING("\n"),
              NS_LITERAL_CSTRING("\\n"));
  return NS_OK;
}
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

class DelayedNote
{
 public:
  DelayedNote(const nsACString& aKey, const nsACString& aData)
  : mKey(aKey), mData(aData), mType(Annotation) {}

  explicit DelayedNote(const nsACString& aData)
  : mData(aData), mType(AppNote) {}

  void Run()
  {
    if (mType == Annotation) {
      AnnotateCrashReport(mKey, mData);
    } else {
      AppendAppNotesToCrashReport(mData);
    }
  }
  
 private:
  nsCString mKey;
  nsCString mData;
  enum AnnotationType { Annotation, AppNote } mType;
};

static void
EnqueueDelayedNote(DelayedNote* aNote)
{
  if (!gDelayedAnnotations) {
    gDelayedAnnotations = new nsTArray<nsAutoPtr<DelayedNote> >();
  }
  gDelayedAnnotations->AppendElement(aNote);
}

nsresult AnnotateCrashReport(const nsACString& key, const nsACString& data)
{
  if (!GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  nsCString escapedData;
  nsresult rv = EscapeAnnotation(key, data, escapedData);
  if (NS_FAILED(rv))
    return rv;

  if (!XRE_IsParentProcess()) {
    if (!NS_IsMainThread()) {
      NS_ERROR("Cannot call AnnotateCrashReport in child processes from non-main thread.");
      return NS_ERROR_FAILURE;
    }
    PCrashReporterChild* reporter = CrashReporterChild::GetCrashReporter();
    if (!reporter) {
      EnqueueDelayedNote(new DelayedNote(key, data));
      return NS_OK;
    }
    if (!reporter->SendAnnotateCrashReport(nsCString(key), escapedData))
      return NS_ERROR_FAILURE;
    return NS_OK;
  }

  MutexAutoLock lock(*crashReporterAPILock);

  crashReporterAPIData_Hash->Put(key, escapedData);

  
  crashReporterAPIData->Truncate(0);
  crashEventAPIData->Truncate(0);
  crashReporterAPIData_Hash->EnumerateRead(EnumerateEntries, nullptr);

  return NS_OK;
}

nsresult RemoveCrashReportAnnotation(const nsACString& key)
{
  return AnnotateCrashReport(key, NS_LITERAL_CSTRING(""));
}

nsresult SetGarbageCollecting(bool collecting)
{
  if (!GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  isGarbageCollecting = collecting;

  return NS_OK;
}

void SetEventloopNestingLevel(uint32_t level)
{
  eventloopNestingLevel = level;
}

nsresult AppendAppNotesToCrashReport(const nsACString& data)
{
  if (!GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  if (DoFindInReadable(data, NS_LITERAL_CSTRING("\0")))
    return NS_ERROR_INVALID_ARG;

  if (!XRE_IsParentProcess()) {
    if (!NS_IsMainThread()) {
      NS_ERROR("Cannot call AnnotateCrashReport in child processes from non-main thread.");
      return NS_ERROR_FAILURE;
    }
    PCrashReporterChild* reporter = CrashReporterChild::GetCrashReporter();
    if (!reporter) {
      EnqueueDelayedNote(new DelayedNote(data));
      return NS_OK;
    }

    
    
    
    nsCString escapedData;
    nsresult rv = EscapeAnnotation(NS_LITERAL_CSTRING("Notes"), data, escapedData);
    if (NS_FAILED(rv))
      return rv;

    if (!reporter->SendAppendAppNotes(escapedData))
      return NS_ERROR_FAILURE;
    return NS_OK;
  }

  MutexAutoLock lock(*notesFieldLock);

  notesField->Append(data);
  return AnnotateCrashReport(NS_LITERAL_CSTRING("Notes"), *notesField);
}


bool GetAnnotation(const nsACString& key, nsACString& data)
{
  if (!gExceptionHandler)
    return false;

  nsAutoCString entry;
  if (!crashReporterAPIData_Hash->Get(key, &entry))
    return false;

  data = entry;
  return true;
}

nsresult RegisterAppMemory(void* ptr, size_t length)
{
  if (!GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

#if defined(XP_LINUX) || defined(XP_WIN32)
  gExceptionHandler->RegisterAppMemory(ptr, length);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

nsresult UnregisterAppMemory(void* ptr)
{
  if (!GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

#if defined(XP_LINUX) || defined(XP_WIN32)
  gExceptionHandler->UnregisterAppMemory(ptr);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

bool GetServerURL(nsACString& aServerURL)
{
  if (!gExceptionHandler)
    return false;

  return GetAnnotation(NS_LITERAL_CSTRING("ServerURL"), aServerURL);
}

nsresult SetServerURL(const nsACString& aServerURL)
{
  
  
  return AnnotateCrashReport(NS_LITERAL_CSTRING("ServerURL"),
                             aServerURL);
}

nsresult
SetRestartArgs(int argc, char** argv)
{
  if (!gExceptionHandler)
    return NS_OK;

  int i;
  nsAutoCString envVar;
  char *env;
  char *argv0 = getenv("MOZ_APP_LAUNCHER");
  for (i = 0; i < argc; i++) {
    envVar = "MOZ_CRASHREPORTER_RESTART_ARG_";
    envVar.AppendInt(i);
    envVar += "=";
    if (argv0 && i == 0) {
      
      envVar += argv0;
    } else {
      envVar += argv[i];
    }

    
    
    env = ToNewCString(envVar);
    if (!env)
      return NS_ERROR_OUT_OF_MEMORY;

    PR_SetEnv(env);
  }

  
  envVar = "MOZ_CRASHREPORTER_RESTART_ARG_";
  envVar.AppendInt(i);
  envVar += "=";

  
  
  env = ToNewCString(envVar);
  if (!env)
    return NS_ERROR_OUT_OF_MEMORY;

  PR_SetEnv(env);

  
  const char *appfile = PR_GetEnv("XUL_APP_FILE");
  if (appfile && *appfile) {
    envVar = "MOZ_CRASHREPORTER_RESTART_XUL_APP_FILE=";
    envVar += appfile;
    env = ToNewCString(envVar);
    PR_SetEnv(env);
  }

  return NS_OK;
}

#ifdef XP_WIN32
nsresult WriteMinidumpForException(EXCEPTION_POINTERS* aExceptionInfo)
{
  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  return gExceptionHandler->WriteMinidumpForException(aExceptionInfo) ? NS_OK : NS_ERROR_FAILURE;
}
#endif

#ifdef XP_LINUX
bool WriteMinidumpForSigInfo(int signo, siginfo_t* info, void* uc)
{
  if (!gExceptionHandler) {
    
    return false;
  }
  return gExceptionHandler->HandleSignal(signo, info, uc);
}
#endif

#ifdef XP_MACOSX
nsresult AppendObjCExceptionInfoToAppNotes(void *inException)
{
  nsAutoCString excString;
  GetObjCExceptionInfo(inException, excString);
  AppendAppNotesToCrashReport(excString);
  return NS_OK;
}
#endif





static nsresult PrefSubmitReports(bool* aSubmitReports, bool writePref)
{
  nsresult rv;
#if defined(XP_WIN32)
  



  nsCOMPtr<nsIXULAppInfo> appinfo =
    do_GetService("@mozilla.org/xre/app-info;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString appVendor, appName;
  rv = appinfo->GetVendor(appVendor);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = appinfo->GetName(appName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWindowsRegKey> regKey
    (do_CreateInstance("@mozilla.org/windows-registry-key;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString regPath;

  regPath.AppendLiteral("Software\\");

  
  
  
  
  if(!appVendor.IsEmpty()) {
    regPath.Append(appVendor);
    regKey->Create(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                   NS_ConvertUTF8toUTF16(regPath),
                   nsIWindowsRegKey::ACCESS_SET_VALUE);
    regPath.Append('\\');
  }

  
  regPath.Append(appName);
  regKey->Create(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                 NS_ConvertUTF8toUTF16(regPath),
                 nsIWindowsRegKey::ACCESS_SET_VALUE);
  regPath.Append('\\');

  
  regPath.AppendLiteral("Crash Reporter");
  regKey->Create(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                 NS_ConvertUTF8toUTF16(regPath),
                 nsIWindowsRegKey::ACCESS_SET_VALUE);

  
  
  if (writePref) {
    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                      NS_ConvertUTF8toUTF16(regPath),
                      nsIWindowsRegKey::ACCESS_SET_VALUE);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t value = *aSubmitReports ? 1 : 0;
    rv = regKey->WriteIntValue(NS_LITERAL_STRING("SubmitCrashReport"), value);
    regKey->Close();
    return rv;
  }

  
  
  
  
  uint32_t value;
  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                    NS_ConvertUTF8toUTF16(regPath),
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_SUCCEEDED(rv)) {
    rv = regKey->ReadIntValue(NS_LITERAL_STRING("SubmitCrashReport"), &value);
    regKey->Close();
    if (NS_SUCCEEDED(rv)) {
      *aSubmitReports = !!value;
      return NS_OK;
    }
  }

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                    NS_ConvertUTF8toUTF16(regPath),
                    nsIWindowsRegKey::ACCESS_QUERY_VALUE);
  if (NS_FAILED(rv)) {
    *aSubmitReports = true;
    return NS_OK;
  }
  
  rv = regKey->ReadIntValue(NS_LITERAL_STRING("SubmitCrashReport"), &value);
  
  if (NS_FAILED(rv)) {
    value = 1;
    rv = NS_OK;
  }
  regKey->Close();

  *aSubmitReports = !!value;
  return NS_OK;
#elif defined(XP_MACOSX)
  rv = NS_OK;
  if (writePref) {
    CFPropertyListRef cfValue = (CFPropertyListRef)(*aSubmitReports ? kCFBooleanTrue : kCFBooleanFalse);
    ::CFPreferencesSetAppValue(CFSTR("submitReport"),
                               cfValue,
                               reporterClientAppID);
    if (!::CFPreferencesAppSynchronize(reporterClientAppID))
      rv = NS_ERROR_FAILURE;
  }
  else {
    *aSubmitReports = true;
    Boolean keyExistsAndHasValidFormat = false;
    Boolean prefValue = ::CFPreferencesGetAppBooleanValue(CFSTR("submitReport"),
                                                          reporterClientAppID,
                                                          &keyExistsAndHasValidFormat);
    if (keyExistsAndHasValidFormat)
      *aSubmitReports = !!prefValue;
  }
  return rv;
#elif defined(XP_UNIX)
  



  nsCOMPtr<nsIFile> reporterINI;
  rv = NS_GetSpecialDirectory("UAppData", getter_AddRefs(reporterINI));
  NS_ENSURE_SUCCESS(rv, rv);
  reporterINI->AppendNative(NS_LITERAL_CSTRING("Crash Reports"));
  reporterINI->AppendNative(NS_LITERAL_CSTRING("crashreporter.ini"));

  bool exists;
  rv = reporterINI->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    if (!writePref) {
        
        *aSubmitReports = true;
        return NS_OK;
    }
    
    rv = reporterINI->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIINIParserFactory> iniFactory =
    do_GetService("@mozilla.org/xpcom/ini-processor-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIINIParser> iniParser;
  rv = iniFactory->CreateINIParser(reporterINI,
                                   getter_AddRefs(iniParser));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (writePref) {
    nsCOMPtr<nsIINIParserWriter> iniWriter = do_QueryInterface(iniParser);
    NS_ENSURE_TRUE(iniWriter, NS_ERROR_FAILURE);

    rv = iniWriter->SetString(NS_LITERAL_CSTRING("Crash Reporter"),
                              NS_LITERAL_CSTRING("SubmitReport"),
                              *aSubmitReports ?  NS_LITERAL_CSTRING("1") :
                                                 NS_LITERAL_CSTRING("0"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = iniWriter->WriteFile(nullptr, 0);
    return rv;
  }
  
  nsAutoCString submitReportValue;
  rv = iniParser->GetString(NS_LITERAL_CSTRING("Crash Reporter"),
                            NS_LITERAL_CSTRING("SubmitReport"),
                            submitReportValue);

  
  if (NS_FAILED(rv))
    *aSubmitReports = true;
  else if (submitReportValue.EqualsASCII("0"))
    *aSubmitReports = false;
  else
    *aSubmitReports = true;

  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

nsresult GetSubmitReports(bool* aSubmitReports)
{
    return PrefSubmitReports(aSubmitReports, false);
}

nsresult SetSubmitReports(bool aSubmitReports)
{
    nsresult rv;

    nsCOMPtr<nsIObserverService> obsServ =
      mozilla::services::GetObserverService();
    if (!obsServ) {
      return NS_ERROR_FAILURE;
    }

    rv = PrefSubmitReports(&aSubmitReports, true);
    if (NS_FAILED(rv)) {
      return rv;
    }

    obsServ->NotifyObservers(nullptr, "submit-reports-pref-changed", nullptr);
    return NS_OK;
}

static void
SetCrashEventsDir(nsIFile* aDir)
{
  nsCOMPtr<nsIFile> eventsDir = aDir;

  const char *env = PR_GetEnv("CRASHES_EVENTS_DIR");
  if (env && *env) {
    NS_NewNativeLocalFile(nsDependentCString(env),
                          false, getter_AddRefs(eventsDir));
    EnsureDirectoryExists(eventsDir);
  }

  if (eventsDirectory) {
    free(eventsDirectory);
  }

#ifdef XP_WIN
  nsString path;
  eventsDir->GetPath(path);
  eventsDirectory = reinterpret_cast<wchar_t*>(ToNewUnicode(path));

  
  nsAutoString eventsDirEnv(NS_LITERAL_STRING("MOZ_CRASHREPORTER_EVENTS_DIRECTORY="));
  eventsDirEnv.Append(path);
  _wputenv(eventsDirEnv.get());
#else
  nsCString path;
  eventsDir->GetNativePath(path);
  eventsDirectory = ToNewCString(path);

  
  nsAutoCString eventsDirEnv("MOZ_CRASHREPORTER_EVENTS_DIRECTORY=");
  eventsDirEnv.Append(path);

  
  
  char* oldEventsEnv = eventsEnv;
  eventsEnv = ToNewCString(eventsDirEnv);
  PR_SetEnv(eventsEnv);

  if (oldEventsEnv) {
    free(oldEventsEnv);
  }
#endif
}

void
SetProfileDirectory(nsIFile* aDir)
{
  nsCOMPtr<nsIFile> dir;
  aDir->Clone(getter_AddRefs(dir));

  dir->Append(NS_LITERAL_STRING("crashes"));
  EnsureDirectoryExists(dir);
  dir->Append(NS_LITERAL_STRING("events"));
  EnsureDirectoryExists(dir);
  SetCrashEventsDir(dir);
}

void
SetUserAppDataDirectory(nsIFile* aDir)
{
  nsCOMPtr<nsIFile> dir;
  aDir->Clone(getter_AddRefs(dir));

  dir->Append(NS_LITERAL_STRING("Crash Reports"));
  EnsureDirectoryExists(dir);
  dir->Append(NS_LITERAL_STRING("events"));
  EnsureDirectoryExists(dir);
  SetCrashEventsDir(dir);
}

void
UpdateCrashEventsDir()
{
  const char *env = PR_GetEnv("CRASHES_EVENTS_DIR");
  if (env && *env) {
    SetCrashEventsDir(nullptr);
  }

  nsCOMPtr<nsIFile> eventsDir;
  nsresult rv = NS_GetSpecialDirectory("ProfD", getter_AddRefs(eventsDir));
  if (NS_SUCCEEDED(rv)) {
    SetProfileDirectory(eventsDir);
    return;
  }

  rv = NS_GetSpecialDirectory("UAppData", getter_AddRefs(eventsDir));
  if (NS_SUCCEEDED(rv)) {
    SetUserAppDataDirectory(eventsDir);
    return;
  }

  NS_WARNING("Couldn't get the user appdata directory. Crash events may not be produced.");
}

bool GetCrashEventsDir(nsAString& aPath)
{
  if (!eventsDirectory) {
    return false;
  }

  aPath = CONVERT_XP_CHAR_TO_UTF16(eventsDirectory);
  return true;
}

void
SetMemoryReportFile(nsIFile* aFile)
{
  if (!gExceptionHandler) {
    return;
  }
#ifdef XP_WIN
  nsString path;
  aFile->GetPath(path);
  memoryReportPath = reinterpret_cast<wchar_t*>(ToNewUnicode(path));
#else
  nsCString path;
  aFile->GetNativePath(path);
  memoryReportPath = ToNewCString(path);
#endif
}

static void
FindPendingDir()
{
  if (pendingDirectory)
    return;

  nsCOMPtr<nsIFile> pendingDir;
  nsresult rv = NS_GetSpecialDirectory("UAppData", getter_AddRefs(pendingDir));
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't get the user appdata directory, crash dumps will go in an unusual location");
  }
  else {
    pendingDir->Append(NS_LITERAL_STRING("Crash Reports"));
    pendingDir->Append(NS_LITERAL_STRING("pending"));

#ifdef XP_WIN
    nsString path;
    pendingDir->GetPath(path);
    pendingDirectory = reinterpret_cast<wchar_t*>(ToNewUnicode(path));
#else
    nsCString path;
    pendingDir->GetNativePath(path);
    pendingDirectory = ToNewCString(path);
#endif
  }
}




static bool
GetPendingDir(nsIFile** dir)
{
  
  if (!pendingDirectory) {
    return false;
  }

  nsCOMPtr<nsIFile> pending = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
  if (!pending) {
    NS_WARNING("Can't set up pending directory during shutdown.");
    return false;
  }
#ifdef XP_WIN
  pending->InitWithPath(nsDependentString(pendingDirectory));
#else
  pending->InitWithNativePath(nsDependentCString(pendingDirectory));
#endif
  pending.swap(*dir);
  return true;
}







static bool
GetMinidumpLimboDir(nsIFile** dir)
{
  if (ShouldReport()) {
    return GetPendingDir(dir);
  }
  else {
#ifndef XP_LINUX
    CreateFileFromPath(gExceptionHandler->dump_path(), dir);
#else
    CreateFileFromPath(gExceptionHandler->minidump_descriptor().directory(),
                       dir);
#endif
    return nullptr != *dir;
  }
}

void
DeleteMinidumpFilesForID(const nsAString& id)
{
  nsCOMPtr<nsIFile> minidumpFile;
  GetMinidumpForID(id, getter_AddRefs(minidumpFile));
  bool exists = false;
  if (minidumpFile && NS_SUCCEEDED(minidumpFile->Exists(&exists)) && exists) {
    nsCOMPtr<nsIFile> childExtraFile;
    GetExtraFileForMinidump(minidumpFile, getter_AddRefs(childExtraFile));
    if (childExtraFile) {
      childExtraFile->Remove(false);
    }
    minidumpFile->Remove(false);
  }
}

bool
GetMinidumpForID(const nsAString& id, nsIFile** minidump)
{
  if (!GetMinidumpLimboDir(minidump))
    return false;
  (*minidump)->Append(id + NS_LITERAL_STRING(".dmp")); 
  return true;
}

bool
GetIDFromMinidump(nsIFile* minidump, nsAString& id)
{
  if (minidump && NS_SUCCEEDED(minidump->GetLeafName(id))) {
    id.Replace(id.Length() - 4, 4, NS_LITERAL_STRING(""));
    return true;
  }
  return false;
}

bool
GetExtraFileForID(const nsAString& id, nsIFile** extraFile)
{
  if (!GetMinidumpLimboDir(extraFile))
    return false;
  (*extraFile)->Append(id + NS_LITERAL_STRING(".extra"));
  return true;
}

bool
GetExtraFileForMinidump(nsIFile* minidump, nsIFile** extraFile)
{
  nsAutoString leafName;
  nsresult rv = minidump->GetLeafName(leafName);
  if (NS_FAILED(rv))
    return false;

  nsCOMPtr<nsIFile> extraF;
  rv = minidump->Clone(getter_AddRefs(extraF));
  if (NS_FAILED(rv))
    return false;

  leafName.Replace(leafName.Length() - 3, 3,
                   NS_LITERAL_STRING("extra"));
  rv = extraF->SetLeafName(leafName);
  if (NS_FAILED(rv))
    return false;

  *extraFile = nullptr;
  extraF.swap(*extraFile);
  return true;
}

bool
AppendExtraData(const nsAString& id, const AnnotationTable& data)
{
  nsCOMPtr<nsIFile> extraFile;
  if (!GetExtraFileForID(id, getter_AddRefs(extraFile)))
    return false;
  return AppendExtraData(extraFile, data);
}




struct Blacklist {
  Blacklist() : mItems(nullptr), mLen(0) { }
  Blacklist(const char** items, int len) : mItems(items), mLen(len) { }

  bool Contains(const nsACString& key) const {
    for (int i = 0; i < mLen; ++i)
      if (key.EqualsASCII(mItems[i]))
        return true;
    return false;
  }

  const char** mItems;
  const int mLen;
};

struct EnumerateAnnotationsContext {
  const Blacklist& blacklist;
  PRFileDesc* fd;
};

static void
WriteAnnotation(PRFileDesc* fd, const nsACString& key, const nsACString& value)
{
  PR_Write(fd, key.BeginReading(), key.Length());
  PR_Write(fd, "=", 1);
  PR_Write(fd, value.BeginReading(), value.Length());
  PR_Write(fd, "\n", 1);
}

static PLDHashOperator
EnumerateAnnotations(const nsACString& key,
                     nsCString entry,
                     void* userData)
{
  EnumerateAnnotationsContext* ctx =
    static_cast<EnumerateAnnotationsContext*>(userData);
  const Blacklist& blacklist = ctx->blacklist;

  
  if (blacklist.Contains(key))
      return PL_DHASH_NEXT;

  WriteAnnotation(ctx->fd, key, entry);

  return PL_DHASH_NEXT;
}

static bool
WriteExtraData(nsIFile* extraFile,
               const AnnotationTable& data,
               const Blacklist& blacklist,
               bool writeCrashTime=false,
               bool truncate=false)
{
  PRFileDesc* fd;
  int truncOrAppend = truncate ? PR_TRUNCATE : PR_APPEND;
  nsresult rv = 
    extraFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | truncOrAppend,
                                0600, &fd);
  if (NS_FAILED(rv))
    return false;

  EnumerateAnnotationsContext ctx = { blacklist, fd };
  data.EnumerateRead(EnumerateAnnotations, &ctx);

  if (writeCrashTime) {
    time_t crashTime = time(nullptr);
    char crashTimeString[32];
    XP_TTOA(crashTime, crashTimeString, 10);

    WriteAnnotation(fd,
                    nsDependentCString("CrashTime"),
                    nsDependentCString(crashTimeString));
  }

  PR_Close(fd);
  return true;
}

bool
AppendExtraData(nsIFile* extraFile, const AnnotationTable& data)
{
  return WriteExtraData(extraFile, data, Blacklist());
}


static bool
WriteExtraForMinidump(nsIFile* minidump,
                      const Blacklist& blacklist,
                      nsIFile** extraFile)
{
  nsCOMPtr<nsIFile> extra;
  if (!GetExtraFileForMinidump(minidump, getter_AddRefs(extra)))
    return false;

  if (!WriteExtraData(extra, *crashReporterAPIData_Hash,
                      blacklist,
                      true ,
                      true ))
    return false;

  *extraFile = nullptr;
  extra.swap(*extraFile);

  return true;
}



static bool
MoveToPending(nsIFile* dumpFile, nsIFile* extraFile)
{
  nsCOMPtr<nsIFile> pendingDir;
  if (!GetPendingDir(getter_AddRefs(pendingDir)))
    return false;

  if (NS_FAILED(dumpFile->MoveTo(pendingDir, EmptyString()))) {
    return false;
  }

  if (extraFile && NS_FAILED(extraFile->MoveTo(pendingDir, EmptyString()))) {
    return false;
  }

  return true;
}

static void
OnChildProcessDumpRequested(void* aContext,
#ifdef XP_MACOSX
                            const ClientInfo& aClientInfo,
                            const xpstring& aFilePath
#else
                            const ClientInfo* aClientInfo,
                            const xpstring* aFilePath
#endif
                            )
{
  nsCOMPtr<nsIFile> minidump;
  nsCOMPtr<nsIFile> extraFile;

  
  
  
  MutexAutoLock lock(*dumpSafetyLock);
  if (!isSafeToDump)
    return;

  CreateFileFromPath(
#ifdef XP_MACOSX
                     aFilePath,
#else
                     *aFilePath,
#endif
                     getter_AddRefs(minidump));

  if (!WriteExtraForMinidump(minidump,
                             Blacklist(kSubprocessBlacklist,
                                       ArrayLength(kSubprocessBlacklist)),
                             getter_AddRefs(extraFile)))
    return;

  if (ShouldReport())
    MoveToPending(minidump, extraFile);

  {
    uint32_t pid =
#ifdef XP_MACOSX
      aClientInfo.pid();
#else
      aClientInfo->pid();
#endif

#ifdef MOZ_CRASHREPORTER_INJECTOR
    bool runCallback;
#endif
    {
      MutexAutoLock lock(*dumpMapLock);
      ChildProcessData* pd = pidToMinidump->PutEntry(pid);
      MOZ_ASSERT(!pd->minidump);
      pd->minidump = minidump;
      pd->sequence = ++crashSequence;
#ifdef MOZ_CRASHREPORTER_INJECTOR
      runCallback = nullptr != pd->callback;
#endif
    }
#ifdef MOZ_CRASHREPORTER_INJECTOR
    if (runCallback)
      NS_DispatchToMainThread(new ReportInjectedCrash(pid));
#endif
  }
}

static bool
OOPInitialized()
{
  return pidToMinidump != nullptr;
}

#ifdef XP_MACOSX
static bool ChildFilter(void *context) {
  mozilla::IOInterposer::Disable();
  return true;
}
#endif

void
OOPInit()
{
  class ProxyToMainThread : public nsRunnable
  {
  public:
    NS_IMETHOD Run() {
      OOPInit();
      return NS_OK;
    }
  };
  if (!NS_IsMainThread()) {
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    mozilla::SyncRunnable::DispatchToThread(mainThread, new ProxyToMainThread());
    return;
  }

  if (OOPInitialized())
    return;

  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(gExceptionHandler != nullptr,
             "attempt to initialize OOP crash reporter before in-process crashreporter!");

#if defined(XP_WIN)
  childCrashNotifyPipe =
    PR_smprintf("\\\\.\\pipe\\gecko-crash-server-pipe.%i",
                static_cast<int>(::GetCurrentProcessId()));

  const std::wstring dumpPath = gExceptionHandler->dump_path();
  crashServer = new CrashGenerationServer(
    NS_ConvertASCIItoUTF16(childCrashNotifyPipe).get(),
    nullptr,                    
    nullptr, nullptr,           
    OnChildProcessDumpRequested, nullptr,
    nullptr, nullptr,           
    nullptr, nullptr,           
    true,                       
    &dumpPath);

#elif defined(XP_LINUX)
  if (!CrashGenerationServer::CreateReportChannel(&serverSocketFd,
                                                  &clientSocketFd))
    NS_RUNTIMEABORT("can't create crash reporter socketpair()");

  const std::string dumpPath =
      gExceptionHandler->minidump_descriptor().directory();
  crashServer = new CrashGenerationServer(
    serverSocketFd,
    OnChildProcessDumpRequested, nullptr,
    nullptr, nullptr,           
    true,
    &dumpPath);

#elif defined(XP_MACOSX)
  childCrashNotifyPipe =
    PR_smprintf("gecko-crash-server-pipe.%i",
                static_cast<int>(getpid()));
  const std::string dumpPath = gExceptionHandler->dump_path();

  crashServer = new CrashGenerationServer(
    childCrashNotifyPipe,
    ChildFilter,
    nullptr,
    OnChildProcessDumpRequested, nullptr,
    nullptr, nullptr,
    true, 
    dumpPath);
#endif

  if (!crashServer->Start())
    NS_RUNTIMEABORT("can't start crash reporter server()");

  pidToMinidump = new ChildMinidumpMap();

  dumpMapLock = new Mutex("CrashReporter::dumpMapLock");

  FindPendingDir();
  UpdateCrashEventsDir();
}

static void
OOPDeinit()
{
  if (!OOPInitialized()) {
    NS_WARNING("OOPDeinit() without successful OOPInit()");
    return;
  }

#ifdef MOZ_CRASHREPORTER_INJECTOR
  if (sInjectorThread) {
    sInjectorThread->Shutdown();
    NS_RELEASE(sInjectorThread);
  }
#endif

  delete crashServer;
  crashServer = nullptr;

  delete dumpMapLock;
  dumpMapLock = nullptr;

  delete pidToMinidump;
  pidToMinidump = nullptr;

#if defined(XP_WIN)
  PR_Free(childCrashNotifyPipe);
  childCrashNotifyPipe = nullptr;
#endif
}

#if defined(XP_WIN) || defined(XP_MACOSX)

const char*
GetChildNotificationPipe()
{
  if (!GetEnabled())
    return kNullNotifyPipe;

  MOZ_ASSERT(OOPInitialized());

  return childCrashNotifyPipe;
}
#endif

#ifdef MOZ_CRASHREPORTER_INJECTOR
void
InjectCrashReporterIntoProcess(DWORD processID, InjectorCrashCallback* cb)
{
  if (!GetEnabled())
    return;

  if (!OOPInitialized())
    OOPInit();

  if (!sInjectorThread) {
    if (NS_FAILED(NS_NewThread(&sInjectorThread)))
      return;
  }

  {
    MutexAutoLock lock(*dumpMapLock);
    ChildProcessData* pd = pidToMinidump->PutEntry(processID);
    MOZ_ASSERT(!pd->minidump && !pd->callback);
    pd->callback = cb;
  }

  nsCOMPtr<nsIRunnable> r = new InjectCrashRunnable(processID);
  sInjectorThread->Dispatch(r, nsIEventTarget::DISPATCH_NORMAL);
}

NS_IMETHODIMP
ReportInjectedCrash::Run()
{
  
  if (!OOPInitialized())
    return NS_OK;

  InjectorCrashCallback* cb;
  {
    MutexAutoLock lock(*dumpMapLock);
    ChildProcessData* pd = pidToMinidump->GetEntry(mPID);
    if (!pd || !pd->callback)
      return NS_OK;

    MOZ_ASSERT(pd->minidump);

    cb = pd->callback;
  }

  cb->OnCrash(mPID);
  return NS_OK;
}

void
UnregisterInjectorCallback(DWORD processID)
{
  if (!OOPInitialized())
    return;

  MutexAutoLock lock(*dumpMapLock);
  pidToMinidump->RemoveEntry(processID);
}

#endif 

bool
CheckForLastRunCrash()
{
  if (lastRunCrashID)
    return true;

  
  
  nsCOMPtr<nsIFile> lastCrashFile;
  CreateFileFromPath(crashMarkerFilename,
                     getter_AddRefs(lastCrashFile));

  bool exists;
  if (NS_FAILED(lastCrashFile->Exists(&exists)) || !exists) {
    return false;
  }

  nsAutoCString lastMinidump_contents;
  if (NS_FAILED(GetFileContents(lastCrashFile, lastMinidump_contents))) {
    return false;
  }
  lastCrashFile->Remove(false);

#ifdef XP_WIN
  
  nsDependentString lastMinidump(
      reinterpret_cast<const char16_t*>(lastMinidump_contents.get()));
#else
  nsAutoCString lastMinidump = lastMinidump_contents;
#endif
  nsCOMPtr<nsIFile> lastMinidumpFile;
  CreateFileFromPath(lastMinidump.get(),
                     getter_AddRefs(lastMinidumpFile));

  if (!lastMinidumpFile || NS_FAILED(lastMinidumpFile->Exists(&exists)) || !exists) {
    return false;
  }

  nsCOMPtr<nsIFile> lastExtraFile;
  if (!GetExtraFileForMinidump(lastMinidumpFile,
                               getter_AddRefs(lastExtraFile))) {
    return false;
  }

  FindPendingDir();

  
  if (!MoveToPending(lastMinidumpFile, lastExtraFile)) {
    return false;
  }

  lastRunCrashID = new nsString();
  return GetIDFromMinidump(lastMinidumpFile, *lastRunCrashID);
}

bool
GetLastRunCrashID(nsAString& id)
{
  if (!lastRunCrashID_checked) {
    CheckForLastRunCrash();
    lastRunCrashID_checked = true;
  }

  if (!lastRunCrashID) {
    return false;
  }

  id = *lastRunCrashID;
  return true;
}

#if defined(XP_WIN)

bool
SetRemoteExceptionHandler(const nsACString& crashPipe)
{
  
  if (crashPipe.Equals(kNullNotifyPipe))
    return true;

  MOZ_ASSERT(!gExceptionHandler, "crash client already init'd");

  gExceptionHandler = new google_breakpad::
    ExceptionHandler(L"",
                     FPEFilter,
                     nullptr,    
                     nullptr,    
                     google_breakpad::ExceptionHandler::HANDLER_ALL,
                     MiniDumpNormal,
                     NS_ConvertASCIItoUTF16(crashPipe).get(),
                     nullptr);
#ifdef XP_WIN
  gExceptionHandler->set_handle_debug_exceptions(true);
#endif

  
  return gExceptionHandler->IsOutOfProcess();
}


#elif defined(XP_LINUX)


bool
CreateNotificationPipeForChild(int* childCrashFd, int* childCrashRemapFd)
{
  if (!GetEnabled()) {
    *childCrashFd = -1;
    *childCrashRemapFd = -1;
    return true;
  }

  MOZ_ASSERT(OOPInitialized());

  *childCrashFd = clientSocketFd;
  *childCrashRemapFd = kMagicChildCrashReportFd;

  return true;
}


bool
SetRemoteExceptionHandler()
{
  MOZ_ASSERT(!gExceptionHandler, "crash client already init'd");

#ifndef XP_LINUX
  xpstring path = "";
#else
  
  google_breakpad::MinidumpDescriptor path(".");
#endif
  gExceptionHandler = new google_breakpad::
    ExceptionHandler(path,
                     nullptr,    
                     nullptr,    
                     nullptr,    
                     true,       
                     kMagicChildCrashReportFd);

  if (gDelayedAnnotations) {
    for (uint32_t i = 0; i < gDelayedAnnotations->Length(); i++) {
      gDelayedAnnotations->ElementAt(i)->Run();
    }
    delete gDelayedAnnotations;
  }

  
  return gExceptionHandler->IsOutOfProcess();
}


#elif defined(XP_MACOSX)

bool
SetRemoteExceptionHandler(const nsACString& crashPipe)
{
  
  if (crashPipe.Equals(kNullNotifyPipe))
    return true;

  MOZ_ASSERT(!gExceptionHandler, "crash client already init'd");

  gExceptionHandler = new google_breakpad::
    ExceptionHandler("",
                     Filter,
                     nullptr,    
                     nullptr,    
                     true,       
                     crashPipe.BeginReading());

  
  return gExceptionHandler->IsOutOfProcess();
}
#endif  


bool
TakeMinidumpForChild(uint32_t childPid, nsIFile** dump, uint32_t* aSequence)
{
  if (!GetEnabled())
    return false;

  MutexAutoLock lock(*dumpMapLock);

  ChildProcessData* pd = pidToMinidump->GetEntry(childPid);
  if (!pd)
    return false;

  NS_IF_ADDREF(*dump = pd->minidump);
  if (aSequence) {
    *aSequence = pd->sequence;
  }
  
  pidToMinidump->RemoveEntry(childPid);

  return !!*dump;
}





void
RenameAdditionalHangMinidump(nsIFile* minidump, nsIFile* childMinidump,
                             const nsACString& name)
{
  nsCOMPtr<nsIFile> directory;
  childMinidump->GetParent(getter_AddRefs(directory));
  if (!directory)
    return;

  nsAutoCString leafName;
  childMinidump->GetNativeLeafName(leafName);

  
  leafName.Insert(NS_LITERAL_CSTRING("-") + name, leafName.Length() - 4);

  if (NS_FAILED(minidump->MoveToNative(directory, leafName))) {
    NS_WARNING("RenameAdditionalHangMinidump failed to move minidump.");
  }
}

static bool
PairedDumpCallback(
#ifdef XP_LINUX
                   const MinidumpDescriptor& descriptor,
#else
                   const XP_CHAR* dump_path,
                   const XP_CHAR* minidump_id,
#endif
                   void* context,
#ifdef XP_WIN32
                   EXCEPTION_POINTERS* ,
                   MDRawAssertionInfo* ,
#endif
                   bool succeeded)
{
  nsCOMPtr<nsIFile>& minidump = *static_cast< nsCOMPtr<nsIFile>* >(context);

  xpstring dump;
#ifdef XP_LINUX
  dump = descriptor.path();
#else
  dump = dump_path;
  dump += XP_PATH_SEPARATOR;
  dump += minidump_id;
  dump += dumpFileExtension;
#endif

  CreateFileFromPath(dump, getter_AddRefs(minidump));
  return true;
}

static bool
PairedDumpCallbackExtra(
#ifdef XP_LINUX
                        const MinidumpDescriptor& descriptor,
#else
                        const XP_CHAR* dump_path,
                        const XP_CHAR* minidump_id,
#endif
                        void* context,
#ifdef XP_WIN32
                        EXCEPTION_POINTERS* ,
                        MDRawAssertionInfo* ,
#endif
                        bool succeeded)
{
  PairedDumpCallback(
#ifdef XP_LINUX
                     descriptor,
#else
                     dump_path, minidump_id,
#endif
                     context,
#ifdef XP_WIN32
                     nullptr, nullptr,
#endif
                     succeeded);

  nsCOMPtr<nsIFile>& minidump = *static_cast< nsCOMPtr<nsIFile>* >(context);

  nsCOMPtr<nsIFile> extra;
  return WriteExtraForMinidump(minidump, Blacklist(), getter_AddRefs(extra));
}

ThreadId
CurrentThreadId()
{
#if defined(XP_WIN)
  return ::GetCurrentThreadId();
#elif defined(XP_LINUX)
  return sys_gettid();
#elif defined(XP_MACOSX)
  
  thread_act_port_array_t   threads_for_task;
  mach_msg_type_number_t    thread_count;

  if (task_threads(mach_task_self(), &threads_for_task, &thread_count))
    return -1;

  for (unsigned int i = 0; i < thread_count; ++i) {
    if (threads_for_task[i] == mach_thread_self())
      return i;
  }
  abort();
#else
#  error "Unsupported platform"
#endif
}

#ifdef XP_MACOSX
static mach_port_t
GetChildThread(ProcessHandle childPid, ThreadId childBlamedThread)
{
  mach_port_t childThread = MACH_PORT_NULL;
  thread_act_port_array_t   threads_for_task;
  mach_msg_type_number_t    thread_count;

  if (task_threads(childPid, &threads_for_task, &thread_count)
      == KERN_SUCCESS && childBlamedThread < thread_count) {
    childThread = threads_for_task[childBlamedThread];
  }

  return childThread;
}
#endif

bool TakeMinidump(nsIFile** aResult, bool aMoveToPending)
{
  if (!GetEnabled())
    return false;

  xpstring dump_path;
#ifndef XP_LINUX
  dump_path = gExceptionHandler->dump_path();
#else
  dump_path = gExceptionHandler->minidump_descriptor().directory();
#endif

  
  if (!google_breakpad::ExceptionHandler::WriteMinidump(
         dump_path,
#ifdef XP_MACOSX
         true,
#endif
         PairedDumpCallback,
         static_cast<void*>(aResult))) {
    return false;
  }

  if (aMoveToPending) {
    MoveToPending(*aResult, nullptr);
  }
  return true;
}

bool
CreateMinidumpsAndPair(ProcessHandle aTargetPid,
                       ThreadId aTargetBlamedThread,
                       const nsACString& aIncomingPairName,
                       nsIFile* aIncomingDumpToPair,
                       nsIFile** aMainDumpOut)
{
  if (!GetEnabled()) {
    return false;
  }

#ifdef XP_MACOSX
  mach_port_t targetThread = GetChildThread(aTargetPid, aTargetBlamedThread);
#else
  ThreadId targetThread = aTargetBlamedThread;
#endif

  xpstring dump_path;
#ifndef XP_LINUX
  dump_path = gExceptionHandler->dump_path();
#else
  dump_path = gExceptionHandler->minidump_descriptor().directory();
#endif

  
  nsCOMPtr<nsIFile> targetMinidump;
  if (!google_breakpad::ExceptionHandler::WriteMinidumpForChild(
         aTargetPid,
         targetThread,
         dump_path,
         PairedDumpCallbackExtra,
         static_cast<void*>(&targetMinidump)))
    return false;

  nsCOMPtr<nsIFile> targetExtra;
  GetExtraFileForMinidump(targetMinidump, getter_AddRefs(targetExtra));

  
  nsCOMPtr<nsIFile> incomingDump;
  if (aIncomingDumpToPair == nullptr) {
    if (!google_breakpad::ExceptionHandler::WriteMinidump(
        dump_path,
#ifdef XP_MACOSX
        true,
#endif
        PairedDumpCallback,
        static_cast<void*>(&incomingDump))) {

      targetMinidump->Remove(false);
      targetExtra->Remove(false);
      return false;
    }
  } else {
    incomingDump = aIncomingDumpToPair;
  }
  
  RenameAdditionalHangMinidump(incomingDump, targetMinidump, aIncomingPairName);

  if (ShouldReport()) {
    MoveToPending(targetMinidump, targetExtra);
    MoveToPending(incomingDump, nullptr);
  }

  targetMinidump.forget(aMainDumpOut);

  return true;
}

bool
CreateAdditionalChildMinidump(ProcessHandle childPid,
                              ThreadId childBlamedThread,
                              nsIFile* parentMinidump,
                              const nsACString& name)
{
  if (!GetEnabled())
    return false;

#ifdef XP_MACOSX
  mach_port_t childThread = GetChildThread(childPid, childBlamedThread);
#else
  ThreadId childThread = childBlamedThread;
#endif

  xpstring dump_path;
#ifndef XP_LINUX
  dump_path = gExceptionHandler->dump_path();
#else
  dump_path = gExceptionHandler->minidump_descriptor().directory();
#endif

  
  nsCOMPtr<nsIFile> childMinidump;
  if (!google_breakpad::ExceptionHandler::WriteMinidumpForChild(
         childPid,
         childThread,
         dump_path,
         PairedDumpCallback,
         static_cast<void*>(&childMinidump))) {
    return false;
  }

  RenameAdditionalHangMinidump(childMinidump, parentMinidump, name);

  return true;
}

bool
UnsetRemoteExceptionHandler()
{
  delete gExceptionHandler;
  gExceptionHandler = nullptr;
  return true;
}

#if defined(MOZ_WIDGET_ANDROID)
void AddLibraryMapping(const char* library_name,
                       uintptr_t   start_address,
                       size_t      mapping_length,
                       size_t      file_offset)
{
  if (!gExceptionHandler) {
    mapping_info info;
    info.name = library_name;
    info.start_address = start_address;
    info.length = mapping_length;
    info.file_offset = file_offset;
    library_mappings.push_back(info);
  }
  else {
    u_int8_t guid[sizeof(MDGUID)];
    google_breakpad::FileID::ElfFileIdentifierFromMappedFile((void const *)start_address, guid);
    gExceptionHandler->AddMappingInfo(library_name,
                                      guid,
                                      start_address,
                                      mapping_length,
                                      file_offset);
  }
}
#endif

} 
