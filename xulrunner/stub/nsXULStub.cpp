




































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
#define PATH_SEPARATOR_CHAR '\\'
#define R_OK 04
#elif defined(XP_MACOSX)
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
#include "nsWindowsWMain.cpp"
#endif

#ifdef XP_BEOS
#include <Entry.h>
#include <Path.h>
#endif

#define VERSION_MAXLEN 128

static void Output(PRBool isError, const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#if (defined(XP_WIN) && !MOZ_WINCONSOLE) || defined(WINCE)
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

  WideCharToMultiByte(CP_ACP, 0, wide_path,-1,
		      iniPath, MAX_PATH, NULL, NULL);
  

#elif defined(XP_OS2)
   PPIB ppib;
   PTIB ptib;

   DosGetInfoBlocks(&ptib, &ppib);
   DosQueryModuleName(ppib->pib_hmte, sizeof(iniPath), iniPath);

#elif defined(XP_BEOS)
   BEntry e((const char *)argv[0], true); 
   BPath p;
   status_t err;
   err = e.GetPath(&p);
   NS_ASSERTION(err == B_OK, "realpath failed");

   if (err == B_OK)
     
     strcpy(iniPath, p.Path());

#else
  
  
  
  
  
  
  
  

  struct stat fileStat;

  if (!realpath(argv[0], iniPath) || stat(iniPath, &fileStat)) {
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
      if (realpath(tmpPath, iniPath) && stat(iniPath, &fileStat) == 0) {
        found = PR_TRUE;
        break;
      }
      token = strtok(NULL, ":");
    }
    free (pathdup);
    if (!found)
      return 1;
  }
#endif

  lastSlash = strrchr(iniPath, PATH_SEPARATOR_CHAR);
  if (!lastSlash)
    return 1;

  *(++lastSlash) = '\0';

  

  snprintf(greDir, sizeof(greDir),
           "%sxulrunner" XPCOM_FILE_PATH_SEPARATOR XPCOM_DLL,
           iniPath);

#ifdef WINCE
  wchar_t wideGreDir[MAX_PATH];
  MultiByteToWideChar(CP_ACP, 0, greDir, -1, wideGreDir, MAX_PATH);
  DWORD fileAttrs = GetFileAttributesW(wideGreDir);
  greFound = fileAttrs != INVALID_FILE_ATTRIBUTES;
#else
  greFound = (access(greDir, R_OK) == 0);
#endif
  strncpy(lastSlash, "application.ini", sizeof(iniPath) - (lastSlash - iniPath));

#endif

  nsINIParser parser;
  rv = parser.Init(iniPath);
  if (NS_FAILED(rv)) {
    fprintf(stderr, "Could not read application.ini\n");
    return 1;
  }

  if (!greFound) {
    char minVersion[VERSION_MAXLEN];

    
    
    char maxVersion[VERSION_MAXLEN] = "1.*";

    GREVersionRange range = {
      minVersion,
      PR_TRUE,
      maxVersion,
      PR_TRUE
    };

    rv = parser.GetString("Gecko", "MinVersion", minVersion, sizeof(minVersion));
    if (NS_FAILED(rv)) {
      fprintf(stderr,
              "The application.ini does not specify a [Gecko] MinVersion\n");
      return 1;
    }

    rv = parser.GetString("Gecko", "MaxVersion", maxVersion, sizeof(maxVersion));
    if (NS_SUCCEEDED(rv))
      range.upperInclusive = PR_TRUE;

    static const GREProperty kProperties[] = {
      { "xulrunner", "true" }
    };

    rv = GRE_GetGREPathWithProperties(&range, 1,
                                      kProperties, NS_ARRAY_LENGTH(kProperties),
                                      greDir, sizeof(greDir));
    if (NS_FAILED(rv)) {
      
      

      Output(PR_FALSE,
             "Could not find compatible GRE between version %s and %s.\n",
             range.lower, range.upper);
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

#ifndef WINCE
  rv = XPCOMGlueStartup(greDir);
  if (NS_FAILED(rv)) {
    Output(PR_TRUE, "Couldn't load XPCOM.\n");
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
    rv = NS_NewNativeLocalFile(nsDependentCString(iniPath), PR_FALSE,
                               getter_AddRefs(iniFile));
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
      NS_NewNativeLocalFile(nsDependentCString(greDir), PR_FALSE,
                            &appData->xreDirectory);
    }

    retval = XRE_main(argc, argv, appData);
  }

  NS_LogTerm();

  XPCOMGlueShutdown();
#else
  
  
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(si));
  si.cb = sizeof(si);
  wchar_t xrPath[MAX_PATH];
  MultiByteToWideChar(CP_ACP, 0, greDir, -1, xrPath, MAX_PATH);
  wchar_t* wLastSlash = wcsrchr(xrPath, PATH_SEPARATOR_CHAR);
  if (wLastSlash) {
    *wLastSlash = L'\0';
  }
  wcscat(xrPath, L"\\xulrunner.exe");
  
  wchar_t wideIniPath[MAX_PATH+2];
  swprintf(wideIniPath, L"\"%S\"", iniPath);
  
  CreateProcessW(xrPath, wideIniPath, NULL, NULL, FALSE, 0, NULL, NULL, 
                 &si, &pi); 
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD retval = 0;
  if (!GetExitCodeProcess(pi.hProcess, &retval))
    printf("failed to get exit code, error = %d\n", retval = GetLastError());
  
  
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
#endif
  return retval;
}
