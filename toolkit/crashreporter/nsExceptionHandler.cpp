






































#include "nsExceptionHandler.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include "nsIWindowsRegKey.h"
#if defined(MOZ_IPC)
#  include "client/windows/crash_generation/crash_generation_server.h"
#endif
#include "client/windows/handler/exception_handler.h"
#include <DbgHelp.h>
#include <string.h>
#elif defined(XP_MACOSX)
#include "client/mac/handler/exception_handler.h"
#include <string>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "mac_utils.h"
#elif defined(XP_LINUX)
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIINIParser.h"
#if defined(MOZ_IPC)
#  include "common/linux/linux_syscall_support.h"
#  include "client/linux/crash_generation/client_info.h"
#  include "client/linux/crash_generation/crash_generation_server.h"
#endif
#include "client/linux/handler/exception_handler.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(XP_SOLARIS)
#include "client/solaris/handler/exception_handler.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "Not yet implemented for this platform"
#endif 

#include <stdlib.h>
#include <time.h>
#include <prenv.h>
#include <prio.h>
#include <prmem.h>
#include "mozilla/Mutex.h"
#include "nsDebug.h"
#include "nsCRT.h"
#include "nsILocalFile.h"
#include "nsIFileStreams.h"
#include "nsInterfaceHashtable.h"
#include "prprf.h"
#include "nsIXULAppInfo.h"

#if defined(XP_MACOSX)
CFStringRef reporterClientAppID = CFSTR("org.mozilla.crashreporter");
#endif

#if defined(MOZ_IPC)
#include "nsIUUIDGenerator.h"

#if !defined(XP_MACOSX)
using google_breakpad::CrashGenerationServer;
using google_breakpad::ClientInfo;
#endif

using mozilla::Mutex;
using mozilla::MutexAutoLock;
#endif 

namespace CrashReporter {

#ifdef XP_WIN32
typedef wchar_t XP_CHAR;
typedef std::wstring xpstring;
#define CONVERT_UTF16_TO_XP_CHAR(x) x
#define CONVERT_XP_CHAR_TO_UTF16(x) x
#define XP_STRLEN(x) wcslen(x)
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
#else
typedef char XP_CHAR;
typedef std::string xpstring;
#define CONVERT_UTF16_TO_XP_CHAR(x) NS_ConvertUTF16toUTF8(x)
#define CONVERT_XP_CHAR_TO_UTF16(x) NS_ConvertUTF8toUTF16(x)
#define XP_STRLEN(x) strlen(x)
#define CRASH_REPORTER_FILENAME "crashreporter"
#define PATH_SEPARATOR "/"
#define XP_PATH_SEPARATOR "/"
#define XP_PATH_MAX PATH_MAX
#define XP_TTOA(time, buffer, base) sprintf(buffer, "%ld", time)
#endif 

static const XP_CHAR dumpFileExtension[] = {'.', 'd', 'm', 'p',
                                            '\0'}; 
static const XP_CHAR extraFileExtension[] = {'.', 'e', 'x', 't',
                                             'r', 'a', '\0'}; 

static google_breakpad::ExceptionHandler* gExceptionHandler = nsnull;

static XP_CHAR* crashReporterPath;


static bool doReport = true;


static bool showOSCrashReporter = false;


static time_t lastCrashTime = 0;

static XP_CHAR lastCrashTimeFilename[XP_PATH_MAX] = {0};


static const char kCrashTimeParameter[] = "CrashTime=";
static const int kCrashTimeParameterLen = sizeof(kCrashTimeParameter)-1;

static const char kTimeSinceLastCrashParameter[] = "SecondsSinceLastCrash=";
static const int kTimeSinceLastCrashParameterLen =
                                     sizeof(kTimeSinceLastCrashParameter)-1;


static AnnotationTable* crashReporterAPIData_Hash;
static nsCString* crashReporterAPIData = nsnull;
static nsCString* notesField = nsnull;

#if defined(MOZ_IPC)
#if !defined(XP_MACOSX)

static CrashGenerationServer* crashServer; 
#endif

#  if defined(XP_WIN)


static const char kNullNotifyPipe[] = "-";
static char* childCrashNotifyPipe;

#  elif defined(XP_LINUX)
static int serverSocketFd = -1;
static int clientSocketFd = -1;
static const int kMagicChildCrashReportFd = 42;
#  endif


static Mutex* dumpMapLock;
typedef nsInterfaceHashtable<nsUint32HashKey, nsILocalFile> ChildMinidumpMap;
static ChildMinidumpMap* pidToMinidump;



static const char* kSubprocessBlacklist[] = {
  "FramePoisonBase",
  "FramePoisonSize",
  "StartupTime",
  "URL"
};


#endif  

#ifdef XP_WIN
static void
CreateFileFromPath(const xpstring& path, nsILocalFile** file)
{
  NS_NewLocalFile(nsDependentString(path.c_str()), PR_FALSE, file);
}
#else
static void
CreateFileFromPath(const xpstring& path, nsILocalFile** file)
{
  NS_NewNativeLocalFile(nsDependentCString(path.c_str()), PR_FALSE, file);
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

bool MinidumpCallback(const XP_CHAR* dump_path,
                      const XP_CHAR* minidump_id,
                      void* context,
#ifdef XP_WIN32
                      EXCEPTION_POINTERS* exinfo,
                      MDRawAssertionInfo* assertion,
#endif
                      bool succeeded)
{
  bool returnValue = showOSCrashReporter ? false : succeeded;

  XP_CHAR minidumpPath[XP_PATH_MAX];
  int size = XP_PATH_MAX;
  XP_CHAR* p = Concat(minidumpPath, dump_path, &size);
  p = Concat(p, XP_PATH_SEPARATOR, &size);
  p = Concat(p, minidump_id, &size);
  Concat(p, dumpFileExtension, &size);

  XP_CHAR extraDataPath[XP_PATH_MAX];
  size = XP_PATH_MAX;
  p = Concat(extraDataPath, dump_path, &size);
  p = Concat(p, XP_PATH_SEPARATOR, &size);
  p = Concat(p, minidump_id, &size);
  Concat(p, extraFileExtension, &size);

  
  
  time_t crashTime = time(NULL);
  time_t timeSinceLastCrash = 0;
  
  char crashTimeString[32];
  int crashTimeStringLen = 0;
  char timeSinceLastCrashString[32];
  int timeSinceLastCrashStringLen = 0;

  XP_TTOA(crashTime, crashTimeString, 10);
  crashTimeStringLen = strlen(crashTimeString);
  if (lastCrashTime != 0) {
    timeSinceLastCrash = crashTime - lastCrashTime;
    XP_TTOA(timeSinceLastCrash, timeSinceLastCrashString, 10);
    timeSinceLastCrashStringLen = strlen(timeSinceLastCrashString);
  }
  
  if (lastCrashTimeFilename[0] != 0) {
#if defined(XP_WIN32)
    HANDLE hFile = CreateFile(lastCrashTimeFilename, GENERIC_WRITE, 0,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if(hFile != INVALID_HANDLE_VALUE) {
      DWORD nBytes;
      WriteFile(hFile, crashTimeString, crashTimeStringLen, &nBytes, NULL);
      CloseHandle(hFile);
    }
#elif defined(XP_UNIX)
    int fd = open(lastCrashTimeFilename,
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0600);
    if (fd != -1) {
      ssize_t ignored = write(fd, crashTimeString, crashTimeStringLen);
      (void)ignored;
      close(fd);
    }
#endif
  }

#if defined(XP_WIN32)
  XP_CHAR cmdLine[CMDLINE_SIZE];
  size = CMDLINE_SIZE;
  p = Concat(cmdLine, L"\"", &size);
  p = Concat(p, crashReporterPath, &size);
  p = Concat(p, L"\" \"", &size);
  p = Concat(p, minidumpPath, &size);
  Concat(p, L"\"", &size);

  if (!crashReporterAPIData->IsEmpty()) {
    
    HANDLE hFile = CreateFile(extraDataPath, GENERIC_WRITE, 0,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if(hFile != INVALID_HANDLE_VALUE) {
      DWORD nBytes;
      WriteFile(hFile, crashReporterAPIData->get(),
                crashReporterAPIData->Length(), &nBytes, NULL);
      WriteFile(hFile, kCrashTimeParameter, kCrashTimeParameterLen,
                &nBytes, NULL);
      WriteFile(hFile, crashTimeString, crashTimeStringLen, &nBytes, NULL);
      WriteFile(hFile, "\n", 1, &nBytes, NULL);
      if (timeSinceLastCrash != 0) {
        WriteFile(hFile, kTimeSinceLastCrashParameter,
                  kTimeSinceLastCrashParameterLen, &nBytes, NULL);
        WriteFile(hFile, timeSinceLastCrashString, timeSinceLastCrashStringLen,
                  &nBytes, NULL);
        WriteFile(hFile, "\n", 1, &nBytes, NULL);
      }
      CloseHandle(hFile);
    }
  }

  if (!doReport) {
    return returnValue;
  }

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOWNORMAL;
  ZeroMemory(&pi, sizeof(pi));

  if (CreateProcess(NULL, (LPWSTR)cmdLine, NULL, NULL, FALSE, 0,
                    NULL, NULL, &si, &pi)) {
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
  }
  
  TerminateProcess(GetCurrentProcess(), 1);
#elif defined(XP_UNIX)
  if (!crashReporterAPIData->IsEmpty()) {
    
    int fd = open(extraDataPath,
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0666);

    if (fd != -1) {
      
      ssize_t ignored = write(fd, crashReporterAPIData->get(),
                              crashReporterAPIData->Length());
      ignored = write(fd, kCrashTimeParameter, kCrashTimeParameterLen);
      ignored = write(fd, crashTimeString, crashTimeStringLen);
      ignored = write(fd, "\n", 1);
      if (timeSinceLastCrash != 0) {
        ignored = write(fd, kTimeSinceLastCrashParameter,
                        kTimeSinceLastCrashParameterLen);
        ignored = write(fd, timeSinceLastCrashString,
                        timeSinceLastCrashStringLen);
        ignored = write(fd, "\n", 1);
      }
      close (fd);
    }
  }

  if (!doReport) {
    return returnValue;
  }

  pid_t pid = fork();

  if (pid == -1)
    return false;
  else if (pid == 0) {
    
    
    unsetenv("LD_LIBRARY_PATH");
    (void) execl(crashReporterPath,
                 crashReporterPath, minidumpPath, (char*)0);
    _exit(1);
  }
#endif

 return returnValue;
}

#ifdef XP_WIN




static bool FPEFilter(void* context, EXCEPTION_POINTERS* exinfo,
                      MDRawAssertionInfo* assertion)
{
  if (!exinfo)
    return true;

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
  return true;
}
#endif 

static bool ShouldReport()
{
  
  
  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER_NO_REPORT");
  return !(envvar && *envvar);
}

nsresult SetExceptionHandler(nsILocalFile* aXREDirectory,
                             bool force)
{
  nsresult rv;

  if (gExceptionHandler)
    return NS_ERROR_ALREADY_INITIALIZED;

  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER_DISABLE");
  if (envvar && *envvar && !force)
    return NS_OK;

  
  
  doReport = ShouldReport();

  
  crashReporterAPIData = new nsCString();
  NS_ENSURE_TRUE(crashReporterAPIData, NS_ERROR_OUT_OF_MEMORY);

  crashReporterAPIData_Hash =
    new nsDataHashtable<nsCStringHashKey,nsCString>();
  NS_ENSURE_TRUE(crashReporterAPIData_Hash, NS_ERROR_OUT_OF_MEMORY);

  rv = crashReporterAPIData_Hash->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  notesField = new nsCString();
  NS_ENSURE_TRUE(notesField, NS_ERROR_OUT_OF_MEMORY);

  
  nsCOMPtr<nsIFile> exePath;
  rv = aXREDirectory->Clone(getter_AddRefs(exePath));
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_MACOSX)
  exePath->Append(NS_LITERAL_STRING("crashreporter.app"));
  exePath->Append(NS_LITERAL_STRING("Contents"));
  exePath->Append(NS_LITERAL_STRING("MacOS"));
#endif

  exePath->AppendNative(NS_LITERAL_CSTRING(CRASH_REPORTER_FILENAME));

#ifdef XP_WIN32
  nsString crashReporterPath_temp;
  exePath->GetPath(crashReporterPath_temp);

  crashReporterPath = ToNewUnicode(crashReporterPath_temp);
#else
  nsCString crashReporterPath_temp;
  exePath->GetNativePath(crashReporterPath_temp);

  crashReporterPath = ToNewCString(crashReporterPath_temp);
#endif

  
#if defined(XP_WIN32)
  nsString tempPath;

  
  int pathLen = GetTempPath(0, NULL);
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

#elif defined(XP_UNIX)
  
  nsCString tempPath = NS_LITERAL_CSTRING("/tmp/");
#else
#error "Implement this for your platform"
#endif

#ifdef XP_UNIX
  
  
  
  
  int fd = open("/dev/null", O_RDONLY);
  close(fd);
  write(-1, NULL, 0);
#endif

  
  gExceptionHandler = new google_breakpad::
    ExceptionHandler(tempPath.get(),
#ifdef XP_WIN
                     FPEFilter,
#else
                     nsnull,
#endif
                     MinidumpCallback,
                     nsnull,
#if defined(XP_WIN32)
                     google_breakpad::ExceptionHandler::HANDLER_ALL);
#else
                     true);
#endif

  if (!gExceptionHandler)
    return NS_ERROR_OUT_OF_MEMORY;

  
  char timeString[32];
  XP_TTOA(time(NULL), timeString, 10);
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

  return NS_OK;
}

bool GetEnabled()
{
  return gExceptionHandler != nsnull && !gExceptionHandler->IsOutOfProcess();
}

bool GetMinidumpPath(nsAString& aPath)
{
  if (!gExceptionHandler)
    return false;

  aPath = CONVERT_XP_CHAR_TO_UTF16(gExceptionHandler->dump_path().c_str());
  return true;
}

nsresult SetMinidumpPath(const nsAString& aPath)
{
  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  gExceptionHandler->set_dump_path(CONVERT_UTF16_TO_XP_CHAR(aPath).BeginReading());

  return NS_OK;
}

static nsresult
WriteDataToFile(nsIFile* aFile, const nsACString& data)
{
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile);
  NS_ENSURE_TRUE(localFile, NS_ERROR_FAILURE);

  PRFileDesc* fd;
  nsresult rv = localFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE, 00600,
                                            &fd);
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
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile);
  NS_ENSURE_TRUE(localFile, NS_ERROR_FAILURE);

  PRFileDesc* fd;
  nsresult rv = localFile->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_OK;
  PRInt32 filesize = PR_Available(fd);
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
  PRBool exists;

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
  time_t t = time(NULL);
  char buf[16];
  sprintf(buf, "%ld", t);
  aInstallTime = buf;

  return NS_OK;
}






nsresult SetupExtraData(nsILocalFile* aAppDataDirectory,
                        const nsACString& aBuildID)
{
  nsCOMPtr<nsIFile> dataDirectory;
  nsresult rv = aAppDataDirectory->Clone(getter_AddRefs(dataDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataDirectory->AppendNative(NS_LITERAL_CSTRING("Crash Reports"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = dataDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    rv = dataDirectory->Create(nsIFile::DIRECTORY_TYPE, 0700);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#if defined(XP_WIN32)
  nsAutoString dataDirEnv(NS_LITERAL_STRING("MOZ_CRASHREPORTER_DATA_DIRECTORY="));

  nsAutoString dataDirectoryPath;
  rv = dataDirectory->GetPath(dataDirectoryPath);
  NS_ENSURE_SUCCESS(rv, rv);

  dataDirEnv.Append(dataDirectoryPath);

  _wputenv(dataDirEnv.get());
#else
  
  nsCAutoString dataDirEnv("MOZ_CRASHREPORTER_DATA_DIRECTORY=");

  nsCAutoString dataDirectoryPath;
  rv = dataDirectory->GetNativePath(dataDirectoryPath);
  NS_ENSURE_SUCCESS(rv, rv);

  dataDirEnv.Append(dataDirectoryPath);

  char* env = ToNewCString(dataDirEnv);
  NS_ENSURE_TRUE(env, NS_ERROR_OUT_OF_MEMORY);

  PR_SetEnv(env);
#endif

  nsCAutoString data;
  if(NS_SUCCEEDED(GetOrInit(dataDirectory,
                            NS_LITERAL_CSTRING("InstallTime") + aBuildID,
                            data, InitInstallTime)))
    AnnotateCrashReport(NS_LITERAL_CSTRING("InstallTime"), data);

  
  
  
  
  
  if(NS_SUCCEEDED(GetOrInit(dataDirectory, NS_LITERAL_CSTRING("LastCrash"),
                            data, NULL))) {
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
  nsCAutoString filename;
  rv = lastCrashFile->GetNativePath(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  if (filename.Length() < XP_PATH_MAX)
    strncpy(lastCrashTimeFilename, filename.get(), filename.Length());
#endif

  return NS_OK;
}

static void OOPDeinit();

nsresult UnsetExceptionHandler()
{
  delete gExceptionHandler;

  
  
  if (crashReporterAPIData_Hash) {
    delete crashReporterAPIData_Hash;
    crashReporterAPIData_Hash = nsnull;
  }

  if (crashReporterAPIData) {
    delete crashReporterAPIData;
    crashReporterAPIData = nsnull;
  }

  if (notesField) {
    delete notesField;
    notesField = nsnull;
  }

  if (crashReporterPath) {
    NS_Free(crashReporterPath);
    crashReporterPath = nsnull;
  }

  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  gExceptionHandler = nsnull;

#ifdef MOZ_IPC
  OOPDeinit();
#endif

  return NS_OK;
}

static void ReplaceChar(nsCString& str, const nsACString& character,
                        const nsACString& replacement)
{
  nsCString::const_iterator start, end;

  str.BeginReading(start);
  str.EndReading(end);

  while (FindInReadable(character, start, end)) {
    PRInt32 pos = end.size_backward();
    str.Replace(pos - 1, 1, replacement);

    str.BeginReading(start);
    start.advance(pos + replacement.Length() - 1);
    str.EndReading(end);
  }
}

static PRBool DoFindInReadable(const nsACString& str, const nsACString& value)
{
  nsACString::const_iterator start, end;
  str.BeginReading(start);
  str.EndReading(end);

  return FindInReadable(value, start, end);
}

static PLDHashOperator EnumerateEntries(const nsACString& key,
                                        nsCString entry,
                                        void* userData)
{
  crashReporterAPIData->Append(key + NS_LITERAL_CSTRING("=") + entry +
                               NS_LITERAL_CSTRING("\n"));
  return PL_DHASH_NEXT;
}

nsresult AnnotateCrashReport(const nsACString& key, const nsACString& data)
{
  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  if (DoFindInReadable(key, NS_LITERAL_CSTRING("=")) ||
      DoFindInReadable(key, NS_LITERAL_CSTRING("\n")))
    return NS_ERROR_INVALID_ARG;

  if (DoFindInReadable(data, NS_LITERAL_CSTRING("\0")))
    return NS_ERROR_INVALID_ARG;

  nsCString escapedData(data);

  
  ReplaceChar(escapedData, NS_LITERAL_CSTRING("\\"),
              NS_LITERAL_CSTRING("\\\\"));
  
  ReplaceChar(escapedData, NS_LITERAL_CSTRING("\n"),
              NS_LITERAL_CSTRING("\\n"));

  nsresult rv = crashReporterAPIData_Hash->Put(key, escapedData);
  NS_ENSURE_SUCCESS(rv, rv);

  
  crashReporterAPIData->Truncate(0);
  crashReporterAPIData_Hash->EnumerateRead(EnumerateEntries,
                                           crashReporterAPIData);

  return NS_OK;
}

nsresult AppendAppNotesToCrashReport(const nsACString& data)
{
  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  if (DoFindInReadable(data, NS_LITERAL_CSTRING("\0")))
    return NS_ERROR_INVALID_ARG;

  notesField->Append(data);
  return AnnotateCrashReport(NS_LITERAL_CSTRING("Notes"), *notesField);
}


bool GetAnnotation(const nsACString& key, nsACString& data)
{
  if (!gExceptionHandler)
    return false;

  nsCAutoString entry;
  if (!crashReporterAPIData_Hash->Get(key, &entry))
    return false;

  data = entry;
  return true;
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
  nsCAutoString envVar;
  char *env;
  for (i = 0; i < argc; i++) {
    envVar = "MOZ_CRASHREPORTER_RESTART_ARG_";
    envVar.AppendInt(i);
    envVar += "=";
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    
    
    
    int arg_len = 0;
    if (i == 0 &&
        (arg_len = strlen(argv[i])) > 4 &&
        strcmp(argv[i] + arg_len - 4, "-bin") == 0) {
      envVar.Append(argv[i], arg_len - 4);
    } else
#endif
    {
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

#ifdef XP_MACOSX
nsresult AppendObjCExceptionInfoToAppNotes(void *inException)
{
  nsCAutoString excString;
  GetObjCExceptionInfo(inException, excString);
  AppendAppNotesToCrashReport(excString);
  return NS_OK;
}
#endif





static nsresult PrefSubmitReports(PRBool* aSubmitReports, bool writePref)
{
  nsresult rv;
#if defined(XP_WIN32)
  



  nsCOMPtr<nsIXULAppInfo> appinfo =
    do_GetService("@mozilla.org/xre/app-info;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString appVendor, appName;
  rv = appinfo->GetVendor(appVendor);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = appinfo->GetName(appName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWindowsRegKey> regKey
    (do_CreateInstance("@mozilla.org/windows-registry-key;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString regPath;

  regPath.AppendLiteral("Software\\");
  if(!appVendor.IsEmpty()) {
    regPath.Append(appVendor);
    regPath.AppendLiteral("\\");
  }
  regPath.Append(appName);
  regPath.AppendLiteral("\\Crash Reporter");

  
  
  if (writePref) {
    rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                      NS_ConvertUTF8toUTF16(regPath),
                      nsIWindowsRegKey::ACCESS_SET_VALUE);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 value = *aSubmitReports ? 1 : 0;
    rv = regKey->WriteIntValue(NS_LITERAL_STRING("SubmitCrashReport"), value);
    regKey->Close();
    return rv;
  }

  
  
  
  
  PRUint32 value;
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
    *aSubmitReports = PR_TRUE;
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
    *aSubmitReports = PR_TRUE;
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

  PRBool exists;
  rv = reporterINI->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    if (!writePref) {
        
        *aSubmitReports = PR_TRUE;
        return NS_OK;
    }
    
    rv = reporterINI->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIINIParserFactory> iniFactory =
    do_GetService("@mozilla.org/xpcom/ini-processor-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(reporterINI);
  NS_ENSURE_TRUE(localFile, NS_ERROR_FAILURE);
  nsCOMPtr<nsIINIParser> iniParser;
  rv = iniFactory->CreateINIParser(localFile,
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
    rv = iniWriter->WriteFile(NULL);
    return rv;
  }
  
  nsCAutoString submitReportValue;
  rv = iniParser->GetString(NS_LITERAL_CSTRING("Crash Reporter"),
                            NS_LITERAL_CSTRING("SubmitReport"),
                            submitReportValue);

  
  if (NS_FAILED(rv))
    *aSubmitReports = PR_TRUE;
  else if (submitReportValue.EqualsASCII("0"))
    *aSubmitReports = PR_FALSE;
  else
    *aSubmitReports = PR_TRUE;

  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

nsresult GetSubmitReports(PRBool* aSubmitReports)
{
    return PrefSubmitReports(aSubmitReports, false);
}

nsresult SetSubmitReports(PRBool aSubmitReports)
{
    return PrefSubmitReports(&aSubmitReports, true);
}



static bool
GetPendingDir(nsILocalFile** dir)
{
  nsCOMPtr<nsIProperties> dirSvc =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
  if (!dirSvc)
    return false;
  nsCOMPtr<nsILocalFile> pendingDir;
  if (NS_FAILED(dirSvc->Get("UAppData",
                            NS_GET_IID(nsILocalFile),
                            getter_AddRefs(pendingDir))) ||
      NS_FAILED(pendingDir->Append(NS_LITERAL_STRING("Crash Reports"))) ||
      NS_FAILED(pendingDir->Append(NS_LITERAL_STRING("pending"))))
    return false;
  *dir = NULL;
  pendingDir.swap(*dir);
  return true;
}







static bool
GetMinidumpLimboDir(nsILocalFile** dir)
{
  if (ShouldReport()) {
    return GetPendingDir(dir);
  }
  else {
    CreateFileFromPath(gExceptionHandler->dump_path(), dir);
    return NULL != *dir;
  }
}

bool
GetMinidumpForID(const nsAString& id, nsILocalFile** minidump)
{
  if (!GetMinidumpLimboDir(minidump))
    return false;
  (*minidump)->Append(id + NS_LITERAL_STRING(".dmp")); 
  return true;
}

bool
GetIDFromMinidump(nsILocalFile* minidump, nsAString& id)
{
  if (NS_SUCCEEDED(minidump->GetLeafName(id))) {
    id.Replace(id.Length() - 4, 4, NS_LITERAL_STRING(""));
    return true;
  }
  return false;
}

bool
GetExtraFileForID(const nsAString& id, nsILocalFile** extraFile)
{
  if (!GetMinidumpLimboDir(extraFile))
    return false;
  (*extraFile)->Append(id + NS_LITERAL_STRING(".extra"));
  return true;
}

bool
GetExtraFileForMinidump(nsILocalFile* minidump, nsILocalFile** extraFile)
{
  nsAutoString leafName;
  nsresult rv = minidump->GetLeafName(leafName);
  if (NS_FAILED(rv))
    return false;

  nsCOMPtr<nsIFile> extraF;
  rv = minidump->Clone(getter_AddRefs(extraF));
  if (NS_FAILED(rv))
    return false;

  nsCOMPtr<nsILocalFile> extra = do_QueryInterface(extraF);
  if (!extra)
    return false;

  leafName.Replace(leafName.Length() - 3, 3,
                   NS_LITERAL_STRING("extra"));
  rv = extra->SetLeafName(leafName);
  if (NS_FAILED(rv))
    return false;

  *extraFile = NULL;
  extra.swap(*extraFile);
  return true;
}

bool
AppendExtraData(const nsAString& id, const AnnotationTable& data)
{
  nsCOMPtr<nsILocalFile> extraFile;
  if (!GetExtraFileForID(id, getter_AddRefs(extraFile)))
    return false;
  return AppendExtraData(extraFile, data);
}




struct Blacklist {
  Blacklist() : mItems(NULL), mLen(0) { }
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
WriteExtraData(nsILocalFile* extraFile,
               const AnnotationTable& data,
               const Blacklist& blacklist,
               bool writeCrashTime=false,
               bool truncate=false)
{
  PRFileDesc* fd;
  PRIntn truncOrAppend = truncate ? PR_TRUNCATE : PR_APPEND;
  nsresult rv = 
    extraFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | truncOrAppend,
                                0600, &fd);
  if (NS_FAILED(rv))
    return false;

  EnumerateAnnotationsContext ctx = { blacklist, fd };
  data.EnumerateRead(EnumerateAnnotations, &ctx);

  if (writeCrashTime) {
    time_t crashTime = time(NULL);
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
AppendExtraData(nsILocalFile* extraFile, const AnnotationTable& data)
{
  return WriteExtraData(extraFile, data, Blacklist());
}


#if defined(MOZ_IPC)

static bool
WriteExtraForMinidump(nsILocalFile* minidump,
                      const Blacklist& blacklist,
                      nsILocalFile** extraFile)
{
  nsCOMPtr<nsILocalFile> extra;
  if (!GetExtraFileForMinidump(minidump, getter_AddRefs(extra)))
    return false;

  if (!WriteExtraData(extra, *crashReporterAPIData_Hash,
                      blacklist,
                      true ,
                      true ))
    return false;

  *extraFile = NULL;
  extra.swap(*extraFile);

  return true;
}



static bool
MoveToPending(nsIFile* dumpFile, nsIFile* extraFile)
{
  nsCOMPtr<nsILocalFile> pendingDir;
  if (!GetPendingDir(getter_AddRefs(pendingDir)))
    return false;

  return NS_SUCCEEDED(dumpFile->MoveTo(pendingDir, EmptyString())) &&
    NS_SUCCEEDED(extraFile->MoveTo(pendingDir, EmptyString()));
}

#if !defined(XP_MACOSX)
static void
OnChildProcessDumpRequested(void* aContext,
                            const ClientInfo* aClientInfo,
                            const xpstring* aFilePath)
{
  nsCOMPtr<nsILocalFile> minidump;
  nsCOMPtr<nsILocalFile> extraFile;

  CreateFileFromPath(*aFilePath, getter_AddRefs(minidump));

  if (!WriteExtraForMinidump(minidump,
                             Blacklist(kSubprocessBlacklist,
                                       NS_ARRAY_LENGTH(kSubprocessBlacklist)),
                             getter_AddRefs(extraFile)))
    return;

  if (ShouldReport())
    MoveToPending(minidump, extraFile);

  {
    PRUint32 pid = aClientInfo->pid();

    MutexAutoLock lock(*dumpMapLock);
    pidToMinidump->Put(pid, minidump);
  }
}
#endif  

static bool
OOPInitialized()
{
  return pidToMinidump != NULL;
}

static void
OOPInit()
{
  NS_ABORT_IF_FALSE(!OOPInitialized(),
                    "OOP crash reporter initialized more than once!");
  NS_ABORT_IF_FALSE(gExceptionHandler != NULL,
                    "attempt to initialize OOP crash reporter before in-process crashreporter!");

#if defined(XP_WIN)
  childCrashNotifyPipe =
    PR_smprintf("\\\\.\\pipe\\gecko-crash-server-pipe.%i",
                static_cast<int>(::GetCurrentProcessId()));

  const std::wstring dumpPath = gExceptionHandler->dump_path();
  crashServer = new CrashGenerationServer(
    NS_ConvertASCIItoUTF16(childCrashNotifyPipe).get(),
    NULL,                       
    NULL, NULL,                 
    OnChildProcessDumpRequested, NULL,
    NULL, NULL,                 
    true,                       
    &dumpPath);

#elif defined(XP_LINUX)
  if (!CrashGenerationServer::CreateReportChannel(&serverSocketFd,
                                                  &clientSocketFd))
    NS_RUNTIMEABORT("can't create crash reporter socketpair()");

  const std::string dumpPath = gExceptionHandler->dump_path();
  crashServer = new CrashGenerationServer(
    serverSocketFd,
    OnChildProcessDumpRequested, NULL,
    NULL, NULL,                 
    true,                       
    &dumpPath);
#endif

#if !defined(XP_MACOSX)
  if (!crashServer->Start())
    NS_RUNTIMEABORT("can't start crash reporter server()");
#endif

  pidToMinidump = new ChildMinidumpMap();
  pidToMinidump->Init();

  dumpMapLock = new Mutex("CrashReporter::dumpMapLock");
}

static void
OOPDeinit()
{
  if (!OOPInitialized()) {
    NS_WARNING("OOPDeinit() without successful OOPInit()");
    return;
  }

#if !defined(XP_MACOSX)
  delete crashServer;
  crashServer = NULL;
#endif

  delete dumpMapLock;
  dumpMapLock = NULL;

  delete pidToMinidump;
  pidToMinidump = NULL;

#if defined(XP_WIN)
  PR_Free(childCrashNotifyPipe);
  childCrashNotifyPipe = NULL;
#endif
}

#if defined(XP_WIN)

const char*
GetChildNotificationPipe()
{
  if (!GetEnabled())
    return kNullNotifyPipe;

  if (!OOPInitialized())
    OOPInit();

  return childCrashNotifyPipe;
}


bool
SetRemoteExceptionHandler(const nsACString& crashPipe)
{
  
  if (crashPipe.Equals(kNullNotifyPipe))
    return true;

  NS_ABORT_IF_FALSE(!gExceptionHandler, "crash client already init'd");

  gExceptionHandler = new google_breakpad::
    ExceptionHandler(L"",
                     NULL,    
                     NULL,    
                     NULL,    
                     google_breakpad::ExceptionHandler::HANDLER_ALL,
                     MiniDumpNormal,
                     NS_ConvertASCIItoUTF16(crashPipe).BeginReading(),
                     NULL);

  
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

  if (!OOPInitialized())
    OOPInit();

  *childCrashFd = clientSocketFd;
  *childCrashRemapFd = kMagicChildCrashReportFd;

  return true;
}


bool
SetRemoteExceptionHandler()
{
  NS_ABORT_IF_FALSE(!gExceptionHandler, "crash client already init'd");

  gExceptionHandler = new google_breakpad::
    ExceptionHandler("",
                     NULL,    
                     NULL,    
                     NULL,    
                     true,    
                     kMagicChildCrashReportFd);

  
  return gExceptionHandler->IsOutOfProcess();
}


#elif defined(XP_MACOSX)
void
CreateNotificationPipeForChild()
{
  if (GetEnabled() && !OOPInitialized())
    OOPInit();
}
#endif  


bool
TakeMinidumpForChild(PRUint32 childPid, nsILocalFile** dump)
{
  if (!GetEnabled())
    return false;

  MutexAutoLock lock(*dumpMapLock);

  nsCOMPtr<nsILocalFile> d;
  bool found = pidToMinidump->Get(childPid, getter_AddRefs(d));
  if (found)
    pidToMinidump->Remove(childPid);

  *dump = NULL;
  d.swap(*dump);

  return found;
}




struct PairedDumpContext {
  nsCOMPtr<nsILocalFile>* minidump;
  nsCOMPtr<nsILocalFile>* extra;
  const Blacklist& blacklist;
};

static bool
PairedDumpCallback(const XP_CHAR* dump_path,
                   const XP_CHAR* minidump_id,
                   void* context,
#ifdef XP_WIN32
                   EXCEPTION_POINTERS* ,
                   MDRawAssertionInfo* ,
#endif
                   bool succeeded)
{
  PairedDumpContext* ctx = static_cast<PairedDumpContext*>(context);
  nsCOMPtr<nsILocalFile>& minidump = *ctx->minidump;
  nsCOMPtr<nsILocalFile>& extra = *ctx->extra;
  const Blacklist& blacklist = ctx->blacklist;

  xpstring dump(dump_path);
  dump += XP_PATH_SEPARATOR;
  dump += minidump_id;
  dump += dumpFileExtension;

  CreateFileFromPath(dump, getter_AddRefs(minidump));
  return WriteExtraForMinidump(minidump, blacklist, getter_AddRefs(extra));
}

ThreadId
CurrentThreadId()
{
#if defined(XP_WIN)
  return ::GetCurrentThreadId();
#elif defined(XP_LINUX)
  return sys_gettid();
#elif defined(XP_MACOSX)
  return -1;
#else
#  error "Unsupported platform"
#endif
}

bool
CreatePairedMinidumps(ProcessHandle childPid,
                      ThreadId childBlamedThread,
                      nsAString* pairGUID,
                      nsILocalFile** childDump,
                      nsILocalFile** parentDump)
{
  if (!GetEnabled())
    return false;

#if defined(XP_MACOSX)
  return false;
#else

  
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, false);  

  nsID id;
  rv = uuidgen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, false);
  
  char chars[NSID_LENGTH];
  id.ToProvidedString(chars);
  CopyASCIItoUTF16(chars, *pairGUID);

  
  pairGUID->Cut(0, 1);
  pairGUID->Cut(pairGUID->Length()-1, 1);

  
  nsCOMPtr<nsILocalFile> childMinidump;
  nsCOMPtr<nsILocalFile> childExtra;
  Blacklist childBlacklist(kSubprocessBlacklist,
                           NS_ARRAY_LENGTH(kSubprocessBlacklist));
  PairedDumpContext childCtx =
    { &childMinidump, &childExtra, childBlacklist };
  if (!google_breakpad::ExceptionHandler::WriteMinidumpForChild(
         childPid,
         childBlamedThread,
         gExceptionHandler->dump_path(),
         PairedDumpCallback,
         &childCtx))
    return false;

  
  nsCOMPtr<nsILocalFile> parentMinidump;
  nsCOMPtr<nsILocalFile> parentExtra;
  
  Blacklist parentBlacklist;
  PairedDumpContext parentCtx =
    { &parentMinidump, &parentExtra, parentBlacklist };
  if (!google_breakpad::ExceptionHandler::WriteMinidump(
         gExceptionHandler->dump_path(),
         true,                  
         PairedDumpCallback,
         &parentCtx))
    return false;

  
  if (ShouldReport()) {
    MoveToPending(childMinidump, childExtra);
    MoveToPending(parentMinidump, parentExtra);
  }

  *childDump = NULL;
  *parentDump = NULL;
  childMinidump.swap(*childDump);
  parentMinidump.swap(*parentDump);

  return true;
#endif  
}

#if !defined(XP_MACOSX)
bool
UnsetRemoteExceptionHandler()
{
  delete gExceptionHandler;
  gExceptionHandler = NULL;
  return true;
}
#endif  

#endif  

} 
