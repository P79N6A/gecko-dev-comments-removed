




#ifndef nsAppRunner_h__
#define nsAppRunner_h__

#ifdef XP_WIN
#include <windows.h>
#else
#include <limits.h>
#endif

#ifndef MAXPATHLEN
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#elif defined(_MAX_PATH)
#define MAXPATHLEN _MAX_PATH
#elif defined(CCHMAXPATH)
#define MAXPATHLEN CCHMAXPATH
#else
#define MAXPATHLEN 1024
#endif
#endif

#include "nscore.h"
#include "nsXULAppAPI.h"




#define NS_LOCALSTORE_UNSAFE_FILE "LStoreS"

class nsINativeAppSupport;
class nsXREDirProvider;
class nsIToolkitProfileService;
class nsIFile;
class nsIProfileLock;
class nsIProfileUnlocker;
class nsIFactory;

extern nsXREDirProvider* gDirServiceProvider;




extern const nsXREAppData* gAppData;
extern bool gSafeMode;

extern int    gArgc;
extern char **gArgv;
extern int    gRestartArgc;
extern char **gRestartArgv;
extern bool gLogConsoleErrors;

extern bool gIsGtest;






nsresult NS_CreateNativeAppSupport(nsINativeAppSupport* *aResult);

nsresult
NS_NewToolkitProfileService(nsIToolkitProfileService* *aResult);

nsresult
NS_NewToolkitProfileFactory(nsIFactory* *aResult);


















nsresult
NS_LockProfilePath(nsIFile* aPath, nsIFile* aTempPath,
                   nsIProfileUnlocker* *aUnlocker, nsIProfileLock* *aResult);

void
WriteConsoleLog();

#ifdef XP_WIN
void
UseParentConsole();

BOOL
WinLaunchChild(const wchar_t *exePath, int argc,
               char **argv, HANDLE userToken = nullptr,
               HANDLE *hProcess = nullptr);
BOOL
WriteStatusPending(LPCWSTR updateDirPath);
BOOL
WriteStatusApplied(LPCWSTR updateDirPath);
#endif

#define NS_NATIVEAPPSUPPORT_CONTRACTID "@mozilla.org/toolkit/native-app-support;1"

namespace mozilla {
namespace startup {
extern GeckoProcessType sChildProcessType;
}
}





void SetupErrorHandling(const char* progname);

#endif 
