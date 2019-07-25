





































#include "nsXULAppAPI.h"
#ifdef XP_WIN
#include <windows.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#include "plstr.h"
#include "prprf.h"
#include "prenv.h"

#include "nsCOMPtr.h"
#include "nsILocalFile.h"
#include "nsStringGlue.h"

#ifdef XP_WIN

#define XRE_WANT_DLL_BLOCKLIST

#include "nsWindowsWMain.cpp"
#endif

static void Output(const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#if defined(XP_WIN) && !MOZ_WINCONSOLE
  PRUnichar msg[2048];
  _vsnwprintf(msg, sizeof(msg)/sizeof(msg[0]), NS_ConvertUTF8toUTF16(fmt).get(), ap);
  MessageBoxW(NULL, msg, L"XULRunner", MB_OK | MB_ICONERROR);
#else
  vfprintf(stderr, fmt, ap);
#endif

  va_end(ap);
}




static PRBool IsArg(const char* arg, const char* s)
{
  if (*arg == '-')
  {
    if (*++arg == '-')
      ++arg;
    return !PL_strcasecmp(arg, s);
  }

#if defined(XP_WIN) || defined(XP_OS2)
  if (*arg == '/')
    return !PL_strcasecmp(++arg, s);
#endif

  return PR_FALSE;
}




class ScopedLogging
{
public:
  ScopedLogging() { NS_LogInit(); }
  ~ScopedLogging() { NS_LogTerm(); }
};

int main(int argc, char* argv[])
{
  ScopedLogging log;

  nsCOMPtr<nsILocalFile> appini;
  nsresult rv = XRE_GetBinaryPath(argv[0], getter_AddRefs(appini));
  if (NS_FAILED(rv)) {
    Output("Couldn't calculate the application directory.");
    return 255;
  }
  appini->SetNativeLeafName(NS_LITERAL_CSTRING("application.ini"));

  
  
  char *appEnv = nsnull;
  const char *appDataFile = PR_GetEnv("XUL_APP_FILE");
  if (appDataFile && *appDataFile) {
    rv = XRE_GetFileFromPath(appDataFile, getter_AddRefs(appini));
    if (NS_FAILED(rv)) {
      Output("Invalid path found: '%s'", appDataFile);
      return 255;
    }
  }
  else if (argc > 1 && IsArg(argv[1], "app")) {
    if (argc == 2) {
      Output("Incorrect number of arguments passed to -app");
      return 255;
    }

    rv = XRE_GetFileFromPath(argv[2], getter_AddRefs(appini));
    if (NS_FAILED(rv)) {
      Output("application.ini path not recognized: '%s'", argv[2]);
      return 255;
    }

    appEnv = PR_smprintf("XUL_APP_FILE=%s", argv[2]);
    PR_SetEnv(appEnv);
    argv[2] = argv[0];
    argv += 2;
    argc -= 2;
  }

  nsXREAppData *appData;
  rv = XRE_CreateAppData(appini, &appData);
  if (NS_FAILED(rv)) {
    Output("Couldn't read application.ini");
    return 255;
  }

  int result = XRE_main(argc, argv, appData);
  XRE_FreeAppData(appData);
  if (appEnv)
    PR_smprintf_free(appEnv);
  return result;
}
