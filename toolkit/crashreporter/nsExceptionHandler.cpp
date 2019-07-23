




































#include "nsAirbagExceptionHandler.h"

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
#else
#error "Not yet implemented for this platform"
#endif 

#ifndef HAVE_CPP_2BYTE_WCHAR_T
#error "This code expects a 2 byte wchar_t.  You should --disable-airbag."
#endif

#include <stdlib.h>
#include <prenv.h>
#include "nsDebug.h"
#include "nsCRT.h"
#include "nsILocalFile.h"
#include "nsDataHashtable.h"

namespace CrashReporter {

#ifdef XP_WIN32
typedef wchar_t XP_CHAR;
#define TO_NEW_XP_CHAR(x) ToNewUnicode(x)
#define CONVERT_UTF16_TO_XP_CHAR(x) x
#define XP_STRLEN(x) wcslen(x)
#define CRASH_REPORTER_FILENAME "crashreporter.exe"
#define PATH_SEPARATOR "\\"
#define XP_PATH_SEPARATOR L"\\"

#define XP_PATH_MAX 4096

#define CMDLINE_SIZE ((XP_PATH_MAX * 2) + 6)
#else
typedef char XP_CHAR;
#define TO_NEW_XP_CHAR(x) ToNewUTF8String(x)
#define CONVERT_UTF16_TO_XP_CHAR(x) NS_ConvertUTF16toUTF8(x)
#define XP_STRLEN(x) strlen(x)
#define CRASH_REPORTER_FILENAME "crashreporter"
#define PATH_SEPARATOR "/"
#define XP_PATH_SEPARATOR "/"
#define XP_PATH_MAX PATH_MAX
#endif 

static const XP_CHAR dumpFileExtension[] = {'.', 'd', 'm', 'p',
                                            '\0'}; 
static const XP_CHAR extraFileExtension[] = {'.', 'e', 'x', 't',
                                             'r', 'a', '\0'}; 

static google_breakpad::ExceptionHandler* gExceptionHandler = nsnull;

static XP_CHAR* crashReporterPath;


static bool doReport = true;


static bool showOSCrashReporter = false;


static nsDataHashtable<nsCStringHashKey,nsCString>* crashReporterAPIData_Hash;
static nsCString* crashReporterAPIData = nsnull;

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

#ifdef XP_WIN32
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
    (void) execl(crashReporterPath,
                 crashReporterPath, minidumpPath, (char*)0);
    _exit(1);
  }
#endif

 return returnValue;
}

nsresult SetExceptionHandler(nsILocalFile* aXREDirectory,
                             const char* aServerURL)
{
  nsresult rv;

  if (gExceptionHandler)
    return NS_ERROR_ALREADY_INITIALIZED;

  const char *envvar = PR_GetEnv("MOZ_CRASHREPORTER_DISABLE");
  if (envvar && *envvar)
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

  
  nsCOMPtr<nsIFile> exePath;
  rv = aXREDirectory->Clone(getter_AddRefs(exePath));
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_MACOSX)
  exePath->Append(NS_LITERAL_STRING("crashreporter.app"));
  exePath->Append(NS_LITERAL_STRING("Contents"));
  exePath->Append(NS_LITERAL_STRING("MacOS"));
#endif

  exePath->Append(NS_LITERAL_STRING(CRASH_REPORTER_FILENAME));

  nsString crashReporterPath_temp;
  exePath->GetPath(crashReporterPath_temp);

  crashReporterPath = TO_NEW_XP_CHAR(crashReporterPath_temp);

  
  nsString tempPath;
#if defined(XP_WIN32)
  
  int pathLen = GetTempPath(0, NULL);
  if (pathLen == 0)
    return NS_ERROR_FAILURE;

  tempPath.SetLength(pathLen);
  GetTempPath(pathLen, (LPWSTR)tempPath.BeginWriting());
#elif defined(XP_MACOSX)
  FSRef fsRef;
  OSErr err = FSFindFolder(kUserDomain, kTemporaryFolderType,
                           kCreateFolder, &fsRef);
  if (err != noErr)
    return NS_ERROR_FAILURE;

  char path[PATH_MAX];
  OSStatus status = FSRefMakePath(&fsRef, (UInt8*)path, PATH_MAX);
  if (status != noErr)
    return NS_ERROR_FAILURE;
  tempPath = NS_ConvertUTF8toUTF16(path);

#elif defined(XP_UNIX)
  
  tempPath = NS_LITERAL_STRING("/tmp/");
#else
  
  return NS_ERROR_NOT_IMPLEMENTED;
#endif

  
  gExceptionHandler = new google_breakpad::
    ExceptionHandler(CONVERT_UTF16_TO_XP_CHAR(tempPath).get(),
                     nsnull,
                     MinidumpCallback,
                     nsnull,
                     true);

  if (!gExceptionHandler)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (aServerURL)
    AnnotateCrashReport(NS_LITERAL_CSTRING("ServerURL"),
                        nsDependentCString(aServerURL));

#if defined(XP_MACOSX)
  
  
  
  showOSCrashReporter = PassToOSCrashReporter();
#endif

  return NS_OK;
}

nsresult SetMinidumpPath(const nsAString& aPath)
{
  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  gExceptionHandler->set_dump_path(CONVERT_UTF16_TO_XP_CHAR(aPath).BeginReading());

  return NS_OK;
}

nsresult UnsetExceptionHandler()
{
  
  
  if (crashReporterAPIData_Hash) {
    delete crashReporterAPIData_Hash;
    crashReporterAPIData_Hash = nsnull;
  }
  if (crashReporterPath) {
    NS_Free(crashReporterPath);
    crashReporterPath = nsnull;
  }

  if (!gExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  delete gExceptionHandler;
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

static PLDHashOperator PR_CALLBACK EnumerateEntries(const nsACString& key,
                                                    nsCString entry,
                                                    void* userData)
{
  crashReporterAPIData->Append(key + NS_LITERAL_CSTRING("=") + entry +
                               NS_LITERAL_CSTRING("\n"));
  return PL_DHASH_NEXT;
}

nsresult AnnotateCrashReport(const nsACString &key, const nsACString &data)
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

nsresult
SetRestartArgs(int argc, char **argv)
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
    envVar += argv[i];

    
    
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

  return NS_OK;
}
} 
