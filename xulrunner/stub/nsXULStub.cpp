




































#include "nsXPCOMGlue.h"
#include "nsINIParser.h"
#include "prtypes.h"
#include "nsXPCOMPrivate.h" 
#include "nsMemory.h" 
#include "nsXULAppAPI.h"
#include "nsILocalFile.h"

#include <stdarg.h>

#ifdef XP_WIN
#include <windows.h>
#include <io.h>
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define PATH_SEPARATOR_CHAR '\\'
#define R_OK 04
#elif defined(XP_MACOSX)
#include <unistd.h>
#include <sys/stat.h>
#include <CoreFoundation/CoreFoundation.h>
#define PATH_SEPARATOR_CHAR '/'
#elif defined (XP_OS2)
#define INCL_DOS
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define PATH_SEPARATOR_CHAR '\\'
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define PATH_SEPARATOR_CHAR '/'
#endif

#ifdef XP_WIN
#define XRE_DONT_PROTECT_DLL_LOAD
#include "nsWindowsWMain.cpp"
#endif

#define VERSION_MAXLEN 128

static void Output(PRBool isError, const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#if (defined(XP_WIN) && !MOZ_WINCONSOLE)
  char msg[2048];

  vsnprintf(msg, sizeof(msg), fmt, ap);

  UINT flags = MB_OK;
  if (isError)
    flags |= MB_ICONERROR;
  else
    flags |= MB_ICONINFORMATION;

  wchar_t wide_msg[1024];
  MultiByteToWideChar(CP_ACP,
		      0,
		      msg,
		      -1,
		      wide_msg,
		      sizeof(wide_msg) / sizeof(wchar_t));
  
  MessageBoxW(NULL, wide_msg, L"XULRunner", flags);
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
    return !strcasecmp(arg, s);
  }

#if defined(XP_WIN) || defined(XP_OS2)
  if (*arg == '/')
    return !strcasecmp(++arg, s);
#endif

  return PR_FALSE;
}




static PRBool FolderExists(const char* aDir)
{
#ifdef XP_WIN
  wchar_t wideDir[MAX_PATH];
  MultiByteToWideChar(CP_UTF8, 0, aDir, -1, wideDir, MAX_PATH);
  DWORD fileAttrs = GetFileAttributesW(wideDir);
  return fileAttrs != INVALID_FILE_ATTRIBUTES;
#else
  return access(aDir, R_OK) == 0;
#endif
}

static nsresult GetRealPath(const char* appDataFile, char* *aResult)
{
#ifdef XP_WIN
  wchar_t wAppDataFile[MAX_PATH];
  wchar_t wIniPath[MAX_PATH];
  MultiByteToWideChar(CP_UTF8, 0, appDataFile, -1, wAppDataFile, MAX_PATH);
  _wfullpath(wIniPath, wAppDataFile, MAX_PATH);
  WideCharToMultiByte(CP_UTF8, 0, wIniPath, -1, *aResult, MAX_PATH, 0, 0);
#else
  struct stat fileStat;
  if (!realpath(appDataFile, *aResult) || stat(*aResult, &fileStat))
    return NS_ERROR_FAILURE;
#endif
  if (!*aResult || !**aResult)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

class AutoAppData
{
public:
  AutoAppData(nsILocalFile* aINIFile) : mAppData(nsnull) {
    nsresult rv = XRE_CreateAppData(aINIFile, &mAppData);
    if (NS_FAILED(rv))
      mAppData = nsnull;
  }
  ~AutoAppData() {
    if (mAppData)
      XRE_FreeAppData(mAppData);
  }

  operator nsXREAppData*() const { return mAppData; }
  nsXREAppData* operator -> () const { return mAppData; }

private:
  nsXREAppData* mAppData;
};

XRE_CreateAppDataType XRE_CreateAppData;
XRE_FreeAppDataType XRE_FreeAppData;
XRE_mainType XRE_main;

int
main(int argc, char **argv)
{
  nsresult rv;
  char *lastSlash;

  char iniPath[MAXPATHLEN];
  char tmpPath[MAXPATHLEN];
  char greDir[MAXPATHLEN];
  PRBool greFound = PR_FALSE;

#if defined(XP_MACOSX)
  CFBundleRef appBundle = CFBundleGetMainBundle();
  if (!appBundle)
    return 1;

  CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(appBundle);
  if (!resourcesURL)
    return 1;

  CFURLRef absResourcesURL = CFURLCopyAbsoluteURL(resourcesURL);
  CFRelease(resourcesURL);
  if (!absResourcesURL)
    return 1;

  CFURLRef iniFileURL =
    CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault,
                                          absResourcesURL,
                                          CFSTR("application.ini"),
                                          false);
  CFRelease(absResourcesURL);
  if (!iniFileURL)
    return 1;

  CFStringRef iniPathStr =
    CFURLCopyFileSystemPath(iniFileURL, kCFURLPOSIXPathStyle);
  CFRelease(iniFileURL);
  if (!iniPathStr)
    return 1;

  CFStringGetCString(iniPathStr, iniPath, sizeof(iniPath),
                     kCFStringEncodingUTF8);
  CFRelease(iniPathStr);

#else

#ifdef XP_WIN
  wchar_t wide_path[MAX_PATH];
  if (!::GetModuleFileNameW(NULL, wide_path, MAX_PATH))
    return 1;

  WideCharToMultiByte(CP_UTF8, 0, wide_path,-1,
		      iniPath, MAX_PATH, NULL, NULL);

#elif defined(XP_OS2)
   PPIB ppib;
   PTIB ptib;

   DosGetInfoBlocks(&ptib, &ppib);
   DosQueryModuleName(ppib->pib_hmte, sizeof(iniPath), iniPath);

#else
  
  
  
  
  
  
  
  

  struct stat fileStat;
  strncpy(tmpPath, argv[0], sizeof(tmpPath));
  lastSlash = strrchr(tmpPath, '/');
  if (lastSlash) {
    *lastSlash = 0;
    realpath(tmpPath, iniPath);
  } else {
    const char *path = getenv("PATH");
    if (!path)
      return 1;

    char *pathdup = strdup(path);
    if (!pathdup)
      return 1;

    PRBool found = PR_FALSE;
    char *token = strtok(pathdup, ":");
    while (token) {
      sprintf(tmpPath, "%s/%s", token, argv[0]);
      if (stat(tmpPath, &fileStat) == 0) {
        found = PR_TRUE;
        lastSlash = strrchr(tmpPath, '/');
        *lastSlash = 0;
        realpath(tmpPath, iniPath);
        break;
      }
      token = strtok(NULL, ":");
    }
    free (pathdup);
    if (!found)
      return 1;
  }
  lastSlash = iniPath + strlen(iniPath);
  *lastSlash = '/';
#endif

#ifndef XP_UNIX
  lastSlash = strrchr(iniPath, PATH_SEPARATOR_CHAR);
  if (!lastSlash)
    return 1;
#endif

  *(++lastSlash) = '\0';

  

  snprintf(greDir, sizeof(greDir),
           "%sxulrunner" XPCOM_FILE_PATH_SEPARATOR XPCOM_DLL,
           iniPath);

  greFound = FolderExists(greDir);

#ifdef XP_UNIX
  if (greFound) {
    char resolved_greDir[MAXPATHLEN] = "";
    if (realpath(greDir, resolved_greDir) && *resolved_greDir) {
      strncpy(greDir, resolved_greDir, MAXPATHLEN);
    }
  }
#endif

  strncpy(lastSlash, "application.ini", sizeof(iniPath) - (lastSlash - iniPath));

#endif

  
  
  const char *appDataFile;
  appDataFile = getenv("XUL_APP_FILE");
  if (!appDataFile || !*appDataFile) 
    if (argc > 1 && IsArg(argv[1], "app")) {
      if (argc == 2) {
        Output(PR_FALSE, "specify APP-FILE (optional)\n");
        return 1;
      }
      argv[1] = argv[0];
      ++argv;
      --argc;

      appDataFile = argv[1];
      argv[1] = argv[0];
      ++argv;
      --argc;

      char kAppEnv[MAXPATHLEN];
      snprintf(kAppEnv, MAXPATHLEN, "XUL_APP_FILE=%s", appDataFile);
      if (putenv(kAppEnv)) 
        Output(PR_FALSE, "Couldn't set %s.\n", kAppEnv);

      char *result = (char*) calloc(sizeof(char), MAXPATHLEN);
      if (NS_FAILED(GetRealPath(appDataFile, &result))) {
        Output(PR_TRUE, "Invalid application.ini path.\n");
        return 1;
      }
      
      
      
      
      if (!greFound) {
        lastSlash = strrchr(iniPath, PATH_SEPARATOR_CHAR);
        if (!lastSlash)
          return 1;

        *(++lastSlash) = '\0';

        snprintf(greDir, sizeof(greDir), "%s" XPCOM_DLL, iniPath);
        greFound = FolderExists(greDir);
      }
      
      
      strcpy(iniPath, result);
    }
  
  nsINIParser parser;
  rv = parser.Init(iniPath);
  if (NS_FAILED(rv)) {
    fprintf(stderr, "Could not read application.ini\n");
    return 1;
  }

  if (!greFound) {
#ifdef XP_MACOSX
    
    CFURLRef fwurl = CFBundleCopyPrivateFrameworksURL(appBundle);
    CFURLRef absfwurl = nsnull;
    if (fwurl) {
      absfwurl = CFURLCopyAbsoluteURL(fwurl);
      CFRelease(fwurl);
    }

    if (absfwurl) {
      CFURLRef xulurl =
        CFURLCreateCopyAppendingPathComponent(NULL, absfwurl,
                                              CFSTR("XUL.Framework"),
                                              PR_TRUE);

      if (xulurl) {
        CFURLRef xpcomurl =
          CFURLCreateCopyAppendingPathComponent(NULL, xulurl,
                                                CFSTR("libxpcom.dylib"),
                                                PR_FALSE);

        if (xpcomurl) {
          char tbuffer[MAXPATHLEN];

          if (CFURLGetFileSystemRepresentation(xpcomurl, PR_TRUE,
                                               (UInt8*) tbuffer,
                                               sizeof(tbuffer)) &&
              access(tbuffer, R_OK | X_OK) == 0) {
            if (realpath(tbuffer, greDir)) {
              greFound = PR_TRUE;
            }
            else {
              greDir[0] = '\0';
            }
          }

          CFRelease(xpcomurl);
        }

        CFRelease(xulurl);
      }

      CFRelease(absfwurl);
    }
#endif
    if (!greFound) {
      Output(PR_FALSE, "Could not find the Mozilla runtime.\n");
      return 1;
    }
  }

#ifdef XP_OS2
  
  strcpy(tmpPath, greDir);
  lastSlash = strrchr(tmpPath, PATH_SEPARATOR_CHAR);
  if (lastSlash) {
    *lastSlash = '\0';
  }
  DosSetExtLIBPATH(tmpPath, BEGIN_LIBPATH);
#endif
  
  rv = XPCOMGlueStartup(greDir);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_OUT_OF_MEMORY) {
      char applicationName[2000] = "this application";
      parser.GetString("App", "Name", applicationName, sizeof(applicationName));
      Output(PR_TRUE, "Not enough memory available to start %s.\n",
             applicationName);
    } else {
      Output(PR_TRUE, "Couldn't load XPCOM.\n");
    }
    return 1;
  }

  static const nsDynamicFunctionLoad kXULFuncs[] = {
    { "XRE_CreateAppData", (NSFuncPtr*) &XRE_CreateAppData },
    { "XRE_FreeAppData", (NSFuncPtr*) &XRE_FreeAppData },
    { "XRE_main", (NSFuncPtr*) &XRE_main },
    { nsnull, nsnull }
  };

  rv = XPCOMGlueLoadXULFunctions(kXULFuncs);
  if (NS_FAILED(rv)) {
    Output(PR_TRUE, "Couldn't load XRE functions.\n");
    return 1;
  }

  NS_LogInit();

  int retval;

  { 
    nsCOMPtr<nsILocalFile> iniFile;
#ifdef XP_WIN
    
    rv = NS_NewLocalFile(NS_ConvertUTF8toUTF16(iniPath), PR_FALSE,
                         getter_AddRefs(iniFile));
#else
    rv = NS_NewNativeLocalFile(nsDependentCString(iniPath), PR_FALSE,
                               getter_AddRefs(iniFile));
#endif
    if (NS_FAILED(rv)) {
      Output(PR_TRUE, "Couldn't find application.ini file.\n");
      return 1;
    }

    AutoAppData appData(iniFile);
    if (!appData) {
      Output(PR_TRUE, "Error: couldn't parse application.ini.\n");
      return 1;
    }

    NS_ASSERTION(appData->directory, "Failed to get app directory.");

    if (!appData->xreDirectory) {
      
      lastSlash = strrchr(greDir, PATH_SEPARATOR_CHAR);
      if (lastSlash) {
        *lastSlash = '\0';
      }
#ifdef XP_WIN
      
      NS_NewLocalFile(NS_ConvertUTF8toUTF16(greDir), PR_FALSE,
                      &appData->xreDirectory);
#else
      NS_NewNativeLocalFile(nsDependentCString(greDir), PR_FALSE,
                            &appData->xreDirectory);
#endif
    }

    retval = XRE_main(argc, argv, appData);
  }

  NS_LogTerm();

  XPCOMGlueShutdown();

  return retval;
}
