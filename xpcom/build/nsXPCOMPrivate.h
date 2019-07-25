





































#ifndef nsXPComPrivate_h__
#define nsXPComPrivate_h__

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsXPCOMStrings.h"
#include "xptcall.h"

class nsStringContainer;
class nsCStringContainer;
class nsIComponentLoader;





#define NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID "xpcom-shutdown-threads"





#define NS_XPCOM_SHUTDOWN_LOADERS_OBSERVER_ID "xpcom-shutdown-loaders"


typedef nsresult   (* InitFunc)(nsIServiceManager* *result, nsIFile* binDirectory, nsIDirectoryServiceProvider* appFileLocationProvider);
typedef nsresult   (* ShutdownFunc)(nsIServiceManager* servMgr);
typedef nsresult   (* GetServiceManagerFunc)(nsIServiceManager* *result);
typedef nsresult   (* GetComponentManagerFunc)(nsIComponentManager* *result);
typedef nsresult   (* GetComponentRegistrarFunc)(nsIComponentRegistrar* *result);
typedef nsresult   (* GetMemoryManagerFunc)(nsIMemory* *result);
typedef nsresult   (* NewLocalFileFunc)(const nsAString &path, PRBool followLinks, nsILocalFile* *result);
typedef nsresult   (* NewNativeLocalFileFunc)(const nsACString &path, PRBool followLinks, nsILocalFile* *result);

typedef nsresult   (* GetDebugFunc)(nsIDebug* *result);
typedef nsresult   (* GetTraceRefcntFunc)(nsITraceRefcnt* *result);

typedef nsresult   (* StringContainerInitFunc)(nsStringContainer&);
typedef nsresult   (* StringContainerInit2Func)(nsStringContainer&, const PRUnichar *, PRUint32, PRUint32);
typedef void       (* StringContainerFinishFunc)(nsStringContainer&);
typedef PRUint32   (* StringGetDataFunc)(const nsAString&, const PRUnichar**, PRBool*);
typedef PRUint32   (* StringGetMutableDataFunc)(nsAString&, PRUint32, PRUnichar**);
typedef PRUnichar* (* StringCloneDataFunc)(const nsAString&);
typedef nsresult   (* StringSetDataFunc)(nsAString&, const PRUnichar*, PRUint32);
typedef nsresult   (* StringSetDataRangeFunc)(nsAString&, PRUint32, PRUint32, const PRUnichar*, PRUint32);
typedef nsresult   (* StringCopyFunc)(nsAString &, const nsAString &);
typedef void       (* StringSetIsVoidFunc)(nsAString &, const PRBool);
typedef PRBool     (* StringGetIsVoidFunc)(const nsAString &);

typedef nsresult   (* CStringContainerInitFunc)(nsCStringContainer&);
typedef nsresult   (* CStringContainerInit2Func)(nsCStringContainer&, const char *, PRUint32, PRUint32);
typedef void       (* CStringContainerFinishFunc)(nsCStringContainer&);
typedef PRUint32   (* CStringGetDataFunc)(const nsACString&, const char**, PRBool*);
typedef PRUint32   (* CStringGetMutableDataFunc)(nsACString&, PRUint32, char**);
typedef char*      (* CStringCloneDataFunc)(const nsACString&);
typedef nsresult   (* CStringSetDataFunc)(nsACString&, const char*, PRUint32);
typedef nsresult   (* CStringSetDataRangeFunc)(nsACString&, PRUint32, PRUint32, const char*, PRUint32);
typedef nsresult   (* CStringCopyFunc)(nsACString &, const nsACString &);
typedef void       (* CStringSetIsVoidFunc)(nsACString &, const PRBool);
typedef PRBool     (* CStringGetIsVoidFunc)(const nsACString &);

typedef nsresult   (* CStringToUTF16)(const nsACString &, nsCStringEncoding, nsAString &);
typedef nsresult   (* UTF16ToCString)(const nsAString &, nsCStringEncoding, nsACString &);

typedef void*      (* AllocFunc)(PRSize size);
typedef void*      (* ReallocFunc)(void* ptr, PRSize size);
typedef void       (* FreeFunc)(void* ptr);

typedef void       (* DebugBreakFunc)(PRUint32 aSeverity,
                                      const char *aStr, const char *aExpr,
                                      const char *aFile, PRInt32 aLine);

typedef void       (* xpcomVoidFunc)();
typedef void       (* LogAddRefFunc)(void*, nsrefcnt, const char*, PRUint32);
typedef void       (* LogReleaseFunc)(void*, nsrefcnt, const char*);
typedef void       (* LogCtorFunc)(void*, const char*, PRUint32);
typedef void       (* LogCOMPtrFunc)(void*, nsISupports*);

typedef nsresult   (* GetXPTCallStubFunc)(REFNSIID, nsIXPTCProxy*, nsISomeInterface**);
typedef void       (* DestroyXPTCallStubFunc)(nsISomeInterface*);
typedef nsresult   (* InvokeByIndexFunc)(nsISupports*, PRUint32, PRUint32, nsXPTCVariant*);
typedef PRBool     (* CycleCollectorFunc)(nsISupports*);
typedef nsPurpleBufferEntry*
                   (* CycleCollectorSuspect2Func)(nsISupports*);
typedef PRBool     (* CycleCollectorForget2Func)(nsPurpleBufferEntry*);


typedef NS_CALLBACK(XPCOMExitRoutine)(void);

typedef nsresult   (* RegisterXPCOMExitRoutineFunc)(XPCOMExitRoutine exitRoutine, PRUint32 priority);
typedef nsresult   (* UnregisterXPCOMExitRoutineFunc)(XPCOMExitRoutine exitRoutine);

typedef struct XPCOMFunctions{
    PRUint32 version;
    PRUint32 size;

    InitFunc init;
    ShutdownFunc shutdown;
    GetServiceManagerFunc getServiceManager;
    GetComponentManagerFunc getComponentManager;
    GetComponentRegistrarFunc getComponentRegistrar;
    GetMemoryManagerFunc getMemoryManager;
    NewLocalFileFunc newLocalFile;
    NewNativeLocalFileFunc newNativeLocalFile;

    RegisterXPCOMExitRoutineFunc registerExitRoutine;
    UnregisterXPCOMExitRoutineFunc unregisterExitRoutine;

    
    GetDebugFunc getDebug;
    GetTraceRefcntFunc getTraceRefcnt;

    
    StringContainerInitFunc stringContainerInit;
    StringContainerFinishFunc stringContainerFinish;
    StringGetDataFunc stringGetData;
    StringSetDataFunc stringSetData;
    StringSetDataRangeFunc stringSetDataRange;
    StringCopyFunc stringCopy;
    CStringContainerInitFunc cstringContainerInit;
    CStringContainerFinishFunc cstringContainerFinish;
    CStringGetDataFunc cstringGetData;
    CStringSetDataFunc cstringSetData;
    CStringSetDataRangeFunc cstringSetDataRange;
    CStringCopyFunc cstringCopy;
    CStringToUTF16 cstringToUTF16;
    UTF16ToCString utf16ToCString;
    StringCloneDataFunc stringCloneData;
    CStringCloneDataFunc cstringCloneData;

    
    AllocFunc allocFunc;
    ReallocFunc reallocFunc;
    FreeFunc freeFunc;
    StringContainerInit2Func stringContainerInit2;
    CStringContainerInit2Func cstringContainerInit2;
    StringGetMutableDataFunc stringGetMutableData;
    CStringGetMutableDataFunc cstringGetMutableData;
    void* init3; 

    
    DebugBreakFunc debugBreakFunc;
    xpcomVoidFunc logInitFunc;
    xpcomVoidFunc logTermFunc;
    LogAddRefFunc logAddRefFunc;
    LogReleaseFunc logReleaseFunc;
    LogCtorFunc logCtorFunc;
    LogCtorFunc logDtorFunc;
    LogCOMPtrFunc logCOMPtrAddRefFunc;
    LogCOMPtrFunc logCOMPtrReleaseFunc;
    GetXPTCallStubFunc getXPTCallStubFunc;
    DestroyXPTCallStubFunc destroyXPTCallStubFunc;
    InvokeByIndexFunc invokeByIndexFunc;
    CycleCollectorFunc cycleSuspectFunc;
    CycleCollectorFunc cycleForgetFunc;
    StringSetIsVoidFunc stringSetIsVoid;
    StringGetIsVoidFunc stringGetIsVoid;
    CStringSetIsVoidFunc cstringSetIsVoid;
    CStringGetIsVoidFunc cstringGetIsVoid;

    
    CycleCollectorSuspect2Func cycleSuspect2Func;
    CycleCollectorForget2Func cycleForget2Func;

} XPCOMFunctions;

typedef nsresult (*GetFrozenFunctionsFunc)(XPCOMFunctions *entryPoints, const char* libraryPath);
XPCOM_API(nsresult)
NS_GetFrozenFunctions(XPCOMFunctions *entryPoints, const char* libraryPath);


namespace mozilla {












nsresult
ShutdownXPCOM(nsIServiceManager* servMgr);




void LogTerm();

} 



#define XPCOM_GLUE_VERSION 1











#if defined(XP_WIN32) || defined(XP_OS2)

#define XPCOM_SEARCH_KEY  "PATH"
#define GRE_CONF_NAME     "gre.config"
#define GRE_WIN_REG_LOC   L"Software\\mozilla.org\\GRE"
#define XPCOM_DLL         "xpcom.dll"
#define LXPCOM_DLL        L"xpcom.dll"
#define XUL_DLL           "xul.dll"
#define LXUL_DLL          L"xul.dll"

#else 
#include <limits.h> 

#define XPCOM_DLL "libxpcom"MOZ_DLL_SUFFIX


#ifdef XP_MACOSX  
#define XPCOM_SEARCH_KEY  "DYLD_LIBRARY_PATH"
#define GRE_FRAMEWORK_NAME "XUL.framework"
#define XUL_DLL            "XUL"
#else
#define XPCOM_SEARCH_KEY  "LD_LIBRARY_PATH"
#define XUL_DLL   "libxul"MOZ_DLL_SUFFIX
#endif

#define GRE_CONF_NAME ".gre.config"
#define GRE_CONF_PATH "/etc/gre.conf"
#define GRE_CONF_DIR  "/etc/gre.d"
#define GRE_USER_CONF_DIR ".gre.d"
#endif

#if defined(XP_WIN) || defined(XP_OS2)
  #define XPCOM_FILE_PATH_SEPARATOR       "\\"
  #define XPCOM_ENV_PATH_SEPARATOR        ";"
#elif defined(XP_UNIX)
  #define XPCOM_FILE_PATH_SEPARATOR       "/"
  #define XPCOM_ENV_PATH_SEPARATOR        ":"
#else
  #error need_to_define_your_file_path_separator_and_illegal_characters
#endif

#ifdef AIX
#include <sys/param.h>
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

extern PRBool gXPCOMShuttingDown;

namespace mozilla {
namespace services {




void Shutdown();

} 
} 

#endif
