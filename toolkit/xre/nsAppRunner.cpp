











































#define XPCOM_TRANSLATE_NSGM_ENTRY_POINT 1

#include "nsAppRunner.h"
#include "nsUpdateDriver.h"

#ifdef XP_MACOSX
#include "MacLaunchHelper.h"
#include "MacApplicationDelegate.h"
#endif

#ifdef XP_OS2
#include "private/pprthred.h"
#endif
#include "prmem.h"
#include "prnetdb.h"
#include "prprf.h"
#include "prproces.h"
#include "prenv.h"

#include "nsIAppShellService.h"
#include "nsIAppStartup.h"
#include "nsIAppStartupNotifier.h"
#include "nsIMutableArray.h"
#include "nsICategoryManager.h"
#include "nsIChromeRegistry.h"
#include "nsICommandLineRunner.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIContentHandler.h"
#include "nsIDialogParamBlock.h"
#include "nsIDOMWindow.h"
#include "nsIExtensionManager.h"
#include "nsIFastLoadService.h" 
#include "nsIGenericFactory.h"
#include "nsIIOService.h"
#include "nsIObserverService.h"
#include "nsINativeAppSupport.h"
#include "nsIProcess.h"
#include "nsIProfileUnlocker.h"
#include "nsIPromptService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsITimelineService.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIToolkitProfile.h"
#include "nsIToolkitProfileService.h"
#include "nsIURI.h"
#include "nsIWindowCreator.h"
#include "nsIWindowMediator.h"
#include "nsIWindowWatcher.h"
#include "nsIXULAppInfo.h"
#include "nsIXULRuntime.h"
#include "nsPIDOMWindow.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIDocShell.h"
#include "nsAppShellCID.h"

#ifdef XP_WIN
#include "nsIWinAppHelper.h"
#endif

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsEmbedCID.h"
#include "nsNetUtil.h"
#include "nsStaticComponents.h"
#include "nsXPCOM.h"
#include "nsXPIDLString.h"
#include "nsXPFEComponentsCID.h"
#include "nsVersionComparator.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsXREDirProvider.h"
#include "nsToolkitCompsCID.h"

#include "nsINIParser.h"

#ifdef MOZ_XPINSTALL
#include "InstallCleanupDefines.h"
#endif

#include <stdlib.h>

#ifdef XP_UNIX
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef XP_BEOS


#include <AppKit.h>
#include <AppFileInfo.h>
#endif 

#ifdef XP_WIN
#include <process.h>
#include <shlobj.h>
#include "nsThreadUtils.h"
#endif

#ifdef XP_OS2
#include <process.h>
#endif 

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
#endif


#ifdef MOZ_ENABLE_XREMOTE
#ifdef MOZ_WIDGET_PHOTON
#include "PhRemoteClient.h"
#else
#include "XRemoteClient.h"
#endif
#include "nsIRemoteService.h"
#endif

#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif

#if defined(DEBUG) && defined(XP_WIN32)
#include <malloc.h>
#endif

#if defined (XP_MACOSX)
#include <Processes.h>
#include <Events.h>
#endif

extern "C" void ShowOSAlert(const char* aMessage);

#ifdef DEBUG
#include "prlog.h"
#endif

#ifdef MOZ_JPROF
#include "jprof.h"
#endif

#ifdef MOZ_AIRBAG
#include "nsAirbagExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#endif












#ifdef MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS

extern "C" {

# ifndef HAVE___BUILTIN_VEC_NEW
  void *__builtin_vec_new(size_t aSize, const std::nothrow_t &aNoThrow) throw()
  {
    return ::operator new(aSize, aNoThrow);
  }
# endif

# ifndef HAVE___BUILTIN_VEC_DELETE
  void __builtin_vec_delete(void *aPtr, const std::nothrow_t &) throw ()
  {
    if (aPtr) {
      free(aPtr);
    }
  }
# endif

# ifndef HAVE___BUILTIN_NEW
	void *__builtin_new(int aSize)
  {
    return malloc(aSize);
  }
# endif

# ifndef HAVE___BUILTIN_DELETE
	void __builtin_delete(void *aPtr)
  {
    free(aPtr);
  }
# endif

# ifndef HAVE___PURE_VIRTUAL
  void __pure_virtual(void) {
#ifdef WRAP_SYSTEM_INCLUDES
#pragma GCC visibility push(default)
#endif
    extern void __cxa_pure_virtual(void);
#ifdef WRAP_SYSTEM_INCLUDES
#pragma GCC visibility pop
#endif

    __cxa_pure_virtual();
  }
# endif
}
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
  extern void InstallUnixSignalHandlers(const char *ProgramName);
#endif

int    gArgc;
char **gArgv;

static char gToolkitVersion[20];
static char gToolkitBuildID[40];

static int    gRestartArgc;
static char **gRestartArgv;

#if defined(MOZ_WIDGET_GTK2)
#include <gtk/gtk.h>
#include "nsGTKToolkit.h"
#endif


static void
SaveFileToEnv(const char *name, nsIFile *file)
{
  nsCAutoString path;
  file->GetNativePath(path);

  char *expr = PR_smprintf("%s=%s", name, path.get());
  if (expr)
    PR_SetEnv(expr);
  
}



static void
SaveFileToEnvIfUnset(const char *name, nsIFile *file)
{
  const char *val = PR_GetEnv(name);
  if (!(val && *val))
    SaveFileToEnv(name, file);
}

static PRBool
strimatch(const char* lowerstr, const char* mixedstr)
{
  while(*lowerstr) {
    if (!*mixedstr) return PR_FALSE; 
    if (tolower(*mixedstr) != *lowerstr) return PR_FALSE; 

    ++lowerstr;
    ++mixedstr;
  }

  if (*mixedstr) return PR_FALSE; 

  return PR_TRUE;
}










static void Output(PRBool isError, const char *fmt, ... )
{
  va_list ap;
  va_start(ap, fmt);

#if defined(XP_WIN) && !MOZ_WINCONSOLE
  char *msg = PR_vsmprintf(fmt, ap);
  if (msg)
  {
    UINT flags = MB_OK;
    if (isError)
      flags |= MB_ICONERROR;
    else 
      flags |= MB_ICONINFORMATION;
    MessageBox(NULL, msg, "XULRunner", flags);
    PR_smprintf_free(msg);
  }
#else
  vfprintf(stderr, fmt, ap);
#endif

  va_end(ap);
}

enum ArgResult {
  ARG_NONE  = 0,
  ARG_FOUND = 1,
  ARG_BAD   = 2 
};

static void RemoveArg(char **argv)
{
  do {
    *argv = *(argv + 1);
    ++argv;
  } while (*argv);

  --gArgc;
}










static ArgResult
CheckArg(const char* aArg, const char **aParam = nsnull)
{
  char **curarg = gArgv + 1; 

  while (*curarg) {
    char *arg = curarg[0];

    if (arg[0] == '-'
#if defined(XP_WIN) || defined(XP_OS2)
        || *arg == '/'
#endif
        ) {
      ++arg;
      if (*arg == '-')
        ++arg;

      if (strimatch(aArg, arg)) {
        RemoveArg(curarg);
        if (!aParam) {
          return ARG_FOUND;
        }

        if (*curarg) {
          if (**curarg == '-'
#if defined(XP_WIN) || defined(XP_OS2)
              || **curarg == '/'
#endif
              )
            return ARG_BAD;

          *aParam = *curarg;
          RemoveArg(curarg);
          return ARG_FOUND;
        }
        return ARG_BAD;
      }
    }

    ++curarg;
  }

  return ARG_NONE;
}

#if defined(XP_WIN)






static ArgResult
CheckArgShell(const char* aArg)
{
  char **curarg = gRestartArgv + 1; 

  while (*curarg) {
    char *arg = curarg[0];

    if (arg[0] == '-') {
      ++arg;

      if (strimatch(aArg, arg)) {
        do {
          *curarg = *(curarg + 1);
          ++curarg;
        } while (*curarg);

        --gRestartArgc;

        return ARG_FOUND;
      }
    }

    ++curarg;
  }

  return ARG_NONE;
}







static void
ProcessDDE(nsINativeAppSupport* aNative)
{
  
  
  
  
  
  
  
  
  ArgResult ar;
  ar = CheckArgShell("requestpending");
  if (ar == ARG_FOUND) {
    aNative->Enable(); 
    nsIThread *thread = NS_GetCurrentThread();
    
    
    PRInt32 count = 20;
    while(--count >= 0) {
      NS_ProcessNextEvent(thread);
      PR_Sleep(PR_MillisecondsToInterval(1));
    }
  }
}
#endif

PRBool gSafeMode = PR_FALSE;





class nsXULAppInfo : public nsIXULAppInfo,
#ifdef XP_WIN
                     public nsIWinAppHelper,
#endif
#ifdef MOZ_AIRBAG
                     public nsICrashReporter,
#endif
                     public nsIXULRuntime
                     
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIXULAPPINFO
  NS_DECL_NSIXULRUNTIME
#ifdef MOZ_AIRBAG
  NS_DECL_NSICRASHREPORTER
#endif
#ifdef XP_WIN
  NS_DECL_NSIWINAPPHELPER
private:
  nsresult LaunchAppHelperWithArgs(int aArgc, char **aArgv);
#endif
};

NS_INTERFACE_MAP_BEGIN(nsXULAppInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULRuntime)
  NS_INTERFACE_MAP_ENTRY(nsIXULRuntime)
#ifdef XP_WIN
  NS_INTERFACE_MAP_ENTRY(nsIWinAppHelper)
#endif
#ifdef MOZ_AIRBAG
  NS_INTERFACE_MAP_ENTRY(nsICrashReporter)
#endif
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIXULAppInfo, gAppData)
NS_INTERFACE_MAP_END

NS_IMETHODIMP_(nsrefcnt)
nsXULAppInfo::AddRef()
{
  return 1;
}

NS_IMETHODIMP_(nsrefcnt)
nsXULAppInfo::Release()
{
  return 1;
}

NS_IMETHODIMP
nsXULAppInfo::GetVendor(nsACString& aResult)
{
  aResult.Assign(gAppData->vendor);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetName(nsACString& aResult)
{
  aResult.Assign(gAppData->name);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetID(nsACString& aResult)
{
  aResult.Assign(gAppData->ID);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetVersion(nsACString& aResult)
{
  aResult.Assign(gAppData->version);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetPlatformVersion(nsACString& aResult)
{
  aResult.AssignLiteral(gToolkitVersion);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetAppBuildID(nsACString& aResult)
{
  aResult.Assign(gAppData->buildID);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetPlatformBuildID(nsACString& aResult)
{
  aResult.Assign(gToolkitBuildID);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetLogConsoleErrors(PRBool *aResult)
{
  *aResult = gLogConsoleErrors;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetLogConsoleErrors(PRBool aValue)
{
  gLogConsoleErrors = aValue;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetInSafeMode(PRBool *aResult)
{
  *aResult = gSafeMode;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetOS(nsACString& aResult)
{
  aResult.AssignLiteral(OS_TARGET);
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetXPCOMABI(nsACString& aResult)
{
#ifdef TARGET_XPCOM_ABI
  aResult.AssignLiteral(TARGET_XPCOM_ABI);
  return NS_OK;
#else
  return NS_ERROR_NOT_AVAILABLE;
#endif
}

#ifdef XP_WIN
nsresult 
nsXULAppInfo::LaunchAppHelperWithArgs(int aArgc, char **aArgv)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> appHelper;
  rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(appHelper));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->AppendNative(NS_LITERAL_CSTRING("uninstall"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->AppendNative(NS_LITERAL_CSTRING("helper.exe"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString appHelperPath;
  rv = appHelper->GetNativePath(appHelperPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!WinLaunchChild(appHelperPath.get(), aArgc, aArgv, 1))
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::FixReg()
{
  int resetRegArgc = 2;
  char **resetRegArgv = (char**) malloc(sizeof(char*) * (resetRegArgc + 1));
  if (!resetRegArgv)
    return NS_ERROR_OUT_OF_MEMORY;

  resetRegArgv[0] = "argv0ignoredbywinlaunchchild";
  resetRegArgv[1] = "/fixreg";
  resetRegArgv[2] = nsnull;
  nsresult rv = LaunchAppHelperWithArgs(resetRegArgc, resetRegArgv);
  free(resetRegArgv);
  return rv;
}

NS_IMETHODIMP
nsXULAppInfo::PostUpdate(nsILocalFile *aLogFile)
{
  nsresult rv;
  int upgradeArgc = aLogFile ? 3 : 2;
  char **upgradeArgv = (char**) malloc(sizeof(char*) * (upgradeArgc + 1));

  if (!upgradeArgv)
    return NS_ERROR_OUT_OF_MEMORY;

  upgradeArgv[0] = "argv0ignoredbywinlaunchchild";
  upgradeArgv[1] = "/postupdate";

  char *pathArg = nsnull;

  if (aLogFile) {
    nsCAutoString logFilePath;
    rv = aLogFile->GetNativePath(logFilePath);
    NS_ENSURE_SUCCESS(rv, rv);

    pathArg = PR_smprintf("/uninstalllog=%s", logFilePath.get());
    if (!pathArg)
      return NS_ERROR_OUT_OF_MEMORY;

    upgradeArgv[2] = pathArg;
    upgradeArgv[3] = nsnull;
  }
  else {
    upgradeArgv[2] = nsnull;
  }

  rv = LaunchAppHelperWithArgs(upgradeArgc, upgradeArgv);
  
  if (pathArg)
    PR_smprintf_free(pathArg);

  free(upgradeArgv);
  return rv;
}
#endif

#ifdef MOZ_AIRBAG
NS_IMETHODIMP
nsXULAppInfo::AnnotateCrashReport(const nsACString& key,
                                  const nsACString& data)
{
  return CrashReporter::AnnotateCrashReport(key, data);
}
#endif

static const nsXULAppInfo kAppInfo;
static NS_METHOD AppInfoConstructor(nsISupports* aOuter,
                                    REFNSIID aIID, void **aResult)
{
  NS_ENSURE_NO_AGGREGATION(aOuter);

  return NS_CONST_CAST(nsXULAppInfo*, &kAppInfo)->
    QueryInterface(aIID, aResult);
}

PRBool gLogConsoleErrors
#ifdef DEBUG
         = PR_TRUE;
#else
         = PR_FALSE;
#endif

#define NS_ENSURE_TRUE_LOG(x, ret)               \
  PR_BEGIN_MACRO                                 \
  if (NS_UNLIKELY(!(x))) {                       \
    NS_WARNING("NS_ENSURE_TRUE(" #x ") failed"); \
    gLogConsoleErrors = PR_TRUE;                 \
    return ret;                                  \
  }                                              \
  PR_END_MACRO

#define NS_ENSURE_SUCCESS_LOG(res, ret)          \
  NS_ENSURE_TRUE_LOG(NS_SUCCEEDED(res), ret)







class ScopedXPCOMStartup
{
public:
  ScopedXPCOMStartup() :
    mServiceManager(nsnull) { }
  ~ScopedXPCOMStartup();

  nsresult Initialize();
  nsresult DoAutoreg();
  nsresult RegisterProfileService(nsIToolkitProfileService* aProfileService);
  nsresult SetWindowCreator(nsINativeAppSupport* native);

private:
  nsIServiceManager* mServiceManager;
};

ScopedXPCOMStartup::~ScopedXPCOMStartup()
{
  if (mServiceManager) {
    gDirServiceProvider->DoShutdown();

    WriteConsoleLog();

    NS_ShutdownXPCOM(mServiceManager);
    mServiceManager = nsnull;
  }
}


#define APPINFO_CID \
  { 0x95d89e3e, 0xa169, 0x41a3, { 0x8e, 0x56, 0x71, 0x99, 0x78, 0xe1, 0x5b, 0x12 } }

static nsModuleComponentInfo kComponents[] =
{
  {
    "nsXULAppInfo",
    APPINFO_CID,
    XULAPPINFO_SERVICE_CONTRACTID,
    AppInfoConstructor
  }
#ifdef MOZ_AIRBAG
,
  {
    "nsXULAppInfo",
    APPINFO_CID,
    NS_CRASHREPORTER_CONTRACTID,
    AppInfoConstructor
  }
#endif
};

NS_IMPL_NSGETMODULE(Apprunner, kComponents)

#if !defined(_BUILD_STATIC_BIN) && !defined(MOZ_ENABLE_LIBXUL)
static nsStaticModuleInfo const kXREStaticModules[] =
{
  {
    "Apprunner",
    Apprunner_NSGetModule
  }
};

nsStaticModuleInfo const *const kPStaticModules = kXREStaticModules;
PRUint32 const kStaticModuleCount = NS_ARRAY_LENGTH(kXREStaticModules);
#endif

nsresult
ScopedXPCOMStartup::Initialize()
{
  NS_ASSERTION(gDirServiceProvider, "Should not get here!");

  nsresult rv;
  rv = NS_InitXPCOM3(&mServiceManager, gDirServiceProvider->GetAppDir(),
                     gDirServiceProvider,
                     kPStaticModules, kStaticModuleCount);
  if (NS_FAILED(rv)) {
    NS_ERROR("Couldn't start xpcom!");
    mServiceManager = nsnull;
  }
  else {
    nsCOMPtr<nsIComponentRegistrar> reg =
      do_QueryInterface(mServiceManager);
    NS_ASSERTION(reg, "Service Manager doesn't QI to Registrar.");
  }

  return rv;
}


static const nsCID kNativeAppSupportCID =
  { 0xc4a446c, 0xee82, 0x41f2, { 0x8d, 0x4, 0xd3, 0x66, 0xd2, 0xc7, 0xa7, 0xd4 } };


static const nsCID kProfileServiceCID =
  { 0x5f5e59ce, 0x27bc, 0x47eb, { 0x9d, 0x1f, 0xb0, 0x9c, 0xa9, 0x4, 0x98, 0x36 } };

nsresult
ScopedXPCOMStartup::RegisterProfileService(nsIToolkitProfileService* aProfileService)
{
  NS_ASSERTION(mServiceManager, "Not initialized!");

  nsCOMPtr<nsIFactory> factory = do_QueryInterface(aProfileService);
  NS_ASSERTION(factory, "Supposed to be an nsIFactory!");

  nsCOMPtr<nsIComponentRegistrar> reg (do_QueryInterface(mServiceManager));
  if (!reg) return NS_ERROR_NO_INTERFACE;

  return reg->RegisterFactory(kProfileServiceCID,
                              "Toolkit Profile Service",
                              NS_PROFILESERVICE_CONTRACTID,
                              factory);
}

nsresult
ScopedXPCOMStartup::DoAutoreg()
{
#ifdef DEBUG
  
  
  
  nsCOMPtr<nsIComponentRegistrar> registrar
    (do_QueryInterface(mServiceManager));
  NS_ASSERTION(registrar, "Where's the component registrar?");

  registrar->AutoRegister(nsnull);
#endif

  return NS_OK;
}





class nsSingletonFactory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  nsSingletonFactory(nsISupports* aSingleton);
  ~nsSingletonFactory() { }

private:
  nsCOMPtr<nsISupports> mSingleton;
};

nsSingletonFactory::nsSingletonFactory(nsISupports* aSingleton)
  : mSingleton(aSingleton)
{
  NS_ASSERTION(mSingleton, "Singleton was null!");
}

NS_IMPL_ISUPPORTS1(nsSingletonFactory, nsIFactory)

NS_IMETHODIMP
nsSingletonFactory::CreateInstance(nsISupports* aOuter,
                                   const nsIID& aIID,
                                   void* *aResult)
{
  NS_ENSURE_NO_AGGREGATION(aOuter);

  return mSingleton->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsSingletonFactory::LockFactory(PRBool)
{
  return NS_OK;
}




nsresult
ScopedXPCOMStartup::SetWindowCreator(nsINativeAppSupport* native)
{
  nsresult rv;

  nsCOMPtr<nsIComponentRegistrar> registrar
    (do_QueryInterface(mServiceManager));
  NS_ASSERTION(registrar, "Where's the component registrar?");

  nsCOMPtr<nsIFactory> nativeFactory = new nsSingletonFactory(native);
  NS_ENSURE_TRUE(nativeFactory, NS_ERROR_OUT_OF_MEMORY);

  rv = registrar->RegisterFactory(kNativeAppSupportCID,
                                  "Native App Support",
                                  NS_NATIVEAPPSUPPORT_CONTRACTID,
                                  nativeFactory);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIToolkitChromeRegistry> cr (do_GetService(NS_CHROMEREGISTRY_CONTRACTID));
  if (cr)
    cr->CheckForOSAccessibility();

  nsCOMPtr<nsIWindowCreator> creator (do_GetService(NS_APPSTARTUP_CONTRACTID));
  if (!creator) return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIWindowWatcher> wwatch
    (do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  return wwatch->SetWindowCreator(creator);
}




class ScopedLogging
{
public:
  ScopedLogging() { NS_LogInit(); }
  ~ScopedLogging() { NS_LogTerm(); }
};

static void DumpArbitraryHelp()
{
  nsresult rv;

  ScopedLogging log;

  {
    nsXREDirProvider dirProvider;
    dirProvider.Initialize(nsnull, gAppData->xreDirectory);

    ScopedXPCOMStartup xpcom;
    xpcom.Initialize();
    xpcom.DoAutoreg();

    nsCOMPtr<nsICommandLineRunner> cmdline
      (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
    if (!cmdline)
      return;

    nsCString text;
    rv = cmdline->GetHelpText(text);
    if (NS_SUCCEEDED(rv))
      printf("%s", text.get());
  }
}




static void
DumpHelp()
{
  printf("Usage: %s [ options ... ] [URL]\n"
         "       where options include:\n\n", gArgv[0]);

#ifdef MOZ_X11
  printf("X11 options\n"
         "\t--display=DISPLAY\t\tX display to use\n"
         "\t--sync\t\tMake X calls synchronous\n"
         "\t--no-xshm\t\tDon't use X shared memory extension\n"
         "\t--xim-preedit=STYLE\n"
         "\t--xim-status=STYLE\n");
#endif
#ifdef XP_UNIX
  printf("\t--g-fatal-warnings\t\tMake all warnings fatal\n"
         "\nMozilla options\n");
#endif

  printf("\t-height <value>\t\tSet height of startup window to <value>.\n"
         "\t-h or -help\t\tPrint this message.\n"
         "\t-width <value>\t\tSet width of startup window to <value>.\n"
         "\t-v or -version\t\tPrint %s version.\n"
         "\t-P <profile>\t\tStart with <profile>.\n"
         "\t-ProfileManager\t\tStart with ProfileManager.\n"
         "\t-no-remote\t\tOpen new instance, not a new window in running instance.\n"
         "\t-UILocale <locale>\t\tStart with <locale> resources as UI Locale.\n"
         "\t-safe-mode\t\tDisables extensions and themes for this session.\n", gAppData->name);

#if defined(XP_WIN) || defined(XP_OS2)
  printf("\t-console\t\tStart %s with a debugging console.\n", gAppData->name);
#endif

  
  
  
  DumpArbitraryHelp();
}

#ifdef MOZ_XPINSTALL

static int
VerifyInstallation(nsIFile* aAppDir)
{
  static const char lastResortMessage[] =
    "A previous install did not complete correctly.  Finishing install.";

  
  

  char message[256];
  PRInt32 numRead = 0;
  const char *messageToShow = lastResortMessage;

  nsresult rv;
  nsCOMPtr<nsIFile> messageFile;
  rv = aAppDir->Clone(getter_AddRefs(messageFile));
  if (NS_SUCCEEDED(rv)) {
    messageFile->AppendNative(NS_LITERAL_CSTRING("res"));
    messageFile->AppendNative(CLEANUP_MESSAGE_FILENAME);
    PRFileDesc* fd = 0;

    nsCOMPtr<nsILocalFile> lf (do_QueryInterface(messageFile));
    if (lf) {
      rv = lf->OpenNSPRFileDesc(PR_RDONLY, 0664, &fd);
      if (NS_SUCCEEDED(rv)) {
        numRead = PR_Read(fd, message, sizeof(message)-1);
        if (numRead > 0) {
          message[numRead] = 0;
          messageToShow = message;
        }
      }
    }
  }

  ShowOSAlert(messageToShow);

  nsCOMPtr<nsIFile> cleanupUtility;
  aAppDir->Clone(getter_AddRefs(cleanupUtility));
  if (!cleanupUtility) return 1;

  cleanupUtility->AppendNative(CLEANUP_UTIL);

  ScopedXPCOMStartup xpcom;
  rv = xpcom.Initialize();
  if (NS_FAILED(rv)) return 1;

  { 
    
    nsCOMPtr<nsIProcess> cleanupProcess
      (do_CreateInstance(NS_PROCESS_CONTRACTID));
    rv = cleanupProcess->Init(cleanupUtility);
    if (NS_FAILED(rv)) return 1;

    rv = cleanupProcess->Run(PR_FALSE,nsnull, 0, nsnull);
    if (NS_FAILED(rv)) return 1;
  }

  return 0;
}
#endif

#ifdef DEBUG_warren
#ifdef XP_WIN
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if defined(FREEBSD)

#include <ieeefp.h>
#endif

static inline void
DumpVersion()
{
  printf("%s %s %s, %s\n", 
         gAppData->vendor ? gAppData->vendor : "", gAppData->name, gAppData->version, gAppData->copyright);
}

#ifdef MOZ_ENABLE_XREMOTE


static int
HandleRemoteArgument(const char* remote, const char* aDesktopStartupID)
{
  nsresult rv;
  ArgResult ar;

  const char *profile = 0;
  nsCAutoString program(gAppData->name);
  ToLowerCase(program);
  const char *username = getenv("LOGNAME");

  ar = CheckArg("p", &profile);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -p requires a profile name\n");
    return 1;
  }

  const char *temp = nsnull;
  ar = CheckArg("a", &temp);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -a requires an application name\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    program.Assign(temp);
  }

  ar = CheckArg("u", &username);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -u requires a username\n");
    return 1;
  }

  XRemoteClient client;
  rv = client.Init();
  if (NS_FAILED(rv)) {
    PR_fprintf(PR_STDERR, "Error: Failed to connect to X server.\n");
    return 1;
  }

  nsXPIDLCString response;
  PRBool success = PR_FALSE;
  rv = client.SendCommand(program.get(), username, profile, remote,
                          aDesktopStartupID, getter_Copies(response), &success);
  
  if (NS_FAILED(rv)) {
    PR_fprintf(PR_STDERR, "Error: Failed to send command: %s\n",
               response ? response.get() : "No response included");
    return 1;
  }

  if (!success) {
    PR_fprintf(PR_STDERR, "Error: No running window found\n");
    return 2;
  }

  return 0;
}

static PRBool
RemoteCommandLine(const char* aDesktopStartupID)
{
  nsresult rv;
  ArgResult ar;

  nsCAutoString program(gAppData->name);
  ToLowerCase(program);
  const char *username = getenv("LOGNAME");

  const char *temp = nsnull;
  ar = CheckArg("a", &temp);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -a requires an application name\n");
    return PR_FALSE;
  } else if (ar == ARG_FOUND) {
    program.Assign(temp);
  }

  ar = CheckArg("u", &username);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -u requires a username\n");
    return PR_FALSE;
  }

  XRemoteClient client;
  rv = client.Init();
  if (NS_FAILED(rv))
    return PR_FALSE;
 
  nsXPIDLCString response;
  PRBool success = PR_FALSE;
  rv = client.SendCommandLine(program.get(), username, nsnull,
                              gArgc, gArgv, aDesktopStartupID,
                              getter_Copies(response), &success);
  
  if (NS_FAILED(rv) || !success)
    return PR_FALSE;

  return PR_TRUE;
}
#endif 

#ifdef XP_MACOSX
static char const *gBinaryPath;
#endif

nsresult
XRE_GetBinaryPath(const char* argv0, nsILocalFile* *aResult)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> lf;

  
  
  

#ifdef XP_WIN
  char exePath[MAXPATHLEN];

  if (!::GetModuleFileName(0, exePath, MAXPATHLEN))
    return NS_ERROR_FAILURE;

  rv = NS_NewNativeLocalFile(nsDependentCString(exePath), PR_TRUE,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

#elif defined(XP_MACOSX)
  if (gBinaryPath)
    return NS_NewNativeLocalFile(nsDependentCString(gBinaryPath), PR_FALSE,
                                 aResult);

  NS_NewNativeLocalFile(EmptyCString(), PR_TRUE, getter_AddRefs(lf));
  nsCOMPtr<nsILocalFileMac> lfm (do_QueryInterface(lf));
  if (!lfm)
    return NS_ERROR_FAILURE;

  
  CFBundleRef appBundle = CFBundleGetMainBundle();
  if (!appBundle)
    return NS_ERROR_FAILURE;

  CFURLRef bundleURL = CFBundleCopyExecutableURL(appBundle);
  if (!bundleURL)
    return NS_ERROR_FAILURE;

  FSRef fileRef;
  if (!CFURLGetFSRef(bundleURL, &fileRef)) {
    CFRelease(bundleURL);
    return NS_ERROR_FAILURE;
  }

  rv = lfm->InitWithFSRef(&fileRef);
  CFRelease(bundleURL);

  if (NS_FAILED(rv))
    return rv;

#elif defined(XP_UNIX)
  struct stat fileStat;
  char exePath[MAXPATHLEN];
  char tmpPath[MAXPATHLEN];

  rv = NS_ERROR_FAILURE;

  
  
  
  
  
  
  
  
  
  


#if 0
  int r = readlink("/proc/self/exe", exePath, MAXPATHLEN);

  
  if (r > 0 && r < MAXPATHLEN && stat(exePath, &fileStat) == 0) {
    rv = NS_OK;
  }

#endif
  if (NS_FAILED(rv) &&
      realpath(argv0, exePath) && stat(exePath, &fileStat) == 0) {
    rv = NS_OK;
  }

  if (NS_FAILED(rv)) {
    const char *path = getenv("PATH");
    if (!path)
      return NS_ERROR_FAILURE;

    char *pathdup = strdup(path);
    if (!pathdup)
      return NS_ERROR_OUT_OF_MEMORY;

    PRBool found = PR_FALSE;
    char *newStr = pathdup;
    char *token;
    while ( (token = nsCRT::strtok(newStr, ":", &newStr)) ) {
      sprintf(tmpPath, "%s/%s", token, argv0);
      if (realpath(tmpPath, exePath) && stat(exePath, &fileStat) == 0) {
        found = PR_TRUE;
        break;
      }
    }
    free(pathdup);
    if (!found)
      return NS_ERROR_FAILURE;
  }

  rv = NS_NewNativeLocalFile(nsDependentCString(exePath), PR_TRUE,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

#elif defined(XP_OS2)
  PPIB ppib;
  PTIB ptib;
  char exePath[MAXPATHLEN];

  DosGetInfoBlocks( &ptib, &ppib);
  DosQueryModuleName( ppib->pib_hmte, MAXPATHLEN, exePath);
  rv = NS_NewNativeLocalFile(nsDependentCString(exePath), PR_TRUE,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

#elif defined(XP_BEOS)
  int32 cookie = 0;
  image_info info;

  if(get_next_image_info(0, &cookie, &info) != B_OK)
    return NS_ERROR_FAILURE;

  rv = NS_NewNativeLocalFile(nsDependentCString(info.name), PR_TRUE,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

#elif
#error Oops, you need platform-specific code here
#endif

  NS_ADDREF(*aResult = lf);
  return NS_OK;
}

#define NS_ERROR_LAUNCHED_CHILD_PROCESS NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_PROFILE, 200)

#ifdef XP_WIN
#include "nsWindowsRestart.cpp"
#endif




static nsresult LaunchChild(nsINativeAppSupport* aNative,
                            PRBool aBlankCommandLine = PR_FALSE,
                            int needElevation = 0)
{
  aNative->Quit(); 

  
  
 
  if (aBlankCommandLine) {
    gRestartArgc = 1;
    gRestartArgv[gRestartArgc] = nsnull;
  }

  PR_SetEnv("MOZ_LAUNCHED_CHILD=1");

#if defined(XP_MACOSX)
  LaunchChildMac(gRestartArgc, gRestartArgv);
#else
  nsCOMPtr<nsILocalFile> lf;
  nsresult rv = XRE_GetBinaryPath(gArgv[0], getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString exePath;
  rv = lf->GetNativePath(exePath);
  if (NS_FAILED(rv))
    return rv;

#if defined(XP_WIN)
  if (!WinLaunchChild(exePath.get(), gRestartArgc, gRestartArgv, needElevation))
    return NS_ERROR_FAILURE;
#elif defined(XP_OS2)
  if (_execv(exePath.get(), gRestartArgv) == -1)
    return NS_ERROR_FAILURE;
#elif defined(XP_UNIX)
  if (execv(exePath.get(), gRestartArgv) == -1)
    return NS_ERROR_FAILURE;
#elif defined(XP_BEOS)
  extern char **environ;
  status_t res;
  res = resume_thread(load_image(gRestartArgc,(const char **)gRestartArgv,(const char **)environ));
  if (res != B_OK)
    return NS_ERROR_FAILURE;
#else
  PRProcess* process = PR_CreateProcess(exePath.get(), gRestartArgv,
                                        nsnull, nsnull);
  if (!process) return NS_ERROR_FAILURE;

  PRInt32 exitCode;
  PRStatus failed = PR_WaitProcess(process, &exitCode);
  if (failed || exitCode)
    return NS_ERROR_FAILURE;
#endif
#endif

  return NS_ERROR_LAUNCHED_CHILD_PROCESS;
}

static const char kProfileProperties[] =
  "chrome://mozapps/locale/profile/profileSelection.properties";

static nsresult
ProfileLockedDialog(nsILocalFile* aProfileDir, nsILocalFile* aProfileLocalDir,
                    nsIProfileUnlocker* aUnlocker,
                    nsINativeAppSupport* aNative, nsIProfileLock* *aResult)
{
  nsresult rv;

  ScopedXPCOMStartup xpcom;
  rv = xpcom.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = xpcom.DoAutoreg();
  rv |= xpcom.SetWindowCreator(aNative);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  { 
    nsCOMPtr<nsIStringBundleService> sbs
      (do_GetService(NS_STRINGBUNDLE_CONTRACTID));
    NS_ENSURE_TRUE(sbs, NS_ERROR_FAILURE);

    nsCOMPtr<nsIStringBundle> sb;
    sbs->CreateBundle(kProfileProperties, getter_AddRefs(sb));
    NS_ENSURE_TRUE_LOG(sbs, NS_ERROR_FAILURE);

    NS_ConvertUTF8toUTF16 appName(gAppData->name);
    const PRUnichar* params[] = {appName.get(), appName.get()};

    nsXPIDLString killMessage;
#ifndef XP_MACOSX
    static const PRUnichar kRestartNoUnlocker[] = {'r','e','s','t','a','r','t','M','e','s','s','a','g','e','N','o','U','n','l','o','c','k','e','r','\0'}; 
    static const PRUnichar kRestartUnlocker[] = {'r','e','s','t','a','r','t','M','e','s','s','a','g','e','U','n','l','o','c','k','e','r','\0'}; 
#else
    static const PRUnichar kRestartNoUnlocker[] = {'r','e','s','t','a','r','t','M','e','s','s','a','g','e','N','o','U','n','l','o','c','k','e','r','M','a','c','\0'}; 
    static const PRUnichar kRestartUnlocker[] = {'r','e','s','t','a','r','t','M','e','s','s','a','g','e','U','n','l','o','c','k','e','r','M','a','c','\0'}; 
#endif

    sb->FormatStringFromName(aUnlocker ? kRestartUnlocker : kRestartNoUnlocker,
                             params, 2, getter_Copies(killMessage));

    nsXPIDLString killTitle;
    sb->FormatStringFromName(NS_LITERAL_STRING("restartTitle").get(),
                             params, 1, getter_Copies(killTitle));

    if (!killMessage || !killTitle)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIPromptService> ps
      (do_GetService(NS_PROMPTSERVICE_CONTRACTID));
    NS_ENSURE_TRUE(ps, NS_ERROR_FAILURE);

    PRUint32 flags = nsIPromptService::BUTTON_TITLE_OK * nsIPromptService::BUTTON_POS_0;

    if (aUnlocker) {
      flags =
        nsIPromptService::BUTTON_TITLE_CANCEL * nsIPromptService::BUTTON_POS_0 +
        nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_1 +
        nsIPromptService::BUTTON_POS_1_DEFAULT;
    }

    PRInt32 button;
    rv = ps->ConfirmEx(nsnull, killTitle, killMessage, flags,
                       killTitle, nsnull, nsnull, nsnull, nsnull, &button);
    NS_ENSURE_SUCCESS_LOG(rv, rv);

    if (button == 1 && aUnlocker) {
      rv = aUnlocker->Unlock(nsIProfileUnlocker::FORCE_QUIT);
      if (NS_FAILED(rv)) return rv;

      return NS_LockProfilePath(aProfileDir, aProfileLocalDir, nsnull, aResult);
    }

    return NS_ERROR_ABORT;
  }
}

static const char kProfileManagerURL[] =
  "chrome://mozapps/content/profile/profileSelection.xul";

static nsresult
ShowProfileManager(nsIToolkitProfileService* aProfileSvc,
                   nsINativeAppSupport* aNative)
{
  nsresult rv;

  nsCOMPtr<nsILocalFile> profD, profLD;

  {
    ScopedXPCOMStartup xpcom;
    rv = xpcom.Initialize();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = xpcom.RegisterProfileService(aProfileSvc);
    rv |= xpcom.DoAutoreg();
    rv |= xpcom.SetWindowCreator(aNative);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

#ifdef XP_MACOSX
    SetupMacCommandLine(gRestartArgc, gRestartArgv);
#endif

#ifdef XP_WIN
    ProcessDDE(aNative);
#endif

    { 
      nsCOMPtr<nsIWindowWatcher> windowWatcher
        (do_GetService(NS_WINDOWWATCHER_CONTRACTID));
      nsCOMPtr<nsIDialogParamBlock> ioParamBlock
        (do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
      nsCOMPtr<nsIMutableArray> dlgArray (do_CreateInstance(NS_ARRAY_CONTRACTID));
      NS_ENSURE_TRUE(windowWatcher && ioParamBlock && dlgArray, NS_ERROR_FAILURE);

      ioParamBlock->SetObjects(dlgArray);

      nsCOMPtr<nsIAppStartup> appStartup
        (do_GetService(NS_APPSTARTUP_CONTRACTID));
      NS_ENSURE_TRUE(appStartup, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMWindow> newWindow;
      rv = windowWatcher->OpenWindow(nsnull,
                                     kProfileManagerURL,
                                     "_blank",
                                     "centerscreen,chrome,modal,titlebar",
                                     ioParamBlock,
                                     getter_AddRefs(newWindow));

      NS_ENSURE_SUCCESS_LOG(rv, rv);

      aProfileSvc->Flush();

      PRInt32 dialogConfirmed;
      rv = ioParamBlock->GetInt(0, &dialogConfirmed);
      if (NS_FAILED(rv) || dialogConfirmed == 0) return NS_ERROR_ABORT;

      nsCOMPtr<nsIProfileLock> lock;
      rv = dlgArray->QueryElementAt(0, NS_GET_IID(nsIProfileLock),
                                    getter_AddRefs(lock));
      NS_ENSURE_SUCCESS_LOG(rv, rv);

      rv = lock->GetDirectory(getter_AddRefs(profD));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = lock->GetLocalDirectory(getter_AddRefs(profLD));
      NS_ENSURE_SUCCESS(rv, rv);

      lock->Unlock();
    }
  }

  SaveFileToEnv("XRE_PROFILE_PATH", profD);
  SaveFileToEnv("XRE_PROFILE_LOCAL_PATH", profLD);

  PRBool offline = PR_FALSE;
  aProfileSvc->GetStartOffline(&offline);
  if (offline) {
    PR_SetEnv("XRE_START_OFFLINE=1");
  }

  return LaunchChild(aNative);
}

static nsresult
ImportProfiles(nsIToolkitProfileService* aPService,
               nsINativeAppSupport* aNative)
{
  nsresult rv;

  PR_SetEnv("XRE_IMPORT_PROFILES=1");

  
  { 
    ScopedXPCOMStartup xpcom;
    rv = xpcom.Initialize();
    if (NS_SUCCEEDED(rv)) {
      xpcom.DoAutoreg();
      xpcom.RegisterProfileService(aPService);

#ifdef XP_MACOSX
      SetupMacCommandLine(gRestartArgc, gRestartArgv);
#endif

      nsCOMPtr<nsIProfileMigrator> migrator
        (do_GetService(NS_PROFILEMIGRATOR_CONTRACTID));
      if (migrator) {
        migrator->Import();
      }
    }
  }

  aPService->Flush();
  return LaunchChild(aNative);
}










static PRBool gDoMigration = PR_FALSE;

static nsresult
SelectProfile(nsIProfileLock* *aResult, nsINativeAppSupport* aNative,
              PRBool* aStartOffline)
{
  nsresult rv;
  ArgResult ar;
  const char* arg;
  *aResult = nsnull;
  *aStartOffline = PR_FALSE;

  arg = PR_GetEnv("XRE_START_OFFLINE");
  if ((arg && *arg) || CheckArg("offline"))
    *aStartOffline = PR_TRUE;

  arg = PR_GetEnv("XRE_PROFILE_PATH");
  if (arg && *arg) {
    nsCOMPtr<nsILocalFile> lf;
    rv = NS_NewNativeLocalFile(nsDependentCString(arg), PR_TRUE,
                               getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsILocalFile> localDir;
    arg = PR_GetEnv("XRE_PROFILE_LOCAL_PATH");
    if (arg && *arg) {
      rv = NS_NewNativeLocalFile(nsDependentCString(arg), PR_TRUE,
                                 getter_AddRefs(localDir));
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      localDir = lf;
    }

    
    const char *dummy;
    CheckArg("p", &dummy);
    CheckArg("profile", &dummy);
    CheckArg("profilemanager");

    return NS_LockProfilePath(lf, localDir, nsnull, aResult);
  }

  if (CheckArg("migration"))
    gDoMigration = PR_TRUE;

  ar = CheckArg("profile", &arg);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -profile requires a path\n");
    return NS_ERROR_FAILURE;
  }
  if (ar) {
    nsCOMPtr<nsILocalFile> lf;
    rv = XRE_GetFileFromPath(arg, getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIProfileUnlocker> unlocker;

    
    
    rv = NS_LockProfilePath(lf, lf, getter_AddRefs(unlocker), aResult);
    if (NS_SUCCEEDED(rv))
      return rv;

    return ProfileLockedDialog(lf, lf, unlocker, aNative, aResult);
  }

  nsCOMPtr<nsIToolkitProfileService> profileSvc;
  rv = NS_NewToolkitProfileService(getter_AddRefs(profileSvc));
  NS_ENSURE_SUCCESS(rv, rv);

  ar = CheckArg("createprofile", &arg);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -createprofile requires a profile name\n");
    return NS_ERROR_FAILURE;
  }
  if (ar) {
    nsCOMPtr<nsIToolkitProfile> profile;

    const char* delim = strchr(arg, ' ');
    if (delim) {
      nsCOMPtr<nsILocalFile> lf;
      rv = NS_NewNativeLocalFile(nsDependentCString(delim + 1),
                                   PR_TRUE, getter_AddRefs(lf));
      if (NS_FAILED(rv)) {
        PR_fprintf(PR_STDERR, "Error: profile path not valid.\n");
        return rv;
      }
      
      
      
      rv = profileSvc->CreateProfile(lf, lf, nsDependentCSubstring(arg, delim),
                                     getter_AddRefs(profile));
    } else {
      rv = profileSvc->CreateProfile(nsnull, nsnull, nsDependentCString(arg),
                                     getter_AddRefs(profile));
    }
    
    if (NS_FAILED(rv)) {
      PR_fprintf(PR_STDERR, "Error creating profile.\n");
      return rv; 
    }
    rv = NS_ERROR_ABORT;  
    profileSvc->Flush();

    
    
    nsCOMPtr<nsILocalFile> prefsJSFile;
    profile->GetRootDir(getter_AddRefs(prefsJSFile));
    prefsJSFile->AppendNative(NS_LITERAL_CSTRING("prefs.js"));
    nsCAutoString pathStr;
    prefsJSFile->GetNativePath(pathStr);
    PR_fprintf(PR_STDERR, "Success: created profile '%s' at '%s'\n", arg, pathStr.get());
    PRBool exists;
    prefsJSFile->Exists(&exists);
    if (!exists)
      prefsJSFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
    

    return rv;
  }

  PRUint32 count;
  rv = profileSvc->GetProfileCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (gAppData->flags & NS_XRE_ENABLE_PROFILE_MIGRATOR) {
    arg = PR_GetEnv("XRE_IMPORT_PROFILES");
    if (!count && (!arg || !*arg)) {
      return ImportProfiles(profileSvc, aNative);
    }
  }

  ar = CheckArg("p", &arg);
  if (ar == ARG_BAD) {
    return ShowProfileManager(profileSvc, aNative);
  }
  if (ar) {
    nsCOMPtr<nsIToolkitProfile> profile;
    rv = profileSvc->GetProfileByName(nsDependentCString(arg),
                                      getter_AddRefs(profile));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIProfileUnlocker> unlocker;
      rv = profile->Lock(nsnull, aResult);
      if (NS_SUCCEEDED(rv))
        return NS_OK;

      nsCOMPtr<nsILocalFile> profileDir;
      rv = profile->GetRootDir(getter_AddRefs(profileDir));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsILocalFile> profileLocalDir;
      rv = profile->GetLocalDir(getter_AddRefs(profileLocalDir));
      NS_ENSURE_SUCCESS(rv, rv);

      return ProfileLockedDialog(profileDir, profileLocalDir, unlocker,
                                 aNative, aResult);
    }

    return ShowProfileManager(profileSvc, aNative);
  }

  if (CheckArg("profilemanager")) {
    return ShowProfileManager(profileSvc, aNative);
  }

  if (!count) {
    gDoMigration = PR_TRUE;

    
    nsCOMPtr<nsIToolkitProfile> profile;
    nsresult rv = profileSvc->CreateProfile(nsnull, 
                                            nsnull, 
                                            NS_LITERAL_CSTRING("default"),
                                            getter_AddRefs(profile));
    if (NS_SUCCEEDED(rv)) {
      profileSvc->Flush();
      rv = profile->Lock(nsnull, aResult);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }

  PRBool useDefault = PR_TRUE;
  if (count > 1)
    profileSvc->GetStartWithLastProfile(&useDefault);

  if (useDefault) {
    nsCOMPtr<nsIToolkitProfile> profile;
    
    profileSvc->GetSelectedProfile(getter_AddRefs(profile));
    if (profile) {
      nsCOMPtr<nsIProfileUnlocker> unlocker;
      rv = profile->Lock(getter_AddRefs(unlocker), aResult);
      if (NS_SUCCEEDED(rv))
        return NS_OK;

      nsCOMPtr<nsILocalFile> profileDir;
      rv = profile->GetRootDir(getter_AddRefs(profileDir));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsILocalFile> profileLocalDir;
      rv = profile->GetRootDir(getter_AddRefs(profileLocalDir));
      NS_ENSURE_SUCCESS(rv, rv);

      return ProfileLockedDialog(profileDir, profileLocalDir, unlocker,
                                 aNative, aResult);
    }
  }

  return ShowProfileManager(profileSvc, aNative);
}

#define FILE_COMPATIBILITY_INFO NS_LITERAL_CSTRING("compatibility.ini")

static PRBool
CheckCompatibility(nsIFile* aProfileDir, const nsCString& aVersion,
                   const nsCString& aOSABI, nsIFile* aXULRunnerDir,
                   nsIFile* aAppDir)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return PR_FALSE;
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsINIParser parser;
  nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(file));
  nsresult rv = parser.Init(localFile);
  if (NS_FAILED(rv))
    return PR_FALSE;

  nsCAutoString buf;
  rv = parser.GetString("Compatibility", "LastVersion", buf);
  if (NS_FAILED(rv) || !aVersion.Equals(buf))
    return PR_FALSE;

  rv = parser.GetString("Compatibility", "LastOSABI", buf);
  if (NS_FAILED(rv) || !aOSABI.Equals(buf))
    return PR_FALSE;

  rv = parser.GetString("Compatibility", "LastPlatformDir", buf);
  if (NS_FAILED(rv))
    return PR_FALSE;

  nsCOMPtr<nsILocalFile> lf;
  rv = NS_NewNativeLocalFile(buf, PR_FALSE,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return PR_FALSE;

  PRBool eq;
  rv = lf->Equals(aXULRunnerDir, &eq);
  if (NS_FAILED(rv) || !eq)
    return PR_FALSE;

  if (aAppDir) {
    rv = parser.GetString("Compatibility", "LastAppDir", buf);
    if (NS_FAILED(rv))
      return PR_FALSE;

    rv = NS_NewNativeLocalFile(buf, PR_FALSE,
                               getter_AddRefs(lf));
    if (NS_FAILED(rv))
      return PR_FALSE;

    rv = lf->Equals(aAppDir, &eq);
    if (NS_FAILED(rv) || !eq)
      return PR_FALSE;
  }

  return PR_TRUE;
}

static void BuildVersion(nsCString &aBuf)
{
  aBuf.Assign(gAppData->version);
  aBuf.Append('_');
  aBuf.Append(gAppData->buildID);
  aBuf.Append('/');
  aBuf.Append(gToolkitBuildID);
}

static void
WriteVersion(nsIFile* aProfileDir, const nsCString& aVersion,
             const nsCString& aOSABI, nsIFile* aXULRunnerDir,
             nsIFile* aAppDir)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return;
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsCOMPtr<nsILocalFile> lf = do_QueryInterface(file);

  nsCAutoString platformDir;
  aXULRunnerDir->GetNativePath(platformDir);

  nsCAutoString appDir;
  if (aAppDir)
    aAppDir->GetNativePath(appDir);

  PRFileDesc *fd = nsnull;
  lf->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0600, &fd);
  if (!fd) {
    NS_ERROR("could not create output stream");
    return;
  }

  static const char kHeader[] = "[Compatibility]" NS_LINEBREAK
                                "LastVersion=";

  PR_Write(fd, kHeader, sizeof(kHeader) - 1);
  PR_Write(fd, aVersion.get(), aVersion.Length());

  static const char kOSABIHeader[] = NS_LINEBREAK "LastOSABI=";
  PR_Write(fd, kOSABIHeader, sizeof(kOSABIHeader) - 1);
  PR_Write(fd, aOSABI.get(), aOSABI.Length());

  static const char kPlatformDirHeader[] = NS_LINEBREAK "LastPlatformDir=";

  PR_Write(fd, kPlatformDirHeader, sizeof(kPlatformDirHeader) - 1);
  PR_Write(fd, platformDir.get(), platformDir.Length());

  static const char kAppDirHeader[] = NS_LINEBREAK "LastAppDir=";
  if (aAppDir) {
    PR_Write(fd, kAppDirHeader, sizeof(kAppDirHeader) - 1);
    PR_Write(fd, appDir.get(), appDir.Length());
  }

  static const char kNL[] = NS_LINEBREAK;
  PR_Write(fd, kNL, sizeof(kNL) - 1);

  PR_Close(fd);
}

static PRBool ComponentsListChanged(nsIFile* aProfileDir)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return PR_TRUE;
  file->AppendNative(NS_LITERAL_CSTRING(".autoreg"));

  PRBool exists = PR_FALSE;
  file->Exists(&exists);
  return exists;
}

static void RemoveComponentRegistries(nsIFile* aProfileDir, nsIFile* aLocalProfileDir,
                                      PRBool aRemoveEMFiles)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return;

  file->AppendNative(NS_LITERAL_CSTRING("compreg.dat"));
  file->Remove(PR_FALSE);

  file->SetNativeLeafName(NS_LITERAL_CSTRING("xpti.dat"));
  file->Remove(PR_FALSE);

  file->SetNativeLeafName(NS_LITERAL_CSTRING(".autoreg"));
  file->Remove(PR_FALSE);

  if (aRemoveEMFiles) {
    file->SetNativeLeafName(NS_LITERAL_CSTRING("extensions.ini"));
    file->Remove(PR_FALSE);
  }

  aLocalProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return;

  file->AppendNative(NS_LITERAL_CSTRING("XUL" PLATFORM_FASL_SUFFIX));
  file->Remove(PR_FALSE);
}





static struct {
  const char *name;
  char *value;
} gSavedVars[] = {
  {"XUL_APP_FILE", nsnull}
};

static void SaveStateForAppInitiatedRestart()
{
  for (size_t i = 0; i < NS_ARRAY_LENGTH(gSavedVars); ++i) {
    const char *s = PR_GetEnv(gSavedVars[i].name);
    if (s)
      gSavedVars[i].value = PR_smprintf("%s=%s", gSavedVars[i].name, s);
  }
}

static void RestoreStateForAppInitiatedRestart()
{
  for (size_t i = 0; i < NS_ARRAY_LENGTH(gSavedVars); ++i) {
    if (gSavedVars[i].value)
      PR_SetEnv(gSavedVars[i].value);
  }
}

#ifdef MOZ_AIRBAG




static void MakeOrSetMinidumpPath(nsIFile* profD)
{
  nsCOMPtr<nsIFile> dumpD;
  nsresult rv = profD->Clone(getter_AddRefs(dumpD));
  
  if(dumpD) {
    PRBool fileExists;
    
    dumpD->Append(NS_LITERAL_STRING("minidumps"));
    rv = dumpD->Exists(&fileExists);
    if(!fileExists) {
      dumpD->Create(nsIFile::DIRECTORY_TYPE, 0700);
    }

    nsAutoString pathStr;
    if(NS_SUCCEEDED(dumpD->GetPath(pathStr)))
      CrashReporter::SetMinidumpPath(pathStr);
  }
}
#endif

const nsXREAppData* gAppData = nsnull;

#if defined(XP_OS2)

class ScopedFPHandler {
private:
  EXCEPTIONREGISTRATIONRECORD excpreg;

public:
  ScopedFPHandler() { PR_OS2_SetFloatExcpHandler(&excpreg); }
  ~ScopedFPHandler() { PR_OS2_UnsetFloatExcpHandler(&excpreg); }
};
#endif

#ifdef MOZ_WIDGET_GTK2
#include "prlink.h"
typedef void (*_g_set_application_name_fn)(const gchar *application_name);
typedef void (*_gtk_window_set_auto_startup_notification_fn)(gboolean setting);

static PRFuncPtr FindFunction(const char* aName)
{
  PRLibrary *lib = nsnull;
  PRFuncPtr result = PR_FindFunctionSymbolAndLibrary(aName, &lib);
  
  if (lib) {
    PR_UnloadLibrary(lib);
  }
  return result;
}

static nsIWidget* GetMainWidget(nsIDOMWindow* aWindow)
{
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(window, nsnull);

  nsCOMPtr<nsIBaseWindow> baseWindow
    (do_QueryInterface(window->GetDocShell()));
  NS_ENSURE_TRUE(baseWindow, nsnull);

  nsCOMPtr<nsIWidget> mainWidget;
  baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  return mainWidget;
}

static nsGTKToolkit* GetGTKToolkit()
{
  nsCOMPtr<nsIAppShellService> svc = do_GetService(NS_APPSHELLSERVICE_CONTRACTID);
  if (!svc)
    return nsnull;
  nsCOMPtr<nsIDOMWindowInternal> window;
  svc->GetHiddenDOMWindow(getter_AddRefs(window));
  if (!window)
    return nsnull;
  nsIWidget* widget = GetMainWidget(window);
  if (!widget)
    return nsnull;
  nsIToolkit* toolkit = widget->GetToolkit();
  if (!toolkit)
    return nsnull;
  return NS_STATIC_CAST(nsGTKToolkit*, toolkit);
}

#endif













PRBool nspr_use_zone_allocator = PR_FALSE;

int
XRE_main(int argc, char* argv[], const nsXREAppData* aAppData)
{
  nsresult rv;
  NS_TIMELINE_MARK("enter main");

#ifdef DEBUG
  if (PR_GetEnv("XRE_MAIN_BREAK"))
    NS_BREAK();
#endif

#ifdef XP_WIN32
  
  
  
  UINT realMode = SetErrorMode(0);
  realMode |= SEM_FAILCRITICALERRORS;
  
  
  
  
  if (getenv("XRE_NO_WINDOWS_CRASH_DIALOG"))
    realMode |= SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX;

  SetErrorMode(realMode);

#ifdef DEBUG
  
  
  
  
  _set_sbh_threshold(0);
#endif
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
  InstallUnixSignalHandlers(argv[0]);
#endif

#ifdef MOZ_ACCESSIBILITY_ATK
  
  
  const char* gtkModules = PR_GetEnv("GTK_MODULES");
  if (gtkModules && *gtkModules) {
    nsCString gtkModulesStr(gtkModules);
    gtkModulesStr.ReplaceSubstring("atk-bridge", "");
    char* expr = PR_smprintf("GTK_MODULES=%s", gtkModulesStr.get());
    if (expr)
      PR_SetEnv(expr);
    
  }
#endif

  
  setbuf(stdout, 0);

#if defined(FREEBSD)
  
  
  
  fpsetmask(0);
#endif

  gArgc = argc;
  gArgv = argv;

  NS_ENSURE_TRUE(aAppData, 2);

#ifdef XP_MACOSX
  
  
  
  gBinaryPath = getenv("XRE_BINARY_PATH");

  if (gBinaryPath && !*gBinaryPath)
    gBinaryPath = nsnull;
#endif

  ScopedAppData appData(aAppData);
  gAppData = &appData;

  

  if (!appData.name) {
    Output(PR_TRUE, "Error: App:Name not specified in application.ini\n");
    return 1;
  }
  if (!appData.buildID) {
    Output(PR_TRUE, "Error: App:BuildID not specified in application.ini\n");
    return 1;
  }

  ScopedLogging log;

  if (!appData.xreDirectory) {
    nsCOMPtr<nsILocalFile> lf;
    rv = XRE_GetBinaryPath(gArgv[0], getter_AddRefs(lf));
    if (NS_FAILED(rv))
      return 2;

    nsCOMPtr<nsIFile> greDir;
    rv = lf->GetParent(getter_AddRefs(greDir));
    if (NS_FAILED(rv))
      return 2;
    
    rv = CallQueryInterface(greDir, &appData.xreDirectory);
    if (NS_FAILED(rv))
      return 2;
  }

  nsCOMPtr<nsIFile> iniFile;
  rv = appData.xreDirectory->Clone(getter_AddRefs(iniFile));
  if (NS_FAILED(rv))
    return 2;

  iniFile->AppendNative(NS_LITERAL_CSTRING("platform.ini"));

  nsCOMPtr<nsILocalFile> localIniFile = do_QueryInterface(iniFile);
  if (!localIniFile)
    return 2;

  nsINIParser parser;
  rv = parser.Init(localIniFile);
  if (NS_SUCCEEDED(rv)) {
    rv = parser.GetString("Build", "Milestone",
                          gToolkitVersion, sizeof(gToolkitVersion));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get toolkit version");

    rv = parser.GetString("Build", "BuildID",
                          gToolkitBuildID, sizeof(gToolkitBuildID));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get toolkit buildid");
  }
  else {
    NS_ERROR("Couldn't parse platform.ini!");
  }

  if (appData.size > offsetof(nsXREAppData, minVersion)) {
    if (!appData.minVersion) {
      Output(PR_TRUE, "Error: Gecko:MinVersion not specified in application.ini\n");
      return 1;
    }

    if (!appData.maxVersion) {
      
      
      SetAllocatedString(appData.maxVersion, "1.*");
    }

    if (NS_CompareVersions(appData.minVersion, gToolkitVersion) > 0 ||
        NS_CompareVersions(appData.maxVersion, gToolkitVersion) < 0) {
      Output(PR_TRUE, "Error: Platform version '%s' is not compatible with\n"
             "minVersion >= %s\nmaxVersion <= %s\n",
             gToolkitVersion,
             appData.minVersion, appData.maxVersion);
      return 1;
    }
  }

#ifdef MOZ_AIRBAG
  const char* airbagEnv = PR_GetEnv("MOZ_CRASHREPORTER");
  if (airbagEnv && *airbagEnv) {
    appData.flags |= NS_XRE_ENABLE_CRASH_REPORTER;
  }

  if ((appData.flags & NS_XRE_ENABLE_CRASH_REPORTER) &&
      NS_SUCCEEDED(
         CrashReporter::SetExceptionHandler(appData.xreDirectory,
                                            appData.crashReporterURL))) {
    
    if (appData.vendor)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("Vendor"),
                                     nsDependentCString(appData.vendor));
    if (appData.name)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ProductName"),
                                     nsDependentCString(appData.name));
    if (appData.version)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("Version"),
                                     nsDependentCString(appData.version));
    if (appData.buildID)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("BuildID"),
                                     nsDependentCString(appData.buildID));
    CrashReporter::SetRestartArgs(argc, argv);
  }
#endif

#ifdef XP_MACOSX
  if (PR_GetEnv("MOZ_LAUNCHED_CHILD")) {
    
    
    
    EnsureUseCocoaDockAPI();

    
    
    
    
    
    
    const EventTypeSpec kFakeEventList[] = { { INT_MAX, INT_MAX } };
    EventRef event;
    ::ReceiveNextEvent(GetEventTypeCount(kFakeEventList), kFakeEventList,
                       kEventDurationNoWait, PR_FALSE, &event);
  }

  if (CheckArg("foreground")) {
    
    
    
    ProcessSerialNumber psn;
    if (::GetCurrentProcess(&psn) == noErr)
      ::SetFrontProcess(&psn);
  }
#endif

  PR_SetEnv("MOZ_LAUNCHED_CHILD=");

  gRestartArgc = gArgc;
  gRestartArgv = (char**) malloc(sizeof(char*) * (gArgc + 1));
  if (!gRestartArgv) return 1;

  int i;
  for (i = 0; i < gArgc; ++i) {
    gRestartArgv[i] = gArgv[i];
  }
  gRestartArgv[gArgc] = nsnull;

#if defined(XP_OS2)
  PRBool StartOS2App(int aArgc, char **aArgv);
  if (!StartOS2App(gArgc, gArgv))
    return 1;
  ScopedFPHandler handler;
#endif 

#ifdef XP_MACOSX
  if (CheckArg("safe-mode") || GetCurrentKeyModifiers() & optionKey)
#else
  if (CheckArg("safe-mode"))
#endif
    gSafeMode = PR_TRUE;

  
  
  if (CheckArg("no-remote"))
    PR_SetEnv("MOZ_NO_REMOTE=1");

  
  
  if (CheckArg("h") || CheckArg("help") || CheckArg("?")) {
    DumpHelp();
    return 0;
  }

  if (CheckArg("v") || CheckArg("version")) {
    DumpVersion();
    return 0;
  }
    
#ifdef NS_TRACE_MALLOC
  gArgc = argc = NS_TraceMallocStartupArgs(gArgc, gArgv);
#endif

  {
    nsXREDirProvider dirProvider;
    rv = dirProvider.Initialize(gAppData->directory, gAppData->xreDirectory);
    if (NS_FAILED(rv))
      return 1;

    
    if (CheckArg("register")) {
      ScopedXPCOMStartup xpcom;
      rv = xpcom.Initialize();
      NS_ENSURE_SUCCESS(rv, 1);

      {
        nsCOMPtr<nsIChromeRegistry> chromeReg
          (do_GetService("@mozilla.org/chrome/chrome-registry;1"));
        NS_ENSURE_TRUE(chromeReg, 1);

        chromeReg->CheckForNewChrome();
      }
      return 0;
    }

#if defined(MOZ_WIDGET_GTK2) || defined(MOZ_ENABLE_XREMOTE)
    
#define HAVE_DESKTOP_STARTUP_ID
    const char* desktopStartupIDEnv = PR_GetEnv("DESKTOP_STARTUP_ID");
    nsCAutoString desktopStartupID;
    if (desktopStartupIDEnv) {
      desktopStartupID.Assign(desktopStartupIDEnv);
    }
#endif

#if defined(MOZ_WIDGET_GTK2)
    g_thread_init(NULL);
    
    
    
    
    if (CheckArg("install"))
      gdk_rgb_set_install(TRUE);

    
    gtk_init(&gArgc, &gArgv);

    
    _g_set_application_name_fn _g_set_application_name =
      (_g_set_application_name_fn)FindFunction("g_set_application_name");
    if (_g_set_application_name) {
      _g_set_application_name(gAppData->name);
    }
    _gtk_window_set_auto_startup_notification_fn _gtk_window_set_auto_startup_notification =
      (_gtk_window_set_auto_startup_notification_fn)FindFunction("gtk_window_set_auto_startup_notification");
    if (_gtk_window_set_auto_startup_notification) {
      _gtk_window_set_auto_startup_notification(PR_FALSE);
    }

    gtk_widget_set_default_visual(gdk_rgb_get_visual());
    gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
#endif 

    
#ifdef MOZ_JPROF
    setupProfilingStuff();
#endif

    
    nsCOMPtr<nsINativeAppSupport> nativeApp;
    rv = NS_CreateNativeAppSupport(getter_AddRefs(nativeApp));
    if (NS_FAILED(rv))
      return 1;

    PRBool canRun = PR_FALSE;
    rv = nativeApp->Start(&canRun);
    if (NS_FAILED(rv) || !canRun) {
      return 1;
    }

#ifdef MOZ_XPINSTALL
    
    
    
    
    
    
    
    
    
    
    
    {
      nsCOMPtr<nsIFile> registryFile;
      rv = dirProvider.GetAppDir()->Clone(getter_AddRefs(registryFile));
      if (NS_SUCCEEDED(rv)) {
        registryFile->AppendNative(CLEANUP_REGISTRY);

        PRBool exists;
        rv = registryFile->Exists(&exists);
        if (NS_SUCCEEDED(rv) && exists) {
          return VerifyInstallation(dirProvider.GetAppDir());
        }
      }
    }
#endif

#ifdef MOZ_ENABLE_XREMOTE
    

    const char* xremotearg;
    ArgResult ar = CheckArg("remote", &xremotearg);
    if (ar == ARG_BAD) {
      PR_fprintf(PR_STDERR, "Error: -remote requires an argument\n");
      return 1;
    }
    const char* desktopStartupIDPtr =
      desktopStartupID.IsEmpty() ? nsnull : desktopStartupID.get();
    if (ar) {
      return HandleRemoteArgument(xremotearg, desktopStartupIDPtr);
    }

    if (!PR_GetEnv("MOZ_NO_REMOTE")) {
      
      if (RemoteCommandLine(desktopStartupIDPtr))
        return 0;
    }
#endif

#if defined(MOZ_UPDATER)
  
  nsCOMPtr<nsIFile> updRoot;
  PRBool persistent;
  rv = dirProvider.GetFile(XRE_UPDATE_ROOT_DIR, &persistent,
                           getter_AddRefs(updRoot));
  
  if (NS_FAILED(rv))
    updRoot = dirProvider.GetAppDir();

  ProcessUpdates(dirProvider.GetGREDir(),
                 dirProvider.GetAppDir(),
                 updRoot,
                 gRestartArgc,
                 gRestartArgv);
#endif

    nsCOMPtr<nsIProfileLock> profileLock;
    PRBool startOffline = PR_FALSE;

    rv = SelectProfile(getter_AddRefs(profileLock), nativeApp, &startOffline);
    if (rv == NS_ERROR_LAUNCHED_CHILD_PROCESS ||
        rv == NS_ERROR_ABORT) return 0;
    if (NS_FAILED(rv)) return 1;

    nsCOMPtr<nsILocalFile> profD;
    rv = profileLock->GetDirectory(getter_AddRefs(profD));
    NS_ENSURE_SUCCESS(rv, 1);

    nsCOMPtr<nsILocalFile> profLD;
    rv = profileLock->GetLocalDirectory(getter_AddRefs(profLD));
    NS_ENSURE_SUCCESS(rv, 1);

    rv = dirProvider.SetProfile(profD, profLD);
    NS_ENSURE_SUCCESS(rv, 1);

    

#ifdef MOZ_AIRBAG
    if (appData.flags & NS_XRE_ENABLE_CRASH_REPORTER)
        MakeOrSetMinidumpPath(profD);
#endif

    PRBool upgraded = PR_FALSE;

    nsCAutoString version;
    BuildVersion(version);

#ifdef TARGET_OS_ABI
    NS_NAMED_LITERAL_CSTRING(osABI, TARGET_OS_ABI);
#else
    
    NS_NAMED_LITERAL_CSTRING(osABI, OS_TARGET "_UNKNOWN");
#endif

    
    
    
    PRBool versionOK = CheckCompatibility(profD, version, osABI,
                                          dirProvider.GetGREDir(),
                                          gAppData->directory);

    
    
    
    
    
    
    
    if (gSafeMode) {
      RemoveComponentRegistries(profD, profLD, PR_FALSE);
      WriteVersion(profD, NS_LITERAL_CSTRING("Safe Mode"), osABI,
                   dirProvider.GetGREDir(), gAppData->directory);
    }
    else if (versionOK) {
      if (ComponentsListChanged(profD)) {
        
        
        
        RemoveComponentRegistries(profD, profLD, PR_FALSE);
      }
      
    }
    else {
      
      
      
      RemoveComponentRegistries(profD, profLD, PR_TRUE);

      
      
      
      upgraded = PR_TRUE;

      
      WriteVersion(profD, version, osABI,
                   dirProvider.GetGREDir(), gAppData->directory);
    }

    PRBool needsRestart = PR_FALSE;
    PRBool appInitiatedRestart = PR_FALSE;

    
    
    
    {
      
      ScopedXPCOMStartup xpcom;
      rv = xpcom.Initialize();
      NS_ENSURE_SUCCESS(rv, 1); 
      rv = xpcom.DoAutoreg();
      rv |= xpcom.SetWindowCreator(nativeApp);
      NS_ENSURE_SUCCESS(rv, 1);

      {
        if (startOffline) {
          nsCOMPtr<nsIIOService> io (do_GetService("@mozilla.org/network/io-service;1"));
          NS_ENSURE_TRUE(io, 1);
          io->SetOffline(PR_TRUE);
        }

        {
          NS_TIMELINE_ENTER("startupNotifier");
          nsCOMPtr<nsIObserver> startupNotifier
            (do_CreateInstance(NS_APPSTARTUPNOTIFIER_CONTRACTID, &rv));
          NS_ENSURE_SUCCESS(rv, 1);

          startupNotifier->Observe(nsnull, APPSTARTUP_TOPIC, nsnull);
          NS_TIMELINE_LEAVE("startupNotifier");
        }

        nsCOMPtr<nsIAppStartup> appStartup
          (do_GetService(NS_APPSTARTUP_CONTRACTID));
        NS_ENSURE_TRUE(appStartup, 1);

        if (gDoMigration) {
          nsCOMPtr<nsIFile> file;
          dirProvider.GetAppDir()->Clone(getter_AddRefs(file));
          file->AppendNative(NS_LITERAL_CSTRING("override.ini"));
          nsINIParser parser;
          nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(file));
          nsresult rv = parser.Init(localFile);
          if (NS_SUCCEEDED(rv)) {
            nsCAutoString buf;
            rv = parser.GetString("XRE", "EnableProfileMigrator", buf);
            if (NS_SUCCEEDED(rv)) {
              if (buf[0] == '0' || buf[0] == 'f' || buf[0] == 'F') {
                gDoMigration = PR_FALSE;
              }
            }
          }
        }

        
        if (gAppData->flags & NS_XRE_ENABLE_PROFILE_MIGRATOR && gDoMigration) {
          gDoMigration = PR_FALSE;
          nsCOMPtr<nsIProfileMigrator> pm
            (do_CreateInstance(NS_PROFILEMIGRATOR_CONTRACTID));
          if (pm)
            pm->Migrate(&dirProvider);
        }
        dirProvider.DoStartup();

        nsCOMPtr<nsICommandLineRunner> cmdLine
          (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
        NS_ENSURE_TRUE(cmdLine, 1);

        nsCOMPtr<nsIFile> workingDir;
        rv = NS_GetSpecialDirectory(NS_OS_CURRENT_WORKING_DIR, getter_AddRefs(workingDir));
        NS_ENSURE_SUCCESS(rv, 1);

        rv = cmdLine->Init(gArgc, gArgv,
                           workingDir, nsICommandLine::STATE_INITIAL_LAUNCH);
        NS_ENSURE_SUCCESS(rv, 1);

        

        nsCOMPtr<nsIObserver> chromeObserver
          (do_GetService("@mozilla.org/chrome/chrome-registry;1"));
        if (chromeObserver) {
          chromeObserver->Observe(cmdLine, "command-line-startup", nsnull);
        }

        NS_TIMELINE_ENTER("appStartup->CreateHiddenWindow");
        rv = appStartup->CreateHiddenWindow();
        NS_TIMELINE_LEAVE("appStartup->CreateHiddenWindow");
        NS_ENSURE_SUCCESS(rv, 1);

#if defined(HAVE_DESKTOP_STARTUP_ID) && defined(MOZ_WIDGET_GTK2)
        nsRefPtr<nsGTKToolkit> toolkit = GetGTKToolkit();
        if (toolkit && !desktopStartupID.IsEmpty()) {
          toolkit->SetDesktopStartupID(desktopStartupID);
        }
#endif

        
        if (gAppData->flags & NS_XRE_ENABLE_EXTENSION_MANAGER) {
          nsCOMPtr<nsIExtensionManager> em(do_GetService("@mozilla.org/extensions/manager;1"));
          NS_ENSURE_TRUE(em, 1);

          if (CheckArg("install-global-extension") || CheckArg("install-global-theme")) {
            
            em->HandleCommandLineArgs(cmdLine);
            return 0;
          }

          if (upgraded) {
            rv = em->CheckForMismatches(&needsRestart);
            if (NS_FAILED(rv)) {
              needsRestart = PR_FALSE;
              upgraded = PR_FALSE;
            }
          }

          if (!upgraded || !needsRestart)
            em->Start(cmdLine, &needsRestart);
        }

        
        
        char* noEMRestart = PR_GetEnv("NO_EM_RESTART");
        if (noEMRestart && *noEMRestart && *noEMRestart == '1') {
          if (upgraded || needsRestart) {
            NS_WARNING("EM tried to force us to restart twice! Forcefully preventing that.");
          }
          needsRestart = upgraded = PR_FALSE;
        }

        if (!upgraded && !needsRestart) {
          SaveStateForAppInitiatedRestart();

          
          
          PR_SetEnv("XRE_PROFILE_PATH=");
          PR_SetEnv("XRE_PROFILE_LOCAL_PATH=");
          PR_SetEnv("XRE_START_OFFLINE=");
          PR_SetEnv("XRE_IMPORT_PROFILES=");
          PR_SetEnv("NO_EM_RESTART=");
          PR_SetEnv("XUL_APP_FILE=");
          PR_SetEnv("XRE_BINARY_PATH=");

#ifdef XP_MACOSX
          
          
          cmdLine = do_CreateInstance("@mozilla.org/toolkit/command-line;1");
          NS_ENSURE_TRUE(cmdLine, 1);

          SetupMacCommandLine(gArgc, gArgv);

          rv = cmdLine->Init(gArgc, gArgv,
                             workingDir, nsICommandLine::STATE_INITIAL_LAUNCH);
          NS_ENSURE_SUCCESS(rv, 1);
#endif
#ifdef MOZ_WIDGET_COCOA
          
          SetupMacApplicationDelegate();
#endif
          nsCOMPtr<nsIObserverService> obsService
            (do_GetService("@mozilla.org/observer-service;1"));
          if (obsService)
            obsService->NotifyObservers(nsnull, "final-ui-startup", nsnull);

          rv = cmdLine->Run();
          NS_ENSURE_SUCCESS_LOG(rv, 1);

#ifdef MOZ_ENABLE_XREMOTE
          
          
          nsCOMPtr<nsIRemoteService> remoteService;
          remoteService = do_GetService("@mozilla.org/toolkit/remote-service;1");
          if (remoteService)
            remoteService->Startup(gAppData->name, nsnull);
#endif 

          
          nativeApp->Enable();

          NS_TIMELINE_ENTER("appStartup->Run");
          rv = appStartup->Run();
          NS_TIMELINE_LEAVE("appStartup->Run");
          if (NS_FAILED(rv)) {
            NS_ERROR("failed to run appstartup");
            gLogConsoleErrors = PR_TRUE;
          }

          
          
          if (rv == NS_SUCCESS_RESTART_APP) {
            needsRestart = PR_TRUE;
            appInitiatedRestart = PR_TRUE;
          }

#ifdef MOZ_ENABLE_XREMOTE
          
          if (remoteService)
            remoteService->Shutdown();
#endif 

#ifdef MOZ_TIMELINE
          
          if (NS_FAILED(NS_TIMELINE_LEAVE("main1")))
            NS_TimelineForceMark("...main1");
#endif
        }
        else {
          
          
          
          
          needsRestart = PR_TRUE;

#ifdef XP_WIN
          ProcessDDE(nativeApp);
#endif

#ifdef XP_MACOSX
          SetupMacCommandLine(gRestartArgc, gRestartArgv);
#endif
        }
      }

      profileLock->Unlock();
    }

    
    if (needsRestart) {
      if (appInitiatedRestart) {
        RestoreStateForAppInitiatedRestart();
      }
      else {
        char* noEMRestart = PR_GetEnv("NO_EM_RESTART");
        if (noEMRestart && *noEMRestart) {
          PR_SetEnv("NO_EM_RESTART=1");
        }
        else {
          PR_SetEnv("NO_EM_RESTART=0");
        }
      }

      
      SaveFileToEnvIfUnset("XRE_PROFILE_PATH", profD);
      SaveFileToEnvIfUnset("XRE_PROFILE_LOCAL_PATH", profLD);

#ifdef XP_MACOSX
      if (gBinaryPath) {
        static char kEnvVar[MAXPATHLEN];
        sprintf(kEnvVar, "XRE_BINARY_PATH=%s", gBinaryPath);
        PR_SetEnv(kEnvVar);
      }
#endif

#if defined(HAVE_DESKTOP_STARTUP_ID) && defined(MOZ_TOOLKIT_GTK2)
      nsGTKToolkit* toolkit = GetGTKToolkit();
      if (toolkit) {
        nsCAutoString currentDesktopStartupID;
        toolkit->GetDesktopStartupID(&currentDesktopStartupID);
        if (!currentDesktopStartupID.IsEmpty()) {
          nsCAutoString desktopStartupEnv;
          desktopStartupEnv.AssignLiteral("DESKTOP_STARTUP_ID=");
          desktopStartupEnv.Append(currentDesktopStartupID);
          
          PR_SetEnv(ToNewCString(desktopStartupEnv));
        }
      }
#endif

      rv = LaunchChild(nativeApp, appInitiatedRestart, upgraded ? -1 : 0);

#ifdef MOZ_AIRBAG
      if (appData.flags & NS_XRE_ENABLE_CRASH_REPORTER)
        CrashReporter::UnsetExceptionHandler();
#endif

      return rv == NS_ERROR_LAUNCHED_CHILD_PROCESS ? 0 : 1;
    }
  }

#ifdef MOZ_AIRBAG
  if (appData.flags & NS_XRE_ENABLE_CRASH_REPORTER)
      CrashReporter::UnsetExceptionHandler();
#endif

  return NS_FAILED(rv) ? 1 : 0;
}
