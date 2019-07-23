




































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









#define XRE_UPDATE_ROOT_DIR "UpdRootD"

class nsACString;
struct nsStaticModuleInfo;

class nsINativeAppSupport;
class nsICmdLineService;
class nsXREDirProvider;
class nsIToolkitProfileService;
class nsILocalFile;
class nsIProfileLock;
class nsIProfileUnlocker;
class nsIFactory;

extern nsXREDirProvider* gDirServiceProvider;




extern const nsXREAppData* gAppData;
extern PRBool gSafeMode;

extern int    gArgc;
extern char **gArgv;
extern PRBool gLogConsoleErrors;






nsresult NS_CreateNativeAppSupport(nsINativeAppSupport* *aResult);

NS_HIDDEN_(nsresult)
NS_NewToolkitProfileService(nsIToolkitProfileService* *aResult);

NS_HIDDEN_(nsresult)
NS_NewToolkitProfileFactory(nsIFactory* *aResult);


















NS_HIDDEN_(nsresult)
NS_LockProfilePath(nsILocalFile* aPath, nsILocalFile* aTempPath,
                   nsIProfileUnlocker* *aUnlocker, nsIProfileLock* *aResult);

NS_HIDDEN_(void)
WriteConsoleLog();

#ifdef XP_WIN
BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, char **argv);
#endif

#define NS_NATIVEAPPSUPPORT_CONTRACTID "@mozilla.org/toolkit/native-app-support;1"



class ScopedAppData : public nsXREAppData
{
public:
  ScopedAppData() { Zero(); this->size = sizeof(*this); }

  ScopedAppData(const nsXREAppData* aAppData);

  void Zero() { memset(this, 0, sizeof(*this)); }

  ~ScopedAppData();
};








void SetAllocatedString(const char *&str, const char *newvalue);








void SetAllocatedString(const char *&str, const nsACString &newvalue);

template<class T>
void SetStrongPtr(T *&ptr, T* newvalue)
{
  NS_IF_RELEASE(ptr);
  ptr = newvalue;
  NS_IF_ADDREF(ptr);
}

#ifdef MOZ_IPC
namespace mozilla {
namespace startup {
extern GeckoProcessType sChildProcessType;
}
}
#endif

#endif 
