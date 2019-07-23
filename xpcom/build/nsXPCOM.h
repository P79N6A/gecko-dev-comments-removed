





































#ifndef nsXPCOM_h__
#define nsXPCOM_h__


#ifdef MOZILLA_INTERNAL_API
# define NS_InitXPCOM2               NS_InitXPCOM2_P
# define NS_InitXPCOM3               NS_InitXPCOM3_P
# define NS_ShutdownXPCOM            NS_ShutdownXPCOM_P
# define NS_GetServiceManager        NS_GetServiceManager_P
# define NS_GetComponentManager      NS_GetComponentManager_P
# define NS_GetComponentRegistrar    NS_GetComponentRegistrar_P
# define NS_GetMemoryManager         NS_GetMemoryManager_P
# define NS_NewLocalFile             NS_NewLocalFile_P
# define NS_NewNativeLocalFile       NS_NewNativeLocalFile_P
# define NS_GetDebug                 NS_GetDebug_P
# define NS_GetTraceRefcnt           NS_GetTraceRefcnt_P
# define NS_Alloc                    NS_Alloc_P
# define NS_Realloc                  NS_Realloc_P
# define NS_Free                     NS_Free_P
# define NS_DebugBreak               NS_DebugBreak_P
# define NS_LogInit                  NS_LogInit_P
# define NS_LogTerm                  NS_LogTerm_P
# define NS_LogAddRef                NS_LogAddRef_P
# define NS_LogRelease               NS_LogRelease_P
# define NS_LogCtor                  NS_LogCtor_P
# define NS_LogDtor                  NS_LogDtor_P
# define NS_LogCOMPtrAddRef          NS_LogCOMPtrAddRef_P
# define NS_LogCOMPtrRelease         NS_LogCOMPtrRelease_P
# define NS_CycleCollectorSuspect    NS_CycleCollectorSuspect_P
# define NS_CycleCollectorForget     NS_CycleCollectorForget_P
# define NS_CycleCollectorSuspect2   NS_CycleCollectorSuspect2_P
# define NS_CycleCollectorForget2    NS_CycleCollectorForget2_P
#endif

#include "nscore.h"
#include "nsXPCOMCID.h"

#ifdef __cplusplus
#define DECL_CLASS(c) class c
#define DECL_STRUCT(c) struct c
#else
#define DECL_CLASS(c) typedef struct c c
#define DECL_STRUCT(c) typedef struct c c
#endif

DECL_CLASS(nsAString);
DECL_CLASS(nsACString);

DECL_CLASS(nsISupports);
DECL_CLASS(nsIModule);
DECL_CLASS(nsIComponentManager);
DECL_CLASS(nsIComponentRegistrar);
DECL_CLASS(nsIServiceManager);
DECL_CLASS(nsIFile);
DECL_CLASS(nsILocalFile);
DECL_CLASS(nsIDirectoryServiceProvider);
DECL_CLASS(nsIMemory);
DECL_CLASS(nsIDebug);
DECL_CLASS(nsITraceRefcnt);
DECL_STRUCT(nsPurpleBufferEntry);







typedef nsresult (*nsGetModuleProc)(nsIComponentManager *aCompMgr,
                                    nsIFile* location,
                                    nsIModule** return_cobj);



































XPCOM_API(nsresult)
NS_InitXPCOM2(nsIServiceManager* *result, 
              nsIFile* binDirectory,
              nsIDirectoryServiceProvider* appFileLocationProvider);








typedef struct nsStaticModuleInfo {
  const char      *name;
  nsGetModuleProc  getModule;
} nsStaticModuleInfo;









































XPCOM_API(nsresult)
NS_InitXPCOM3(nsIServiceManager* *result, 
              nsIFile* binDirectory,
              nsIDirectoryServiceProvider* appFileLocationProvider,
              nsStaticModuleInfo const *staticComponents,
              PRUint32 componentCount);














XPCOM_API(nsresult)
NS_ShutdownXPCOM(nsIServiceManager* servMgr);












XPCOM_API(nsresult)
NS_GetServiceManager(nsIServiceManager* *result);











XPCOM_API(nsresult)
NS_GetComponentManager(nsIComponentManager* *result);











XPCOM_API(nsresult)
NS_GetComponentRegistrar(nsIComponentRegistrar* *result);











XPCOM_API(nsresult)
NS_GetMemoryManager(nsIMemory* *result);
























#ifdef __cplusplus

XPCOM_API(nsresult)
NS_NewLocalFile(const nsAString &path, 
                PRBool followLinks, 
                nsILocalFile* *result);

XPCOM_API(nsresult)
NS_NewNativeLocalFile(const nsACString &path, 
                      PRBool followLinks, 
                      nsILocalFile* *result);

#endif











XPCOM_API(void*)
NS_Alloc(PRSize size);



















XPCOM_API(void*)
NS_Realloc(void* ptr, PRSize size);











XPCOM_API(void)
NS_Free(void* ptr);





enum {
    NS_DEBUG_WARNING = 0,
    NS_DEBUG_ASSERTION = 1,
    NS_DEBUG_BREAK = 2,
    NS_DEBUG_ABORT = 3
};
















XPCOM_API(void)
NS_DebugBreak(PRUint32 aSeverity,
              const char *aStr, const char *aExpr,
              const char *aFile, PRInt32 aLine);

















XPCOM_API(void)
NS_LogInit();

XPCOM_API(void)
NS_LogTerm();











XPCOM_API(void)
NS_LogCtor(void *aPtr, const char *aTypeName, PRUint32 aInstanceSize);

XPCOM_API(void)
NS_LogDtor(void *aPtr, const char *aTypeName, PRUint32 aInstanceSize);











XPCOM_API(void)
NS_LogAddRef(void *aPtr, nsrefcnt aNewRefCnt,
             const char *aTypeName, PRUint32 aInstanceSize);

XPCOM_API(void)
NS_LogRelease(void *aPtr, nsrefcnt aNewRefCnt, const char *aTypeName);











XPCOM_API(void)
NS_LogCOMPtrAddRef(void *aCOMPtr, nsISupports *aObject);

XPCOM_API(void)
NS_LogCOMPtrRelease(void *aCOMPtr, nsISupports *aObject);









XPCOM_API(PRBool)
NS_CycleCollectorSuspect(nsISupports *n);

XPCOM_API(PRBool)
NS_CycleCollectorForget(nsISupports *n);

XPCOM_API(nsPurpleBufferEntry*)
NS_CycleCollectorSuspect2(nsISupports *n);

XPCOM_API(PRBool)
NS_CycleCollectorForget2(nsPurpleBufferEntry *e);














#define XPCOM_DIRECTORY_PROVIDER_CATEGORY "xpcom-directory-providers"









#define NS_XPCOM_STARTUP_CATEGORY "xpcom-startup"














#define NS_XPCOM_STARTUP_OBSERVER_ID "xpcom-startup"






#define NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID "xpcom-will-shutdown"








#define NS_XPCOM_SHUTDOWN_OBSERVER_ID "xpcom-shutdown"









#define NS_XPCOM_CATEGORY_ENTRY_ADDED_OBSERVER_ID \
  "xpcom-category-entry-added"









#define NS_XPCOM_CATEGORY_ENTRY_REMOVED_OBSERVER_ID \
  "xpcom-category-entry-removed"









#define NS_XPCOM_CATEGORY_CLEARED_OBSERVER_ID "xpcom-category-cleared"

XPCOM_API(nsresult)
NS_GetDebug(nsIDebug* *result);

XPCOM_API(nsresult)
NS_GetTraceRefcnt(nsITraceRefcnt* *result);

#endif
