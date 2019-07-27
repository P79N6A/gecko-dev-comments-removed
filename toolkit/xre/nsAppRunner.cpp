




#if defined(MOZ_WIDGET_QT)
#include <QGuiApplication>
#include <QStringList>
#include "nsQAppInstance.h"
#endif 

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/ChaosMode.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/Likely.h"
#include "mozilla/Poison.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

#include "nsAppRunner.h"
#include "mozilla/AppData.h"
#include "nsUpdateDriver.h"
#include "ProfileReset.h"

#ifdef MOZ_INSTRUMENT_EVENT_LOOP
#include "EventTracer.h"
#endif

#ifdef XP_MACOSX
#include "nsVersionComparator.h"
#include "MacLaunchHelper.h"
#include "MacApplicationDelegate.h"
#include "MacAutoreleasePool.h"

#include <sys/types.h>
#include <sys/sysctl.h>
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
#include "mozilla/ModuleUtils.h"
#include "nsIIOService2.h"
#include "nsIObserverService.h"
#include "nsINativeAppSupport.h"
#include "nsIProcess.h"
#include "nsIProfileUnlocker.h"
#include "nsIPromptService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
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
#include "mozilla/scache/StartupCache.h"
#include "nsIGfxInfo.h"

#include "mozilla/unused.h"

#ifdef XP_WIN
#include "nsIWinAppHelper.h"
#include <windows.h>
#include "cairo/cairo-features.h"
#include "mozilla/WindowsVersion.h"
#ifdef MOZ_METRO
#include <roapi.h>
#endif

#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#endif
#endif

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsEmbedCID.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCIDInternal.h"
#include "nsXPIDLString.h"
#include "nsPrintfCString.h"
#include "nsVersionComparator.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsXREDirProvider.h"
#include "nsToolkitCompsCID.h"

#if defined(XP_WIN) && defined(MOZ_METRO)
#include "updatehelper.h"
#endif

#include "nsINIParser.h"
#include "mozilla/Omnijar.h"
#include "mozilla/StartupTimeline.h"
#include "mozilla/LateWriteChecks.h"

#include <stdlib.h>

#ifdef XP_UNIX
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif

#ifdef XP_WIN
#include <process.h>
#include <shlobj.h>
#include "nsThreadUtils.h"
#include <comdef.h>
#include <wbemidl.h>
#endif

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
#include "nsCocoaFeatures.h"
#endif


#ifdef MOZ_ENABLE_XREMOTE
#include "XRemoteClient.h"
#include "nsIRemoteService.h"
#endif

#if defined(DEBUG) && defined(XP_WIN32)
#include <malloc.h>
#endif

#if defined (XP_MACOSX)
#include <Carbon/Carbon.h>
#endif

#ifdef DEBUG
#include "prlog.h"
#endif

#ifdef MOZ_JPROF
#include "jprof.h"
#endif

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#include "nsIPrefService.h"
#include "nsIMemoryInfoDumper.h"
#endif

#include "base/command_line.h"
#ifdef MOZ_ENABLE_TESTS
#include "GTestRunner.h"
#endif

#ifdef MOZ_B2G_LOADER
#include "ProcessUtils.h"
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

extern uint32_t gRestartMode;
extern void InstallSignalHandlers(const char *ProgramName);

#define FILE_COMPATIBILITY_INFO NS_LITERAL_CSTRING("compatibility.ini")
#define FILE_INVALIDATE_CACHES NS_LITERAL_CSTRING(".purgecaches")

int    gArgc;
char **gArgv;

static const char gToolkitVersion[] = NS_STRINGIFY(GRE_MILESTONE);
static const char gToolkitBuildID[] = NS_STRINGIFY(GRE_BUILDID);

static nsIProfileLock* gProfileLock;

int    gRestartArgc;
char **gRestartArgv;

bool gIsGtest = false;

#ifdef MOZ_WIDGET_QT
static int    gQtOnlyArgc;
static char **gQtOnlyArgv;
#endif

#if defined(MOZ_WIDGET_GTK)
#if defined(DEBUG) || defined(NS_BUILD_REFCNT_LOGGING)
#define CLEANUP_MEMORY 1
#define PANGO_ENABLE_BACKEND
#include <pango/pangofc-fontmap.h>
#endif
#include <gtk/gtk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 
#include "nsGTKToolkit.h"
#include <fontconfig/fontconfig.h>
#endif
#include "BinaryPath.h"

#ifdef MOZ_LINKER
extern "C" MFBT_API bool IsSignalHandlingBroken();
#endif

namespace mozilla {
int (*RunGTest)() = 0;
}

using namespace mozilla;
using mozilla::unused;
using mozilla::scache::StartupCache;
using mozilla::dom::ContentParent;
using mozilla::dom::ContentChild;


static void
SaveToEnv(const char *putenv)
{
  char *expr = strdup(putenv);
  if (expr)
    PR_SetEnv(expr);
  
}


static bool
EnvHasValue(const char *name)
{
  const char *val = PR_GetEnv(name);
  return (val && *val);
}


static void
SaveWordToEnv(const char *name, const nsACString & word)
{
  char *expr = PR_smprintf("%s=%s", name, PromiseFlatCString(word).get());
  if (expr)
    PR_SetEnv(expr);
  
}


static void
SaveFileToEnv(const char *name, nsIFile *file)
{
#ifdef XP_WIN
  nsAutoString path;
  file->GetPath(path);
  SetEnvironmentVariableW(NS_ConvertASCIItoUTF16(name).get(), path.get());
#else
  nsAutoCString path;
  file->GetNativePath(path);
  SaveWordToEnv(name, path);
#endif
}


static already_AddRefed<nsIFile>
GetFileFromEnv(const char *name)
{
  nsresult rv;
  nsCOMPtr<nsIFile> file;

#ifdef XP_WIN
  WCHAR path[_MAX_PATH];
  if (!GetEnvironmentVariableW(NS_ConvertASCIItoUTF16(name).get(),
                               path, _MAX_PATH))
    return nullptr;

  rv = NS_NewLocalFile(nsDependentString(path), true, getter_AddRefs(file));
  if (NS_FAILED(rv))
    return nullptr;

  return file.forget();
#else
  const char *arg = PR_GetEnv(name);
  if (!arg || !*arg)
    return nullptr;

  rv = NS_NewNativeLocalFile(nsDependentCString(arg), true,
                             getter_AddRefs(file));
  if (NS_FAILED(rv))
    return nullptr;

  return file.forget();
#endif
}



static void
SaveWordToEnvIfUnset(const char *name, const nsACString & word)
{
  if (!EnvHasValue(name))
    SaveWordToEnv(name, word);
}



static void
SaveFileToEnvIfUnset(const char *name, nsIFile *file)
{
  if (!EnvHasValue(name))
    SaveFileToEnv(name, file);
}

static bool
strimatch(const char* lowerstr, const char* mixedstr)
{
  while(*lowerstr) {
    if (!*mixedstr) return false; 
    if (tolower(*mixedstr) != *lowerstr) return false; 

    ++lowerstr;
    ++mixedstr;
  }

  if (*mixedstr) return false; 

  return true;
}










static void Output(bool isError, const char *fmt, ... )
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

    wchar_t wide_msg[1024];
    MultiByteToWideChar(CP_ACP,
                        0,
                        msg,
                        -1,
                        wide_msg,
                        sizeof(wide_msg) / sizeof(wchar_t));

    MessageBoxW(nullptr, wide_msg, L"XULRunner", flags);
    PR_smprintf_free(msg);
  }
#else
  vfprintf(stderr, fmt, ap);
#endif

  va_end(ap);
}

enum RemoteResult {
  REMOTE_NOT_FOUND  = 0,
  REMOTE_FOUND      = 1,
  REMOTE_ARG_BAD    = 2
};

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
CheckArg(const char* aArg, bool aCheckOSInt = false, const char **aParam = nullptr, bool aRemArg = true)
{
  MOZ_ASSERT(gArgv, "gArgv must be initialized before CheckArg()");

  char **curarg = gArgv + 1; 
  ArgResult ar = ARG_NONE;

  while (*curarg) {
    char *arg = curarg[0];

    if (arg[0] == '-'
#if defined(XP_WIN)
        || *arg == '/'
#endif
        ) {
      ++arg;
      if (*arg == '-')
        ++arg;

      if (strimatch(aArg, arg)) {
        if (aRemArg)
          RemoveArg(curarg);
        else
          ++curarg;
        if (!aParam) {
          ar = ARG_FOUND;
          break;
        }

        if (*curarg) {
          if (**curarg == '-'
#if defined(XP_WIN)
              || **curarg == '/'
#endif
              )
            return ARG_BAD;

          *aParam = *curarg;
          if (aRemArg)
            RemoveArg(curarg);
          ar = ARG_FOUND;
          break;
        }
        return ARG_BAD;
      }
    }

    ++curarg;
  }

  if (aCheckOSInt && ar == ARG_FOUND) {
    ArgResult arOSInt = CheckArg("osint");
    if (arOSInt == ARG_FOUND) {
      ar = ARG_BAD;
      PR_fprintf(PR_STDERR, "Error: argument --osint is invalid\n");
    }
  }

  return ar;
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
ProcessDDE(nsINativeAppSupport* aNative, bool aWait)
{
  
  
  
  
  
  
  
  
  ArgResult ar;
  ar = CheckArgShell("requestpending");
  if (ar == ARG_FOUND) {
    aNative->Enable(); 
    if (aWait) {
      nsIThread *thread = NS_GetCurrentThread();
      
      
      int32_t count = 20;
      while(--count >= 0) {
        NS_ProcessNextEvent(thread);
        PR_Sleep(PR_MillisecondsToInterval(1));
      }
    }
  }
}
#endif






static bool
CanShowProfileManager()
{
#if defined(XP_WIN)
  return XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop;
#else
  return true;
#endif
}

bool gSafeMode = false;





class nsXULAppInfo : public nsIXULAppInfo,
#ifdef NIGHTLY_BUILD
                     public nsIObserver,
#endif
#ifdef XP_WIN
                     public nsIWinAppHelper,
#endif
#ifdef MOZ_CRASHREPORTER
                     public nsICrashReporter,
                     public nsIFinishDumpingCallback,
#endif
                     public nsIXULRuntime

{
public:
  MOZ_CONSTEXPR nsXULAppInfo() {}
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIXULAPPINFO
  NS_DECL_NSIXULRUNTIME
#ifdef NIGHTLY_BUILD
  NS_DECL_NSIOBSERVER
#endif
#ifdef MOZ_CRASHREPORTER
  NS_DECL_NSICRASHREPORTER
  NS_DECL_NSIFINISHDUMPINGCALLBACK
#endif
#ifdef XP_WIN
  NS_DECL_NSIWINAPPHELPER
#endif
};

NS_INTERFACE_MAP_BEGIN(nsXULAppInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULRuntime)
  NS_INTERFACE_MAP_ENTRY(nsIXULRuntime)
#ifdef NIGHTLY_BUILD
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
#endif
#ifdef XP_WIN
  NS_INTERFACE_MAP_ENTRY(nsIWinAppHelper)
#endif
#ifdef MOZ_CRASHREPORTER
  NS_INTERFACE_MAP_ENTRY(nsICrashReporter)
  NS_INTERFACE_MAP_ENTRY(nsIFinishDumpingCallback)
#endif
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIXULAppInfo, gAppData || 
                                     XRE_GetProcessType() == GeckoProcessType_Content)
NS_INTERFACE_MAP_END

NS_IMETHODIMP_(MozExternalRefCountType)
nsXULAppInfo::AddRef()
{
  return 1;
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsXULAppInfo::Release()
{
  return 1;
}

NS_IMETHODIMP
nsXULAppInfo::GetVendor(nsACString& aResult)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cc = ContentChild::GetSingleton();
    aResult = cc->GetAppInfo().vendor;
    return NS_OK;
  }
  aResult.Assign(gAppData->vendor);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetName(nsACString& aResult)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cc = ContentChild::GetSingleton();
    aResult = cc->GetAppInfo().name;
    return NS_OK;
  }
  aResult.Assign(gAppData->name);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetID(nsACString& aResult)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cc = ContentChild::GetSingleton();
    aResult = cc->GetAppInfo().ID;
    return NS_OK;
  }
  aResult.Assign(gAppData->ID);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetVersion(nsACString& aResult)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cc = ContentChild::GetSingleton();
    aResult = cc->GetAppInfo().version;
    return NS_OK;
  }
  aResult.Assign(gAppData->version);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetPlatformVersion(nsACString& aResult)
{
  aResult.Assign(gToolkitVersion);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetAppBuildID(nsACString& aResult)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cc = ContentChild::GetSingleton();
    aResult = cc->GetAppInfo().buildID;
    return NS_OK;
  }
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
nsXULAppInfo::GetUAName(nsACString& aResult)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cc = ContentChild::GetSingleton();
    aResult = cc->GetAppInfo().UAName;
    return NS_OK;
  }
  aResult.Assign(gAppData->UAName);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetLogConsoleErrors(bool *aResult)
{
  *aResult = gLogConsoleErrors;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetLogConsoleErrors(bool aValue)
{
  gLogConsoleErrors = aValue;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetInSafeMode(bool *aResult)
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

NS_IMETHODIMP
nsXULAppInfo::GetWidgetToolkit(nsACString& aResult)
{
  aResult.AssignLiteral(MOZ_WIDGET_TOOLKIT);
  return NS_OK;
}




#define SYNC_ENUMS(a,b) \
  static_assert(nsIXULRuntime::PROCESS_TYPE_ ## a == \
                static_cast<int>(GeckoProcessType_ ## b), \
                "GeckoProcessType in nsXULAppAPI.h not synchronized with nsIXULRuntime.idl");

SYNC_ENUMS(DEFAULT, Default)
SYNC_ENUMS(PLUGIN, Plugin)
SYNC_ENUMS(CONTENT, Content)
SYNC_ENUMS(IPDLUNITTEST, IPDLUnitTest)


static_assert(GeckoProcessType_GMPlugin + 1 == GeckoProcessType_End,
              "Did not find the final GeckoProcessType");

NS_IMETHODIMP
nsXULAppInfo::GetProcessType(uint32_t* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = XRE_GetProcessType();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetProcessID(uint32_t* aResult)
{
#ifdef XP_WIN
  *aResult = GetCurrentProcessId();
#else
  *aResult = getpid();
#endif
  return NS_OK;
}

static bool gBrowserTabsRemoteAutostart = false;
static nsString gBrowserTabsRemoteDisabledReason;
static bool gBrowserTabsRemoteAutostartInitialized = false;

#ifdef NIGHTLY_BUILD
NS_IMETHODIMP
nsXULAppInfo::Observe(nsISupports *aSubject, const char *aTopic, const char16_t *aData) {
  if (!nsCRT::strcmp(aTopic, "getE10SBlocked")) {
    nsCOMPtr<nsISupportsString> ret = do_QueryInterface(aSubject);
    if (!ret)
      return NS_ERROR_FAILURE;

    ret->SetData(gBrowserTabsRemoteDisabledReason);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}
#endif

NS_IMETHODIMP
nsXULAppInfo::GetBrowserTabsRemoteAutostart(bool* aResult)
{
  *aResult = BrowserTabsRemoteAutostart();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetAccessibilityEnabled(bool* aResult)
{
#ifdef ACCESSIBILITY
  *aResult = GetAccService() != nullptr;
#else
  *aResult = false;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetAccessibilityIsUIA(bool* aResult)
{
  *aResult = false;
#if defined(ACCESSIBILITY) && defined(XP_WIN)
  
  if (GetAccService() != nullptr &&
      (::GetModuleHandleW(L"uiautomation") ||
       ::GetModuleHandleW(L"uiautomationcore"))) {
    *aResult = true;
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetIs64Bit(bool* aResult)
{
#ifdef HAVE_64BIT_BUILD
  *aResult = true;
#else
  *aResult = false;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::EnsureContentProcess()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<ContentParent> unused = ContentParent::GetNewOrUsedBrowserProcess();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::InvalidateCachesOnRestart()
{
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DIR_STARTUP, 
                                       getter_AddRefs(file));
  if (NS_FAILED(rv))
    return rv;
  if (!file)
    return NS_ERROR_NOT_AVAILABLE;
  
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsINIParser parser;
  rv = parser.Init(file);
  if (NS_FAILED(rv)) {
    
    
    return NS_OK;
  }
  
  nsAutoCString buf;
  rv = parser.GetString("Compatibility", "InvalidateCaches", buf);
  
  if (NS_FAILED(rv)) {
    PRFileDesc *fd = nullptr;
    file->OpenNSPRFileDesc(PR_RDWR | PR_APPEND, 0600, &fd);
    if (!fd) {
      NS_ERROR("could not create output stream");
      return NS_ERROR_NOT_AVAILABLE;
    }
    static const char kInvalidationHeader[] = NS_LINEBREAK "InvalidateCaches=1" NS_LINEBREAK;
    PR_Write(fd, kInvalidationHeader, sizeof(kInvalidationHeader) - 1);
    PR_Close(fd);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetReplacedLockTime(PRTime *aReplacedLockTime)
{
  if (!gProfileLock)
    return NS_ERROR_NOT_AVAILABLE;
  gProfileLock->GetReplacedLockTime(aReplacedLockTime);
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetLastRunCrashID(nsAString &aLastRunCrashID)
{
#ifdef MOZ_CRASHREPORTER
  CrashReporter::GetLastRunCrashID(aLastRunCrashID);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsXULAppInfo::GetIsReleaseBuild(bool* aResult)
{
#ifdef RELEASE_BUILD
  *aResult = true;
#else
  *aResult = false;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetIsOfficialBranding(bool* aResult)
{
#ifdef MOZ_OFFICIAL_BRANDING
  *aResult = true;
#else
  *aResult = false;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetDefaultUpdateChannel(nsACString& aResult)
{
  aResult.AssignLiteral(NS_STRINGIFY(MOZ_UPDATE_CHANNEL));
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetDistributionID(nsACString& aResult)
{
  aResult.AssignLiteral(MOZ_DISTRIBUTION_ID);
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::GetIsOfficial(bool* aResult)
{
#ifdef MOZILLA_OFFICIAL
  *aResult = true;
#else
  *aResult = false;
#endif
  return NS_OK;
}

#ifdef XP_WIN


typedef enum 
{
  VistaTokenElevationTypeDefault = 1,
  VistaTokenElevationTypeFull,
  VistaTokenElevationTypeLimited
} VISTA_TOKEN_ELEVATION_TYPE;



#define VistaTokenElevationType static_cast< TOKEN_INFORMATION_CLASS >( 18 )

NS_IMETHODIMP
nsXULAppInfo::GetUserCanElevate(bool *aUserCanElevate)
{
  HANDLE hToken;

  VISTA_TOKEN_ELEVATION_TYPE elevationType;
  DWORD dwSize; 

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) ||
      !GetTokenInformation(hToken, VistaTokenElevationType, &elevationType,
                           sizeof(elevationType), &dwSize)) {
    *aUserCanElevate = false;
  } 
  else {
    
    
    
    
    
    
    
    
    
    *aUserCanElevate = (elevationType == VistaTokenElevationTypeLimited);
  }

  if (hToken)
    CloseHandle(hToken);

  return NS_OK;
}
#endif

#ifdef MOZ_CRASHREPORTER
NS_IMETHODIMP
nsXULAppInfo::GetEnabled(bool *aEnabled)
{
  *aEnabled = CrashReporter::GetEnabled();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetEnabled(bool aEnabled)
{
  if (aEnabled) {
    if (CrashReporter::GetEnabled()) {
      
      return NS_OK;
    }

    nsCOMPtr<nsIFile> greBinDir;
    NS_GetSpecialDirectory(NS_GRE_BIN_DIR, getter_AddRefs(greBinDir));
    if (!greBinDir) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIFile> xreBinDirectory = do_QueryInterface(greBinDir);
    if (!xreBinDirectory) {
      return NS_ERROR_FAILURE;
    }

    return CrashReporter::SetExceptionHandler(xreBinDirectory, true);
  }
  else {
    if (!CrashReporter::GetEnabled()) {
      
      return NS_OK;
    }

    return CrashReporter::UnsetExceptionHandler();
  }
}

NS_IMETHODIMP
nsXULAppInfo::GetServerURL(nsIURL** aServerURL)
{
  if (!CrashReporter::GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  nsAutoCString data;
  if (!CrashReporter::GetServerURL(data)) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), data);
  if (!uri)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIURL> url;
  url = do_QueryInterface(uri);
  NS_ADDREF(*aServerURL = url);

  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetServerURL(nsIURL* aServerURL)
{
  bool schemeOk;
  
  nsresult rv = aServerURL->SchemeIs("https", &schemeOk);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!schemeOk) {
    rv = aServerURL->SchemeIs("http", &schemeOk);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!schemeOk)
      return NS_ERROR_INVALID_ARG;
  }
  nsAutoCString spec;
  rv = aServerURL->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return CrashReporter::SetServerURL(spec);
}

NS_IMETHODIMP
nsXULAppInfo::GetMinidumpPath(nsIFile** aMinidumpPath)
{
  if (!CrashReporter::GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  nsAutoString path;
  if (!CrashReporter::GetMinidumpPath(path))
    return NS_ERROR_FAILURE;

  nsresult rv = NS_NewLocalFile(path, false, aMinidumpPath);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetMinidumpPath(nsIFile* aMinidumpPath)
{
  nsAutoString path;
  nsresult rv = aMinidumpPath->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);
  return CrashReporter::SetMinidumpPath(path);
}

NS_IMETHODIMP
nsXULAppInfo::AnnotateCrashReport(const nsACString& key,
                                  const nsACString& data)
{
  return CrashReporter::AnnotateCrashReport(key, data);
}

NS_IMETHODIMP
nsXULAppInfo::AppendAppNotesToCrashReport(const nsACString& data)
{
  return CrashReporter::AppendAppNotesToCrashReport(data);
}

NS_IMETHODIMP
nsXULAppInfo::RegisterAppMemory(uint64_t pointer,
                                uint64_t len)
{
  return CrashReporter::RegisterAppMemory((void *)pointer, len);
}

NS_IMETHODIMP
nsXULAppInfo::WriteMinidumpForException(void* aExceptionInfo)
{
#ifdef XP_WIN32
  return CrashReporter::WriteMinidumpForException(static_cast<EXCEPTION_POINTERS*>(aExceptionInfo));
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsXULAppInfo::AppendObjCExceptionInfoToAppNotes(void* aException)
{
#ifdef XP_MACOSX
  return CrashReporter::AppendObjCExceptionInfoToAppNotes(aException);
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsXULAppInfo::GetSubmitReports(bool* aEnabled)
{
  return CrashReporter::GetSubmitReports(aEnabled);
}

NS_IMETHODIMP
nsXULAppInfo::SetSubmitReports(bool aEnabled)
{
  return CrashReporter::SetSubmitReports(aEnabled);
}

NS_IMETHODIMP
nsXULAppInfo::UpdateCrashEventsDir()
{
  CrashReporter::UpdateCrashEventsDir();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SaveMemoryReport()
{
  if (!CrashReporter::GetEnabled()) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DIR_STARTUP,
                                       getter_AddRefs(file));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  file->AppendNative(NS_LITERAL_CSTRING("memory-report.json.gz"));

  nsString path;
  file->GetPath(path);

  nsCOMPtr<nsIMemoryInfoDumper> dumper =
    do_GetService("@mozilla.org/memory-info-dumper;1");
  if (NS_WARN_IF(!dumper)) {
    return NS_ERROR_UNEXPECTED;
  }

  rv = dumper->DumpMemoryReportsToNamedFile(path, this, file, true );
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::Callback(nsISupports* aData)
{
  nsCOMPtr<nsIFile> file = do_QueryInterface(aData);
  MOZ_ASSERT(file);

  CrashReporter::SetMemoryReportFile(file);
  return NS_OK;
}
#endif

static const nsXULAppInfo kAppInfo;
static nsresult AppInfoConstructor(nsISupports* aOuter,
                                   REFNSIID aIID, void **aResult)
{
  NS_ENSURE_NO_AGGREGATION(aOuter);

  return const_cast<nsXULAppInfo*>(&kAppInfo)->
    QueryInterface(aIID, aResult);
}

bool gLogConsoleErrors = false;

#define NS_ENSURE_TRUE_LOG(x, ret)               \
  PR_BEGIN_MACRO                                 \
  if (MOZ_UNLIKELY(!(x))) {                      \
    NS_WARNING("NS_ENSURE_TRUE(" #x ") failed"); \
    gLogConsoleErrors = true;                    \
    return ret;                                  \
  }                                              \
  PR_END_MACRO

#define NS_ENSURE_SUCCESS_LOG(res, ret)          \
  NS_ENSURE_TRUE_LOG(NS_SUCCEEDED(res), ret)







class ScopedXPCOMStartup
{
public:
  ScopedXPCOMStartup() :
    mServiceManager(nullptr) { }
  ~ScopedXPCOMStartup();

  nsresult Initialize();
  nsresult SetWindowCreator(nsINativeAppSupport* native);

  static nsresult CreateAppSupport(nsISupports* aOuter, REFNSIID aIID, void** aResult);

private:
  nsIServiceManager* mServiceManager;
  static nsINativeAppSupport* gNativeAppSupport;
};

ScopedXPCOMStartup::~ScopedXPCOMStartup()
{
  NS_IF_RELEASE(gNativeAppSupport);

  if (mServiceManager) {
#ifdef XP_MACOSX
    
    
    mozilla::MacAutoreleasePool pool;
#endif

    nsCOMPtr<nsIAppStartup> appStartup (do_GetService(NS_APPSTARTUP_CONTRACTID));
    if (appStartup)
      appStartup->DestroyHiddenWindow();

    gDirServiceProvider->DoShutdown();
    PROFILER_MARKER("Shutdown early");

    WriteConsoleLog();

    NS_ShutdownXPCOM(mServiceManager);
    mServiceManager = nullptr;
  }
}


#define APPINFO_CID \
  { 0x95d89e3e, 0xa169, 0x41a3, { 0x8e, 0x56, 0x71, 0x99, 0x78, 0xe1, 0x5b, 0x12 } }


static const nsCID kNativeAppSupportCID =
  { 0xc4a446c, 0xee82, 0x41f2, { 0x8d, 0x4, 0xd3, 0x66, 0xd2, 0xc7, 0xa7, 0xd4 } };


static const nsCID kProfileServiceCID =
  { 0x5f5e59ce, 0x27bc, 0x47eb, { 0x9d, 0x1f, 0xb0, 0x9c, 0xa9, 0x4, 0x98, 0x36 } };

static already_AddRefed<nsIFactory>
ProfileServiceFactoryConstructor(const mozilla::Module& module, const mozilla::Module::CIDEntry& entry)
{
  nsCOMPtr<nsIFactory> factory;
  NS_NewToolkitProfileFactory(getter_AddRefs(factory));
  return factory.forget();
}

NS_DEFINE_NAMED_CID(APPINFO_CID);

static const mozilla::Module::CIDEntry kXRECIDs[] = {
  { &kAPPINFO_CID, false, nullptr, AppInfoConstructor },
  { &kProfileServiceCID, false, ProfileServiceFactoryConstructor, nullptr },
  { &kNativeAppSupportCID, false, nullptr, ScopedXPCOMStartup::CreateAppSupport },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kXREContracts[] = {
  { XULAPPINFO_SERVICE_CONTRACTID, &kAPPINFO_CID },
  { XULRUNTIME_SERVICE_CONTRACTID, &kAPPINFO_CID },
#ifdef MOZ_CRASHREPORTER
  { NS_CRASHREPORTER_CONTRACTID, &kAPPINFO_CID },
#endif
  { NS_PROFILESERVICE_CONTRACTID, &kProfileServiceCID },
  { NS_NATIVEAPPSUPPORT_CONTRACTID, &kNativeAppSupportCID },
  { nullptr }
};

static const mozilla::Module kXREModule = {
  mozilla::Module::kVersion,
  kXRECIDs,
  kXREContracts
};

NSMODULE_DEFN(Apprunner) = &kXREModule;

nsresult
ScopedXPCOMStartup::Initialize()
{
  NS_ASSERTION(gDirServiceProvider, "Should not get here!");

  nsresult rv;

  rv = NS_InitXPCOM2(&mServiceManager, gDirServiceProvider->GetAppDir(),
                     gDirServiceProvider);
  if (NS_FAILED(rv)) {
    NS_ERROR("Couldn't start xpcom!");
    mServiceManager = nullptr;
  }
  else {
    nsCOMPtr<nsIComponentRegistrar> reg =
      do_QueryInterface(mServiceManager);
    NS_ASSERTION(reg, "Service Manager doesn't QI to Registrar.");
  }

  return rv;
}





class nsSingletonFactory final : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  explicit nsSingletonFactory(nsISupports* aSingleton);

private:
  ~nsSingletonFactory() { }
  nsCOMPtr<nsISupports> mSingleton;
};

nsSingletonFactory::nsSingletonFactory(nsISupports* aSingleton)
  : mSingleton(aSingleton)
{
  NS_ASSERTION(mSingleton, "Singleton was null!");
}

NS_IMPL_ISUPPORTS(nsSingletonFactory, nsIFactory)

NS_IMETHODIMP
nsSingletonFactory::CreateInstance(nsISupports* aOuter,
                                   const nsIID& aIID,
                                   void* *aResult)
{
  NS_ENSURE_NO_AGGREGATION(aOuter);

  return mSingleton->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsSingletonFactory::LockFactory(bool)
{
  return NS_OK;
}




nsresult
ScopedXPCOMStartup::SetWindowCreator(nsINativeAppSupport* native)
{
  nsresult rv;

  NS_IF_ADDREF(gNativeAppSupport = native);

  
  nsCOMPtr<nsIToolkitChromeRegistry> cr =
    mozilla::services::GetToolkitChromeRegistryService();

  if (cr)
    cr->CheckForOSAccessibility();

  nsCOMPtr<nsIWindowCreator> creator (do_GetService(NS_APPSTARTUP_CONTRACTID));
  if (!creator) return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIWindowWatcher> wwatch
    (do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  return wwatch->SetWindowCreator(creator);
}

 nsresult
ScopedXPCOMStartup::CreateAppSupport(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  if (!gNativeAppSupport)
    return NS_ERROR_NOT_INITIALIZED;

  return gNativeAppSupport->QueryInterface(aIID, aResult);
}

nsINativeAppSupport* ScopedXPCOMStartup::gNativeAppSupport;




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
    ScopedXPCOMStartup xpcom;
    xpcom.Initialize();

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
         "  --display=DISPLAY  X display to use\n"
         "  --sync             Make X calls synchronous\n");
#endif
#ifdef XP_UNIX
  printf("  --g-fatal-warnings Make all warnings fatal\n"
         "\n%s options\n", gAppData->name);
#endif

  printf("  -h or --help       Print this message.\n"
         "  -v or --version    Print %s version.\n"
         "  -P <profile>       Start with <profile>.\n"
         "  --profile <path>   Start with profile at <path>.\n"
         "  --migration        Start with migration wizard.\n"
         "  --ProfileManager   Start with ProfileManager.\n"
         "  --no-remote        Do not accept or send remote commands; implies --new-instance.\n"
         "  --new-instance     Open new instance, not a new window in running instance.\n"
         "  --UILocale <locale> Start with <locale> resources as UI Locale.\n"
         "  --safe-mode        Disables extensions and themes for this session.\n", gAppData->name);

#if defined(XP_WIN)
  printf("  --console          Start %s with a debugging console.\n", gAppData->name);
#endif

  
  
  
  DumpArbitraryHelp();
}

#if defined(DEBUG) && defined(XP_WIN)
#ifdef DEBUG_warren
#define _CRTDBG_MAP_ALLOC
#endif



#include <stdio.h>
#include <crtdbg.h>
#include "mozilla/mozalloc_abort.h"
static int MSCRTReportHook( int aReportType, char *aMessage, int *oReturnValue)
{
  *oReturnValue = 0; 

  
  
  
  
  

  switch(aReportType) {
  case 0:
    fputs("\nWARNING: CRT WARNING", stderr);
    fputs(aMessage, stderr);
    fputs("\n", stderr);
    break;
  case 1:
    fputs("\n###!!! ABORT: CRT ERROR ", stderr);
    mozalloc_abort(aMessage);
    break;
  case 2:
    fputs("\n###!!! ABORT: CRT ASSERT ", stderr);
    mozalloc_abort(aMessage);
    break;
  }

  
  return 1;
}

#endif

static inline void
DumpVersion()
{
  if (gAppData->vendor)
    printf("%s ", gAppData->vendor);
  printf("%s %s", gAppData->name, gAppData->version);
  if (gAppData->copyright)
      printf(", %s", gAppData->copyright);
  printf("\n");
}

#ifdef MOZ_ENABLE_XREMOTE
static RemoteResult
RemoteCommandLine(const char* aDesktopStartupID)
{
  nsresult rv;
  ArgResult ar;

  const char *profile = 0;
  nsAutoCString program(gAppData->remotingName);
  ToLowerCase(program);
  const char *username = getenv("LOGNAME");

  ar = CheckArg("p", false, &profile, false);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -p requires a profile name\n");
    return REMOTE_ARG_BAD;
  }

  const char *temp = nullptr;
  ar = CheckArg("a", true, &temp);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -a requires an application name\n");
    return REMOTE_ARG_BAD;
  } else if (ar == ARG_FOUND) {
    program.Assign(temp);
  }

  ar = CheckArg("u", true, &username);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -u requires a username\n");
    return REMOTE_ARG_BAD;
  }

  XRemoteClient client;
  rv = client.Init();
  if (NS_FAILED(rv))
    return REMOTE_NOT_FOUND;
 
  nsXPIDLCString response;
  bool success = false;
  rv = client.SendCommandLine(program.get(), username, profile,
                              gArgc, gArgv, aDesktopStartupID,
                              getter_Copies(response), &success);
  
  if (!success)
    return REMOTE_NOT_FOUND;

  
  
  if (response.EqualsLiteral("500 command not parseable"))
    return REMOTE_ARG_BAD;

  if (NS_FAILED(rv))
    return REMOTE_NOT_FOUND;

  return REMOTE_FOUND;
}
#endif 

void
XRE_InitOmnijar(nsIFile* greOmni, nsIFile* appOmni)
{
  mozilla::Omnijar::Init(greOmni, appOmni);
}

nsresult
XRE_GetBinaryPath(const char* argv0, nsIFile* *aResult)
{
  return mozilla::BinaryPath::GetFile(argv0, aResult);
}

#ifdef XP_WIN
#include "nsWindowsRestart.cpp"
#include <shellapi.h>

typedef BOOL (WINAPI* SetProcessDEPPolicyFunc)(DWORD dwFlags);
#endif




static nsresult LaunchChild(nsINativeAppSupport* aNative,
                            bool aBlankCommandLine = false)
{
  aNative->Quit(); 

  
  

#ifdef MOZ_JPROF
  
  unsetenv("JPROF_SLAVE");
#endif

  if (aBlankCommandLine) {
#if defined(MOZ_WIDGET_QT)
    
    gRestartArgc = gQtOnlyArgc;
    gRestartArgv = gQtOnlyArgv;
#else
    gRestartArgc = 1;
    gRestartArgv[gRestartArgc] = nullptr;
#endif
  }

  SaveToEnv("MOZ_LAUNCHED_CHILD=1");

#if defined(MOZ_WIDGET_ANDROID)
  mozilla::widget::GeckoAppShell::ScheduleRestart();
#else
#if defined(XP_MACOSX)
  CommandLineServiceMac::SetupMacCommandLine(gRestartArgc, gRestartArgv, true);
  uint32_t restartMode = 0;
  restartMode = gRestartMode;
  LaunchChildMac(gRestartArgc, gRestartArgv, restartMode);
#else
  nsCOMPtr<nsIFile> lf;
  nsresult rv = XRE_GetBinaryPath(gArgv[0], getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

#if defined(XP_WIN)
  nsAutoString exePath;
  rv = lf->GetPath(exePath);
  if (NS_FAILED(rv))
    return rv;

  if (!WinLaunchChild(exePath.get(), gRestartArgc, gRestartArgv))
    return NS_ERROR_FAILURE;

#else
  nsAutoCString exePath;
  rv = lf->GetNativePath(exePath);
  if (NS_FAILED(rv))
    return rv;

#if defined(XP_UNIX)
  if (execv(exePath.get(), gRestartArgv) == -1)
    return NS_ERROR_FAILURE;
#else
  PRProcess* process = PR_CreateProcess(exePath.get(), gRestartArgv,
                                        nullptr, nullptr);
  if (!process) return NS_ERROR_FAILURE;

  int32_t exitCode;
  PRStatus failed = PR_WaitProcess(process, &exitCode);
  if (failed || exitCode)
    return NS_ERROR_FAILURE;
#endif 
#endif 
#endif 
#endif 

  return NS_ERROR_LAUNCHED_CHILD_PROCESS;
}

static const char kProfileProperties[] =
  "chrome://mozapps/locale/profile/profileSelection.properties";

namespace {





class ReturnAbortOnError
{
public:
  MOZ_IMPLICIT ReturnAbortOnError(nsresult aRv)
  {
    mRv = ConvertRv(aRv);
  }

  operator nsresult()
  {
    return mRv;
  }

private:
  inline nsresult
  ConvertRv(nsresult aRv)
  {
    if (NS_SUCCEEDED(aRv) || aRv == NS_ERROR_LAUNCHED_CHILD_PROCESS) {
      return aRv;
    }
    return NS_ERROR_ABORT;
  }

  nsresult mRv;
};

} 

static ReturnAbortOnError
ProfileLockedDialog(nsIFile* aProfileDir, nsIFile* aProfileLocalDir,
                    nsIProfileUnlocker* aUnlocker,
                    nsINativeAppSupport* aNative, nsIProfileLock* *aResult)
{
  nsresult rv;

  ScopedXPCOMStartup xpcom;
  rv = xpcom.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::Telemetry::WriteFailedProfileLock(aProfileDir);

  rv = xpcom.SetWindowCreator(aNative);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  { 
    nsCOMPtr<nsIStringBundleService> sbs =
      mozilla::services::GetStringBundleService();
    NS_ENSURE_TRUE(sbs, NS_ERROR_FAILURE);

    nsCOMPtr<nsIStringBundle> sb;
    sbs->CreateBundle(kProfileProperties, getter_AddRefs(sb));
    NS_ENSURE_TRUE_LOG(sbs, NS_ERROR_FAILURE);

    NS_ConvertUTF8toUTF16 appName(gAppData->name);
    const char16_t* params[] = {appName.get(), appName.get()};

    nsXPIDLString killMessage;
#ifndef XP_MACOSX
    sb->FormatStringFromName(aUnlocker ? MOZ_UTF16("restartMessageUnlocker")
                                       : MOZ_UTF16("restartMessageNoUnlocker"),
                             params, 2, getter_Copies(killMessage));
#else
    sb->FormatStringFromName(aUnlocker ? MOZ_UTF16("restartMessageUnlockerMac")
                                       : MOZ_UTF16("restartMessageNoUnlockerMac"),
                             params, 2, getter_Copies(killMessage));
#endif

    nsXPIDLString killTitle;
    sb->FormatStringFromName(MOZ_UTF16("restartTitle"),
                             params, 1, getter_Copies(killTitle));

    if (!killMessage || !killTitle)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIPromptService> ps
      (do_GetService(NS_PROMPTSERVICE_CONTRACTID));
    NS_ENSURE_TRUE(ps, NS_ERROR_FAILURE);

    if (aUnlocker) {
      int32_t button;
#ifdef MOZ_WIDGET_ANDROID
      mozilla::widget::GeckoAppShell::KillAnyZombies();
      button = 0;
#else
      const uint32_t flags =
        (nsIPromptService::BUTTON_TITLE_IS_STRING *
         nsIPromptService::BUTTON_POS_0) +
        (nsIPromptService::BUTTON_TITLE_CANCEL *
         nsIPromptService::BUTTON_POS_1);

      bool checkState = false;
      rv = ps->ConfirmEx(nullptr, killTitle, killMessage, flags,
                         killTitle, nullptr, nullptr, nullptr,
                         &checkState, &button);
      NS_ENSURE_SUCCESS_LOG(rv, rv);
#endif

      if (button == 0) {
        rv = aUnlocker->Unlock(nsIProfileUnlocker::FORCE_QUIT);
        if (NS_FAILED(rv)) {
          return rv;
        }

        SaveFileToEnv("XRE_PROFILE_PATH", aProfileDir);
        SaveFileToEnv("XRE_PROFILE_LOCAL_PATH", aProfileLocalDir);

        return LaunchChild(aNative);
      }
    } else {
#ifdef MOZ_WIDGET_ANDROID
      if (mozilla::widget::GeckoAppShell::UnlockProfile()) {
        return NS_LockProfilePath(aProfileDir, aProfileLocalDir, 
                                  nullptr, aResult);
      }
#else
      rv = ps->Alert(nullptr, killTitle, killMessage);
      NS_ENSURE_SUCCESS_LOG(rv, rv);
#endif
    }

    return NS_ERROR_ABORT;
  }
}

static nsresult
ProfileMissingDialog(nsINativeAppSupport* aNative)
{
  nsresult rv;

  ScopedXPCOMStartup xpcom;
  rv = xpcom.Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = xpcom.SetWindowCreator(aNative);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  { 
    nsCOMPtr<nsIStringBundleService> sbs =
      mozilla::services::GetStringBundleService();
    NS_ENSURE_TRUE(sbs, NS_ERROR_FAILURE);
  
    nsCOMPtr<nsIStringBundle> sb;
    sbs->CreateBundle(kProfileProperties, getter_AddRefs(sb));
    NS_ENSURE_TRUE_LOG(sbs, NS_ERROR_FAILURE);
  
    NS_ConvertUTF8toUTF16 appName(gAppData->name);
    const char16_t* params[] = {appName.get(), appName.get()};
  
    nsXPIDLString missingMessage;
  
    
    sb->FormatStringFromName(MOZ_UTF16("profileMissing"), params, 2, getter_Copies(missingMessage));
  
    nsXPIDLString missingTitle;
    sb->FormatStringFromName(MOZ_UTF16("profileMissingTitle"),
                             params, 1, getter_Copies(missingTitle));
  
    if (missingMessage && missingTitle) {
      nsCOMPtr<nsIPromptService> ps
        (do_GetService(NS_PROMPTSERVICE_CONTRACTID));
      NS_ENSURE_TRUE(ps, NS_ERROR_FAILURE);
  
      ps->Alert(nullptr, missingTitle, missingMessage);
    }

    return NS_ERROR_ABORT;
  }
}

static nsresult
ProfileLockedDialog(nsIToolkitProfile* aProfile, nsIProfileUnlocker* aUnlocker,
                    nsINativeAppSupport* aNative, nsIProfileLock* *aResult)
{
  nsCOMPtr<nsIFile> profileDir;
  nsresult rv = aProfile->GetRootDir(getter_AddRefs(profileDir));
  if (NS_FAILED(rv)) return rv;

  bool exists;
  profileDir->Exists(&exists);
  if (!exists) {
    return ProfileMissingDialog(aNative);
  }

  nsCOMPtr<nsIFile> profileLocalDir;
  rv = aProfile->GetLocalDir(getter_AddRefs(profileLocalDir));
  if (NS_FAILED(rv)) return rv;

  return ProfileLockedDialog(profileDir, profileLocalDir, aUnlocker, aNative,
                             aResult);
}

static const char kProfileManagerURL[] =
  "chrome://mozapps/content/profile/profileSelection.xul";

static ReturnAbortOnError
ShowProfileManager(nsIToolkitProfileService* aProfileSvc,
                   nsINativeAppSupport* aNative)
{
  if (!CanShowProfileManager()) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsresult rv;

  nsCOMPtr<nsIFile> profD, profLD;
  char16_t* profileNamePtr;
  nsAutoCString profileName;

  {
    ScopedXPCOMStartup xpcom;
    rv = xpcom.Initialize();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = xpcom.SetWindowCreator(aNative);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

#ifdef XP_MACOSX
    CommandLineServiceMac::SetupMacCommandLine(gRestartArgc, gRestartArgv, true);
#endif

#ifdef XP_WIN
    
    
    ProcessDDE(aNative, false);
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
      rv = windowWatcher->OpenWindow(nullptr,
                                     kProfileManagerURL,
                                     "_blank",
                                     "centerscreen,chrome,modal,titlebar",
                                     ioParamBlock,
                                     getter_AddRefs(newWindow));

      NS_ENSURE_SUCCESS_LOG(rv, rv);

      aProfileSvc->Flush();

      int32_t dialogConfirmed;
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

      rv = ioParamBlock->GetString(0, &profileNamePtr);
      NS_ENSURE_SUCCESS(rv, rv);

      CopyUTF16toUTF8(profileNamePtr, profileName);
      NS_Free(profileNamePtr);

      lock->Unlock();
    }
  }

  SaveFileToEnv("XRE_PROFILE_PATH", profD);
  SaveFileToEnv("XRE_PROFILE_LOCAL_PATH", profLD);
  SaveWordToEnv("XRE_PROFILE_NAME", profileName);

  bool offline = false;
  aProfileSvc->GetStartOffline(&offline);
  if (offline) {
    SaveToEnv("XRE_START_OFFLINE=1");
  }

  return LaunchChild(aNative);
}

static nsresult
GetCurrentProfileIsDefault(nsIToolkitProfileService* aProfileSvc,
                           nsIFile* aCurrentProfileRoot, bool *aResult)
{
  nsresult rv;
  
  
  nsCOMPtr<nsIToolkitProfile> selectedProfile;
  nsCOMPtr<nsIFile> selectedProfileRoot;
  rv = aProfileSvc->GetSelectedProfile(getter_AddRefs(selectedProfile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectedProfile->GetRootDir(getter_AddRefs(selectedProfileRoot));
  NS_ENSURE_SUCCESS(rv, rv);

  bool currentIsSelected;
  rv = aCurrentProfileRoot->Equals(selectedProfileRoot, &currentIsSelected);

  *aResult = currentIsSelected;
  return rv;
}








static nsresult
SetCurrentProfileAsDefault(nsIToolkitProfileService* aProfileSvc,
                           nsIFile* aCurrentProfileRoot)
{
  NS_ENSURE_ARG_POINTER(aProfileSvc);

  nsCOMPtr<nsISimpleEnumerator> profiles;
  nsresult rv = aProfileSvc->GetProfiles(getter_AddRefs(profiles));
  if (NS_FAILED(rv))
    return rv;

  bool foundMatchingProfile = false;
  nsCOMPtr<nsISupports> supports;
  rv = profiles->GetNext(getter_AddRefs(supports));
  while (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIToolkitProfile> profile = do_QueryInterface(supports);
    nsCOMPtr<nsIFile> profileRoot;
    profile->GetRootDir(getter_AddRefs(profileRoot));
    profileRoot->Equals(aCurrentProfileRoot, &foundMatchingProfile);
    if (foundMatchingProfile && profile) {
      rv = aProfileSvc->SetSelectedProfile(profile);
      if (NS_SUCCEEDED(rv))
        rv = aProfileSvc->Flush();
      return rv;
    }
    rv = profiles->GetNext(getter_AddRefs(supports));
  }
  return rv;
}

static bool gDoMigration = false;
static bool gDoProfileReset = false;









static nsresult
SelectProfile(nsIProfileLock* *aResult, nsIToolkitProfileService* aProfileSvc, nsINativeAppSupport* aNative,
              bool* aStartOffline, nsACString* aProfileName)
{
  StartupTimeline::Record(StartupTimeline::SELECT_PROFILE);

  nsresult rv;
  ArgResult ar;
  const char* arg;
  *aResult = nullptr;
  *aStartOffline = false;

  ar = CheckArg("offline", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --offline is invalid when argument --osint is specified\n");
    return NS_ERROR_FAILURE;
  }

  if (ar || EnvHasValue("XRE_START_OFFLINE"))
    *aStartOffline = true;

  if (EnvHasValue("MOZ_RESET_PROFILE_RESTART")) {
    gDoProfileReset = true;
    gDoMigration = true;
    SaveToEnv("MOZ_RESET_PROFILE_RESTART=");
  }

  
  ar = CheckArg("reset-profile", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --reset-profile is invalid when argument --osint is specified\n");
    return NS_ERROR_FAILURE;
  } else if (ar == ARG_FOUND) {
    gDoProfileReset = true;
  }

  ar = CheckArg("migration", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --migration is invalid when argument --osint is specified\n");
    return NS_ERROR_FAILURE;
  } else if (ar == ARG_FOUND) {
    gDoMigration = true;
  }

  nsCOMPtr<nsIFile> lf = GetFileFromEnv("XRE_PROFILE_PATH");
  if (lf) {
    nsCOMPtr<nsIFile> localDir =
      GetFileFromEnv("XRE_PROFILE_LOCAL_PATH");
    if (!localDir) {
      localDir = lf;
    }

    arg = PR_GetEnv("XRE_PROFILE_NAME");
    if (arg && *arg && aProfileName)
      aProfileName->Assign(nsDependentCString(arg));

    
    const char *dummy;
    CheckArg("p", false, &dummy);
    CheckArg("profile", false, &dummy);
    CheckArg("profilemanager");

    if (gDoProfileReset) {
      
      
      bool currentIsSelected;
      rv = GetCurrentProfileIsDefault(aProfileSvc, lf, &currentIsSelected);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!currentIsSelected) {
        NS_WARNING("Profile reset is only supported for the default profile.");
        gDoProfileReset = gDoMigration = false;
      }

      
      nsCOMPtr<nsIToolkitProfile> newProfile;
      rv = CreateResetProfile(aProfileSvc, getter_AddRefs(newProfile));
      if (NS_SUCCEEDED(rv)) {
        rv = newProfile->GetRootDir(getter_AddRefs(lf));
        NS_ENSURE_SUCCESS(rv, rv);
        SaveFileToEnv("XRE_PROFILE_PATH", lf);

        rv = newProfile->GetLocalDir(getter_AddRefs(localDir));
        NS_ENSURE_SUCCESS(rv, rv);
        SaveFileToEnv("XRE_PROFILE_LOCAL_PATH", localDir);

        rv = newProfile->GetName(*aProfileName);
        if (NS_FAILED(rv))
          aProfileName->Truncate(0);
        SaveWordToEnv("XRE_PROFILE_NAME", *aProfileName);
      } else {
        NS_WARNING("Profile reset failed.");
        gDoProfileReset = false;
      }
    }

    return NS_LockProfilePath(lf, localDir, nullptr, aResult);
  }

  ar = CheckArg("profile", true, &arg);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --profile requires a path\n");
    return NS_ERROR_FAILURE;
  }
  if (ar) {
    if (gDoProfileReset) {
      NS_WARNING("Profile reset is only supported for the default profile.");
      gDoProfileReset = false;
    }

    nsCOMPtr<nsIFile> lf;
    rv = XRE_GetFileFromPath(arg, getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIProfileUnlocker> unlocker;

    
    bool exists;
    lf->Exists(&exists);
    if (!exists) {
        rv = lf->Create(nsIFile::DIRECTORY_TYPE, 0700);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    rv = NS_LockProfilePath(lf, lf, getter_AddRefs(unlocker), aResult);
    if (NS_SUCCEEDED(rv))
      return rv;

    return ProfileLockedDialog(lf, lf, unlocker, aNative, aResult);
  }

  ar = CheckArg("createprofile", true, &arg);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --createprofile requires a profile name\n");
    return NS_ERROR_FAILURE;
  }
  if (ar) {
    nsCOMPtr<nsIToolkitProfile> profile;

    const char* delim = strchr(arg, ' ');
    if (delim) {
      nsCOMPtr<nsIFile> lf;
      rv = NS_NewNativeLocalFile(nsDependentCString(delim + 1),
                                   true, getter_AddRefs(lf));
      if (NS_FAILED(rv)) {
        PR_fprintf(PR_STDERR, "Error: profile path not valid.\n");
        return rv;
      }

      
      
      rv = aProfileSvc->CreateProfile(lf, nsDependentCSubstring(arg, delim),
                                     getter_AddRefs(profile));
    } else {
      rv = aProfileSvc->CreateProfile(nullptr, nsDependentCString(arg),
                                     getter_AddRefs(profile));
    }
    
    if (NS_FAILED(rv)) {
      PR_fprintf(PR_STDERR, "Error creating profile.\n");
      return rv; 
    }
    rv = NS_ERROR_ABORT;  
    aProfileSvc->Flush();

    
    
    nsCOMPtr<nsIFile> prefsJSFile;
    profile->GetRootDir(getter_AddRefs(prefsJSFile));
    prefsJSFile->AppendNative(NS_LITERAL_CSTRING("prefs.js"));
    nsAutoCString pathStr;
    prefsJSFile->GetNativePath(pathStr);
    PR_fprintf(PR_STDERR, "Success: created profile '%s' at '%s'\n", arg, pathStr.get());
    bool exists;
    prefsJSFile->Exists(&exists);
    if (!exists)
      prefsJSFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
    

    return rv;
  }

  uint32_t count;
  rv = aProfileSvc->GetProfileCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  ar = CheckArg("p", false, &arg);
  if (ar == ARG_BAD) {
    ar = CheckArg("osint");
    if (ar == ARG_FOUND) {
      PR_fprintf(PR_STDERR, "Error: argument -p is invalid when argument --osint is specified\n");
      return NS_ERROR_FAILURE;
    }

    if (CanShowProfileManager()) {
      return ShowProfileManager(aProfileSvc, aNative);
    }
  }
  if (ar) {
    ar = CheckArg("osint");
    if (ar == ARG_FOUND) {
      PR_fprintf(PR_STDERR, "Error: argument -p is invalid when argument --osint is specified\n");
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIToolkitProfile> profile;
    rv = aProfileSvc->GetProfileByName(nsDependentCString(arg),
                                      getter_AddRefs(profile));
    if (NS_SUCCEEDED(rv)) {
      if (gDoProfileReset) {
        NS_WARNING("Profile reset is only supported for the default profile.");
        gDoProfileReset = false;
      }

      nsCOMPtr<nsIProfileUnlocker> unlocker;
      rv = profile->Lock(getter_AddRefs(unlocker), aResult);
      if (NS_SUCCEEDED(rv)) {
        if (aProfileName)
          aProfileName->Assign(nsDependentCString(arg));
        return NS_OK;
      }

      return ProfileLockedDialog(profile, unlocker, aNative, aResult);
    }

    if (CanShowProfileManager()) {
      return ShowProfileManager(aProfileSvc, aNative);
    }
  }

  ar = CheckArg("profilemanager", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --profilemanager is invalid when argument --osint is specified\n");
    return NS_ERROR_FAILURE;
  } else if (ar == ARG_FOUND && CanShowProfileManager()) {
    return ShowProfileManager(aProfileSvc, aNative);
  }

#ifndef MOZ_DEV_EDITION
  
  
  if (count == 1) {
    nsCOMPtr<nsIToolkitProfile> deProfile;
    
    aProfileSvc->GetSelectedProfile(getter_AddRefs(deProfile));
    nsAutoCString profileName;
    deProfile->GetName(profileName);
    if (profileName.EqualsLiteral("dev-edition-default")) {
      count = 0;
    }
  }
#endif

  if (!count) {
    gDoMigration = true;
    gDoProfileReset = false;

    
    nsCOMPtr<nsIToolkitProfile> profile;
    nsresult rv = aProfileSvc->CreateProfile(nullptr, 
#ifdef MOZ_DEV_EDITION
                                             NS_LITERAL_CSTRING("dev-edition-default"),
#else
                                             NS_LITERAL_CSTRING("default"),
#endif
                                             getter_AddRefs(profile));
    if (NS_SUCCEEDED(rv)) {
#ifndef MOZ_DEV_EDITION
      aProfileSvc->SetDefaultProfile(profile);
#endif
      aProfileSvc->Flush();
      rv = profile->Lock(nullptr, aResult);
      if (NS_SUCCEEDED(rv)) {
        if (aProfileName)
#ifdef MOZ_DEV_EDITION
          aProfileName->AssignLiteral("dev-edition-default");
#else
          aProfileName->AssignLiteral("default");
#endif
        return NS_OK;
      }
    }
  }

  bool useDefault = true;
  if (count > 1 && CanShowProfileManager()) {
    aProfileSvc->GetStartWithLastProfile(&useDefault);
  }

  if (useDefault) {
    nsCOMPtr<nsIToolkitProfile> profile;
    
    aProfileSvc->GetSelectedProfile(getter_AddRefs(profile));
    if (profile) {
      
      if (gDoProfileReset) {
        {
          
          nsIProfileLock* tempProfileLock;
          nsCOMPtr<nsIProfileUnlocker> unlocker;
          rv = profile->Lock(getter_AddRefs(unlocker), &tempProfileLock);
          if (NS_FAILED(rv))
            return ProfileLockedDialog(profile, unlocker, aNative, &tempProfileLock);
        }

        nsCOMPtr<nsIToolkitProfile> newProfile;
        rv = CreateResetProfile(aProfileSvc, getter_AddRefs(newProfile));
        if (NS_SUCCEEDED(rv))
          profile = newProfile;
        else
          gDoProfileReset = false;
      }

      
      
      
      

      static const int kLockRetrySeconds = 5;
      static const int kLockRetrySleepMS = 100;

      nsCOMPtr<nsIProfileUnlocker> unlocker;
      const TimeStamp start = TimeStamp::Now();
      do {
        rv = profile->Lock(getter_AddRefs(unlocker), aResult);
        if (NS_SUCCEEDED(rv)) {
          StartupTimeline::Record(StartupTimeline::AFTER_PROFILE_LOCKED);
          
          if (aProfileName) {
            rv = profile->GetName(*aProfileName);
            if (NS_FAILED(rv))
              aProfileName->Truncate(0);
          }
          return NS_OK;
        }
        PR_Sleep(kLockRetrySleepMS);
      } while (TimeStamp::Now() - start < TimeDuration::FromSeconds(kLockRetrySeconds));

      return ProfileLockedDialog(profile, unlocker, aNative, aResult);
    }
  }

  if (!CanShowProfileManager()) {
    return NS_ERROR_FAILURE;
  }

  return ShowProfileManager(aProfileSvc, aNative);
}








static bool
CheckCompatibility(nsIFile* aProfileDir, const nsCString& aVersion,
                   const nsCString& aOSABI, nsIFile* aXULRunnerDir,
                   nsIFile* aAppDir, nsIFile* aFlagFile, 
                   bool* aCachesOK)
{
  *aCachesOK = false;
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return false;
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsINIParser parser;
  nsresult rv = parser.Init(file);
  if (NS_FAILED(rv))
    return false;

  nsAutoCString buf;
  rv = parser.GetString("Compatibility", "LastVersion", buf);
  if (NS_FAILED(rv) || !aVersion.Equals(buf))
    return false;

  rv = parser.GetString("Compatibility", "LastOSABI", buf);
  if (NS_FAILED(rv) || !aOSABI.Equals(buf))
    return false;

  rv = parser.GetString("Compatibility", "LastPlatformDir", buf);
  if (NS_FAILED(rv))
    return false;

  nsCOMPtr<nsIFile> lf;
  rv = NS_NewNativeLocalFile(buf, false,
                             getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return false;

  bool eq;
  rv = lf->Equals(aXULRunnerDir, &eq);
  if (NS_FAILED(rv) || !eq)
    return false;

  if (aAppDir) {
    rv = parser.GetString("Compatibility", "LastAppDir", buf);
    if (NS_FAILED(rv))
      return false;

    rv = NS_NewNativeLocalFile(buf, false,
                               getter_AddRefs(lf));
    if (NS_FAILED(rv))
      return false;

    rv = lf->Equals(aAppDir, &eq);
    if (NS_FAILED(rv) || !eq)
      return false;
  }

  
  rv = parser.GetString("Compatibility", "InvalidateCaches", buf);
  *aCachesOK = (NS_FAILED(rv) || !buf.EqualsLiteral("1"));
  
  bool purgeCaches = false;
  if (aFlagFile) {
    aFlagFile->Exists(&purgeCaches);
  }

  *aCachesOK = !purgeCaches && *aCachesOK;
  return true;
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
             nsIFile* aAppDir, bool invalidateCache)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return;
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsAutoCString platformDir;
  aXULRunnerDir->GetNativePath(platformDir);

  nsAutoCString appDir;
  if (aAppDir)
    aAppDir->GetNativePath(appDir);

  PRFileDesc *fd = nullptr;
  file->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0600, &fd);
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

  static const char kInvalidationHeader[] = NS_LINEBREAK "InvalidateCaches=1";
  if (invalidateCache)
    PR_Write(fd, kInvalidationHeader, sizeof(kInvalidationHeader) - 1);

  static const char kNL[] = NS_LINEBREAK;
  PR_Write(fd, kNL, sizeof(kNL) - 1);

  PR_Close(fd);
}







static bool
RemoveComponentRegistries(nsIFile* aProfileDir, nsIFile* aLocalProfileDir,
                                      bool aRemoveEMFiles)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return false;

  if (aRemoveEMFiles) {
    file->SetNativeLeafName(NS_LITERAL_CSTRING("extensions.ini"));
    file->Remove(false);
  }

  aLocalProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return false;

#if defined(XP_UNIX) || defined(XP_BEOS)
#define PLATFORM_FASL_SUFFIX ".mfasl"
#elif defined(XP_WIN)
#define PLATFORM_FASL_SUFFIX ".mfl"
#endif

  file->AppendNative(NS_LITERAL_CSTRING("XUL" PLATFORM_FASL_SUFFIX));
  file->Remove(false);
  
  file->SetNativeLeafName(NS_LITERAL_CSTRING("XPC" PLATFORM_FASL_SUFFIX));
  file->Remove(false);

  file->SetNativeLeafName(NS_LITERAL_CSTRING("startupCache"));
  nsresult rv = file->Remove(true);
  return NS_SUCCEEDED(rv) || rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
}





static struct SavedVar {
  const char *name;
  char *value;
} gSavedVars[] = {
  {"XUL_APP_FILE", nullptr}
};

static void SaveStateForAppInitiatedRestart()
{
  for (size_t i = 0; i < ArrayLength(gSavedVars); ++i) {
    const char *s = PR_GetEnv(gSavedVars[i].name);
    if (s)
      gSavedVars[i].value = PR_smprintf("%s=%s", gSavedVars[i].name, s);
  }
}

static void RestoreStateForAppInitiatedRestart()
{
  for (size_t i = 0; i < ArrayLength(gSavedVars); ++i) {
    if (gSavedVars[i].value)
      PR_SetEnv(gSavedVars[i].value);
  }
}

#ifdef MOZ_CRASHREPORTER




static void MakeOrSetMinidumpPath(nsIFile* profD)
{
  nsCOMPtr<nsIFile> dumpD;
  profD->Clone(getter_AddRefs(dumpD));
  
  if(dumpD) {
    bool fileExists;
    
    dumpD->Append(NS_LITERAL_STRING("minidumps"));
    dumpD->Exists(&fileExists);
    if(!fileExists) {
      dumpD->Create(nsIFile::DIRECTORY_TYPE, 0700);
    }

    nsAutoString pathStr;
    if(NS_SUCCEEDED(dumpD->GetPath(pathStr)))
      CrashReporter::SetMinidumpPath(pathStr);
  }
}
#endif

const nsXREAppData* gAppData = nullptr;

#ifdef MOZ_WIDGET_GTK
#include "prlink.h"
typedef void (*_g_set_application_name_fn)(const gchar *application_name);
typedef void (*_gtk_window_set_auto_startup_notification_fn)(gboolean setting);

static PRFuncPtr FindFunction(const char* aName)
{
  PRLibrary *lib = nullptr;
  PRFuncPtr result = PR_FindFunctionSymbolAndLibrary(aName, &lib);
  
  if (lib) {
    PR_UnloadLibrary(lib);
  }
  return result;
}

static void MOZ_gdk_display_close(GdkDisplay *display)
{
#if CLEANUP_MEMORY
  
  
  bool theme_is_qt = false;
  GtkSettings* settings =
    gtk_settings_get_for_screen(gdk_display_get_default_screen(display));
  gchar *theme_name;
  g_object_get(settings, "gtk-theme-name", &theme_name, nullptr);
  if (theme_name) {
    theme_is_qt = strcmp(theme_name, "Qt") == 0;
    if (theme_is_qt)
      NS_WARNING("wallpaper bug 417163 for Qt theme");
    g_free(theme_name);
  }

  
  
  
  PangoContext *pangoContext = gdk_pango_context_get();

  bool buggyCairoShutdown = cairo_version() < CAIRO_VERSION_ENCODE(1, 4, 0);

  if (!buggyCairoShutdown) {
    
    
    
    
    if (!theme_is_qt)
      gdk_display_close(display);
  }

  
  
  
  
  
  
  
  
  

  
  PangoFontMap *fontmap = pango_context_get_font_map(pangoContext);
  
  
  
  
  if (PANGO_IS_FC_FONT_MAP(fontmap))
      pango_fc_font_map_shutdown(PANGO_FC_FONT_MAP(fontmap));
  g_object_unref(pangoContext);

  
  pango_cairo_font_map_set_default(nullptr);

  
  
#ifdef cairo_debug_reset_static_data
#error "Looks like we're including Mozilla's cairo instead of system cairo"
#endif
  cairo_debug_reset_static_data();
  
  FcFini();

  if (buggyCairoShutdown) {
    if (!theme_is_qt)
      gdk_display_close(display);
  }
#else 
  
  
  (void) display;
#endif
}
#endif 













NS_VISIBILITY_DEFAULT PRBool nspr_use_zone_allocator = PR_FALSE;

#ifdef CAIRO_HAS_DWRITE_FONT

#include <dwrite.h>

#ifdef DEBUG_DWRITE_STARTUP

#define LOGREGISTRY(msg) LogRegistryEvent(msg)


static void LogRegistryEvent(const wchar_t *msg)
{
  HKEY dummyKey;
  HRESULT hr;
  wchar_t buf[512];

  wsprintf(buf, L" log %s", msg);
  hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &dummyKey);
  if (SUCCEEDED(hr)) {
    RegCloseKey(dummyKey);
  }
}
#else

#define LOGREGISTRY(msg)

#endif

static DWORD WINAPI InitDwriteBG(LPVOID lpdwThreadParam)
{
  SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
  LOGREGISTRY(L"loading dwrite.dll");
  HMODULE dwdll = LoadLibraryW(L"dwrite.dll");
  if (dwdll) {
    decltype(DWriteCreateFactory)* createDWriteFactory = (decltype(DWriteCreateFactory)*)
      GetProcAddress(dwdll, "DWriteCreateFactory");
    if (createDWriteFactory) {
      LOGREGISTRY(L"creating dwrite factory");
      IDWriteFactory *factory;
      HRESULT hr = createDWriteFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&factory));
      if (SUCCEEDED(hr)) {
        LOGREGISTRY(L"dwrite factory done");
        factory->Release();
        LOGREGISTRY(L"freed factory");
      } else {
        LOGREGISTRY(L"failed to create factory");
      }
    }
  }
  SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
  return 0;
}
#endif

#ifdef USE_GLX_TEST
bool fire_glxtest_process();
#endif

#if defined(XP_WIN) && defined(MOZ_METRO)
#ifndef AHE_TYPE
enum AHE_TYPE {
  AHE_DESKTOP = 0,
  AHE_IMMERSIVE = 1
};
#endif







void
SetLastWinRunType(AHE_TYPE aType)
{
  HKEY key;
  LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
                              L"SOFTWARE\\Mozilla\\Firefox",
                              0, KEY_WRITE, &key);
  if (result != ERROR_SUCCESS) {
    return;
  }
  DWORD value = (DWORD)aType;
  result = RegSetValueEx(key, L"MetroLastAHE", 0, REG_DWORD,
                         reinterpret_cast<LPBYTE>(&value),
                         sizeof(DWORD));
  RegCloseKey(key);
}
#endif 

#include "GeckoProfiler.h"


class XREMain
{
public:
  XREMain() :
    mScopedXPCOM(nullptr)
    , mAppData(nullptr)
    , mStartOffline(false)
    , mShuttingDown(false)
#ifdef MOZ_ENABLE_XREMOTE
    , mDisableRemote(false)
#endif
#if defined(MOZ_WIDGET_GTK)
    , mGdkDisplay(nullptr)
#endif
  {};

  ~XREMain() {
    if (mAppData) {
      delete mAppData;
    }
    if (mScopedXPCOM) {
      NS_WARNING("Scoped xpcom should have been deleted!");
      delete mScopedXPCOM;
    }
  }

  int XRE_main(int argc, char* argv[], const nsXREAppData* aAppData);
  int XRE_mainInit(bool* aExitFlag);
  int XRE_mainStartup(bool* aExitFlag);
  nsresult XRE_mainRun();
  
  nsCOMPtr<nsINativeAppSupport> mNativeApp;
  nsCOMPtr<nsIToolkitProfileService> mProfileSvc;
  nsCOMPtr<nsIFile> mProfD;
  nsCOMPtr<nsIFile> mProfLD;
  nsCOMPtr<nsIProfileLock> mProfileLock;
#ifdef MOZ_ENABLE_XREMOTE
  nsCOMPtr<nsIRemoteService> mRemoteService;
#endif

  ScopedXPCOMStartup* mScopedXPCOM;
  ScopedAppData* mAppData;
  nsXREDirProvider mDirProvider;
  nsAutoCString mProfileName;
  nsAutoCString mDesktopStartupID;

  bool mStartOffline;
  bool mShuttingDown;
#ifdef MOZ_ENABLE_XREMOTE
  bool mDisableRemote;
#endif

#if defined(MOZ_WIDGET_GTK)
  GdkDisplay* mGdkDisplay;
#endif
};






int
XREMain::XRE_mainInit(bool* aExitFlag)
{
  if (!aExitFlag)
    return 1;
  *aExitFlag = false;

  StartupTimeline::Record(StartupTimeline::MAIN);

  if (ChaosMode::isActive(ChaosMode::Any)) {
    printf_stderr("*** You are running in chaos test mode. See ChaosMode.h. ***\n");
  }

  nsresult rv;
  ArgResult ar;

#ifdef DEBUG
  if (PR_GetEnv("XRE_MAIN_BREAK"))
    NS_BREAK();
#endif

#ifdef USE_GLX_TEST
  
  
  
  
  
  
  
  fire_glxtest_process();
#endif

#if defined(XP_WIN) && defined(MOZ_METRO)
  
  if (CheckArg("metro-update", false, nullptr, false) == ARG_FOUND ||
      XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
    
    
    
    SetLastWinRunType(AHE_IMMERSIVE);
  } else {
    SetLastWinRunType(AHE_DESKTOP);
  }
#endif

  SetupErrorHandling(gArgv[0]);

#ifdef CAIRO_HAS_DWRITE_FONT
  {
    
    
    
    
    
    
    
    
    

    if (IsVistaOrLater()) {
      CreateThread(nullptr, 0, &InitDwriteBG, nullptr, 0, nullptr);
    }
  }
#endif

#ifdef XP_UNIX
  const char *home = PR_GetEnv("HOME");
  if (!home || !*home) {
    struct passwd *pw = getpwuid(geteuid());
    if (!pw || !pw->pw_dir) {
      Output(true, "Could not determine HOME directory");
      return 1;
    }
    SaveWordToEnv("HOME", nsDependentCString(pw->pw_dir));
  }
#endif

#ifdef MOZ_ACCESSIBILITY_ATK
  
  
  SaveToEnv("NO_AT_BRIDGE=1");
#endif

  
  const char* override = nullptr;
  ar = CheckArg("override", true, &override);
  if (ar == ARG_BAD) {
    Output(true, "Incorrect number of arguments passed to --override");
    return 1;
  }
  else if (ar == ARG_FOUND) {
    nsCOMPtr<nsIFile> overrideLF;
    rv = XRE_GetFileFromPath(override, getter_AddRefs(overrideLF));
    if (NS_FAILED(rv)) {
      Output(true, "Error: unrecognized override.ini path.\n");
      return 1;
    }

    rv = XRE_ParseAppData(overrideLF, mAppData);
    if (NS_FAILED(rv)) {
      Output(true, "Couldn't read override.ini");
      return 1;
    }
  }

  

  if (!mAppData->name) {
    Output(true, "Error: App:Name not specified in application.ini\n");
    return 1;
  }
  if (!mAppData->buildID) {
    Output(true, "Error: App:BuildID not specified in application.ini\n");
    return 1;
  }

  
  

  if (!mAppData->xreDirectory) {
    nsCOMPtr<nsIFile> lf;
    rv = XRE_GetBinaryPath(gArgv[0], getter_AddRefs(lf));
    if (NS_FAILED(rv))
      return 2;

    nsCOMPtr<nsIFile> greDir;
    rv = lf->GetParent(getter_AddRefs(greDir));
    if (NS_FAILED(rv))
      return 2;

#ifdef XP_MACOSX
    nsCOMPtr<nsIFile> parent;
    greDir->GetParent(getter_AddRefs(parent));
    greDir = parent.forget();
    greDir->AppendNative(NS_LITERAL_CSTRING("Resources"));
#endif

    greDir.forget(&mAppData->xreDirectory);
  }

  if (!mAppData->directory) {
    NS_IF_ADDREF(mAppData->directory = mAppData->xreDirectory);
  }

  if (mAppData->size > offsetof(nsXREAppData, minVersion)) {
    if (!mAppData->minVersion) {
      Output(true, "Error: Gecko:MinVersion not specified in application.ini\n");
      return 1;
    }

    if (!mAppData->maxVersion) {
      
      
      SetAllocatedString(mAppData->maxVersion, "1.*");
    }

    if (mozilla::Version(mAppData->minVersion) > gToolkitVersion ||
        mozilla::Version(mAppData->maxVersion) < gToolkitVersion) {
      Output(true, "Error: Platform version '%s' is not compatible with\n"
             "minVersion >= %s\nmaxVersion <= %s\n",
             gToolkitVersion,
             mAppData->minVersion, mAppData->maxVersion);
      return 1;
    }
  }

  rv = mDirProvider.Initialize(mAppData->directory, mAppData->xreDirectory);
  if (NS_FAILED(rv))
    return 1;

#ifdef MOZ_CRASHREPORTER
  if (EnvHasValue("MOZ_CRASHREPORTER")) {
    mAppData->flags |= NS_XRE_ENABLE_CRASH_REPORTER;
  }

  nsCOMPtr<nsIFile> xreBinDirectory;
  xreBinDirectory = mDirProvider.GetGREBinDir();

  if ((mAppData->flags & NS_XRE_ENABLE_CRASH_REPORTER) &&
      NS_SUCCEEDED(
        CrashReporter::SetExceptionHandler(xreBinDirectory))) {
    nsCOMPtr<nsIFile> file;
    rv = mDirProvider.GetUserAppDataDirectory(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
      CrashReporter::SetUserAppDataDirectory(file);
    }
    if (mAppData->crashReporterURL)
      CrashReporter::SetServerURL(nsDependentCString(mAppData->crashReporterURL));

    
    if (mAppData->vendor)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("Vendor"),
                                         nsDependentCString(mAppData->vendor));
    if (mAppData->name)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ProductName"),
                                         nsDependentCString(mAppData->name));
    if (mAppData->ID)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ProductID"),
                                         nsDependentCString(mAppData->ID));
    if (mAppData->version)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("Version"),
                                         nsDependentCString(mAppData->version));
    if (mAppData->buildID)
      CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("BuildID"),
                                         nsDependentCString(mAppData->buildID));

    nsDependentCString releaseChannel(NS_STRINGIFY(MOZ_UPDATE_CHANNEL));
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ReleaseChannel"),
                                       releaseChannel);
#ifdef MOZ_LINKER
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("CrashAddressLikelyWrong"),
                                       IsSignalHandlingBroken() ? NS_LITERAL_CSTRING("1")
                                                                : NS_LITERAL_CSTRING("0"));
#endif
    CrashReporter::SetRestartArgs(gArgc, gArgv);

    
    nsCOMPtr<nsIFile> userAppDataDir;
    if (NS_SUCCEEDED(mDirProvider.GetUserAppDataDirectory(
                                                         getter_AddRefs(userAppDataDir)))) {
      CrashReporter::SetupExtraData(userAppDataDir,
                                    nsDependentCString(mAppData->buildID));

      
      nsCOMPtr<nsIFile> overrideini;
      bool exists;
      if (NS_SUCCEEDED(mDirProvider.GetAppDir()->Clone(getter_AddRefs(overrideini))) &&
          NS_SUCCEEDED(overrideini->AppendNative(NS_LITERAL_CSTRING("crashreporter-override.ini"))) &&
          NS_SUCCEEDED(overrideini->Exists(&exists)) &&
          exists) {
#ifdef XP_WIN
        nsAutoString overridePathW;
        overrideini->GetPath(overridePathW);
        NS_ConvertUTF16toUTF8 overridePath(overridePathW);
#else
        nsAutoCString overridePath;
        overrideini->GetNativePath(overridePath);
#endif

        SaveWordToEnv("MOZ_CRASHREPORTER_STRINGS_OVERRIDE", overridePath);
      }
    }
  }
#endif

#ifdef XP_MACOSX
  if (EnvHasValue("MOZ_LAUNCHED_CHILD")) {
    
    
    
    EnsureUseCocoaDockAPI();

    
    
    
    
    
    
    const EventTypeSpec kFakeEventList[] = { { INT_MAX, INT_MAX } };
    EventRef event;
    ::ReceiveNextEvent(GetEventTypeCount(kFakeEventList), kFakeEventList,
                       kEventDurationNoWait, false, &event);
  }

  if (CheckArg("foreground")) {
    
    
    
    ProcessSerialNumber psn;
    if (::GetCurrentProcess(&psn) == noErr)
      ::SetFrontProcess(&psn);
  }
#endif

  SaveToEnv("MOZ_LAUNCHED_CHILD=");

  gRestartArgc = gArgc;
  gRestartArgv = (char**) malloc(sizeof(char*) * (gArgc + 1 + (override ? 2 : 0)));
  if (!gRestartArgv) {
    return 1;
  }

  int i;
  for (i = 0; i < gArgc; ++i) {
    gRestartArgv[i] = gArgv[i];
  }
  
  
  if (override) {
    gRestartArgv[gRestartArgc++] = const_cast<char*>("-override");
    gRestartArgv[gRestartArgc++] = const_cast<char*>(override);
  }

  gRestartArgv[gRestartArgc] = nullptr;
  

  if (EnvHasValue("MOZ_SAFE_MODE_RESTART")) {
    gSafeMode = true;
    
    SaveToEnv("MOZ_SAFE_MODE_RESTART=");
  }

  ar = CheckArg("safe-mode", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --safe-mode is invalid when argument --osint is specified\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    gSafeMode = true;
  }

#ifdef XP_WIN
  
  
  
  
  
  if ((GetKeyState(VK_SHIFT) & 0x8000) &&
      !(GetKeyState(VK_CONTROL) & 0x8000) &&
      !(GetKeyState(VK_MENU) & 0x8000) &&
      !EnvHasValue("MOZ_DISABLE_SAFE_MODE_KEY")) {
    gSafeMode = true;
  }
#endif

#ifdef XP_MACOSX
  if ((GetCurrentEventKeyModifiers() & optionKey) &&
      !EnvHasValue("MOZ_DISABLE_SAFE_MODE_KEY"))
    gSafeMode = true;
#endif

  
  
  
  ar = CheckArg("no-remote", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --no-remote is invalid when argument --osint is specified\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    SaveToEnv("MOZ_NO_REMOTE=1");
  }

  ar = CheckArg("new-instance", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --new-instance is invalid when argument --osint is specified\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    SaveToEnv("MOZ_NEW_INSTANCE=1");
  }

  
  
  if (CheckArg("h") || CheckArg("help") || CheckArg("?")) {
    DumpHelp();
    *aExitFlag = true;
    return 0;
  }

  if (CheckArg("v") || CheckArg("version")) {
    DumpVersion();
    *aExitFlag = true;
    return 0;
  }

  rv = XRE_InitCommandLine(gArgc, gArgv);
  NS_ENSURE_SUCCESS(rv, 1);

  
  ar = CheckArg("register", true);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --register is invalid when argument --osint is specified\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    ScopedXPCOMStartup xpcom;
    rv = xpcom.Initialize();
    NS_ENSURE_SUCCESS(rv, 1);
    {
      nsCOMPtr<nsIChromeRegistry> chromeReg =
        mozilla::services::GetChromeRegistryService();
      NS_ENSURE_TRUE(chromeReg, 1);

      chromeReg->CheckForNewChrome();
    }
    *aExitFlag = true;
    return 0;
  }

  return 0;
}

#ifdef MOZ_CRASHREPORTER
#ifdef XP_WIN





static void AnnotateSystemManufacturer()
{
  nsRefPtr<IWbemLocator> locator;

  HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IWbemLocator, getter_AddRefs(locator));

  if (FAILED(hr)) {
    return;
  }

  nsRefPtr<IWbemServices> services;

  hr = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr,
                              0, nullptr, nullptr, getter_AddRefs(services));

  if (FAILED(hr)) {
    return;
  }

  hr = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                         RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                         nullptr, EOAC_NONE);

  if (FAILED(hr)) {
    return;
  }

  nsRefPtr<IEnumWbemClassObject> enumerator;

  hr = services->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM Win32_BIOS"),
                           WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                           nullptr, getter_AddRefs(enumerator));

  if (FAILED(hr) || !enumerator) {
    return;
  }

  nsRefPtr<IWbemClassObject> classObject;
  ULONG results;

  hr = enumerator->Next(WBEM_INFINITE, 1, getter_AddRefs(classObject), &results);

  if (FAILED(hr) || results == 0) {
    return;
  }

  VARIANT value;
  VariantInit(&value);

  hr = classObject->Get(L"Manufacturer", 0, &value, 0, 0);

  if (SUCCEEDED(hr) && V_VT(&value) == VT_BSTR) {
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("BIOS_Manufacturer"),
                                       NS_ConvertUTF16toUTF8(V_BSTR(&value)));
  }

  VariantClear(&value);
}

static void PR_CALLBACK AnnotateSystemManufacturer_ThreadStart(void*)
{
  HRESULT hr = CoInitialize(nullptr);

  if (FAILED(hr)) {
    return;
  }

  AnnotateSystemManufacturer();

  CoUninitialize();
}
#endif
#endif

namespace mozilla {
  ShutdownChecksMode gShutdownChecks = SCM_NOTHING;
}

static void SetShutdownChecks() {
  
  
  
  

#ifdef DEBUG
  gShutdownChecks = SCM_CRASH;
#else
  const char* releaseChannel = NS_STRINGIFY(MOZ_UPDATE_CHANNEL);
  if (strcmp(releaseChannel, "nightly") == 0 ||
      strcmp(releaseChannel, "default") == 0) {
    gShutdownChecks = SCM_RECORD;
  } else {
    gShutdownChecks = SCM_NOTHING;
  }
#endif

  
  
  const char* mozShutdownChecksEnv = PR_GetEnv("MOZ_SHUTDOWN_CHECKS");
  if (mozShutdownChecksEnv) {
    if (strcmp(mozShutdownChecksEnv, "crash") == 0) {
      gShutdownChecks = SCM_CRASH;
    } else if (strcmp(mozShutdownChecksEnv, "record") == 0) {
      gShutdownChecks = SCM_RECORD;
    } else if (strcmp(mozShutdownChecksEnv, "nothing") == 0) {
      gShutdownChecks = SCM_NOTHING;
    }
  }

}






int
XREMain::XRE_mainStartup(bool* aExitFlag)
{
  nsresult rv;

  if (!aExitFlag)
    return 1;
  *aExitFlag = false;

  SetShutdownChecks();

  
#ifdef DEBUG
  mozilla::Telemetry::InitIOReporting(gAppData->xreDirectory);
#else
  {
    const char* releaseChannel = NS_STRINGIFY(MOZ_UPDATE_CHANNEL);
    if (strcmp(releaseChannel, "nightly") == 0 ||
        strcmp(releaseChannel, "default") == 0) {
      mozilla::Telemetry::InitIOReporting(gAppData->xreDirectory);
    }
  }
#endif 

#if defined(MOZ_WIDGET_GTK) || defined(MOZ_ENABLE_XREMOTE)
  
#define HAVE_DESKTOP_STARTUP_ID
  const char* desktopStartupIDEnv = PR_GetEnv("DESKTOP_STARTUP_ID");
  if (desktopStartupIDEnv) {
    mDesktopStartupID.Assign(desktopStartupIDEnv);
  }
#endif

#if defined(MOZ_WIDGET_QT)
  nsQAppInstance::AddRef(gArgc, gArgv, true);

  QStringList nonQtArguments = qApp->arguments();
  gQtOnlyArgc = 1;
  gQtOnlyArgv = (char**) malloc(sizeof(char*) 
                * (gRestartArgc - nonQtArguments.size() + 2));

  
  gQtOnlyArgv[0] = gRestartArgv[0];

  for (int i = 1; i < gRestartArgc; ++i) {
    if (!nonQtArguments.contains(gRestartArgv[i])) {
      
      gQtOnlyArgv[gQtOnlyArgc++] = gRestartArgv[i];
    }
  }
  gQtOnlyArgv[gQtOnlyArgc] = nullptr;
#endif
#if defined(MOZ_WIDGET_GTK)
  
  
  
  
#if (MOZ_WIDGET_GTK == 2)
  if (CheckArg("install"))
    gdk_rgb_set_install(TRUE);
#endif

  
  {
    nsAutoCString program(gAppData->name);
    ToLowerCase(program);
    g_set_prgname(program.get());
  }

  

  
  
  
  if (!gtk_parse_args(&gArgc, &gArgv))
    return 1;
#endif 

  if (PR_GetEnv("MOZ_RUN_GTEST")) {
    int result;
#ifdef XP_WIN
    UseParentConsole();
#endif
    
    if (mozilla::RunGTest) {
      gIsGtest = true;
      result = mozilla::RunGTest();
      gIsGtest = false;
    } else {
      result = 1;
      printf("TEST-UNEXPECTED-FAIL | gtest | Not compiled with enable-tests\n");
    }
    *aExitFlag = true;
    return result;
  }

#if defined(MOZ_WIDGET_GTK)
  
  const char *display_name = gdk_get_display_arg_name();
  if (display_name) {
    SaveWordToEnv("DISPLAY", nsDependentCString(display_name));
  } else {
    display_name = PR_GetEnv("DISPLAY");
    if (!display_name) {
      PR_fprintf(PR_STDERR, "Error: no display specified\n");
      return 1;
    }
  }
#endif 
#ifdef MOZ_X11
  
  
  
  
  
  
  
  
  
  
  
  
# ifndef NIGHTLY_BUILD
  if (PR_GetEnv("MOZ_USE_OMTC") ||
      PR_GetEnv("MOZ_OMTC_ENABLED"))
# endif
  {
    XInitThreads();
  }
#endif
#if defined(MOZ_WIDGET_GTK)
  {
    mGdkDisplay = gdk_display_open(display_name);
    if (!mGdkDisplay) {
      PR_fprintf(PR_STDERR, "Error: cannot open display: %s\n", display_name);
      return 1;
    }
    gdk_display_manager_set_default_display (gdk_display_manager_get(),
                                             mGdkDisplay);
    if (!GDK_IS_X11_DISPLAY(mGdkDisplay))
      mDisableRemote = true;
  }
#endif
#ifdef MOZ_ENABLE_XREMOTE
  
  bool newInstance;
  {
    char *e = PR_GetEnv("MOZ_NO_REMOTE");
    mDisableRemote = (mDisableRemote || (e && *e));
    if (mDisableRemote) {
      newInstance = true;
    } else {
      e = PR_GetEnv("MOZ_NEW_INSTANCE");
      newInstance = (e && *e);
    }
  }

  if (!newInstance) {
    
    const char* desktopStartupIDPtr =
      mDesktopStartupID.IsEmpty() ? nullptr : mDesktopStartupID.get();

    RemoteResult rr = RemoteCommandLine(desktopStartupIDPtr);
    if (rr == REMOTE_FOUND) {
      *aExitFlag = true;
      return 0;
    }
    else if (rr == REMOTE_ARG_BAD)
      return 1;
  }
#endif
#if defined(MOZ_WIDGET_GTK)
  
  _g_set_application_name_fn _g_set_application_name =
    (_g_set_application_name_fn)FindFunction("g_set_application_name");
  if (_g_set_application_name) {
    _g_set_application_name(mAppData->name);
  }
  _gtk_window_set_auto_startup_notification_fn _gtk_window_set_auto_startup_notification =
    (_gtk_window_set_auto_startup_notification_fn)FindFunction("gtk_window_set_auto_startup_notification");
  if (_gtk_window_set_auto_startup_notification) {
    _gtk_window_set_auto_startup_notification(false);
  }

#if (MOZ_WIDGET_GTK == 2)
  gtk_widget_set_default_colormap(gdk_rgb_get_colormap());
#endif 
#endif 
#ifdef MOZ_X11
  
  XRE_InstallX11ErrorHandler();
#endif

  
#ifdef MOZ_JPROF
  setupProfilingStuff();
#endif

  rv = NS_CreateNativeAppSupport(getter_AddRefs(mNativeApp));
  if (NS_FAILED(rv))
    return 1;

  bool canRun = false;
  rv = mNativeApp->Start(&canRun);
  if (NS_FAILED(rv) || !canRun) {
    return 1;
  }

#if defined(HAVE_DESKTOP_STARTUP_ID) && defined(MOZ_WIDGET_GTK)
  
  
  if (!mDesktopStartupID.IsEmpty()) {
    nsAutoCString desktopStartupEnv;
    desktopStartupEnv.AssignLiteral("DESKTOP_STARTUP_ID=");
    desktopStartupEnv.Append(mDesktopStartupID);
    
    PR_SetEnv(ToNewCString(desktopStartupEnv));
  }
#endif

#if defined(USE_MOZ_UPDATER)
  
  nsCOMPtr<nsIFile> updRoot;
  bool persistent;
  rv = mDirProvider.GetFile(XRE_UPDATE_ROOT_DIR, &persistent,
                            getter_AddRefs(updRoot));
  
  if (NS_FAILED(rv))
    updRoot = mDirProvider.GetAppDir();

  
  
  if (EnvHasValue("MOZ_PROCESS_UPDATES")) {
    
    
    
    const char *logFile = nullptr;
    if (ARG_FOUND == CheckArg("dump-args", false, &logFile)) {
      FILE* logFP = fopen(logFile, "wb");
      if (logFP) {
        for (int i = 1; i < gRestartArgc; ++i) {
          fprintf(logFP, "%s\n", gRestartArgv[i]);
        }
        fclose(logFP);
      }
    }
    *aExitFlag = true;
    return 0;
  }

  
  
  
  
  
  if (CheckArg("process-updates")) {
    SaveToEnv("MOZ_PROCESS_UPDATES=1");
  }
  nsCOMPtr<nsIFile> exeFile, exeDir;
  rv = mDirProvider.GetFile(XRE_EXECUTABLE_FILE, &persistent,
                            getter_AddRefs(exeFile));
  NS_ENSURE_SUCCESS(rv, 1);
  rv = exeFile->GetParent(getter_AddRefs(exeDir));
  NS_ENSURE_SUCCESS(rv, 1);
  ProcessUpdates(mDirProvider.GetGREDir(),
                 exeDir,
                 updRoot,
                 gRestartArgc,
                 gRestartArgv,
                 mAppData->version);
  if (EnvHasValue("MOZ_PROCESS_UPDATES")) {
    SaveToEnv("MOZ_PROCESS_UPDATES=");
    *aExitFlag = true;
    return 0;
  }
#if defined(XP_WIN) && defined(MOZ_METRO)
  if (CheckArg("metro-update", false) == ARG_FOUND) {
    *aExitFlag = true;
    return 0;
  }
#endif
#endif

  rv = NS_NewToolkitProfileService(getter_AddRefs(mProfileSvc));
  if (rv == NS_ERROR_FILE_ACCESS_DENIED) {
    PR_fprintf(PR_STDERR, "Error: Access was denied while trying to open files in " \
                "your profile directory.\n");
  }
  if (NS_FAILED(rv)) {
    
    ProfileMissingDialog(mNativeApp);
    return 1;
  }

  rv = SelectProfile(getter_AddRefs(mProfileLock), mProfileSvc, mNativeApp, &mStartOffline,
                      &mProfileName);
  if (rv == NS_ERROR_LAUNCHED_CHILD_PROCESS ||
      rv == NS_ERROR_ABORT) {
    *aExitFlag = true;
    return 0;
  }

  if (NS_FAILED(rv)) {
    
    ProfileMissingDialog(mNativeApp);
    return 1;
  }
  gProfileLock = mProfileLock;

  rv = mProfileLock->GetDirectory(getter_AddRefs(mProfD));
  NS_ENSURE_SUCCESS(rv, 1);

  rv = mProfileLock->GetLocalDirectory(getter_AddRefs(mProfLD));
  NS_ENSURE_SUCCESS(rv, 1);

  rv = mDirProvider.SetProfile(mProfD, mProfLD);
  NS_ENSURE_SUCCESS(rv, 1);

  

  mozilla::Telemetry::SetProfileDir(mProfD);

#ifdef MOZ_CRASHREPORTER
  if (mAppData->flags & NS_XRE_ENABLE_CRASH_REPORTER)
      MakeOrSetMinidumpPath(mProfD);

  CrashReporter::SetProfileDirectory(mProfD);
#endif

  nsAutoCString version;
  BuildVersion(version);

#ifdef TARGET_OS_ABI
  NS_NAMED_LITERAL_CSTRING(osABI, TARGET_OS_ABI);
#else
  
  NS_NAMED_LITERAL_CSTRING(osABI, OS_TARGET "_UNKNOWN");
#endif

  
  
  
  
  
 
  
  
  nsCOMPtr<nsIFile> flagFile;

  rv = NS_ERROR_FILE_NOT_FOUND;
  nsCOMPtr<nsIFile> fFlagFile;
  if (mAppData->directory) {
    rv = mAppData->directory->Clone(getter_AddRefs(fFlagFile));
  }
  flagFile = do_QueryInterface(fFlagFile);
  if (flagFile) {
    flagFile->AppendNative(FILE_INVALIDATE_CACHES);
  }

  bool cachesOK;
  bool versionOK = CheckCompatibility(mProfD, version, osABI, 
                                      mDirProvider.GetGREDir(),
                                      mAppData->directory, flagFile,
                                      &cachesOK);
  if (CheckArg("purgecaches")) {
    cachesOK = false;
  }
  if (PR_GetEnv("MOZ_PURGE_CACHES")) {
    cachesOK = false;
  }
 
  
  
  
  
  
  
  
  bool startupCacheValid = true;
  if (gSafeMode) {
    startupCacheValid = RemoveComponentRegistries(mProfD, mProfLD, false);
    WriteVersion(mProfD, NS_LITERAL_CSTRING("Safe Mode"), osABI,
                 mDirProvider.GetGREDir(), mAppData->directory, !startupCacheValid);
  }
  else if (versionOK) {
    if (!cachesOK) {
      
      
      
      startupCacheValid = RemoveComponentRegistries(mProfD, mProfLD, false);
        
      
      WriteVersion(mProfD, version, osABI,
                   mDirProvider.GetGREDir(), mAppData->directory, !startupCacheValid);
    }
    
  }
  else {
    
    
    
    startupCacheValid = RemoveComponentRegistries(mProfD, mProfLD, true);

    
    WriteVersion(mProfD, version, osABI,
                 mDirProvider.GetGREDir(), mAppData->directory, !startupCacheValid);
  }

  if (!startupCacheValid)
    StartupCache::IgnoreDiskCache();

  if (flagFile) {
    flagFile->Remove(true);
  }

  return 0;
}





nsresult
XREMain::XRE_mainRun()
{
  nsresult rv = NS_OK;
  NS_ASSERTION(mScopedXPCOM, "Scoped xpcom not initialized.");

#ifdef MOZ_B2G_LOADER
  mozilla::ipc::ProcLoaderClientGeckoInit();
#endif

#ifdef NS_FUNCTION_TIMER
  
  
  {
    nsCOMPtr<nsISupports> comp;

    comp = do_GetService("@mozilla.org/preferences-service;1");

    comp = do_GetService("@mozilla.org/network/socket-transport-service;1");

    comp = do_GetService("@mozilla.org/network/dns-service;1");

    comp = do_GetService("@mozilla.org/network/io-service;1");

    comp = do_GetService("@mozilla.org/chrome/chrome-registry;1");

    comp = do_GetService("@mozilla.org/focus-event-suppressor-service;1");
  }
#endif

  rv = mScopedXPCOM->SetWindowCreator(mNativeApp);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

#ifdef MOZ_CRASHREPORTER
  
  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPrefBranch> defaultPrefBranch;
    rv = prefs->GetDefaultBranch(nullptr, getter_AddRefs(defaultPrefBranch));

    if (NS_SUCCEEDED(rv)) {
      nsXPIDLCString sval;
      rv = defaultPrefBranch->GetCharPref("app.update.channel", getter_Copies(sval));
      if (NS_SUCCEEDED(rv)) {
        CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ReleaseChannel"),
                                            sval);
      }
    }
  }
  
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("FramePoisonBase"),
                                     nsPrintfCString("%.16llx", uint64_t(gMozillaPoisonBase)));
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("FramePoisonSize"),
                                     nsPrintfCString("%lu", uint32_t(gMozillaPoisonSize)));

#ifdef XP_WIN
  PR_CreateThread(PR_USER_THREAD, AnnotateSystemManufacturer_ThreadStart, 0,
                  PR_PRIORITY_LOW, PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD, 0);
#endif

#endif

  if (mStartOffline) {
    nsCOMPtr<nsIIOService2> io (do_GetService("@mozilla.org/network/io-service;1"));
    NS_ENSURE_TRUE(io, NS_ERROR_FAILURE);
    io->SetManageOfflineStatus(false);
    io->SetOffline(true);
  }

  {
    nsCOMPtr<nsIObserver> startupNotifier
      (do_CreateInstance(NS_APPSTARTUPNOTIFIER_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    startupNotifier->Observe(nullptr, APPSTARTUP_TOPIC, nullptr);
  }

  nsCOMPtr<nsIAppStartup> appStartup
    (do_GetService(NS_APPSTARTUP_CONTRACTID));
  NS_ENSURE_TRUE(appStartup, NS_ERROR_FAILURE);

  if (gDoMigration) {
    nsCOMPtr<nsIFile> file;
    mDirProvider.GetAppDir()->Clone(getter_AddRefs(file));
    file->AppendNative(NS_LITERAL_CSTRING("override.ini"));
    nsINIParser parser;
    nsresult rv = parser.Init(file);
    if (NS_SUCCEEDED(rv)) {
      nsAutoCString buf;
      rv = parser.GetString("XRE", "EnableProfileMigrator", buf);
      if (NS_SUCCEEDED(rv)) {
        if (buf[0] == '0' || buf[0] == 'f' || buf[0] == 'F') {
          gDoMigration = false;
        }
      }
    }
  }

  {
    nsCOMPtr<nsIToolkitProfile> selectedProfile;
    if (gDoProfileReset) {
      
      rv = mProfileSvc->GetSelectedProfile(getter_AddRefs(selectedProfile));
      if (NS_FAILED(rv)) {
        gDoProfileReset = false;
        return NS_ERROR_FAILURE;
      }
    }

    
    if (mAppData->flags & NS_XRE_ENABLE_PROFILE_MIGRATOR && gDoMigration) {
      gDoMigration = false;
      nsCOMPtr<nsIProfileMigrator> pm(do_CreateInstance(NS_PROFILEMIGRATOR_CONTRACTID));
      if (pm) {
        nsAutoCString aKey;
        if (gDoProfileReset) {
          
          
          aKey = MOZ_APP_NAME;
        }
        pm->Migrate(&mDirProvider, aKey);
      }
    }

    if (gDoProfileReset) {
      nsresult backupCreated = ProfileResetCleanup(selectedProfile);
      if (NS_FAILED(backupCreated)) NS_WARNING("Could not cleanup the profile that was reset");

      
      rv = SetCurrentProfileAsDefault(mProfileSvc, mProfD);
      if (NS_FAILED(rv)) NS_WARNING("Could not set current profile as the default");
    }
  }

  mDirProvider.DoStartup();

#ifdef MOZ_CRASHREPORTER
  nsCString userAgentLocale;
  if (NS_SUCCEEDED(Preferences::GetCString("general.useragent.locale", &userAgentLocale))) {
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("useragent_locale"), userAgentLocale);
  }
#endif

  appStartup->GetShuttingDown(&mShuttingDown);

  nsCOMPtr<nsICommandLineRunner> cmdLine;

  nsCOMPtr<nsIFile> workingDir;
  rv = NS_GetSpecialDirectory(NS_OS_CURRENT_WORKING_DIR, getter_AddRefs(workingDir));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  if (!mShuttingDown) {
    cmdLine = do_CreateInstance("@mozilla.org/toolkit/command-line;1");
    NS_ENSURE_TRUE(cmdLine, NS_ERROR_FAILURE);

    rv = cmdLine->Init(gArgc, gArgv, workingDir,
                       nsICommandLine::STATE_INITIAL_LAUNCH);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    

    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService) {
      obsService->NotifyObservers(cmdLine, "command-line-startup", nullptr);
    }
  }

  SaveStateForAppInitiatedRestart();

  
  
  SaveToEnv("XRE_PROFILE_PATH=");
  SaveToEnv("XRE_PROFILE_LOCAL_PATH=");
  SaveToEnv("XRE_PROFILE_NAME=");
  SaveToEnv("XRE_START_OFFLINE=");
  SaveToEnv("NO_EM_RESTART=");
  SaveToEnv("XUL_APP_FILE=");
  SaveToEnv("XRE_BINARY_PATH=");

  if (!mShuttingDown) {
    rv = appStartup->CreateHiddenWindow();
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

#if defined(HAVE_DESKTOP_STARTUP_ID) && defined(MOZ_WIDGET_GTK)
    nsGTKToolkit* toolkit = nsGTKToolkit::GetToolkit();
    if (toolkit && !mDesktopStartupID.IsEmpty()) {
      toolkit->SetDesktopStartupID(mDesktopStartupID);
    }
    
    
    g_unsetenv ("DESKTOP_STARTUP_ID");
#endif

#ifdef XP_MACOSX
    
    
    SetupMacApplicationDelegate();

    
    
    cmdLine = do_CreateInstance("@mozilla.org/toolkit/command-line;1");
    NS_ENSURE_TRUE(cmdLine, NS_ERROR_FAILURE);

    CommandLineServiceMac::SetupMacCommandLine(gArgc, gArgv, false);

    rv = cmdLine->Init(gArgc, gArgv,
                        workingDir, nsICommandLine::STATE_INITIAL_LAUNCH);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
#endif

    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService)
      obsService->NotifyObservers(nullptr, "final-ui-startup", nullptr);

    (void)appStartup->DoneStartingUp();
    appStartup->GetShuttingDown(&mShuttingDown);
  }

  if (!mShuttingDown) {
    rv = cmdLine->Run();
    NS_ENSURE_SUCCESS_LOG(rv, NS_ERROR_FAILURE);

    appStartup->GetShuttingDown(&mShuttingDown);
  }

  if (!mShuttingDown) {
#ifdef MOZ_ENABLE_XREMOTE
    
    
    if (!mDisableRemote)
      mRemoteService = do_GetService("@mozilla.org/toolkit/remote-service;1");
    if (mRemoteService)
      mRemoteService->Startup(mAppData->remotingName, mProfileName.get());
#endif 

    mNativeApp->Enable();
  }

#ifdef MOZ_INSTRUMENT_EVENT_LOOP
  if (PR_GetEnv("MOZ_INSTRUMENT_EVENT_LOOP")) {
    bool logToConsole = true;
    mozilla::InitEventTracing(logToConsole);
  }
#endif 

  {
    rv = appStartup->Run();
    if (NS_FAILED(rv)) {
      NS_ERROR("failed to run appstartup");
      gLogConsoleErrors = true;
    }
  }

  return rv;
}






int
XREMain::XRE_main(int argc, char* argv[], const nsXREAppData* aAppData)
{
  char aLocal;
  GeckoProfilerInitRAII profilerGuard(&aLocal);

  PROFILER_LABEL("Startup", "XRE_Main",
    js::ProfileEntry::Category::OTHER);

  nsresult rv = NS_OK;

  gArgc = argc;
  gArgv = argv;

  NS_ENSURE_TRUE(aAppData, 2);

  mAppData = new ScopedAppData(aAppData);
  if (!mAppData)
    return 1;
  if (!mAppData->remotingName) {
    SetAllocatedString(mAppData->remotingName, mAppData->name);
  }
  
  gAppData = mAppData;

  ScopedLogging log;

  mozilla::IOInterposerInit ioInterposerGuard;

#if defined(MOZ_WIDGET_GTK)
#if defined(MOZ_MEMORY) || defined(__FreeBSD__) || defined(__NetBSD__)
  
  
  
  g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, 1);
#endif
  g_thread_init(nullptr);
#endif

  
  bool exit = false;
  int result = XRE_mainInit(&exit);
  if (result != 0 || exit)
    return result;

  
  result = XRE_mainStartup(&exit);
  if (result != 0 || exit)
    return result;

  bool appInitiatedRestart = false;

  
  mScopedXPCOM = new ScopedXPCOMStartup();
  if (!mScopedXPCOM)
    return 1;

  rv = mScopedXPCOM->Initialize();
  NS_ENSURE_SUCCESS(rv, 1);

  
  rv = XRE_mainRun();

#ifdef MOZ_INSTRUMENT_EVENT_LOOP
  mozilla::ShutdownEventTracing();
#endif

  
  
  if (rv == NS_SUCCESS_RESTART_APP
      || rv == NS_SUCCESS_RESTART_METRO_APP
      || rv == NS_SUCCESS_RESTART_APP_NOT_SAME_PROFILE) {
    appInitiatedRestart = true;

    
    
    gShutdownChecks = SCM_NOTHING;
  }

  if (!mShuttingDown) {
#ifdef MOZ_ENABLE_XREMOTE
    
    if (mRemoteService) {
      mRemoteService->Shutdown();
    }
#endif 
  }

  delete mScopedXPCOM;
  mScopedXPCOM = nullptr;

  
  
  mProfileLock->Unlock();
  gProfileLock = nullptr;

#if defined(MOZ_WIDGET_QT)
  nsQAppInstance::Release();
#endif

  
  if (appInitiatedRestart) {
    RestoreStateForAppInitiatedRestart();

    if (rv != NS_SUCCESS_RESTART_APP_NOT_SAME_PROFILE) {
      
      SaveFileToEnvIfUnset("XRE_PROFILE_PATH", mProfD);
      SaveFileToEnvIfUnset("XRE_PROFILE_LOCAL_PATH", mProfLD);
      SaveWordToEnvIfUnset("XRE_PROFILE_NAME", mProfileName);
    }

#ifdef MOZ_WIDGET_GTK
    MOZ_gdk_display_close(mGdkDisplay);
#endif

#if defined(MOZ_METRO) && defined(XP_WIN)
    if (rv == NS_SUCCESS_RESTART_METRO_APP) {
      LaunchDefaultMetroBrowser();
      rv = NS_OK;
    } else
#endif
    {
      rv = LaunchChild(mNativeApp, true);
    }

#ifdef MOZ_CRASHREPORTER
    if (mAppData->flags & NS_XRE_ENABLE_CRASH_REPORTER)
      CrashReporter::UnsetExceptionHandler();
#endif
    return rv == NS_ERROR_LAUNCHED_CHILD_PROCESS ? 0 : 1;
  }

#ifdef MOZ_WIDGET_GTK
  
  
  MOZ_gdk_display_close(mGdkDisplay);
#endif

#ifdef MOZ_CRASHREPORTER
  if (mAppData->flags & NS_XRE_ENABLE_CRASH_REPORTER)
      CrashReporter::UnsetExceptionHandler();
#endif

  XRE_DeinitCommandLine();

  return NS_FAILED(rv) ? 1 : 0;
}

#if defined(MOZ_METRO) && defined(XP_WIN)
extern bool XRE_MetroCoreApplicationRun();
static XREMain* xreMainPtr;


nsresult
XRE_metroStartup(bool runXREMain)
{
  nsresult rv;

  
  
  ScopedLogging log;

  bool exit = false;
  if (xreMainPtr->XRE_mainStartup(&exit) != 0 || exit)
    return NS_ERROR_FAILURE;

  
  xreMainPtr->mScopedXPCOM = new ScopedXPCOMStartup();
  if (!xreMainPtr->mScopedXPCOM)
    return NS_ERROR_FAILURE;

  rv = xreMainPtr->mScopedXPCOM->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  if (runXREMain) {
    rv = xreMainPtr->XRE_mainRun();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

void
XRE_metroShutdown()
{
  delete xreMainPtr->mScopedXPCOM;
  xreMainPtr->mScopedXPCOM = nullptr;

#ifdef MOZ_INSTRUMENT_EVENT_LOOP
  mozilla::ShutdownEventTracing();
#endif

  
  
  xreMainPtr->mProfileLock->Unlock();
  gProfileLock = nullptr;

#ifdef MOZ_CRASHREPORTER
  if (xreMainPtr->mAppData->flags & NS_XRE_ENABLE_CRASH_REPORTER)
      CrashReporter::UnsetExceptionHandler();
#endif

  XRE_DeinitCommandLine();
}

class WinRTInitWrapper
{
public:
  WinRTInitWrapper() {
    mResult = ::RoInitialize(RO_INIT_MULTITHREADED);
  }
  ~WinRTInitWrapper() {
    if (SUCCEEDED(mResult)) {
      ::RoUninitialize();
    }
  }
  HRESULT mResult;
};

int
XRE_mainMetro(int argc, char* argv[], const nsXREAppData* aAppData)
{
  char aLocal;
  GeckoProfilerInitRAII profilerGuard(&aLocal);

  PROFILER_LABEL("Startup", "XRE_Main",
    js::ProfileEntry::Category::OTHER);

  mozilla::IOInterposerInit ioInterposerGuard;

  nsresult rv = NS_OK;

  xreMainPtr = new XREMain();
  if (!xreMainPtr) {
    return 1;
  }

  
  WinRTInitWrapper wrap;

  gArgc = argc;
  gArgv = argv;

  NS_ENSURE_TRUE(aAppData, 2);

  xreMainPtr->mAppData = new ScopedAppData(aAppData);
  if (!xreMainPtr->mAppData)
    return 1;
  
  gAppData = xreMainPtr->mAppData;

  
  bool exit = false;
  if (xreMainPtr->XRE_mainInit(&exit) != 0 || exit)
    return 1;

  
  
  if (!XRE_MetroCoreApplicationRun()) {
    return 1;
  }

  
  
  NS_ASSERTION(!xreMainPtr->mScopedXPCOM,
               "XPCOM Shutdown hasn't occured, and we are exiting.");
  return 0;
}

void SetWindowsEnvironment(WindowsEnvironmentType aEnvID);
#endif 

void
XRE_StopLateWriteChecks(void) {
  mozilla::StopLateWriteChecks();
}

int
XRE_main(int argc, char* argv[], const nsXREAppData* aAppData, uint32_t aFlags)
{
#if !defined(MOZ_METRO) || !defined(XP_WIN)
  XREMain main;
  int result = main.XRE_main(argc, argv, aAppData);
  mozilla::RecordShutdownEndTimeStamp();
  return result;
#else
  if (aFlags == XRE_MAIN_FLAG_USE_METRO) {
    SetWindowsEnvironment(WindowsEnvironmentType_Metro);
  }

  
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop) {
    XREMain main;
    int result = main.XRE_main(argc, argv, aAppData);
    mozilla::RecordShutdownEndTimeStamp();
    return result;
  }

  
  NS_ASSERTION(XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro,
               "Unknown Windows environment");

  SetLastWinRunType(AHE_IMMERSIVE);

  int result = XRE_mainMetro(argc, argv, aAppData);
  mozilla::RecordShutdownEndTimeStamp();
  return result;
#endif 
}

nsresult
XRE_InitCommandLine(int aArgc, char* aArgv[])
{
  nsresult rv = NS_OK;

#if defined(OS_WIN)
  CommandLine::Init(aArgc, aArgv);
#else

  
  char** canonArgs = new char*[aArgc];

  
  nsCOMPtr<nsIFile> binFile;
  rv = XRE_GetBinaryPath(aArgv[0], getter_AddRefs(binFile));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsAutoCString canonBinPath;
  rv = binFile->GetNativePath(canonBinPath);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  canonArgs[0] = strdup(canonBinPath.get());

  for (int i = 1; i < aArgc; ++i) {
    if (aArgv[i]) {
      canonArgs[i] = strdup(aArgv[i]);
    }
  }
 
  NS_ASSERTION(!CommandLine::IsInitialized(), "Bad news!");
  CommandLine::Init(aArgc, canonArgs);

  for (int i = 0; i < aArgc; ++i)
      free(canonArgs[i]);
  delete[] canonArgs;
#endif

  const char *path = nullptr;
  ArgResult ar = CheckArg("greomni", false, &path);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --greomni requires a path argument\n");
    return NS_ERROR_FAILURE;
  }

  if (!path)
    return rv;

  nsCOMPtr<nsIFile> greOmni;
  rv = XRE_GetFileFromPath(path, getter_AddRefs(greOmni));
  if (NS_FAILED(rv)) {
    PR_fprintf(PR_STDERR, "Error: argument --greomni requires a valid path\n");
    return rv;
  }

  ar = CheckArg("appomni", false, &path);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument --appomni requires a path argument\n");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIFile> appOmni;
  if (path) {
      rv = XRE_GetFileFromPath(path, getter_AddRefs(appOmni));
      if (NS_FAILED(rv)) {
        PR_fprintf(PR_STDERR, "Error: argument --appomni requires a valid path\n");
        return rv;
      }
  }

  mozilla::Omnijar::Init(greOmni, appOmni);
  return rv;
}

nsresult
XRE_DeinitCommandLine()
{
  nsresult rv = NS_OK;

  CommandLine::Terminate();

  return rv;
}

GeckoProcessType
XRE_GetProcessType()
{
  return mozilla::startup::sChildProcessType;
}

bool
XRE_IsParentProcess()
{
  return XRE_GetProcessType() == GeckoProcessType_Default;
}

#ifdef NIGHTLY_BUILD
static void
LogE10sBlockedReason(const char *reason) {
  gBrowserTabsRemoteDisabledReason.Assign(NS_ConvertASCIItoUTF16(reason));

  nsAutoString msg(NS_LITERAL_STRING("==================\nE10s has been blocked from running because:\n"));
  msg.Append(gBrowserTabsRemoteDisabledReason);
  msg.AppendLiteral("\n==================\n");
  nsCOMPtr<nsIConsoleService> console(do_GetService("@mozilla.org/consoleservice;1"));
  if (console) {
    console->LogStringMessage(msg.get());
  }
}
#endif

bool
mozilla::BrowserTabsRemoteAutostart()
{
  if (gBrowserTabsRemoteAutostartInitialized) {
    return gBrowserTabsRemoteAutostart;
  }
  gBrowserTabsRemoteAutostartInitialized = true;
  bool optInPref = Preferences::GetBool("browser.tabs.remote.autostart", false);
  bool trialPref = Preferences::GetBool("browser.tabs.remote.autostart.1", false);
  bool prefEnabled = optInPref || trialPref;
#if !defined(NIGHTLY_BUILD)
  
  
  
  bool testPref = Preferences::GetBool("layers.offmainthreadcomposition.testing.enabled", false);
  if (testPref && optInPref) {
    gBrowserTabsRemoteAutostart = true;
  }
#else
  
  
  bool disabledForA11y = Preferences::GetBool("browser.tabs.remote.autostart.disabled-because-using-a11y", false);
  
  bool disabledForVR = Preferences::GetBool("dom.vr.enabled", false);

  if (prefEnabled) {
    if (gSafeMode) {
      LogE10sBlockedReason("Safe mode");
    } else if (disabledForA11y) {
      LogE10sBlockedReason("An accessibility tool is or was active. See bug 1115956.");
    } else if (disabledForVR) {
      LogE10sBlockedReason("Experimental VR interfaces are enabled");
    } else {
      gBrowserTabsRemoteAutostart = true;
    }
  }
#endif

#if defined(XP_MACOSX)
  
  
  if (gBrowserTabsRemoteAutostart) {
    
    bool accelDisabled = Preferences::GetBool("layers.acceleration.disabled", false) &&
                         !Preferences::GetBool("layers.acceleration.force-enabled", false);

    accelDisabled = accelDisabled || !nsCocoaFeatures::AccelerateByDefault();

    
    if (!accelDisabled) {
      nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
      if (gfxInfo) {
        int32_t status;
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_OPENGL_LAYERS, &status)) &&
            status != nsIGfxInfo::FEATURE_STATUS_OK) {
          accelDisabled = true;
        }
      }
    }

    
    if (accelDisabled) {
      const char *acceleratedEnv = PR_GetEnv("MOZ_ACCELERATED");
      if (acceleratedEnv && (*acceleratedEnv != '0')) {
        accelDisabled = false;
      }
    }

    if (accelDisabled) {
      gBrowserTabsRemoteAutostart = false;

#ifdef NIGHTLY_BUILD
      LogE10sBlockedReason("Hardware acceleration is disabled");
#endif
    }
  }
#endif 

  mozilla::Telemetry::Accumulate(mozilla::Telemetry::E10S_AUTOSTART, gBrowserTabsRemoteAutostart);
  if (Preferences::GetBool("browser.enabledE10SFromPrompt", false)) {
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::E10S_STILL_ACCEPTED_FROM_PROMPT,
                                    gBrowserTabsRemoteAutostart);
  }
  if (prefEnabled) {
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::E10S_BLOCKED_FROM_RUNNING,
                                    !gBrowserTabsRemoteAutostart);
  }
  return gBrowserTabsRemoteAutostart;
}

void
SetupErrorHandling(const char* progname)
{
#ifdef XP_WIN
  





  HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
  SetProcessDEPPolicyFunc _SetProcessDEPPolicy =
    (SetProcessDEPPolicyFunc) GetProcAddress(kernel32, "SetProcessDEPPolicy");
  if (_SetProcessDEPPolicy)
    _SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
#endif

#ifdef XP_WIN32
  
  
  
  UINT realMode = SetErrorMode(0);
  realMode |= SEM_FAILCRITICALERRORS;
  
  
  
  
  if (getenv("XRE_NO_WINDOWS_CRASH_DIALOG"))
    realMode |= SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX;

  SetErrorMode(realMode);

#endif

#if defined (DEBUG) && defined(XP_WIN)
  
  
  

  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

  _CrtSetReportHook(MSCRTReportHook);
#endif

  InstallSignalHandlers(progname);

  
  setbuf(stdout, 0);
}
