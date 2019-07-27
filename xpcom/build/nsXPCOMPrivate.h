





#ifndef nsXPCOMPrivate_h__
#define nsXPCOMPrivate_h__

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsXPCOMStrings.h"
#include "xptcall.h"

class nsStringContainer;
class nsCStringContainer;
class nsPurpleBufferEntry;





#define NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID "xpcom-shutdown-threads"





#define NS_XPCOM_SHUTDOWN_LOADERS_OBSERVER_ID "xpcom-shutdown-loaders"


typedef nsresult   (*InitFunc)(nsIServiceManager** aResult,
                               nsIFile* aBinDirectory,
                               nsIDirectoryServiceProvider* aAppFileLocationProvider);
typedef nsresult   (*ShutdownFunc)(nsIServiceManager* aServMgr);
typedef nsresult   (*GetServiceManagerFunc)(nsIServiceManager** aResult);
typedef nsresult   (*GetComponentManagerFunc)(nsIComponentManager** aResult);
typedef nsresult   (*GetComponentRegistrarFunc)(nsIComponentRegistrar** aResult);
typedef nsresult   (*GetMemoryManagerFunc)(nsIMemory** aResult);
typedef nsresult   (*NewLocalFileFunc)(const nsAString& aPath,
                                       bool aFollowLinks, nsIFile** aResult);
typedef nsresult   (*NewNativeLocalFileFunc)(const nsACString& aPath,
                                             bool aFollowLinks,
                                             nsIFile** aResult);

typedef nsresult   (*GetDebugFunc)(nsIDebug** aResult);

typedef nsresult   (*StringContainerInitFunc)(nsStringContainer&);
typedef nsresult   (*StringContainerInit2Func)(nsStringContainer&,
                                               const char16_t*,
                                               uint32_t, uint32_t);
typedef void       (*StringContainerFinishFunc)(nsStringContainer&);
typedef uint32_t   (*StringGetDataFunc)(const nsAString&, const char16_t**,
                                        bool*);
typedef uint32_t   (*StringGetMutableDataFunc)(nsAString&, uint32_t,
                                               char16_t**);
typedef char16_t*  (*StringCloneDataFunc)(const nsAString&);
typedef nsresult   (*StringSetDataFunc)(nsAString&, const char16_t*, uint32_t);
typedef nsresult   (*StringSetDataRangeFunc)(nsAString&, uint32_t, uint32_t,
                                             const char16_t*, uint32_t);
typedef nsresult   (*StringCopyFunc)(nsAString&, const nsAString&);
typedef void       (*StringSetIsVoidFunc)(nsAString&, const bool);
typedef bool       (*StringGetIsVoidFunc)(const nsAString&);

typedef nsresult   (*CStringContainerInitFunc)(nsCStringContainer&);
typedef nsresult   (*CStringContainerInit2Func)(nsCStringContainer&,
                                                const char*,
                                                uint32_t, uint32_t);
typedef void       (*CStringContainerFinishFunc)(nsCStringContainer&);
typedef uint32_t   (*CStringGetDataFunc)(const nsACString&, const char**,
                                         bool*);
typedef uint32_t   (*CStringGetMutableDataFunc)(nsACString&, uint32_t, char**);
typedef char*      (*CStringCloneDataFunc)(const nsACString&);
typedef nsresult   (*CStringSetDataFunc)(nsACString&, const char*, uint32_t);
typedef nsresult   (*CStringSetDataRangeFunc)(nsACString&, uint32_t, uint32_t,
                                              const char*, uint32_t);
typedef nsresult   (*CStringCopyFunc)(nsACString&, const nsACString&);
typedef void       (*CStringSetIsVoidFunc)(nsACString&, const bool);
typedef bool       (*CStringGetIsVoidFunc)(const nsACString&);

typedef nsresult   (*CStringToUTF16)(const nsACString&, nsCStringEncoding,
                                     nsAString&);
typedef nsresult   (*UTF16ToCString)(const nsAString&, nsCStringEncoding,
                                     nsACString&);

typedef void*      (*AllocFunc)(size_t aSize);
typedef void*      (*ReallocFunc)(void* aPtr, size_t aSize);
typedef void       (*FreeFunc)(void* aPtr);

typedef void       (*DebugBreakFunc)(uint32_t aSeverity,
                                     const char* aStr, const char* aExpr,
                                     const char* aFile, int32_t aLine);

typedef void       (*xpcomVoidFunc)();
typedef void       (*LogAddRefFunc)(void*, nsrefcnt, const char*, uint32_t);
typedef void       (*LogReleaseFunc)(void*, nsrefcnt, const char*);
typedef void       (*LogCtorFunc)(void*, const char*, uint32_t);
typedef void       (*LogCOMPtrFunc)(void*, nsISupports*);

typedef nsresult   (*GetXPTCallStubFunc)(REFNSIID, nsIXPTCProxy*,
                                         nsISomeInterface**);
typedef void       (*DestroyXPTCallStubFunc)(nsISomeInterface*);
typedef nsresult   (*InvokeByIndexFunc)(nsISupports*, uint32_t, uint32_t,
                                        nsXPTCVariant*);
typedef bool       (*CycleCollectorFunc)(nsISupports*);
typedef nsPurpleBufferEntry*
                   (*CycleCollectorSuspect2Func)(void*,
                                                 nsCycleCollectionParticipant*);
typedef bool       (*CycleCollectorForget2Func)(nsPurpleBufferEntry*);
typedef void       (*CycleCollectorSuspect3Func)(void*,
                                                 nsCycleCollectionParticipant*,
                                                 nsCycleCollectingAutoRefCnt*,
                                                 bool*);

typedef NS_CALLBACK(XPCOMExitRoutine)(void);

typedef nsresult   (*RegisterXPCOMExitRoutineFunc)(XPCOMExitRoutine aExitRoutine,
                                                   uint32_t aPriority);
typedef nsresult   (*UnregisterXPCOMExitRoutineFunc)(XPCOMExitRoutine aExitRoutine);

typedef struct XPCOMFunctions
{
  uint32_t version;
  uint32_t size;

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
  void* getTraceRefcnt;

  
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

  CycleCollectorSuspect3Func cycleSuspect3Func;

} XPCOMFunctions;

typedef nsresult (*GetFrozenFunctionsFunc)(XPCOMFunctions* aEntryPoints,
                                           const char* aLibraryPath);
XPCOM_API(nsresult) NS_GetFrozenFunctions(XPCOMFunctions* aEntryPoints,
                                          const char* aLibraryPath);


namespace mozilla {












nsresult
ShutdownXPCOM(nsIServiceManager* aServMgr);

void SetICUMemoryFunctions();




void LogTerm();

} 



#define XPCOM_GLUE_VERSION 1











#if defined(XP_WIN32)

#define XPCOM_SEARCH_KEY  "PATH"
#define GRE_CONF_NAME     "gre.config"
#define GRE_WIN_REG_LOC   L"Software\\mozilla.org\\GRE"
#define XPCOM_DLL         XUL_DLL
#define LXPCOM_DLL        LXUL_DLL
#define XUL_DLL           "xul.dll"
#define LXUL_DLL          L"xul.dll"

#else 
#include <limits.h> 

#define XPCOM_DLL         XUL_DLL


#ifdef XP_MACOSX
#define XPCOM_SEARCH_KEY  "DYLD_LIBRARY_PATH"
#define GRE_FRAMEWORK_NAME "XUL.framework"
#define XUL_DLL            "XUL"
#else
#define XPCOM_SEARCH_KEY  "LD_LIBRARY_PATH"
#define XUL_DLL   "libxul" MOZ_DLL_SUFFIX
#endif

#define GRE_CONF_NAME ".gre.config"
#define GRE_CONF_PATH "/etc/gre.conf"
#define GRE_CONF_DIR  "/etc/gre.d"
#define GRE_USER_CONF_DIR ".gre.d"
#endif

#if defined(XP_WIN)
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

extern bool gXPCOMShuttingDown;
extern bool gXPCOMThreadsShutDown;


extern char16_t* gGREBinPath;

namespace mozilla {
namespace services {




void Shutdown();

} 
} 

#endif
