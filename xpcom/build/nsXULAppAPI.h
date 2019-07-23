






































#ifndef _nsXULAppAPI_h__
#define _nsXULAppAPI_h__

#include "prtypes.h"
#include "nsID.h"
#include "xrecore.h"
#include "nsXPCOM.h"
#include "nsISupports.h"
#include "prlog.h"













struct nsXREAppData
{
  




  PRUint32 size;

  



  nsILocalFile* directory;

  




  const char *vendor;

  




  const char *name;

  




  const char *version;

  


  const char *buildID;

  









  const char *ID;

  



  const char *copyright;

  


  PRUint32 flags;

  



  nsILocalFile* xreDirectory;

  


  const char *minVersion;
  const char *maxVersion;

  


  const char *crashReporterURL;

  














  const char *profile;
};





#define NS_XRE_ENABLE_PROFILE_MIGRATOR (1 << 1)





#define NS_XRE_ENABLE_EXTENSION_MANAGER (1 << 2)




#define NS_XRE_ENABLE_CRASH_REPORTER (1 << 3)






























#define XRE_USER_APP_DATA_DIR "UAppData"









#define XRE_EXTENSIONS_DIR_LIST "XREExtDL"






#define XRE_EXECUTABLE_FILE "XREExeF"










#define NS_APP_PROFILE_DIR_STARTUP "ProfDS"










#define NS_APP_PROFILE_LOCAL_DIR_STARTUP "ProfLDS"






#define XRE_SYS_LOCAL_EXTENSION_PARENT_DIR "XRESysLExtPD"








#define XRE_SYS_SHARE_EXTENSION_PARENT_DIR "XRESysSExtPD"





#define XRE_USER_SYS_EXTENSION_DIR "XREUSysExt"

















XRE_API(int,
        XRE_main, (int argc, char* argv[], const nsXREAppData* sAppData))







XRE_API(nsresult,
        XRE_GetFileFromPath, (const char *aPath, nsILocalFile* *aResult))







XRE_API(nsresult,
        XRE_GetBinaryPath, (const char *argv0, nsILocalFile* *aResult))




XRE_API(void,
        XRE_GetStaticComponents, (nsStaticModuleInfo const **aStaticComponents,
                                  PRUint32 *aComponentCount))








XRE_API(nsresult,
        XRE_LockProfileDirectory, (nsILocalFile* aDirectory,
                                   nsISupports* *aLockObject))



























XRE_API(nsresult,
        XRE_InitEmbedding, (nsILocalFile *aLibXULDirectory,
                            nsILocalFile *aAppDirectory,
                            nsIDirectoryServiceProvider *aAppDirProvider,
                            nsStaticModuleInfo const *aStaticComponents,
                            PRUint32 aStaticComponentCount))



























XRE_API(void,
        XRE_NotifyProfile, ())




XRE_API(void,
        XRE_TermEmbedding, ())









XRE_API(nsresult,
        XRE_CreateAppData, (nsILocalFile* aINIFile,
                            nsXREAppData **aAppData))








XRE_API(nsresult,
        XRE_ParseAppData, (nsILocalFile* aINIFile,
                           nsXREAppData *aAppData))




XRE_API(void,
        XRE_FreeAppData, (nsXREAppData *aAppData))

enum GeckoProcessType {
  GeckoProcessType_Default = 0,

  GeckoProcessType_Plugin,
  GeckoProcessType_Content,

  GeckoProcessType_TestHarness,

  GeckoProcessType_End,
  GeckoProcessType_Invalid = GeckoProcessType_End
};

static const char* const kGeckoProcessTypeString[] = {
  "default",
  "plugin",
  "tab",
  "testharness",
};

PR_STATIC_ASSERT(sizeof(kGeckoProcessTypeString) /
                 sizeof(kGeckoProcessTypeString[0]) ==
                 GeckoProcessType_End);


XRE_API(const char*,
        XRE_ChildProcessTypeToString, (GeckoProcessType aProcessType))

XRE_API(GeckoProcessType,
        XRE_StringToChildProcessType, (const char* aProcessTypeString))

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
        XRE_RunTestShell, (int aArgc,
                           char* aArgv[]))

XRE_API(int,
        XRE_RunIPCTestHarness, (int aArgc,
                                char* aArgv[]))

#endif 
