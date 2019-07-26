



#ifndef mozilla_BinaryPath_h
#define mozilla_BinaryPath_h

#include "nsXPCOMPrivate.h" 
#ifdef XP_WIN
#include <windows.h>
#elif defined(XP_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#elif defined(XP_UNIX)
#include <sys/stat.h>
#include <string.h>
#endif

namespace mozilla {

class BinaryPath {
public:
#ifdef XP_WIN
  static nsresult Get(const char *argv0, char aResult[MAXPATHLEN])
  {
    PRUnichar wide_path[MAXPATHLEN];
    nsresult rv = GetW(argv0, wide_path);
    if (NS_FAILED(rv))
      return rv;
    WideCharToMultiByte(CP_UTF8, 0, wide_path, -1,
                        aResult, MAXPATHLEN, nullptr, nullptr);
    return NS_OK;
  }

private:
  static nsresult GetW(const char *argv0, PRUnichar aResult[MAXPATHLEN])
  {
    if (::GetModuleFileNameW(0, aResult, MAXPATHLEN))
      return NS_OK;
    return NS_ERROR_FAILURE;
  }

#elif defined(XP_MACOSX)
  static nsresult Get(const char *argv0, char aResult[MAXPATHLEN])
  {
    
    CFBundleRef appBundle = CFBundleGetMainBundle();
    if (!appBundle)
      return NS_ERROR_FAILURE;

    CFURLRef executableURL = CFBundleCopyExecutableURL(appBundle);
    if (!executableURL)
      return NS_ERROR_FAILURE;

    nsresult rv;
    if (CFURLGetFileSystemRepresentation(executableURL, false, (UInt8 *)aResult, MAXPATHLEN))
      rv = NS_OK;
    else
      rv = NS_ERROR_FAILURE;
    CFRelease(executableURL);
    return rv;
  }

#elif defined(ANDROID)
  static nsresult Get(const char *argv0, char aResult[MAXPATHLEN])
  {
    
    
    const char *greHome = getenv("GRE_HOME");
#if defined(MOZ_WIDGET_GONK)
    if (!greHome)
      greHome = "/system/b2g";
#endif

    if (!greHome)
      return NS_ERROR_FAILURE;

    snprintf(aResult, MAXPATHLEN, "%s/%s", greHome, "dummy");
    aResult[MAXPATHLEN-1] = '\0';
    return NS_OK;
  }

#elif defined(XP_UNIX)
  static nsresult Get(const char *argv0, char aResult[MAXPATHLEN])
  {
    struct stat fileStat;
    
    
    
    
    
    
    
    
    if (strchr(argv0, '/') && realpath(argv0, aResult) &&
        stat(aResult, &fileStat) == 0)
      return NS_OK;

    const char *path = getenv("PATH");
    if (!path)
      return NS_ERROR_FAILURE;

    char *pathdup = strdup(path);
    if (!pathdup)
      return NS_ERROR_OUT_OF_MEMORY;

    bool found = false;
    char *token = strtok(pathdup, ":");
    while (token) {
      char tmpPath[MAXPATHLEN];
      sprintf(tmpPath, "%s/%s", token, argv0);
      if (realpath(tmpPath, aResult) && stat(aResult, &fileStat) == 0) {
        found = true;
        break;
      }
      token = strtok(nullptr, ":");
    }
    free(pathdup);
    if (found)
      return NS_OK;
    return NS_ERROR_FAILURE;
  }

#elif defined(XP_OS2)
  static nsresult Get(const char *argv0, char aResult[MAXPATHLEN])
  {
    PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks( &ptib, &ppib);
    DosQueryModuleName(ppib->pib_hmte, MAXPATHLEN, aResult);
  }

#else
#error Oops, you need platform-specific code here
#endif

public:
  static nsresult GetFile(const char *argv0, nsIFile* *aResult)
  {
    nsCOMPtr<nsIFile> lf;
#ifdef XP_WIN
    PRUnichar exePath[MAXPATHLEN];
    nsresult rv = GetW(argv0, exePath);
#else
    char exePath[MAXPATHLEN];
    nsresult rv = Get(argv0, exePath);
#endif
    if (NS_FAILED(rv))
      return rv;
#ifdef XP_WIN
    rv = NS_NewLocalFile(nsDependentString(exePath), true,
                         getter_AddRefs(lf));
#else
    rv = NS_NewNativeLocalFile(nsDependentCString(exePath), true,
                               getter_AddRefs(lf));
#endif
    if (NS_FAILED(rv))
      return rv;
    NS_ADDREF(*aResult = lf);
    return NS_OK;
  }
};

}

#endif 
