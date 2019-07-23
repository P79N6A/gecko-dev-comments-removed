





































#include "nsExceptionHandler.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include "client/windows/handler/exception_handler.h"
#include <string.h>
#elif defined(XP_MACOSX)
#include "client/mac/handler/exception_handler.h"
#include <string>
#include <Carbon/Carbon.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "mac_utils.h"
#elif defined(XP_LINUX)
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
#include "nsDebug.h"
#include "nsCRT.h"
#include "nsILocalFile.h"
#include "nsDataHashtable.h"

namespace CrashReporter {

#ifdef XP_WIN32
typedef wchar_t XP_CHAR;
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


static nsDataHashtable<nsCStringHashKey,nsCString>* crashReporterAPIData_Hash;
static nsCString* crashReporterAPIData = nsnull;
static nsCString* notesField = nsnull;

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
      write(fd, crashTimeString, crashTimeStringLen);
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
      
      write(fd, crashReporterAPIData->get(), crashReporterAPIData->Length());
      write(fd, kCrashTimeParameter, kCrashTimeParameterLen);
      write(fd, crashTimeString, crashTimeStringLen);
      write(fd, "\n", 1);
      if (timeSinceLastCrash != 0) {
        write(fd, kTimeSinceLastCrashParameter,kTimeSinceLastCrashParameterLen);
        write(fd, timeSinceLastCrashString, timeSinceLastCrashStringLen);
        write(fd, "\n", 1);
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

nsresult SetExceptionHandler(nsILocalFile* aXREDirectory,
                             bool force)
{
  nsresult rv;

  if (gExceptionHandler)
    return NS_ERROR_ALREADY_INITIALIZED;

  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER_DISABLE");
  if (envvar && *envvar && !force)
    return NS_OK;

  
  
  envvar = PR_GetEnv("MOZ_CRASHREPORTER_NO_REPORT");
  if (envvar && *envvar)
    doReport = false;

  
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

  
  gExceptionHandler = new google_breakpad::
    ExceptionHandler(tempPath.get(),
                     nsnull,
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
  
  
  
  showOSCrashReporter = PassToOSCrashReporter();
#endif

  return NS_OK;
}

bool GetEnabled()
{
  return gExceptionHandler != nsnull;
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

} 
