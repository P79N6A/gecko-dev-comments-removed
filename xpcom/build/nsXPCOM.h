





#ifndef nsXPCOM_h__
#define nsXPCOM_h__

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

#ifdef __cplusplus
namespace mozilla {
struct Module;
}
#endif

































XPCOM_API(nsresult)
NS_InitXPCOM2(nsIServiceManager** aResult,
              nsIFile* aBinDirectory,
              nsIDirectoryServiceProvider* aAppFileLocationProvider);











XPCOM_API(nsresult) NS_ShutdownXPCOM(nsIServiceManager* aServMgr);










XPCOM_API(nsresult) NS_GetServiceManager(nsIServiceManager** aResult);









XPCOM_API(nsresult) NS_GetComponentManager(nsIComponentManager** aResult);










XPCOM_API(nsresult) NS_GetComponentRegistrar(nsIComponentRegistrar** aResult);









XPCOM_API(nsresult) NS_GetMemoryManager(nsIMemory** aResult);






















#ifdef __cplusplus

XPCOM_API(nsresult) NS_NewLocalFile(const nsAString& aPath,
                                    bool aFollowLinks,
                                    nsIFile** aResult);

XPCOM_API(nsresult) NS_NewNativeLocalFile(const nsACString& aPath,
                                          bool aFollowLinks,
                                          nsIFile** aResult);

#endif






#ifdef XPCOM_GLUE








XPCOM_API(void*) NS_Alloc(size_t aSize);
















XPCOM_API(void*) NS_Realloc(void* aPtr, size_t aSize);









XPCOM_API(void) NS_Free(void* aPtr);
#else
#define NS_Alloc moz_xmalloc
#define NS_Realloc moz_xrealloc
#define NS_Free free
#endif





enum
{
  NS_DEBUG_WARNING = 0,
  NS_DEBUG_ASSERTION = 1,
  NS_DEBUG_BREAK = 2,
  NS_DEBUG_ABORT = 3
};
















XPCOM_API(void) NS_DebugBreak(uint32_t aSeverity,
                              const char* aStr, const char* aExpr,
                              const char* aFile, int32_t aLine);

















XPCOM_API(void) NS_LogInit();

XPCOM_API(void) NS_LogTerm();











XPCOM_API(void) NS_LogCtor(void* aPtr, const char* aTypeName,
                           uint32_t aInstanceSize);

XPCOM_API(void) NS_LogDtor(void* aPtr, const char* aTypeName,
                           uint32_t aInstanceSize);











XPCOM_API(void) NS_LogAddRef(void* aPtr, nsrefcnt aNewRefCnt,
                             const char* aTypeName, uint32_t aInstanceSize);

XPCOM_API(void) NS_LogRelease(void* aPtr, nsrefcnt aNewRefCnt,
                              const char* aTypeName);











XPCOM_API(void) NS_LogCOMPtrAddRef(void* aCOMPtr, nsISupports* aObject);

XPCOM_API(void) NS_LogCOMPtrRelease(void* aCOMPtr, nsISupports* aObject);







#ifdef __cplusplus

class nsCycleCollectionParticipant;
class nsCycleCollectingAutoRefCnt;

XPCOM_API(void) NS_CycleCollectorSuspect3(void* aPtr,
                                          nsCycleCollectionParticipant* aCp,
                                          nsCycleCollectingAutoRefCnt* aRefCnt,
                                          bool* aShouldDelete);

#endif












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

XPCOM_API(nsresult) NS_GetDebug(nsIDebug** aResult);

#endif
