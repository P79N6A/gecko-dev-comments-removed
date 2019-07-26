




#ifndef _nsXULAppAPI_h__
#define _nsXULAppAPI_h__

#include "prtypes.h"
#include "nsID.h"
#include "xrecore.h"
#include "nsXPCOM.h"
#include "nsISupports.h"
#include "prlog.h"
#include "nsXREAppData.h"

#include "mozilla/Assertions.h"






























#define XRE_USER_APP_DATA_DIR "UAppData"









#define XRE_EXTENSIONS_DIR_LIST "XREExtDL"






#define XRE_EXECUTABLE_FILE "XREExeF"










#define NS_APP_PROFILE_DIR_STARTUP "ProfDS"










#define NS_APP_PROFILE_LOCAL_DIR_STARTUP "ProfLDS"






#define XRE_SYS_LOCAL_EXTENSION_PARENT_DIR "XRESysLExtPD"








#define XRE_SYS_SHARE_EXTENSION_PARENT_DIR "XRESysSExtPD"





#define XRE_USER_SYS_EXTENSION_DIR "XREUSysExt"





#define XRE_APP_DISTRIBUTION_DIR "XREAppDist"









#define XRE_UPDATE_ROOT_DIR "UpdRootD"







#define XRE_MAIN_FLAG_USE_METRO 0x01


















XRE_API(int,
        XRE_main, (int argc, char* argv[], const nsXREAppData* aAppData,
                   PRUint32 aFlags))







XRE_API(nsresult,
        XRE_GetFileFromPath, (const char *aPath, nsIFile* *aResult))







XRE_API(nsresult,
        XRE_GetBinaryPath, (const char *argv0, nsIFile* *aResult))




XRE_API(const mozilla::Module*,
        XRE_GetStaticModule, ())








XRE_API(nsresult,
        XRE_LockProfileDirectory, (nsIFile* aDirectory,
                                   nsISupports* *aLockObject))





















XRE_API(nsresult,
        XRE_InitEmbedding2, (nsIFile *aLibXULDirectory,
                             nsIFile *aAppDirectory,
                             nsIDirectoryServiceProvider *aAppDirProvider))






XRE_API(nsresult,
        XRE_AddStaticComponent, (const mozilla::Module* aComponent))
















enum NSLocationType
{
  NS_COMPONENT_LOCATION,
  NS_SKIN_LOCATION,
  NS_BOOTSTRAPPED_LOCATION
};

XRE_API(nsresult,
        XRE_AddManifestLocation, (NSLocationType aType,
                                  nsIFile* aLocation))


















XRE_API(nsresult,
        XRE_AddJarManifestLocation, (NSLocationType aType,
                                     nsIFile* aLocation))



























XRE_API(void,
        XRE_NotifyProfile, ())




XRE_API(void,
        XRE_TermEmbedding, ())









XRE_API(nsresult,
        XRE_CreateAppData, (nsIFile* aINIFile,
                            nsXREAppData **aAppData))








XRE_API(nsresult,
        XRE_ParseAppData, (nsIFile* aINIFile,
                           nsXREAppData *aAppData))




XRE_API(void,
        XRE_FreeAppData, (nsXREAppData *aAppData))

enum GeckoProcessType {
  GeckoProcessType_Default = 0,

  GeckoProcessType_Plugin,
  GeckoProcessType_Content,

  GeckoProcessType_IPDLUnitTest,

  GeckoProcessType_End,
  GeckoProcessType_Invalid = GeckoProcessType_End
};

static const char* const kGeckoProcessTypeString[] = {
  "default",
  "plugin",
  "tab",
  "ipdlunittest"
};



MOZ_STATIC_ASSERT(sizeof(kGeckoProcessTypeString) /
                  sizeof(kGeckoProcessTypeString[0]) ==
                  GeckoProcessType_End,
                  "Array length mismatch");

XRE_API(const char*,
        XRE_ChildProcessTypeToString, (GeckoProcessType aProcessType))

XRE_API(GeckoProcessType,
        XRE_StringToChildProcessType, (const char* aProcessTypeString))

#if defined(MOZ_CRASHREPORTER)

XRE_API(bool,
        XRE_TakeMinidumpForChild, (PRUint32 aChildPid, nsIFile** aDump))


XRE_API(bool,
        XRE_SetRemoteExceptionHandler, (const char* aPipe))
#endif

XRE_API(nsresult,
        XRE_InitChildProcess, (int aArgc,
                               char* aArgv[],
                               GeckoProcessType aProcess))

XRE_API(GeckoProcessType,
        XRE_GetProcessType, ())

typedef void (*MainFunction)(void* aData);

XRE_API(nsresult,
        XRE_InitParentProcess, (int aArgc,
                                char* aArgv[],
                                MainFunction aMainFunction,
                                void* aMainFunctionExtraData))

XRE_API(int,
        XRE_RunIPDLTest, (int aArgc,
                          char* aArgv[]))

XRE_API(nsresult,
        XRE_RunAppShell, ())

XRE_API(nsresult,
        XRE_InitCommandLine, (int aArgc, char* aArgv[]))

XRE_API(nsresult,
        XRE_DeinitCommandLine, ())

class MessageLoop;

XRE_API(void,
        XRE_ShutdownChildProcess, ())

XRE_API(MessageLoop*,
        XRE_GetIOMessageLoop, ())

struct JSContext;
class JSString;

XRE_API(bool,
        XRE_SendTestShellCommand, (JSContext* aCx,
                                   JSString* aCommand,
                                   void* aCallback))
struct JSObject;

XRE_API(bool,
        XRE_GetChildGlobalObject, (JSContext* aCx,
                                   JSObject** globalp))

XRE_API(bool,
        XRE_ShutdownTestShell, ())

XRE_API(void,
        XRE_InstallX11ErrorHandler, ())

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#define XRE_HAS_DLL_BLOCKLIST
XRE_API(void,
        XRE_SetupDllBlocklist, ())
#endif

XRE_API(void,
        XRE_TelemetryAccumulate, (int aID, PRUint32 aSample))

XRE_API(void,
        XRE_InitOmnijar, (nsIFile* greOmni,
                          nsIFile* appOmni))

#ifdef XP_WIN



enum WindowsEnvironmentType {
  WindowsEnvironmentType_Desktop = 0,
  WindowsEnvironmentType_Metro = 1
};





XRE_API(WindowsEnvironmentType,
        XRE_GetWindowsEnvironment, ())
#endif 

#endif 
