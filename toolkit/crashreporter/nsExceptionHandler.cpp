




































#include "nsAirbagExceptionHandler.h"

#ifdef XP_WIN32
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include "client/windows/handler/exception_handler.h"
#include <string.h>
#endif

#include <stdlib.h>
#include <prenv.h>
#include "nsDebug.h"

static google_airbag::ExceptionHandler* gAirbagExceptionHandler = nsnull;

#ifdef XP_WIN32
static TCHAR crashReporterExe[MAX_PATH] = L"\0";
static TCHAR minidumpPath[MAX_PATH] = L"\0";
#endif

void AirbagMinidumpCallback(const std::wstring &minidump_id,
                            void *context, bool succeeded)
{
#ifdef XP_WIN32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  TCHAR cmdLine[2*MAX_PATH];

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOWNORMAL;
  ZeroMemory(&pi, sizeof(pi));

  wcscat(minidumpPath, minidump_id.c_str());
  wcscat(minidumpPath, L".dmp");
  wsprintf(cmdLine, L"\"%s\" \"%s\"", crashReporterExe, minidumpPath);
  
  if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
  }
  
  TerminateProcess(GetCurrentProcess(), 1);
#endif
}

nsresult SetAirbagExceptionHandler()
{
  if (gAirbagExceptionHandler)
    return NS_ERROR_ALREADY_INITIALIZED;

  
  
  
  
  const char *airbagEnv = PR_GetEnv("MOZ_AIRBAG");
  if (airbagEnv == NULL || atoi(airbagEnv) == 0)
    return NS_OK;

#ifdef XP_WIN32
  
  if (!GetTempPath(MAX_PATH, minidumpPath))
    return NS_ERROR_FAILURE;

  std::wstring tempStr(minidumpPath);
  gAirbagExceptionHandler = new google_airbag::ExceptionHandler(tempStr,
                                                 AirbagMinidumpCallback,
                                                 nsnull,
                                                 true);
  if (GetModuleFileName(NULL, crashReporterExe, MAX_PATH)) {
    
    LPTSTR s = wcsrchr(crashReporterExe, '\\');
    if (s) {
      s++;
      wcscpy(s, L"crashreporter.exe");
    }
  }
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif

  if (!gAirbagExceptionHandler)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsresult SetAirbagMinidumpPath(const nsAString* aPath)
{
  NS_ENSURE_ARG_POINTER(aPath);

  if (!gAirbagExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  std::wstring path;
#ifdef XP_WIN32
  wcsncpy(minidumpPath, PromiseFlatString(*aPath).get(), MAX_PATH);
  path = std::wstring(minidumpPath);
  int l = wcslen(minidumpPath);
  if (minidumpPath[l-1] != '\\' && l < MAX_PATH - 1) {
    minidumpPath[l] = '\\';
    minidumpPath[l+1] = '\0';
  }
#endif
  gAirbagExceptionHandler->set_dump_path(path);
  return NS_OK;
}

nsresult UnsetAirbagExceptionHandler()
{
  if (!gAirbagExceptionHandler)
    return NS_ERROR_NOT_INITIALIZED;

  delete gAirbagExceptionHandler;
  gAirbagExceptionHandler = nsnull;

  return NS_OK;
}
