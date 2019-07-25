










































#if defined(XP_OS2) && defined(MOZ_OS2_HIGH_MEMORY)

#include <os2safe.h>
#endif

#define XPCOM_TRANSLATE_NSGM_ENTRY_POINT 1

#if defined(MOZ_WIDGET_QT)
#include <QtGui/QApplication>
#include <QtCore/QScopedPointer>
#include <QtGui/QApplication>
#include <QtGui/QInputContextFactory>
#include <QtGui/QInputContext>
#endif 

#ifdef MOZ_IPC
#include "mozilla/dom/ContentParent.h"
using mozilla::dom::ContentParent;
#endif

#include "nsAppRunner.h"
#include "nsUpdateDriver.h"

#ifdef XP_MACOSX
#include "MacLaunchHelper.h"
#include "MacApplicationDelegate.h"
#include "MacAutoreleasePool.h"
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
#include "nsIFastLoadService.h" 
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

#include "mozilla/FunctionTimer.h"

#ifdef XP_WIN
#include "nsIWinAppHelper.h"
#include <windows.h>
#include "cairo/cairo-features.h"

#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#endif
#endif

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsEmbedCID.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsStaticComponents.h"
#include "nsXPCOM.h"
#include "nsXPCOMCIDInternal.h"
#include "nsXPIDLString.h"
#include "nsVersionComparator.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsXREDirProvider.h"
#include "nsToolkitCompsCID.h"

#include "nsINIParser.h"
#include "mozilla/Omnijar.h"

#include <stdlib.h>

#if defined(MOZ_SPLASHSCREEN)
#include "nsSplashScreen.h"
#endif

#ifdef XP_UNIX
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif

#ifdef XP_BEOS


#include <AppKit.h>
#include <AppFileInfo.h>
#endif 

#ifdef XP_WIN
#ifndef WINCE
#include <process.h>
#include <shlobj.h>
#endif
#include "nsThreadUtils.h"
#endif

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
#endif


#ifdef MOZ_ENABLE_XREMOTE
#include "XRemoteClient.h"
#include "nsIRemoteService.h"
#endif

#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
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
#endif

#ifdef MOZ_IPC
#include "base/command_line.h"
#endif

#include "mozilla/FunctionTimer.h"

#ifdef ANDROID
#include "AndroidBridge.h"
#endif

#ifdef WINCE
class WindowsMutex {
public:
  WindowsMutex(const wchar_t *name) {
    mHandle = CreateMutexW(0, FALSE, name);
  }

  ~WindowsMutex() {
    Unlock();
    CloseHandle(mHandle);
  }

  PRBool Lock(DWORD timeout = INFINITE) {
    DWORD state = WaitForSingleObject(mHandle, timeout);
    return state == WAIT_OBJECT_0;
  }
  
  void Unlock() {
    if (mHandle)
      ReleaseMutex(mHandle);
  }

protected:
  HANDLE mHandle;
};
#endif

extern PRUint32 gRestartMode;
extern void InstallSignalHandlers(const char *ProgramName);
#include "nsX11ErrorHandler.h"

#define FILE_COMPATIBILITY_INFO NS_LITERAL_CSTRING("compatibility.ini")
#define FILE_INVALIDATE_CACHES NS_LITERAL_CSTRING(".purgecaches")

int    gArgc;
char **gArgv;

static const char gToolkitVersion[] = NS_STRINGIFY(GRE_MILESTONE);
static const char gToolkitBuildID[] = NS_STRINGIFY(GRE_BUILDID);

static int    gRestartArgc;
static char **gRestartArgv;

#ifdef MOZ_WIDGET_QT
static int    gQtOnlyArgc;
static char **gQtOnlyArgv;
#endif

#if defined(MOZ_WIDGET_GTK2)
#if defined(DEBUG) || defined(NS_BUILD_REFCNT_LOGGING) \
  || defined(NS_TRACE_MALLOC)
#define CLEANUP_MEMORY 1
#define PANGO_ENABLE_BACKEND
#include <pango/pangofc-fontmap.h>
#endif
#include <gtk/gtk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 
#include "nsGTKToolkit.h"
#endif


static void
SaveToEnv(const char *putenv)
{
  char *expr = strdup(putenv);
  if (expr)
    PR_SetEnv(expr);
  
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
  nsCAutoString path;
  file->GetNativePath(path);
  SaveWordToEnv(name, path);
#endif
}


static already_AddRefed<nsILocalFile>
GetFileFromEnv(const char *name)
{
  nsresult rv;
  nsILocalFile *file = nsnull;

#ifdef XP_WIN
  WCHAR path[_MAX_PATH];
  if (!GetEnvironmentVariableW(NS_ConvertASCIItoUTF16(name).get(),
                               path, _MAX_PATH))
    return nsnull;

  rv = NS_NewLocalFile(nsDependentString(path), PR_TRUE, &file);
  if (NS_FAILED(rv))
    return nsnull;

  return file;
#else
  const char *arg = PR_GetEnv(name);
  if (!arg || !*arg)
    return nsnull;

  rv = NS_NewNativeLocalFile(nsDependentCString(arg), PR_TRUE, &file);
  if (NS_FAILED(rv))
    return nsnull;

  return file;
#endif
}



static void
SaveWordToEnvIfUnset(const char *name, const nsACString & word)
{
  const char *val = PR_GetEnv(name);
  if (!(val && *val))
    SaveWordToEnv(name, word);
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

    wchar_t wide_msg[1024];
    MultiByteToWideChar(CP_ACP,
                        0,
                        msg,
                        -1,
                        wide_msg,
                        sizeof(wide_msg) / sizeof(wchar_t));

    MessageBoxW(NULL, wide_msg, L"XULRunner", flags);
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
CheckArg(const char* aArg, PRBool aCheckOSInt = PR_FALSE, const char **aParam = nsnull, PRBool aRemArg = PR_TRUE)
{
  NS_ABORT_IF_FALSE(gArgv, "gArgv must be initialized before CheckArg()");

  char **curarg = gArgv + 1; 
  ArgResult ar = ARG_NONE;

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
        if (aRemArg)
          RemoveArg(curarg);
        if (!aParam) {
          ar = ARG_FOUND;
          break;
        }

        if (*curarg) {
          if (**curarg == '-'
#if defined(XP_WIN) || defined(XP_OS2)
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
      PR_fprintf(PR_STDERR, "Error: argument -osint is invalid\n");
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
ProcessDDE(nsINativeAppSupport* aNative, PRBool aWait)
{
  
  
  
  
  
  
  
  
  ArgResult ar;
  ar = CheckArgShell("requestpending");
  if (ar == ARG_FOUND) {
    aNative->Enable(); 
    if (aWait) {
      nsIThread *thread = NS_GetCurrentThread();
      
      
      PRInt32 count = 20;
      while(--count >= 0) {
        NS_ProcessNextEvent(thread);
        PR_Sleep(PR_MillisecondsToInterval(1));
      }
    }
  }
}
#endif

PRBool gSafeMode = PR_FALSE;





class nsXULAppInfo : public nsIXULAppInfo,
#ifdef XP_WIN
                     public nsIWinAppHelper,
#endif
#ifdef MOZ_CRASHREPORTER
                     public nsICrashReporter,
#endif
                     public nsIXULRuntime

{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIXULAPPINFO
  NS_DECL_NSIXULRUNTIME
#ifdef MOZ_CRASHREPORTER
  NS_DECL_NSICRASHREPORTER
#endif
#ifdef XP_WIN
  NS_DECL_NSIWINAPPHELPER
#endif
};

NS_INTERFACE_MAP_BEGIN(nsXULAppInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULRuntime)
  NS_INTERFACE_MAP_ENTRY(nsIXULRuntime)
#ifdef XP_WIN
  NS_INTERFACE_MAP_ENTRY(nsIWinAppHelper)
#endif
#ifdef MOZ_CRASHREPORTER
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
  aResult.Assign(gToolkitVersion);

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

NS_IMETHODIMP
nsXULAppInfo::GetWidgetToolkit(nsACString& aResult)
{
  aResult.AssignLiteral(MOZ_WIDGET_TOOLKIT);
  return NS_OK;
}




#define SYNC_ENUMS(a,b) \
  PR_STATIC_ASSERT(nsIXULRuntime::PROCESS_TYPE_ ## a == \
                   static_cast<int>(GeckoProcessType_ ## b));

SYNC_ENUMS(DEFAULT, Default)
SYNC_ENUMS(PLUGIN, Plugin)
SYNC_ENUMS(CONTENT, Content)
SYNC_ENUMS(JETPACK, Jetpack)
SYNC_ENUMS(IPDLUNITTEST, IPDLUnitTest)


PR_STATIC_ASSERT(GeckoProcessType_IPDLUnitTest + 1 == GeckoProcessType_End);

NS_IMETHODIMP
nsXULAppInfo::GetProcessType(PRUint32* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = XRE_GetProcessType();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::EnsureContentProcess()
{
#ifdef MOZ_IPC
  if (XRE_GetProcessType() != GeckoProcessType_Default)
    return NS_ERROR_NOT_AVAILABLE;

  ContentParent* c = ContentParent::GetSingleton();
  if (!c)
    return NS_ERROR_NOT_AVAILABLE;
  return NS_OK;
#else
  return NS_ERROR_NOT_AVAILABLE;
#endif
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

  nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(file));
  nsINIParser parser;
  rv = parser.Init(localFile);
  if (NS_FAILED(rv)) {
    
    
    return NS_OK;
  }
  
  nsCAutoString buf;
  rv = parser.GetString("Compatibility", "InvalidateCaches", buf);
  
  if (NS_FAILED(rv)) {
    PRFileDesc *fd = nsnull;
    localFile->OpenNSPRFileDesc(PR_RDWR | PR_APPEND, 0600, &fd);
    if (!fd) {
      NS_ERROR("could not create output stream");
      return NS_ERROR_NOT_AVAILABLE;
    }
    static const char kInvalidationHeader[] = NS_LINEBREAK "InvalidateCaches=1" NS_LINEBREAK;
    rv = PR_Write(fd, kInvalidationHeader, sizeof(kInvalidationHeader) - 1);
    PR_Close(fd);
    
    if (NS_FAILED(rv))
      return rv;
  }
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
nsXULAppInfo::GetUserCanElevate(PRBool *aUserCanElevate)
{
#ifdef WINCE
  *aUserCanElevate = PR_FALSE;
  return NS_OK;
#else
  HANDLE hToken;

  VISTA_TOKEN_ELEVATION_TYPE elevationType;
  DWORD dwSize; 

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) ||
      !GetTokenInformation(hToken, VistaTokenElevationType, &elevationType,
                           sizeof(elevationType), &dwSize)) {
    *aUserCanElevate = PR_FALSE;
  } 
  else {
    
    
    
    
    
    
    
    
    
    *aUserCanElevate = (elevationType == VistaTokenElevationTypeLimited);
  }

  if (hToken)
    CloseHandle(hToken);

  return NS_OK;
#endif 
}
#endif

#ifdef MOZ_CRASHREPORTER
NS_IMETHODIMP
nsXULAppInfo::GetEnabled(PRBool *aEnabled)
{
  *aEnabled = CrashReporter::GetEnabled();
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetEnabled(PRBool aEnabled)
{
  if (aEnabled) {
    if (CrashReporter::GetEnabled())
      
      return NS_OK;

    nsCOMPtr<nsILocalFile> xreDirectory;
    if (gAppData) {
      xreDirectory = gAppData->xreDirectory;
    }
    else {
      
      nsCOMPtr<nsIFile> greDir;
      NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(greDir));
      if (!greDir)
        return NS_ERROR_FAILURE;

      xreDirectory = do_QueryInterface(greDir);
      if (!xreDirectory)
        return NS_ERROR_FAILURE;
    }
    return CrashReporter::SetExceptionHandler(xreDirectory, true);
  }
  else {
    if (!CrashReporter::GetEnabled())
      
      return NS_OK;

    return CrashReporter::UnsetExceptionHandler();
  }
}

NS_IMETHODIMP
nsXULAppInfo::GetServerURL(nsIURL** aServerURL)
{
  if (!CrashReporter::GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  nsCAutoString data;
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
  PRBool schemeOk;
  
  nsresult rv = aServerURL->SchemeIs("https", &schemeOk);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!schemeOk) {
    rv = aServerURL->SchemeIs("http", &schemeOk);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!schemeOk)
      return NS_ERROR_INVALID_ARG;
  }
  nsCAutoString spec;
  rv = aServerURL->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return CrashReporter::SetServerURL(spec);
}

NS_IMETHODIMP
nsXULAppInfo::GetMinidumpPath(nsILocalFile** aMinidumpPath)
{
  if (!CrashReporter::GetEnabled())
    return NS_ERROR_NOT_INITIALIZED;

  nsAutoString path;
  if (!CrashReporter::GetMinidumpPath(path))
    return NS_ERROR_FAILURE;

  nsresult rv = NS_NewLocalFile(path, PR_FALSE, aMinidumpPath);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsXULAppInfo::SetMinidumpPath(nsILocalFile* aMinidumpPath)
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
nsXULAppInfo::GetSubmitReports(PRBool* aEnabled)
{
  return CrashReporter::GetSubmitReports(aEnabled);
}

NS_IMETHODIMP
nsXULAppInfo::SetSubmitReports(PRBool aEnabled)
{
  return CrashReporter::SetSubmitReports(aEnabled);
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

    WriteConsoleLog();

    NS_ShutdownXPCOM(mServiceManager);
    mServiceManager = nsnull;
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
  { &kAPPINFO_CID, false, NULL, AppInfoConstructor },
  { &kProfileServiceCID, false, ProfileServiceFactoryConstructor, NULL },
  { &kNativeAppSupportCID, false, NULL, ScopedXPCOMStartup::CreateAppSupport },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kXREContracts[] = {
  { XULAPPINFO_SERVICE_CONTRACTID, &kAPPINFO_CID },
  { XULRUNTIME_SERVICE_CONTRACTID, &kAPPINFO_CID },
#ifdef MOZ_CRASHREPORTER
  { NS_CRASHREPORTER_CONTRACTID, &kAPPINFO_CID },
#endif
  { NS_PROFILESERVICE_CONTRACTID, &kProfileServiceCID },
  { NS_NATIVEAPPSUPPORT_CONTRACTID, &kNativeAppSupportCID },
  { NULL }
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

#ifndef MOZ_ENABLE_LIBXUL
#ifndef _BUILD_STATIC_BIN
  XRE_AddStaticComponent(&kXREModule);
#else
  for (const mozilla::Module *const *const *staticModules = kPStaticModules;
       *staticModules; ++staticModules)
      XRE_AddStaticComponent(**staticModules);
#endif
#endif

  rv = NS_InitXPCOM2(&mServiceManager, gDirServiceProvider->GetAppDir(),
                     gDirServiceProvider);
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
  NS_TIME_FUNCTION;
  nsresult rv;

  NS_IF_ADDREF(gNativeAppSupport = native);

  
  nsCOMPtr<nsIToolkitChromeRegistry> cr =
    mozilla::services::GetToolkitChromeRegistryService();
  NS_TIME_FUNCTION_MARK("Got ToolkitChromeRegistry service");

  if (cr)
    cr->CheckForOSAccessibility();

  NS_TIME_FUNCTION_MARK("OS Accessibility check");

  nsCOMPtr<nsIWindowCreator> creator (do_GetService(NS_APPSTARTUP_CONTRACTID));
  if (!creator) return NS_ERROR_UNEXPECTED;

  NS_TIME_FUNCTION_MARK("Got AppStartup service");

  nsCOMPtr<nsIWindowWatcher> wwatch
    (do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_TIME_FUNCTION_MARK("Got WindowWatcher service");

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
         "  --sync             Make X calls synchronous\n"
         "  --no-xshm          Don't use X shared memory extension\n"
         "  --xim-preedit=STYLE\n"
         "  --xim-status=STYLE\n");
#endif
#ifdef XP_UNIX
  printf("  --g-fatal-warnings Make all warnings fatal\n"
         "\n%s options\n", gAppData->name);
#endif

  printf("  -h or -help        Print this message.\n"
         "  -v or -version     Print %s version.\n"
         "  -P <profile>       Start with <profile>.\n"
         "  -migration         Start with migration wizard.\n"
         "  -ProfileManager    Start with ProfileManager.\n"
         "  -no-remote         Open new instance, not a new window in running instance.\n"
         "  -UILocale <locale> Start with <locale> resources as UI Locale.\n"
         "  -safe-mode         Disables extensions and themes for this session.\n", gAppData->name);

#if defined(XP_WIN) || defined(XP_OS2)
  printf("  -console           Start %s with a debugging console.\n", gAppData->name);
#endif

  
  
  
  DumpArbitraryHelp();
}

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
  printf("%s %s %s", 
         gAppData->vendor ? gAppData->vendor : "", gAppData->name, gAppData->version);
  if (gAppData->copyright)
      printf(", %s", gAppData->copyright);
  printf("\n");
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

  ar = CheckArg("p", PR_FALSE, &profile);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -p requires a profile name\n");
    return 1;
  }

  const char *temp = nsnull;
  ar = CheckArg("a", PR_FALSE, &temp);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -a requires an application name\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    program.Assign(temp);
  }

  ar = CheckArg("u", PR_FALSE, &username);
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

static RemoteResult
RemoteCommandLine(const char* aDesktopStartupID)
{
  nsresult rv;
  ArgResult ar;

  nsCAutoString program(gAppData->name);
  ToLowerCase(program);
  const char *username = getenv("LOGNAME");

  const char *temp = nsnull;
  ar = CheckArg("a", PR_TRUE, &temp);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -a requires an application name\n");
    return REMOTE_ARG_BAD;
  } else if (ar == ARG_FOUND) {
    program.Assign(temp);
  }

  ar = CheckArg("u", PR_TRUE, &username);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -u requires a username\n");
    return REMOTE_ARG_BAD;
  }

  XRemoteClient client;
  rv = client.Init();
  if (NS_FAILED(rv))
    return REMOTE_NOT_FOUND;
 
  nsXPIDLCString response;
  PRBool success = PR_FALSE;
  rv = client.SendCommandLine(program.get(), username, nsnull,
                              gArgc, gArgv, aDesktopStartupID,
                              getter_Copies(response), &success);
  
  if (NS_FAILED(rv) || !success)
    return REMOTE_NOT_FOUND;

  return REMOTE_FOUND;
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
  PRUnichar exePath[MAXPATHLEN];

  if (!::GetModuleFileNameW(0, exePath, MAXPATHLEN))
    return NS_ERROR_FAILURE;

  rv = NS_NewLocalFile(nsDependentString(exePath), PR_TRUE,
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

  CFURLRef executableURL = CFBundleCopyExecutableURL(appBundle);
  if (!executableURL)
    return NS_ERROR_FAILURE;
  rv = lfm->InitWithCFURL(executableURL);
  CFRelease(executableURL);
  if (NS_FAILED(rv))
    return rv;

  
  lfm->Normalize();

#elif defined(XP_UNIX)
  struct stat fileStat;
  char exePath[MAXPATHLEN];
  char tmpPath[MAXPATHLEN];

  rv = NS_ERROR_FAILURE;

  
  
  
  
  
  
  
  
  
  




#if 0
  int r = readlink("/proc/self/exe", exePath, MAXPATHLEN);

  if (r > 0 && r < MAXPATHLEN) {
    exePath[r] = '\0';
    if (stat(exePath, &fileStat) == 0) {
      rv = NS_OK;
    }
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

#else
#error Oops, you need platform-specific code here
#endif

  NS_ADDREF(*aResult = lf);
  return NS_OK;
}

#define NS_ERROR_LAUNCHED_CHILD_PROCESS NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_PROFILE, 200)

#ifdef XP_WIN
#include "nsWindowsRestart.cpp"
#include <shellapi.h>

typedef BOOL (WINAPI* SetProcessDEPPolicyFunc)(DWORD dwFlags);
#endif

#if defined(XP_OS2) && (__KLIBC__ == 0 && __KLIBC_MINOR__ >= 6) 



char *createEnv()
{
  
  
  char *env = (char *)calloc(0x6000, sizeof(char));
  if (!env) {
    return NULL;
  }

  
  
  
  char *penv = env; 
  int i = 0, space = 0x6000;
  while (environ[i] && environ[i][0]) {
    int len = strlen(environ[i]);
    if (space - len <= 0) {
      break;
    }
    strcpy(penv, environ[i]);
    i++; 
    penv += len + 1; 
    space -= len - 1; 
  }

  return env;
}






int OS2LaunchChild(const char *aExePath, int aArgc, char **aArgv)
{
  
  int len = 0;
  for (int i = 0; i < aArgc; i++) {
    len += strlen(aArgv[i]) + 1; 
  }
  len++; 
  
  
  char *args = (char *)calloc(len, sizeof(char));
  if (!args) {
    return -1;
  }
  char *pargs = args; 
  
  
  for (int i = 0; i < aArgc; i++, *pargs++ = ' ') {
    strcpy(pargs, aArgv[i]);
    pargs += strlen(aArgv[i]);
  }
  if (aArgc > 1) {
    *(pargs-1) = '\0'; 
  }
  *pargs = '\0';
  
  pargs = strchr(args, ' ');
  if (pargs) {
    *pargs = '\0';
  }

  char *env = createEnv();

  char error[CCHMAXPATH] = { 0 };
  RESULTCODES crc = { 0 };
  ULONG rc = DosExecPgm(error, sizeof(error), EXEC_ASYNC, args, env,
                        &crc, (PSZ)aExePath);
  free(args); 
  if (env) {
    free(env);
  }
  if (rc != NO_ERROR) {
    return -1;
  }

  return 0;
}
#endif




static nsresult LaunchChild(nsINativeAppSupport* aNative,
                            PRBool aBlankCommandLine = PR_FALSE)
{
  aNative->Quit(); 

  
  
 
  if (aBlankCommandLine) {
#if defined(MOZ_WIDGET_QT)
    
    gRestartArgc = gQtOnlyArgc;
    gRestartArgv = gQtOnlyArgv;
#else
    gRestartArgc = 1;
    gRestartArgv[gRestartArgc] = nsnull;
#endif
  }

  SaveToEnv("MOZ_LAUNCHED_CHILD=1");

#if defined(ANDROID)
  mozilla::AndroidBridge::Bridge()->ScheduleRestart();
#else
#if defined(XP_MACOSX)
  CommandLineServiceMac::SetupMacCommandLine(gRestartArgc, gRestartArgv, PR_TRUE);
  PRUint32 restartMode = 0;
#if defined(MOZ_ENABLE_LIBXUL)
  restartMode = gRestartMode;
#endif
  LaunchChildMac(gRestartArgc, gRestartArgv, restartMode);
#else
  nsCOMPtr<nsILocalFile> lf;
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
  nsCAutoString exePath;
  rv = lf->GetNativePath(exePath);
  if (NS_FAILED(rv))
    return rv;

#if defined(XP_OS2) && (__KLIBC__ == 0 && __KLIBC_MINOR__ >= 6)
  
  if (OS2LaunchChild(exePath.get(), gRestartArgc, gRestartArgv) == -1)
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
    PRBool checkState;
    rv = ps->ConfirmEx(nsnull, killTitle, killMessage, flags,
                       killTitle, nsnull, nsnull, nsnull, &checkState, &button);
    NS_ENSURE_SUCCESS_LOG(rv, rv);

    if (button == 1 && aUnlocker) {
      rv = aUnlocker->Unlock(nsIProfileUnlocker::FORCE_QUIT);
      if (NS_FAILED(rv)) return rv;

      return NS_LockProfilePath(aProfileDir, aProfileLocalDir, nsnull, aResult);
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
    const PRUnichar* params[] = {appName.get(), appName.get()};
  
    nsXPIDLString missingMessage;
  
    
    static const PRUnichar kMissing[] = {'p','r','o','f','i','l','e','M','i','s','s','i','n','g','\0'};
    sb->FormatStringFromName(kMissing, params, 2, getter_Copies(missingMessage));
  
    nsXPIDLString missingTitle;
    sb->FormatStringFromName(NS_LITERAL_STRING("profileMissingTitle").get(),
                             params, 1, getter_Copies(missingTitle));
  
    if (missingMessage && missingTitle) {
      nsCOMPtr<nsIPromptService> ps
        (do_GetService(NS_PROMPTSERVICE_CONTRACTID));
      NS_ENSURE_TRUE(ps, NS_ERROR_FAILURE);
  
      ps->Alert(nsnull, missingTitle, missingMessage);
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
  PRUnichar* profileNamePtr;
  nsCAutoString profileName;

  {
    ScopedXPCOMStartup xpcom;
    rv = xpcom.Initialize();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = xpcom.SetWindowCreator(aNative);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

#ifdef XP_MACOSX
    CommandLineServiceMac::SetupMacCommandLine(gRestartArgc, gRestartArgv, PR_TRUE);
#endif

#ifdef XP_WIN
    
    
    ProcessDDE(aNative, PR_FALSE);
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

  PRBool offline = PR_FALSE;
  aProfileSvc->GetStartOffline(&offline);
  if (offline) {
    SaveToEnv("XRE_START_OFFLINE=1");
  }

  return LaunchChild(aNative);
}

static nsresult
ImportProfiles(nsIToolkitProfileService* aPService,
               nsINativeAppSupport* aNative)
{
  nsresult rv;

  SaveToEnv("XRE_IMPORT_PROFILES=1");

  
  { 
    ScopedXPCOMStartup xpcom;
    rv = xpcom.Initialize();
    if (NS_SUCCEEDED(rv)) {
#ifdef XP_MACOSX
      CommandLineServiceMac::SetupMacCommandLine(gRestartArgc, gRestartArgv, PR_TRUE);
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
              PRBool* aStartOffline, nsACString* aProfileName)
{
  nsresult rv;
  ArgResult ar;
  const char* arg;
  *aResult = nsnull;
  *aStartOffline = PR_FALSE;

  ar = CheckArg("offline", PR_TRUE);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -offline is invalid when argument -osint is specified\n");
    return NS_ERROR_FAILURE;
  }

  arg = PR_GetEnv("XRE_START_OFFLINE");
  if ((arg && *arg) || ar)
    *aStartOffline = PR_TRUE;


  nsCOMPtr<nsILocalFile> lf = GetFileFromEnv("XRE_PROFILE_PATH");
  if (lf) {
    nsCOMPtr<nsILocalFile> localDir =
      GetFileFromEnv("XRE_PROFILE_LOCAL_PATH");
    if (!localDir) {
      localDir = lf;
    }

    arg = PR_GetEnv("XRE_PROFILE_NAME");
    if (arg && *arg && aProfileName)
      aProfileName->Assign(nsDependentCString(arg));

    
    const char *dummy;
    CheckArg("p", PR_FALSE, &dummy);
    CheckArg("profile", PR_FALSE, &dummy);
    CheckArg("profilemanager");

    return NS_LockProfilePath(lf, localDir, nsnull, aResult);
  }

  ar = CheckArg("migration", PR_TRUE);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -migration is invalid when argument -osint is specified\n");
    return NS_ERROR_FAILURE;
  } else if (ar == ARG_FOUND) {
    gDoMigration = PR_TRUE;
  }

  ar = CheckArg("profile", PR_TRUE, &arg);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -profile requires a path\n");
    return NS_ERROR_FAILURE;
  }
  if (ar) {
    nsCOMPtr<nsILocalFile> lf;
    rv = XRE_GetFileFromPath(arg, getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIProfileUnlocker> unlocker;

    
    PRBool exists;
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

  nsCOMPtr<nsIToolkitProfileService> profileSvc;
  rv = NS_NewToolkitProfileService(getter_AddRefs(profileSvc));
  if (rv == NS_ERROR_FILE_ACCESS_DENIED)
    PR_fprintf(PR_STDERR, "Error: Access was denied while trying to open files in " \
                "your profile directory.\n"); 
  NS_ENSURE_SUCCESS(rv, rv);

  ar = CheckArg("createprofile", PR_TRUE, &arg);
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

  ar = CheckArg("p", PR_FALSE, &arg);
  if (ar == ARG_BAD) {
    ar = CheckArg("osint");
    if (ar == ARG_FOUND) {
      PR_fprintf(PR_STDERR, "Error: argument -p is invalid when argument -osint is specified\n");
      return NS_ERROR_FAILURE;
    }
    return ShowProfileManager(profileSvc, aNative);
  }
  if (ar) {
    ar = CheckArg("osint");
    if (ar == ARG_FOUND) {
      PR_fprintf(PR_STDERR, "Error: argument -p is invalid when argument -osint is specified\n");
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIToolkitProfile> profile;
    rv = profileSvc->GetProfileByName(nsDependentCString(arg),
                                      getter_AddRefs(profile));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIProfileUnlocker> unlocker;
      rv = profile->Lock(nsnull, aResult);
      if (NS_SUCCEEDED(rv)) {
        if (aProfileName)
          aProfileName->Assign(nsDependentCString(arg));
        return NS_OK;
      }

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

  ar = CheckArg("profilemanager", PR_TRUE);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -profilemanager is invalid when argument -osint is specified\n");
    return NS_ERROR_FAILURE;
  } else if (ar == ARG_FOUND) {
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
      if (NS_SUCCEEDED(rv)) {
        if (aProfileName)
          aProfileName->Assign(NS_LITERAL_CSTRING("default"));
        return NS_OK;
      }
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
      if (NS_SUCCEEDED(rv)) {
        
        if (aProfileName) {
          rv = profile->GetName(*aProfileName);
          if (NS_FAILED(rv))
            aProfileName->Truncate(0);
        }
        return NS_OK;
      }

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








static PRBool
CheckCompatibility(nsIFile* aProfileDir, const nsCString& aVersion,
                   const nsCString& aOSABI, nsIFile* aXULRunnerDir,
                   nsIFile* aAppDir, nsILocalFile* aFlagFile, 
                   PRBool* aCachesOK)
{
  *aCachesOK = PR_FALSE;
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

  
  rv = parser.GetString("Compatibility", "InvalidateCaches", buf);
  *aCachesOK = (NS_FAILED(rv) || !buf.EqualsLiteral("1"));
  
  PRBool purgeCaches = PR_FALSE;
  if (aFlagFile) {
    aFlagFile->Exists(&purgeCaches);
  }

  *aCachesOK = !purgeCaches && *aCachesOK;
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

static void RemoveComponentRegistries(nsIFile* aProfileDir, nsIFile* aLocalProfileDir,
                                      PRBool aRemoveEMFiles)
{
  nsCOMPtr<nsIFile> file;
  aProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return;

  if (aRemoveEMFiles) {
    file->SetNativeLeafName(NS_LITERAL_CSTRING("extensions.ini"));
    file->Remove(PR_FALSE);
  }

  aLocalProfileDir->Clone(getter_AddRefs(file));
  if (!file)
    return;

  file->AppendNative(NS_LITERAL_CSTRING("XUL" PLATFORM_FASL_SUFFIX));
  file->Remove(PR_FALSE);
  
  file->SetNativeLeafName(NS_LITERAL_CSTRING("XPC" PLATFORM_FASL_SUFFIX));
  file->Remove(PR_FALSE);

  file->SetNativeLeafName(NS_LITERAL_CSTRING("startupCache"));
  file->Remove(PR_TRUE);
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

#ifdef MOZ_CRASHREPORTER




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
  return static_cast<nsGTKToolkit*>(toolkit);
}

static void MOZ_gdk_display_close(GdkDisplay *display)
{
  
  
  PRBool theme_is_qt = PR_FALSE;
  GtkSettings* settings =
    gtk_settings_get_for_screen(gdk_display_get_default_screen(display));
  gchar *theme_name;
  g_object_get(settings, "gtk-theme-name", &theme_name, NULL);
  if (theme_name) {
    theme_is_qt = strcmp(theme_name, "Qt") == 0;
    if (theme_is_qt)
      NS_WARNING("wallpaper bug 417163 for Qt theme");
    g_free(theme_name);
  }

  
  
  
  
  if (gtk_check_version(2,10,0) != NULL) {
#ifdef MOZ_X11
    
    
    
    
    Display* dpy = GDK_DISPLAY_XDISPLAY(display);
    if (!theme_is_qt)
      XCloseDisplay(dpy);
#else
    gdk_display_close(display);
#endif 
  }
  else {
#if CLEANUP_MEMORY
    
    
    
    PangoContext *pangoContext = gdk_pango_context_get();
#endif

    PRBool buggyCairoShutdown = cairo_version() < CAIRO_VERSION_ENCODE(1, 4, 0);

    if (!buggyCairoShutdown) {
      
      
      
      
      if (!theme_is_qt)
        gdk_display_close(display);
    }

#if CLEANUP_MEMORY
    
    PangoFontMap *fontmap = pango_context_get_font_map(pangoContext);
    
    
    
    
    if (PANGO_IS_FC_FONT_MAP(fontmap))
        pango_fc_font_map_shutdown(PANGO_FC_FONT_MAP(fontmap));
    g_object_unref(pangoContext);
    
    
    
    
    
    

#if GTK_CHECK_VERSION(2,8,0)
    
    
#ifdef cairo_debug_reset_static_data
#error "Looks like we're including Mozilla's cairo instead of system cairo"
#endif
    cairo_debug_reset_static_data();
#endif 
#endif 

    if (buggyCairoShutdown) {
      if (!theme_is_qt)
        gdk_display_close(display);
    }
  }
}
#endif 













NS_VISIBILITY_DEFAULT PRBool nspr_use_zone_allocator = PR_FALSE;

#ifdef MOZ_SPLASHSCREEN
#define MOZ_SPLASHSCREEN_UPDATE(_i)  do { if (splashScreen) splashScreen->Update(_i); } while(0)
#else
#define MOZ_SPLASHSCREEN_UPDATE(_i)  do { } while(0)
#endif

#ifdef CAIRO_HAS_DWRITE_FONT

#include <dwrite.h>

typedef HRESULT (WINAPI*DWriteCreateFactoryFunc)(
  __in   DWRITE_FACTORY_TYPE factoryType,
  __in   REFIID iid,
  __out  IUnknown **factory
);

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

static DWORD InitDwriteBG(LPVOID lpdwThreadParam)
{
  SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
  LOGREGISTRY(L"loading dwrite.dll");
  HMODULE dwdll = LoadLibraryW(L"dwrite.dll");
  if (dwdll) {
    DWriteCreateFactoryFunc createDWriteFactory = (DWriteCreateFactoryFunc)
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

PRTime gXRE_mainTimestamp = 0;

int
XRE_main(int argc, char* argv[], const nsXREAppData* aAppData)
{
  NS_TIME_FUNCTION;

  gXRE_mainTimestamp = PR_Now();

#ifdef MOZ_SPLASHSCREEN
  nsSplashScreen *splashScreen = nsnull;
#endif

  nsresult rv;
  ArgResult ar;
  NS_TIMELINE_MARK("enter main");

#ifdef DEBUG
  if (PR_GetEnv("XRE_MAIN_BREAK"))
    NS_BREAK();
#endif

  SetupErrorHandling(argv[0]);

#ifdef CAIRO_HAS_DWRITE_FONT

  
  
  
  
  
  
  
  
  
      
  OSVERSIONINFO vinfo;
  vinfo.dwOSVersionInfoSize = sizeof(vinfo);
  if (GetVersionEx(&vinfo) && vinfo.dwMajorVersion >= 6) {
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&InitDwriteBG, NULL, 0, NULL);
  }

#endif

#ifdef XP_UNIX
  const char *home = PR_GetEnv("HOME");
  if (!home || !*home) {
    struct passwd *pw = getpwuid(geteuid());
    if (!pw || !pw->pw_dir) {
      Output(PR_TRUE, "Could not determine HOME directory");
      return 1;
    }
    SaveWordToEnv("HOME", nsDependentCString(pw->pw_dir));
  }
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

  
  PR_SetEnv("NO_AT_BRIDGE=1");
#endif

  gArgc = argc;
  gArgv = argv;

  NS_ENSURE_TRUE(aAppData, 2);

#ifdef XP_MACOSX
  
  
  
  gBinaryPath = getenv("XRE_BINARY_PATH");

  if (gBinaryPath && !*gBinaryPath)
    gBinaryPath = nsnull;
#endif

  
  const char* override = nsnull;
  ar = CheckArg("override", PR_TRUE, &override);
  if (ar == ARG_BAD) {
    Output(PR_TRUE, "Incorrect number of arguments passed to -override");
    return 1;
  }
  else if (ar == ARG_FOUND) {
    nsCOMPtr<nsILocalFile> overrideLF;
    rv = XRE_GetFileFromPath(override, getter_AddRefs(overrideLF));
    if (NS_FAILED(rv)) {
      Output(PR_TRUE, "Error: unrecognized override.ini path.\n");
      return 1;
    }

    nsXREAppData* overrideAppData = const_cast<nsXREAppData*>(aAppData);
    rv = XRE_ParseAppData(overrideLF, overrideAppData);
    if (NS_FAILED(rv)) {
      Output(PR_TRUE, "Couldn't read override.ini");
      return 1;
    }
  }

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

#ifdef MOZ_SPLASHSCREEN
  
  PRBool wantsSplash = PR_TRUE;
  PRBool isNoSplash = (CheckArg("nosplash", PR_FALSE, NULL, PR_FALSE) == ARG_FOUND);
  isNoSplash |= (PR_GetEnv("NO_SPLASH") != 0);
  PRBool isNoRemote = (CheckArg("no-remote", PR_FALSE, NULL, PR_FALSE) == ARG_FOUND);

#ifdef WINCE
  
  
  WindowsMutex winStartupMutex(L"FirefoxStartupMutex");

  
  PRBool needsMutexLock = ! winStartupMutex.Lock(100);

  
  
  
  
  
  
  if (!needsMutexLock && !isNoRemote) {
    
    static PRUnichar classNameBuffer[128];
    _snwprintf(classNameBuffer, sizeof(classNameBuffer) / sizeof(PRUnichar),
               L"%S%s",
               gAppData->name, L"MessageWindow");
    HANDLE h = FindWindowW(classNameBuffer, 0);
    if (h) {
      
      
      
      wantsSplash = PR_FALSE;
      CloseHandle(h);
    } else {
      
      
      
      wantsSplash = PR_TRUE;
    }
  }
#endif 

  if (wantsSplash && !isNoSplash)
    splashScreen = nsSplashScreen::GetOrCreate();

  if (splashScreen)
    splashScreen->Open();

#ifdef WINCE
  
  
  if (needsMutexLock)
    winStartupMutex.Lock();
#endif 

#endif 


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

  nsXREDirProvider dirProvider;
  rv = dirProvider.Initialize(gAppData->directory, gAppData->xreDirectory);
  if (NS_FAILED(rv))
    return 1;

#ifdef MOZ_CRASHREPORTER
  const char* crashreporterEnv = PR_GetEnv("MOZ_CRASHREPORTER");
  if (crashreporterEnv && *crashreporterEnv) {
    appData.flags |= NS_XRE_ENABLE_CRASH_REPORTER;
  }

  if ((appData.flags & NS_XRE_ENABLE_CRASH_REPORTER) &&
      NS_SUCCEEDED(
         CrashReporter::SetExceptionHandler(appData.xreDirectory))) {
    if (appData.crashReporterURL)
      CrashReporter::SetServerURL(nsDependentCString(appData.crashReporterURL));

    
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

    
    nsCOMPtr<nsILocalFile> userAppDataDir;
    if (NS_SUCCEEDED(dirProvider.GetUserAppDataDirectory(
                                                         getter_AddRefs(userAppDataDir)))) {
      CrashReporter::SetupExtraData(userAppDataDir,
                                    nsDependentCString(appData.buildID));

      
      nsCOMPtr<nsIFile> overrideini;
      PRBool exists;
      static char overrideEnv[MAXPATHLEN];
      if (NS_SUCCEEDED(dirProvider.GetAppDir()->Clone(getter_AddRefs(overrideini))) &&
          NS_SUCCEEDED(overrideini->AppendNative(NS_LITERAL_CSTRING("crashreporter-override.ini"))) &&
          NS_SUCCEEDED(overrideini->Exists(&exists)) &&
          exists) {
#ifdef XP_WIN
        nsAutoString overridePathW;
        overrideini->GetPath(overridePathW);
        NS_ConvertUTF16toUTF8 overridePath(overridePathW);
#else
        nsCAutoString overridePath;
        overrideini->GetNativePath(overridePath);
#endif

        sprintf(overrideEnv, "MOZ_CRASHREPORTER_STRINGS_OVERRIDE=%s",
                overridePath.get());
        PR_SetEnv(overrideEnv);
      }
    }
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

  SaveToEnv("MOZ_LAUNCHED_CHILD=");

  gRestartArgc = gArgc;
  gRestartArgv = (char**) malloc(sizeof(char*) * (gArgc + 1 + (override ? 2 : 0)));
  if (!gRestartArgv) return 1;

  int i;
  for (i = 0; i < gArgc; ++i) {
    gRestartArgv[i] = gArgv[i];
  }
  
  
  if (override) {
    gRestartArgv[gRestartArgc++] = const_cast<char*>("-override");
    gRestartArgv[gRestartArgc++] = const_cast<char*>(override);
  }

  gRestartArgv[gRestartArgc] = nsnull;
  

#if defined(XP_OS2)
  PRBool StartOS2App(int aArgc, char **aArgv);
  if (!StartOS2App(gArgc, gArgv))
    return 1;
  ScopedFPHandler handler;
#endif 

  if (PR_GetEnv("MOZ_SAFE_MODE_RESTART")) {
    gSafeMode = PR_TRUE;
    
    PR_SetEnv("MOZ_SAFE_MODE_RESTART=");
  }

  ar = CheckArg("safe-mode", PR_TRUE);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -safe-mode is invalid when argument -osint is specified\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    gSafeMode = PR_TRUE;
  }

#ifdef XP_WIN
  
  
  
  
  
  if (GetKeyState(VK_SHIFT) & 0x8000 &&
      !(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
    gSafeMode = PR_TRUE;
  }
#endif

#ifdef XP_MACOSX
  if (GetCurrentEventKeyModifiers() & optionKey)
    gSafeMode = PR_TRUE;
#endif

  
  
  ar = CheckArg("no-remote", PR_TRUE);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -a requires an application name\n");
    return 1;
  } else if (ar == ARG_FOUND) {
    SaveToEnv("MOZ_NO_REMOTE=1");
  }

  
  
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

  MOZ_SPLASHSCREEN_UPDATE(20);

  rv = XRE_InitCommandLine(gArgc, gArgv);
  NS_ENSURE_SUCCESS(rv, 1);

  {
    
    ar = CheckArg("register", PR_TRUE);
    if (ar == ARG_BAD) {
      PR_fprintf(PR_STDERR, "Error: argument -register is invalid when argument -osint is specified\n");
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

#if defined(MOZ_WIDGET_QT)
    const char* qgraphicssystemARG = NULL;
    ar = CheckArg("graphicssystem", PR_TRUE, &qgraphicssystemARG, PR_FALSE);
    if (ar == ARG_FOUND)
      PR_SetEnv(PR_smprintf("MOZ_QT_GRAPHICSSYSTEM=%s", qgraphicssystemARG));

    QScopedPointer<QApplication> app(new QApplication(gArgc, gArgv));

#if MOZ_PLATFORM_MAEMO > 5
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
      
      QInputContext* inputContext = app->inputContext();
      if (inputContext && inputContext->identifierName() != "MInputContext") {
          QInputContext* context = QInputContextFactory::create("MInputContext",
                                                                app.data());
          if (context)
              app->setInputContext(context);
      }
    }
#endif
    QStringList nonQtArguments = app->arguments();
    gQtOnlyArgc = 1;
    gQtOnlyArgv = (char**) malloc(sizeof(char*) 
                  * (gRestartArgc - nonQtArguments.size() + 2));

    
    gQtOnlyArgv[0] = gRestartArgv[0];

    for (int i = 1; i < gRestartArgc; ++i) {
      if (!nonQtArguments.contains(gRestartArgv[i])) {
        
        gQtOnlyArgv[gQtOnlyArgc++] = gRestartArgv[i];
      }
    }
    gQtOnlyArgv[gQtOnlyArgc] = nsnull;
#endif
#if defined(MOZ_WIDGET_GTK2)
#ifdef MOZ_MEMORY
    
    
    
    g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, 1);
#endif
    g_thread_init(NULL);
    
    
    
    
    if (CheckArg("install"))
      gdk_rgb_set_install(TRUE);

    

    
    
    
    if (!gtk_parse_args(&gArgc, &gArgv))
      return 1;

    
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

#ifdef MOZ_ENABLE_XREMOTE
    

    const char* xremotearg;
    ar = CheckArg("remote", PR_TRUE, &xremotearg);
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
      
      RemoteResult rr = RemoteCommandLine(desktopStartupIDPtr);
      if (rr == REMOTE_FOUND)
        return 0;
      else if (rr == REMOTE_ARG_BAD)
        return 1;
    }
#endif

#if defined(MOZ_WIDGET_GTK2)
    GdkDisplay* display = nsnull;
    display = gdk_display_open(display_name);
    if (!display) {
      PR_fprintf(PR_STDERR, "Error: cannot open display: %s\n", display_name);
      return 1;
    }
    gdk_display_manager_set_default_display (gdk_display_manager_get(),
                                             display);
    
    
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

    gtk_widget_set_default_colormap(gdk_rgb_get_colormap());
#endif 
#ifdef MOZ_X11
    
    InstallX11ErrorHandler();
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

#if defined(HAVE_DESKTOP_STARTUP_ID) && defined(MOZ_WIDGET_GTK2)
    
    
    if (!desktopStartupID.IsEmpty()) {
      nsCAutoString desktopStartupEnv;
      desktopStartupEnv.AssignLiteral("DESKTOP_STARTUP_ID=");
      desktopStartupEnv.Append(desktopStartupID);
      
      PR_SetEnv(ToNewCString(desktopStartupEnv));
    }
#endif

#if defined(MOZ_UPDATER) && !defined(ANDROID)
    
    nsCOMPtr<nsIFile> updRoot;
    PRBool persistent;
    rv = dirProvider.GetFile(XRE_UPDATE_ROOT_DIR, &persistent,
                             getter_AddRefs(updRoot));
    
    if (NS_FAILED(rv))
      updRoot = dirProvider.GetAppDir();

    
    
    
    
    
    if (CheckArg("process-updates")) {
      SaveToEnv("MOZ_PROCESS_UPDATES=1");
    }
    ProcessUpdates(dirProvider.GetGREDir(),
                   dirProvider.GetAppDir(),
                   updRoot,
                   gRestartArgc,
                   gRestartArgv,
                   appData.version);
    if (PR_GetEnv("MOZ_PROCESS_UPDATES")) {
      PR_SetEnv("MOZ_PROCESS_UPDATES=");
      return 0;
    }
#endif

    nsCOMPtr<nsIProfileLock> profileLock;
    PRBool startOffline = PR_FALSE;
    nsCAutoString profileName;

    rv = SelectProfile(getter_AddRefs(profileLock), nativeApp, &startOffline,
                       &profileName);
    if (rv == NS_ERROR_LAUNCHED_CHILD_PROCESS ||
        rv == NS_ERROR_ABORT) return 0;

    if (NS_FAILED(rv)) {
      
      ProfileMissingDialog(nativeApp);
      return 1;
    }

    nsCOMPtr<nsILocalFile> profD;
    rv = profileLock->GetDirectory(getter_AddRefs(profD));
    NS_ENSURE_SUCCESS(rv, 1);

    nsCOMPtr<nsILocalFile> profLD;
    rv = profileLock->GetLocalDirectory(getter_AddRefs(profLD));
    NS_ENSURE_SUCCESS(rv, 1);

    rv = dirProvider.SetProfile(profD, profLD);
    NS_ENSURE_SUCCESS(rv, 1);

#if defined(WINCE) && defined(MOZ_SPLASHSCREEN)
    
    winStartupMutex.Unlock();
#endif

    

#ifdef MOZ_CRASHREPORTER
    if (appData.flags & NS_XRE_ENABLE_CRASH_REPORTER)
        MakeOrSetMinidumpPath(profD);
#endif

    nsCAutoString version;
    BuildVersion(version);

#ifdef TARGET_OS_ABI
    NS_NAMED_LITERAL_CSTRING(osABI, TARGET_OS_ABI);
#else
    
    NS_NAMED_LITERAL_CSTRING(osABI, OS_TARGET "_UNKNOWN");
#endif

    
    
    
    
    
 
    
    
    
    nsCOMPtr<nsILocalFile> flagFile;

    rv = NS_ERROR_FILE_NOT_FOUND;
    nsCOMPtr<nsIFile> fFlagFile;
    if (gAppData->directory) {
      rv = gAppData->directory->Clone(getter_AddRefs(fFlagFile));
    }
    flagFile = do_QueryInterface(fFlagFile);
    if (flagFile) {
      flagFile->AppendNative(FILE_INVALIDATE_CACHES);
    }

    PRBool cachesOK;
    PRBool versionOK = CheckCompatibility(profD, version, osABI, 
                                          dirProvider.GetGREDir(),
                                          gAppData->directory, flagFile,
                                          &cachesOK);
    if (CheckArg("purgecaches")) {
      cachesOK = PR_FALSE;
    }
    if (PR_GetEnv("MOZ_PURGE_CACHES")) {
      cachesOK = PR_FALSE;
    }
 
    
    
    
    
    
    
    
    if (gSafeMode) {
      RemoveComponentRegistries(profD, profLD, PR_FALSE);
      WriteVersion(profD, NS_LITERAL_CSTRING("Safe Mode"), osABI,
                   dirProvider.GetGREDir(), gAppData->directory);
    }
    else if (versionOK) {
      if (!cachesOK) {
        
        
        
        RemoveComponentRegistries(profD, profLD, PR_FALSE);
        
        
        WriteVersion(profD, version, osABI,
                     dirProvider.GetGREDir(), gAppData->directory);
      }
      
    }
    else {
      
      
      
      RemoveComponentRegistries(profD, profLD, PR_TRUE);

      
      WriteVersion(profD, version, osABI,
                   dirProvider.GetGREDir(), gAppData->directory);
    }

    if (flagFile) {
      flagFile->Remove(PR_TRUE);
    }

    PRBool appInitiatedRestart = PR_FALSE;

    MOZ_SPLASHSCREEN_UPDATE(30);

    NS_TIME_FUNCTION_MARK("Next: ScopedXPCOMStartup");

    NS_TIME_FUNCTION_MARK("ScopedXPCOMStartup");

    
    
    
    {
      
      ScopedXPCOMStartup xpcom;
      rv = xpcom.Initialize();
      NS_TIME_FUNCTION_MARK("ScopedXPCOMStartup: Initialize");
      NS_ENSURE_SUCCESS(rv, 1); 


#ifdef NS_FUNCTION_TIMER
      
      
      {
        nsCOMPtr<nsISupports> comp;

        comp = do_GetService("@mozilla.org/preferences-service;1");
        NS_TIME_FUNCTION_MARK("Pref Service");

        comp = do_GetService("@mozilla.org/network/socket-transport-service;1");
        NS_TIME_FUNCTION_MARK("Socket Transport Service");

        comp = do_GetService("@mozilla.org/network/dns-service;1");
        NS_TIME_FUNCTION_MARK("DNS Service");

        comp = do_GetService("@mozilla.org/network/io-service;1");
        NS_TIME_FUNCTION_MARK("IO Service");

        comp = do_GetService("@mozilla.org/chrome/chrome-registry;1");
        NS_TIME_FUNCTION_MARK("Chrome Registry Service");

        comp = do_GetService("@mozilla.org/focus-event-suppressor-service;1");
        NS_TIME_FUNCTION_MARK("Focus Event Suppressor Service");
      }
#endif

      rv = xpcom.SetWindowCreator(nativeApp);
      NS_TIME_FUNCTION_MARK("ScopedXPCOMStartup: SetWindowCreator");
      NS_ENSURE_SUCCESS(rv, 1);

      NS_TIME_FUNCTION_MARK("ScopedXPCOMStartup: Done");

#ifdef MOZ_CRASHREPORTER
      
      nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIPrefBranch> defaultPrefBranch;
        rv = prefs->GetDefaultBranch(nsnull, getter_AddRefs(defaultPrefBranch));

        if (NS_SUCCEEDED(rv)) {
          nsXPIDLCString sval;
          rv = defaultPrefBranch->GetCharPref("app.update.channel", getter_Copies(sval));
          if (NS_SUCCEEDED(rv)) {
            CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ReleaseChannel"),
                                               sval);
          }
        }
      }
#endif

      NS_TIME_FUNCTION_MARK("Next: AppStartup");

      {
        if (startOffline) {
          nsCOMPtr<nsIIOService2> io (do_GetService("@mozilla.org/network/io-service;1"));
          NS_ENSURE_TRUE(io, 1);
          io->SetManageOfflineStatus(PR_FALSE);
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

        NS_TIME_FUNCTION_MARK("Finished startupNotifier");

        nsCOMPtr<nsIAppStartup2> appStartup
          (do_GetService(NS_APPSTARTUP_CONTRACTID));
        NS_ENSURE_TRUE(appStartup, 1);

        NS_TIME_FUNCTION_MARK("Created AppStartup");

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

        NS_TIME_FUNCTION_MARK("Profile migration");

        dirProvider.DoStartup();

        NS_TIME_FUNCTION_MARK("dirProvider.DoStartup() (profile-after-change)");

        PRBool shuttingDown = PR_FALSE;
        appStartup->GetShuttingDown(&shuttingDown);

        nsCOMPtr<nsICommandLineRunner> cmdLine;

        nsCOMPtr<nsIFile> workingDir;
        rv = NS_GetSpecialDirectory(NS_OS_CURRENT_WORKING_DIR, getter_AddRefs(workingDir));
        NS_ENSURE_SUCCESS(rv, 1);

        if (!shuttingDown) {
          cmdLine = do_CreateInstance("@mozilla.org/toolkit/command-line;1");
          NS_ENSURE_TRUE(cmdLine, 1);

          rv = cmdLine->Init(gArgc, gArgv,
                             workingDir, nsICommandLine::STATE_INITIAL_LAUNCH);
          NS_ENSURE_SUCCESS(rv, 1);

          

          nsCOMPtr<nsIObserverService> obsService =
            mozilla::services::GetObserverService();
          if (obsService) {
            obsService->NotifyObservers(cmdLine, "command-line-startup", nsnull);
          }

          NS_TIME_FUNCTION_MARK("Early command line init");

          NS_TIME_FUNCTION_MARK("Next: prepare for Run");
        }

        SaveStateForAppInitiatedRestart();

        
        
        SaveToEnv("XRE_PROFILE_PATH=");
        SaveToEnv("XRE_PROFILE_LOCAL_PATH=");
        SaveToEnv("XRE_PROFILE_NAME=");
        SaveToEnv("XRE_START_OFFLINE=");
        SaveToEnv("XRE_IMPORT_PROFILES=");
        SaveToEnv("NO_EM_RESTART=");
        SaveToEnv("XUL_APP_FILE=");
        SaveToEnv("XRE_BINARY_PATH=");

        NS_TIME_FUNCTION_MARK("env munging");

        if (!shuttingDown) {
          NS_TIME_FUNCTION_MARK("Next: CreateHiddenWindow");

          NS_TIMELINE_ENTER("appStartup->CreateHiddenWindow");
          rv = appStartup->CreateHiddenWindow();
          NS_TIMELINE_LEAVE("appStartup->CreateHiddenWindow");
          NS_ENSURE_SUCCESS(rv, 1);

          MOZ_SPLASHSCREEN_UPDATE(50);

#if defined(HAVE_DESKTOP_STARTUP_ID) && defined(MOZ_WIDGET_GTK2)
          nsRefPtr<nsGTKToolkit> toolkit = GetGTKToolkit();
          if (toolkit && !desktopStartupID.IsEmpty()) {
            toolkit->SetDesktopStartupID(desktopStartupID);
          }
          
          
          g_unsetenv ("DESKTOP_STARTUP_ID");
#endif

#ifdef XP_MACOSX
          
          
          SetupMacApplicationDelegate();

          
          
          cmdLine = do_CreateInstance("@mozilla.org/toolkit/command-line;1");
          NS_ENSURE_TRUE(cmdLine, 1);

          CommandLineServiceMac::SetupMacCommandLine(gArgc, gArgv, PR_FALSE);

          rv = cmdLine->Init(gArgc, gArgv,
                             workingDir, nsICommandLine::STATE_INITIAL_LAUNCH);
          NS_ENSURE_SUCCESS(rv, 1);
#endif

          MOZ_SPLASHSCREEN_UPDATE(70);

          nsCOMPtr<nsIObserverService> obsService =
            mozilla::services::GetObserverService();
          if (obsService)
            obsService->NotifyObservers(nsnull, "final-ui-startup", nsnull);

          NS_TIME_FUNCTION_MARK("final-ui-startup done");

          appStartup->GetShuttingDown(&shuttingDown);
        }

        if (!shuttingDown) {
          rv = cmdLine->Run();
          NS_ENSURE_SUCCESS_LOG(rv, 1);

          appStartup->GetShuttingDown(&shuttingDown);
        }

#ifdef MOZ_ENABLE_XREMOTE
        nsCOMPtr<nsIRemoteService> remoteService;
#endif 
        if (!shuttingDown) {
#ifdef MOZ_ENABLE_XREMOTE
          
          
          remoteService = do_GetService("@mozilla.org/toolkit/remote-service;1");
          if (remoteService)
            remoteService->Startup(gAppData->name,
                                   PromiseFlatCString(profileName).get());
#endif 

          nativeApp->Enable();
        }

        NS_TIME_FUNCTION_MARK("Next: Run");

        NS_TIME_FUNCTION_MARK("appStartup->Run");

        MOZ_SPLASHSCREEN_UPDATE(90);
        {
          NS_TIMELINE_ENTER("appStartup->Run");
          rv = appStartup->Run();
          NS_TIMELINE_LEAVE("appStartup->Run");
          if (NS_FAILED(rv)) {
            NS_ERROR("failed to run appstartup");
            gLogConsoleErrors = PR_TRUE;
          }
        }

        NS_TIME_FUNCTION_MARK("Next: Finish");

        NS_TIME_FUNCTION_MARK("appStartup->Run done");

        
        
        if (rv == NS_SUCCESS_RESTART_APP)
          appInitiatedRestart = PR_TRUE;

        if (!shuttingDown) {
#ifdef MOZ_ENABLE_XREMOTE
          
          if (remoteService)
            remoteService->Shutdown();
#endif 
        }

#ifdef MOZ_TIMELINE
        
        if (NS_FAILED(NS_TIMELINE_LEAVE("main1")))
          NS_TimelineForceMark("...main1");
#endif
      }
    }

    
    
    profileLock->Unlock();

    
    if (appInitiatedRestart) {
      MOZ_SPLASHSCREEN_UPDATE(90);

      RestoreStateForAppInitiatedRestart();

      
      SaveFileToEnvIfUnset("XRE_PROFILE_PATH", profD);
      SaveFileToEnvIfUnset("XRE_PROFILE_LOCAL_PATH", profLD);
      SaveWordToEnvIfUnset("XRE_PROFILE_NAME", profileName);

#ifdef XP_MACOSX
      if (gBinaryPath) {
        static char kEnvVar[MAXPATHLEN];
        sprintf(kEnvVar, "XRE_BINARY_PATH=%s", gBinaryPath);
        PR_SetEnv(kEnvVar);
      }
#endif

#ifdef MOZ_WIDGET_GTK2
      MOZ_gdk_display_close(display);
#endif

      rv = LaunchChild(nativeApp, PR_TRUE);

#ifdef MOZ_CRASHREPORTER
      if (appData.flags & NS_XRE_ENABLE_CRASH_REPORTER)
        CrashReporter::UnsetExceptionHandler();
#endif

      return rv == NS_ERROR_LAUNCHED_CHILD_PROCESS ? 0 : 1;
    }

#ifdef MOZ_WIDGET_GTK2
    
    
    MOZ_gdk_display_close(display);
#endif
  }

#ifdef MOZ_CRASHREPORTER
  if (appData.flags & NS_XRE_ENABLE_CRASH_REPORTER)
      CrashReporter::UnsetExceptionHandler();
#endif

  XRE_DeinitCommandLine();

  return NS_FAILED(rv) ? 1 : 0;
}

nsresult
XRE_InitCommandLine(int aArgc, char* aArgv[])
{
  nsresult rv = NS_OK;

#if defined(MOZ_IPC)

#if defined(OS_WIN)
  CommandLine::Init(aArgc, aArgv);
#else

  
  char** canonArgs = new char*[aArgc];

  
  nsCOMPtr<nsILocalFile> binFile;
  rv = XRE_GetBinaryPath(aArgv[0], getter_AddRefs(binFile));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsCAutoString canonBinPath;
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
#endif

#ifdef MOZ_OMNIJAR
  const char *omnijarPath = nsnull;
  ArgResult ar = CheckArg("omnijar", PR_FALSE, &omnijarPath);
  if (ar == ARG_BAD) {
    PR_fprintf(PR_STDERR, "Error: argument -omnijar requires an omnijar path\n");
    return NS_ERROR_FAILURE;
  }

  if (!omnijarPath)
    return rv;

  nsCOMPtr<nsILocalFile> omnijar;
  rv = NS_NewNativeLocalFile(nsDependentCString(omnijarPath), PR_TRUE,
                             getter_AddRefs(omnijar));
  if (NS_SUCCEEDED(rv))
    mozilla::SetOmnijar(omnijar);
#endif

  return rv;
}

nsresult
XRE_DeinitCommandLine()
{
  nsresult rv = NS_OK;

#if defined(MOZ_IPC)
  CommandLine::Terminate();
#endif

  return rv;
}

GeckoProcessType
XRE_GetProcessType()
{
#ifdef MOZ_IPC
  return mozilla::startup::sChildProcessType;
#else
  return GeckoProcessType_Default;
#endif
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

#if defined (XP_WIN32) && !defined (WINCE)
  
  
  
  UINT realMode = SetErrorMode(0);
  realMode |= SEM_FAILCRITICALERRORS;
  
  
  
  
  if (getenv("XRE_NO_WINDOWS_CRASH_DIALOG"))
    realMode |= SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX;

  SetErrorMode(realMode);

#endif

#ifndef XP_OS2
  InstallSignalHandlers(progname);
#endif

#ifndef WINCE
  
  setbuf(stdout, 0);
#endif

#if defined(FREEBSD)
  
  
  
  fpsetmask(0);
#endif
}
