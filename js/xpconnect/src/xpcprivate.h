









































































#ifndef xpcprivate_h___
#define xpcprivate_h___

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Util.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xpcpublic.h"
#include "js/Tracer.h"
#include "pldhash.h"
#include "nscore.h"
#include "nsXPCOM.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsIClassInfoImpl.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsISupportsPrimitives.h"
#include "nsMemory.h"
#include "nsIXPConnect.h"
#include "nsIInterfaceInfo.h"
#include "nsIXPCScriptable.h"
#include "nsIXPCSecurityManager.h"
#include "nsIJSRuntimeService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsXPTCUtils.h"
#include "xptinfo.h"
#include "XPCForwards.h"
#include "XPCLog.h"
#include "xpccomponents.h"
#include "xpcexception.h"
#include "xpcjsid.h"
#include "prenv.h"
#include "prclist.h"
#include "prcvar.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsAutoJSValHolder.h"

#include "MainThreadUtils.h"
#include "nsIJSEngineTelemetryStats.h"

#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIException.h"

#include "nsVariant.h"
#include "nsIPropertyBag.h"
#include "nsIProperty.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsWrapperCache.h"
#include "nsStringBuffer.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"

#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsISecurityCheckedComponent.h"
#include "xpcObjectHelper.h"
#include "nsIThreadInternal.h"

#include "SandboxPrivate.h"
#include "BackstagePass.h"
#include "nsCxPusher.h"
#include "nsAXPCNativeCallContext.h"

#ifdef XP_WIN

#ifdef GetClassInfo
#undef GetClassInfo
#endif
#ifdef GetClassName
#undef GetClassName
#endif
#endif 

#include "nsINode.h"






#if defined(DEBUG_jband) || defined(DEBUG_jst) || defined(DEBUG_dbradley) || defined(DEBUG_shaver_no) || defined(DEBUG_timeless)
#define DEBUG_xpc_hacker
#endif

#if defined(DEBUG_brendan)
#define DEBUG_XPCNativeWrapper 1
#endif

#ifdef DEBUG
#define XPC_DETECT_LEADING_UPPERCASE_ACCESS_ERRORS
#endif

#if defined(DEBUG_xpc_hacker)
#define XPC_DUMP_AT_SHUTDOWN
#define XPC_TRACK_WRAPPER_STATS
#define XPC_TRACK_SCOPE_STATS
#define XPC_TRACK_PROTO_STATS
#define XPC_TRACK_DEFERRED_RELEASES
#define XPC_CHECK_WRAPPERS_AT_SHUTDOWN
#define XPC_REPORT_SHADOWED_WRAPPED_NATIVE_MEMBERS
#define XPC_CHECK_CLASSINFO_CLAIMS
#if defined(DEBUG_jst)
#define XPC_ASSERT_CLASSINFO_CLAIMS
#endif




#endif

#if defined(DEBUG_dbaron) || defined(DEBUG_bzbarsky) 
#define XPC_DUMP_AT_SHUTDOWN
#endif




#ifdef XPC_REPORT_SHADOWED_WRAPPED_NATIVE_MEMBERS
void DEBUG_ReportShadowedMembers(XPCNativeSet* set,
                                 XPCWrappedNative* wrapper,
                                 XPCWrappedNativeProto* proto);
#else
#define DEBUG_ReportShadowedMembers(set, wrapper, proto) ((void)0)
#endif




#define XPC_CONTEXT_MAP_SIZE                16
#define XPC_JS_MAP_SIZE                     64
#define XPC_JS_CLASS_MAP_SIZE               64

#define XPC_NATIVE_MAP_SIZE                 64
#define XPC_NATIVE_PROTO_MAP_SIZE           16
#define XPC_DYING_NATIVE_PROTO_MAP_SIZE     16
#define XPC_DETACHED_NATIVE_PROTO_MAP_SIZE  32
#define XPC_NATIVE_INTERFACE_MAP_SIZE       64
#define XPC_NATIVE_SET_MAP_SIZE             64
#define XPC_NATIVE_JSCLASS_MAP_SIZE         32
#define XPC_THIS_TRANSLATOR_MAP_SIZE         8
#define XPC_NATIVE_WRAPPER_MAP_SIZE         16
#define XPC_WRAPPER_MAP_SIZE                16



extern const char XPC_CONTEXT_STACK_CONTRACTID[];
extern const char XPC_RUNTIME_CONTRACTID[];
extern const char XPC_EXCEPTION_CONTRACTID[];
extern const char XPC_CONSOLE_CONTRACTID[];
extern const char XPC_SCRIPT_ERROR_CONTRACTID[];
extern const char XPC_ID_CONTRACTID[];
extern const char XPC_XPCONNECT_CONTRACTID[];




#define XPC_STRING_GETTER_BODY(dest, src)                                     \
    NS_ENSURE_ARG_POINTER(dest);                                              \
    char* result;                                                             \
    if (src)                                                                  \
        result = (char*) nsMemory::Clone(src,                                 \
                                         sizeof(char)*(strlen(src)+1));       \
    else                                                                      \
        result = nullptr;                                                      \
    *dest = result;                                                           \
    return (result || !src) ? NS_OK : NS_ERROR_OUT_OF_MEMORY


#define WRAPPER_SLOTS (JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS | \
                       JSCLASS_HAS_RESERVED_SLOTS(1))

#define INVALID_OBJECT ((JSObject *)1)





#define WN_XRAYEXPANDOCHAIN_SLOT 0

inline void SetWNExpandoChain(JSObject *obj, JSObject *chain)
{
    MOZ_ASSERT(IS_WN_REFLECTOR(obj));
    JS_SetReservedSlot(obj, WN_XRAYEXPANDOCHAIN_SLOT, JS::ObjectOrNullValue(chain));
}

inline JSObject* GetWNExpandoChain(JSObject *obj)
{
    MOZ_ASSERT(IS_WN_REFLECTOR(obj));
    return JS_GetReservedSlot(obj, WN_XRAYEXPANDOCHAIN_SLOT).toObjectOrNull();
}





#ifdef _MSC_VER
#pragma warning(disable : 4355) // OK to pass "this" in member initializer
#endif

typedef mozilla::ReentrantMonitor XPCLock;

static inline void xpc_Wait(XPCLock* lock)
    {
        MOZ_ASSERT(lock, "xpc_Wait called with null lock!");
        lock->Wait();
    }

static inline void xpc_NotifyAll(XPCLock* lock)
    {
        MOZ_ASSERT(lock, "xpc_NotifyAll called with null lock!");
        lock->NotifyAll();
    }











class MOZ_STACK_CLASS XPCAutoLock {
public:

    static XPCLock* NewLock(const char* name)
                        {return new mozilla::ReentrantMonitor(name);}
    static void     DestroyLock(XPCLock* lock)
                        {delete lock;}

    XPCAutoLock(XPCLock* lock MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : mLock(lock)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (mLock)
            mLock->Enter();
    }

    ~XPCAutoLock()
    {
        if (mLock) {
            mLock->Exit();
        }
    }

private:
    XPCLock*  mLock;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
    XPCAutoLock(void) {}
    XPCAutoLock(XPCAutoLock& ) {}
    XPCAutoLock& operator =(XPCAutoLock& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nullptr;
    }
    static void operator delete(void* ) {}
};



class MOZ_STACK_CLASS XPCAutoUnlock {
public:
    XPCAutoUnlock(XPCLock* lock MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : mLock(lock)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (mLock) {
            mLock->Exit();
        }
    }

    ~XPCAutoUnlock()
    {
        if (mLock)
            mLock->Enter();
    }

private:
    XPCLock*  mLock;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
    XPCAutoUnlock(void) {}
    XPCAutoUnlock(XPCAutoUnlock& ) {}
    XPCAutoUnlock& operator =(XPCAutoUnlock& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nullptr;
    }
    static void operator delete(void* ) {}
};














inline bool
AddToCCKind(JSGCTraceKind kind)
{
    return kind == JSTRACE_OBJECT || kind == JSTRACE_SCRIPT;
}

class nsXPConnect : public nsIXPConnect,
                    public nsIThreadObserver,
                    public nsSupportsWeakReference,
                    public nsIJSRuntimeService,
                    public nsIJSEngineTelemetryStats
{
public:
    
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIXPCONNECT
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIJSRUNTIMESERVICE
    NS_DECL_NSIJSENGINETELEMETRYSTATS

    
public:
    
    static nsXPConnect*  XPConnect()
    {
        
        
        
        if (!MOZ_LIKELY(NS_IsMainThread()))
            MOZ_CRASH();

        return gSelf;
    }

    static XPCJSRuntime* GetRuntimeInstance();
    XPCJSRuntime* GetRuntime() {return mRuntime;}

    static bool IsISupportsDescendant(nsIInterfaceInfo* info);

    nsIXPCSecurityManager* GetDefaultSecurityManager() const
    {
        
        if (!NS_IsMainThread()) {
            return nullptr;
        }
        return mDefaultSecurityManager;
    }

    
    
    
    static nsXPConnect* GetSingleton();

    
    static void InitStatics();
    
    static void ReleaseXPConnectSingleton();

    virtual ~nsXPConnect();

    bool IsShuttingDown() const {return mShuttingDown;}

    nsresult GetInfoForIID(const nsIID * aIID, nsIInterfaceInfo** info);
    nsresult GetInfoForName(const char * name, nsIInterfaceInfo** info);

    virtual nsIPrincipal* GetPrincipal(JSObject* obj,
                                       bool allowShortCircuit) const;

    void RecordTraversal(void *p, nsISupports *s);
    virtual char* DebugPrintJSStack(bool showArgs,
                                    bool showLocals,
                                    bool showThisProps);


    static bool ReportAllJSExceptions()
    {
      return gReportAllJSExceptions > 0;
    }

    static void CheckForDebugMode(JSRuntime *rt);

protected:
    nsXPConnect();

private:
    static PRThread* FindMainThread();

private:
    
    static nsXPConnect*      gSelf;
    static bool              gOnceAliveNowDead;

    XPCJSRuntime*            mRuntime;
    nsIXPCSecurityManager*   mDefaultSecurityManager;
    bool                     mShuttingDown;

    
    
    
    
    
    uint16_t                 mEventDepth;

    static uint32_t gReportAllJSExceptions;

public:
    static nsIScriptSecurityManager *gScriptSecurityManager;
};



class XPCRootSetElem
{
public:
    XPCRootSetElem()
    {
#ifdef DEBUG
        mNext = nullptr;
        mSelfp = nullptr;
#endif
    }

    ~XPCRootSetElem()
    {
        MOZ_ASSERT(!mNext, "Must be unlinked");
        MOZ_ASSERT(!mSelfp, "Must be unlinked");
    }

    inline XPCRootSetElem* GetNextRoot() { return mNext; }
    void AddToRootSet(XPCLock *lock, XPCRootSetElem **listHead);
    void RemoveFromRootSet(XPCLock *lock);

private:
    XPCRootSetElem *mNext;
    XPCRootSetElem **mSelfp;
};




class XPCReadableJSStringWrapper : public nsDependentString
{
public:
    typedef nsDependentString::char_traits char_traits;

    XPCReadableJSStringWrapper(const PRUnichar *chars, size_t length) :
        nsDependentString(chars, length)
    { }

    XPCReadableJSStringWrapper() :
        nsDependentString(char_traits::sEmptyBuffer, char_traits::sEmptyBuffer)
    { SetIsVoid(true); }

    bool init(JSContext* aContext, JSString* str)
    {
        size_t length;
        const jschar* chars = JS_GetStringCharsZAndLength(aContext, str, &length);
        if (!chars)
            return false;

        MOZ_ASSERT(IsEmpty(), "init() on initialized string");
        new(static_cast<nsDependentString *>(this)) nsDependentString(chars, length);
        return true;
    }
};




class XPCJSContextStack;
class WatchdogManager;

enum WatchdogTimestampCategory
{
    TimestampRuntimeStateChange = 0,
    TimestampWatchdogWakeup,
    TimestampWatchdogHibernateStart,
    TimestampWatchdogHibernateStop,
    TimestampCount
};

class AsyncFreeSnowWhite;

class XPCJSRuntime : public mozilla::CycleCollectedJSRuntime
{
public:
    static XPCJSRuntime* newXPCJSRuntime(nsXPConnect* aXPConnect);
    static XPCJSRuntime* Get() { return nsXPConnect::XPConnect()->GetRuntime(); }

    XPCJSContextStack* GetJSContextStack() {return mJSContextStack;}
    void DestroyJSContextStack();

    XPCCallContext*  GetCallContext() const {return mCallContext;}
    XPCCallContext*  SetCallContext(XPCCallContext* ccx)
        {XPCCallContext* old = mCallContext; mCallContext = ccx; return old;}

    jsid GetResolveName() const {return mResolveName;}
    jsid SetResolveName(jsid name)
        {jsid old = mResolveName; mResolveName = name; return old;}

    XPCWrappedNative* GetResolvingWrapper() const {return mResolvingWrapper;}
    XPCWrappedNative* SetResolvingWrapper(XPCWrappedNative* w)
        {XPCWrappedNative* old = mResolvingWrapper;
         mResolvingWrapper = w; return old;}

    JSObject2WrappedJSMap*     GetWrappedJSMap()        const
        {return mWrappedJSMap;}

    IID2WrappedJSClassMap*     GetWrappedJSClassMap()   const
        {return mWrappedJSClassMap;}

    IID2NativeInterfaceMap* GetIID2NativeInterfaceMap() const
        {return mIID2NativeInterfaceMap;}

    ClassInfo2NativeSetMap* GetClassInfo2NativeSetMap() const
        {return mClassInfo2NativeSetMap;}

    NativeSetMap* GetNativeSetMap() const
        {return mNativeSetMap;}

    IID2ThisTranslatorMap* GetThisTranslatorMap() const
        {return mThisTranslatorMap;}

    XPCNativeScriptableSharedMap* GetNativeScriptableSharedMap() const
        {return mNativeScriptableSharedMap;}

    XPCWrappedNativeProtoMap* GetDyingWrappedNativeProtoMap() const
        {return mDyingWrappedNativeProtoMap;}

    XPCWrappedNativeProtoMap* GetDetachedWrappedNativeProtoMap() const
        {return mDetachedWrappedNativeProtoMap;}

    XPCLock* GetMapLock() const {return mMapLock;}

    bool OnJSContextNew(JSContext* cx);

    virtual bool
    DescribeCustomObjects(JSObject* aObject, const js::Class* aClasp,
                          char (&aName)[72]) const MOZ_OVERRIDE;
    virtual bool
    NoteCustomGCThingXPCOMChildren(const js::Class* aClasp, JSObject* aObj,
                                   nsCycleCollectionTraversalCallback& aCb) const MOZ_OVERRIDE;

    





public:
    bool GetDoingFinalization() const {return mDoingFinalization;}

    
    
    
    
    enum {
        IDX_CONSTRUCTOR             = 0 ,
        IDX_TO_STRING               ,
        IDX_TO_SOURCE               ,
        IDX_LAST_RESULT             ,
        IDX_RETURN_CODE             ,
        IDX_VALUE                   ,
        IDX_QUERY_INTERFACE         ,
        IDX_COMPONENTS              ,
        IDX_WRAPPED_JSOBJECT        ,
        IDX_OBJECT                  ,
        IDX_FUNCTION                ,
        IDX_PROTOTYPE               ,
        IDX_CREATE_INSTANCE         ,
        IDX_ITEM                    ,
        IDX_PROTO                   ,
        IDX_ITERATOR                ,
        IDX_EXPOSEDPROPS            ,
        IDX_TOTAL_COUNT 
    };

    jsid GetStringID(unsigned index) const
    {
        MOZ_ASSERT(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrIDs[index];
    }
    jsval GetStringJSVal(unsigned index) const
    {
        MOZ_ASSERT(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrJSVals[index];
    }
    const char* GetStringName(unsigned index) const
    {
        MOZ_ASSERT(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrings[index];
    }

    void TraceNativeBlackRoots(JSTracer* trc) MOZ_OVERRIDE;
    void TraceAdditionalNativeGrayRoots(JSTracer* aTracer) MOZ_OVERRIDE;
    void TraverseAdditionalNativeRoots(nsCycleCollectionNoteRootCallback& cb) MOZ_OVERRIDE;
    void UnmarkSkippableJSHolders();
    void PrepareForForgetSkippable() MOZ_OVERRIDE;
    void PrepareForCollection() MOZ_OVERRIDE;
    void DispatchDeferredDeletion(bool continuation) MOZ_OVERRIDE;

    void CustomGCCallback(JSGCStatus status) MOZ_OVERRIDE;
    bool CustomContextCallback(JSContext *cx, unsigned operation) MOZ_OVERRIDE;
    static void GCSliceCallback(JSRuntime *rt,
                                JS::GCProgress progress,
                                const JS::GCDescription &desc);
    static void FinalizeCallback(JSFreeOp *fop, JSFinalizeStatus status, bool isCompartmentGC);

    inline void AddVariantRoot(XPCTraceableVariant* variant);
    inline void AddWrappedJSRoot(nsXPCWrappedJS* wrappedJS);
    inline void AddObjectHolderRoot(XPCJSObjectHolder* holder);

    static void SuspectWrappedNative(XPCWrappedNative *wrapper,
                                     nsCycleCollectionNoteRootCallback &cb);

    void DebugDump(int16_t depth);

    void SystemIsBeingShutDown();

    PRThread* GetThreadRunningGC() const {return mThreadRunningGC;}

    ~XPCJSRuntime();

    XPCReadableJSStringWrapper *NewStringWrapper(const PRUnichar *str, uint32_t len);
    void DeleteString(nsAString *string);

#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
   void DEBUG_AddWrappedNative(nsIXPConnectWrappedNative* wrapper)
        {XPCAutoLock lock(GetMapLock());
         PLDHashEntryHdr *entry =
            PL_DHashTableOperate(DEBUG_WrappedNativeHashtable,
                                 wrapper, PL_DHASH_ADD);
         if (entry) ((PLDHashEntryStub *)entry)->key = wrapper;}

   void DEBUG_RemoveWrappedNative(nsIXPConnectWrappedNative* wrapper)
        {XPCAutoLock lock(GetMapLock());
         PL_DHashTableOperate(DEBUG_WrappedNativeHashtable,
                              wrapper, PL_DHASH_REMOVE);}
private:
   PLDHashTable* DEBUG_WrappedNativeHashtable;
public:
#endif

    void AddGCCallback(xpcGCCallback cb);
    void RemoveGCCallback(xpcGCCallback cb);
    void AddContextCallback(xpcContextCallback cb);
    void RemoveContextCallback(xpcContextCallback cb);

    static JSContext* DefaultJSContextCallback(JSRuntime *rt);
    static void ActivityCallback(void *arg, bool active);
    static void CTypesActivityCallback(JSContext *cx,
                                       js::CTypesActivityType type);
    static bool OperationCallback(JSContext *cx);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    AutoMarkingPtr**  GetAutoRootsAdr() {return &mAutoRoots;}

    JSObject* GetJunkScope();
    void DeleteJunkScope();

    PRTime GetWatchdogTimestamp(WatchdogTimestampCategory aCategory);
    void OnAfterProcessNextEvent() { mSlowScriptCheckpoint = mozilla::TimeStamp(); }

private:
    XPCJSRuntime(); 
    XPCJSRuntime(nsXPConnect* aXPConnect);

    void ReleaseIncrementally(nsTArray<nsISupports *> &array);

    static const char* mStrings[IDX_TOTAL_COUNT];
    jsid mStrIDs[IDX_TOTAL_COUNT];
    jsval mStrJSVals[IDX_TOTAL_COUNT];

    XPCJSContextStack*       mJSContextStack;
    XPCCallContext*          mCallContext;
    AutoMarkingPtr*          mAutoRoots;
    jsid                     mResolveName;
    XPCWrappedNative*        mResolvingWrapper;
    JSObject2WrappedJSMap*   mWrappedJSMap;
    IID2WrappedJSClassMap*   mWrappedJSClassMap;
    IID2NativeInterfaceMap*  mIID2NativeInterfaceMap;
    ClassInfo2NativeSetMap*  mClassInfo2NativeSetMap;
    NativeSetMap*            mNativeSetMap;
    IID2ThisTranslatorMap*   mThisTranslatorMap;
    XPCNativeScriptableSharedMap* mNativeScriptableSharedMap;
    XPCWrappedNativeProtoMap* mDyingWrappedNativeProtoMap;
    XPCWrappedNativeProtoMap* mDetachedWrappedNativeProtoMap;
    XPCLock* mMapLock;
    PRThread* mThreadRunningGC;
    nsTArray<nsXPCWrappedJS*> mWrappedJSToReleaseArray;
    nsTArray<nsISupports*> mNativesToReleaseArray;
    bool mDoingFinalization;
    XPCRootSetElem *mVariantRoots;
    XPCRootSetElem *mWrappedJSRoots;
    XPCRootSetElem *mObjectHolderRoots;
    nsTArray<xpcGCCallback> extraGCCallbacks;
    nsTArray<xpcContextCallback> extraContextCallbacks;
    nsRefPtr<WatchdogManager> mWatchdogManager;
    JS::GCSliceCallback mPrevGCSliceCallback;
    JSObject* mJunkScope;
    nsRefPtr<AsyncFreeSnowWhite> mAsyncSnowWhiteFreer;

    mozilla::TimeStamp mSlowScriptCheckpoint;

#define XPCCCX_STRING_CACHE_SIZE 2

    
    
    
    
    
    struct StringWrapperEntry
    {
        StringWrapperEntry() : mInUse(false) { }

        mozilla::AlignedStorage2<XPCReadableJSStringWrapper> mString;
        bool mInUse;
    };

    StringWrapperEntry mScratchStrings[XPCCCX_STRING_CACHE_SIZE];

    friend class Watchdog;
    friend class AutoLockWatchdog;
    friend class XPCIncrementalReleaseRunnable;
};







class XPCContext
{
    friend class XPCJSRuntime;
public:
    static XPCContext* GetXPCContext(JSContext* aJSContext)
        {
            MOZ_ASSERT(JS_GetSecondContextPrivate(aJSContext), "should already have XPCContext");
            return static_cast<XPCContext *>(JS_GetSecondContextPrivate(aJSContext));
        }

    XPCJSRuntime* GetRuntime() const {return mRuntime;}
    JSContext* GetJSContext() const {return mJSContext;}

    enum LangType {LANG_UNKNOWN, LANG_JS, LANG_NATIVE};

    LangType GetCallingLangType() const
        {
            return mCallingLangType;
        }
    LangType SetCallingLangType(LangType lt)
        {
            LangType tmp = mCallingLangType;
            mCallingLangType = lt;
            return tmp;
        }
    bool CallerTypeIsJavaScript() const
        {
            return LANG_JS == mCallingLangType;
        }
    bool CallerTypeIsNative() const
        {
            return LANG_NATIVE == mCallingLangType;
        }
    bool CallerTypeIsKnown() const
        {
            return LANG_UNKNOWN != mCallingLangType;
        }

    nsresult GetException(nsIException** e)
        {
            NS_IF_ADDREF(mException);
            *e = mException;
            return NS_OK;
        }
    void SetException(nsIException* e)
        {
            NS_IF_ADDREF(e);
            NS_IF_RELEASE(mException);
            mException = e;
        }

    nsresult GetLastResult() {return mLastResult;}
    void SetLastResult(nsresult rc) {mLastResult = rc;}

    nsresult GetPendingResult() {return mPendingResult;}
    void SetPendingResult(nsresult rc) {mPendingResult = rc;}

    void DebugDump(int16_t depth);
    void AddScope(PRCList *scope) { PR_INSERT_AFTER(scope, &mScopes); }
    void RemoveScope(PRCList *scope) { PR_REMOVE_LINK(scope); }

    void MarkErrorUnreported() { mErrorUnreported = true; }
    void ClearUnreportedError() { mErrorUnreported = false; }
    bool WasErrorReported() { return !mErrorUnreported; }

    ~XPCContext();

private:
    XPCContext();    
    XPCContext(XPCJSRuntime* aRuntime, JSContext* aJSContext);

    static XPCContext* newXPCContext(XPCJSRuntime* aRuntime,
                                     JSContext* aJSContext);
private:
    XPCJSRuntime* mRuntime;
    JSContext*  mJSContext;
    nsresult mLastResult;
    nsresult mPendingResult;
    nsIException* mException;
    LangType mCallingLangType;
    bool mErrorUnreported;

    
    PRCList mScopes;
};



#define NATIVE_CALLER  XPCContext::LANG_NATIVE
#define JS_CALLER      XPCContext::LANG_JS
















class MOZ_STACK_CLASS XPCCallContext : public nsAXPCNativeCallContext
{
public:
    NS_IMETHOD GetCallee(nsISupports **aResult);
    NS_IMETHOD GetCalleeMethodIndex(uint16_t *aResult);
    NS_IMETHOD GetCalleeWrapper(nsIXPConnectWrappedNative **aResult);
    NS_IMETHOD GetJSContext(JSContext **aResult);
    NS_IMETHOD GetArgc(uint32_t *aResult);
    NS_IMETHOD GetArgvPtr(jsval **aResult);
    NS_IMETHOD GetCalleeInterface(nsIInterfaceInfo **aResult);
    NS_IMETHOD GetCalleeClassInfo(nsIClassInfo **aResult);
    NS_IMETHOD GetPreviousCallContext(nsAXPCNativeCallContext **aResult);
    NS_IMETHOD GetLanguage(uint16_t *aResult);

    enum {NO_ARGS = (unsigned) -1};

    static JSContext* GetDefaultJSContext();

    XPCCallContext(XPCContext::LangType callerLanguage,
                   JSContext* cx           = GetDefaultJSContext(),
                   JS::HandleObject obj    = JS::NullPtr(),
                   JS::HandleObject funobj = JS::NullPtr(),
                   JS::HandleId id         = JSID_VOIDHANDLE,
                   unsigned argc           = NO_ARGS,
                   jsval *argv             = nullptr,
                   jsval *rval             = nullptr);

    virtual ~XPCCallContext();

    inline bool                         IsValid() const ;

    inline XPCJSRuntime*                GetRuntime() const ;
    inline XPCContext*                  GetXPCContext() const ;
    inline JSContext*                   GetJSContext() const ;
    inline bool                         GetContextPopRequired() const ;
    inline XPCContext::LangType         GetCallerLanguage() const ;
    inline XPCContext::LangType         GetPrevCallerLanguage() const ;
    inline XPCCallContext*              GetPrevCallContext() const ;

    inline JSObject*                    GetFlattenedJSObject() const ;
    inline nsISupports*                 GetIdentityObject() const ;
    inline XPCWrappedNative*            GetWrapper() const ;
    inline XPCWrappedNativeProto*       GetProto() const ;

    inline bool                         CanGetTearOff() const ;
    inline XPCWrappedNativeTearOff*     GetTearOff() const ;

    inline XPCNativeScriptableInfo*     GetScriptableInfo() const ;
    inline bool                         CanGetSet() const ;
    inline XPCNativeSet*                GetSet() const ;
    inline bool                         CanGetInterface() const ;
    inline XPCNativeInterface*          GetInterface() const ;
    inline XPCNativeMember*             GetMember() const ;
    inline bool                         HasInterfaceAndMember() const ;
    inline jsid                         GetName() const ;
    inline bool                         GetStaticMemberIsLocal() const ;
    inline unsigned                     GetArgc() const ;
    inline jsval*                       GetArgv() const ;
    inline jsval*                       GetRetVal() const ;

    inline uint16_t                     GetMethodIndex() const ;
    inline void                         SetMethodIndex(uint16_t index) ;

    inline jsid GetResolveName() const;
    inline jsid SetResolveName(JS::HandleId name);

    inline XPCWrappedNative* GetResolvingWrapper() const;
    inline XPCWrappedNative* SetResolvingWrapper(XPCWrappedNative* w);

    inline void SetRetVal(jsval val);

    void SetName(jsid name);
    void SetArgsAndResultPtr(unsigned argc, jsval *argv, jsval *rval);
    void SetCallInfo(XPCNativeInterface* iface, XPCNativeMember* member,
                     bool isSetter);

    nsresult  CanCallNow();

    void SystemIsBeingShutDown();

    operator JSContext*() const {return GetJSContext();}

private:

    
    XPCCallContext(const XPCCallContext& r); 
    XPCCallContext& operator= (const XPCCallContext& r); 

    XPCWrappedNative* UnwrapThisIfAllowed(JS::HandleObject obj, JS::HandleObject fun,
                                          unsigned argc);

private:
    
    enum State {
        INIT_FAILED,
        SYSTEM_SHUTDOWN,
        HAVE_CONTEXT,
        HAVE_OBJECT,
        HAVE_NAME,
        HAVE_ARGS,
        READY_TO_CALL,
        CALL_DONE
    };

#ifdef DEBUG
inline void CHECK_STATE(int s) const {MOZ_ASSERT(mState >= s, "bad state");}
#else
#define CHECK_STATE(s) ((void)0)
#endif

private:
    mozilla::AutoPushJSContext      mPusher;
    State                           mState;

    nsRefPtr<nsXPConnect>           mXPC;

    XPCContext*                     mXPCContext;
    JSContext*                      mJSContext;

    XPCContext::LangType            mCallerLanguage;

    

    XPCContext::LangType            mPrevCallerLanguage;

    XPCCallContext*                 mPrevCallContext;

    JS::RootedObject                mFlattenedJSObject;
    XPCWrappedNative*               mWrapper;
    XPCWrappedNativeTearOff*        mTearOff;

    XPCNativeScriptableInfo*        mScriptableInfo;

    XPCNativeSet*                   mSet;
    XPCNativeInterface*             mInterface;
    XPCNativeMember*                mMember;

    JS::RootedId                    mName;
    bool                            mStaticMemberIsLocal;

    unsigned                        mArgc;
    jsval*                          mArgv;
    jsval*                          mRetVal;

    uint16_t                        mMethodIndex;
};












struct XPCWrappedNativeJSClass;
extern const XPCWrappedNativeJSClass XPC_WN_NoHelper_JSClass;
extern const js::Class XPC_WN_NoMods_WithCall_Proto_JSClass;
extern const js::Class XPC_WN_NoMods_NoCall_Proto_JSClass;
extern const js::Class XPC_WN_ModsAllowed_WithCall_Proto_JSClass;
extern const js::Class XPC_WN_ModsAllowed_NoCall_Proto_JSClass;
extern const js::Class XPC_WN_Tearoff_JSClass;
extern const js::Class XPC_WN_NoHelper_Proto_JSClass;

extern bool
XPC_WN_CallMethod(JSContext *cx, unsigned argc, jsval *vp);

extern bool
XPC_WN_GetterSetter(JSContext *cx, unsigned argc, jsval *vp);

extern bool
XPC_WN_JSOp_Enumerate(JSContext *cx, JS::HandleObject obj, JSIterateOp enum_op,
                      JS::MutableHandleValue statep, JS::MutableHandleId idp);

extern JSObject*
XPC_WN_JSOp_ThisObject(JSContext *cx, JS::HandleObject obj);


#define XPC_WN_WithCall_ObjectOps                                             \
    {                                                                         \
        nullptr, /* lookupGeneric */                                          \
        nullptr, /* lookupProperty */                                         \
        nullptr, /* lookupElement */                                          \
        nullptr, /* lookupSpecial */                                          \
        nullptr, /* defineGeneric */                                          \
        nullptr, /* defineProperty */                                         \
        nullptr, /* defineElement */                                          \
        nullptr, /* defineSpecial */                                          \
        nullptr, /* getGeneric    */                                          \
        nullptr, /* getProperty    */                                         \
        nullptr, /* getElement    */                                          \
        nullptr, /* getElementIfPresent */                                    \
        nullptr, /* getSpecial    */                                          \
        nullptr, /* setGeneric    */                                          \
        nullptr, /* setProperty    */                                         \
        nullptr, /* setElement    */                                          \
        nullptr, /* setSpecial    */                                          \
        nullptr, /* getGenericAttributes  */                                  \
        nullptr, /* setGenericAttributes  */                                  \
        nullptr, /* deleteProperty */                                         \
        nullptr, /* deleteElement */                                          \
        nullptr, /* deleteSpecial */                                          \
        XPC_WN_JSOp_Enumerate,                                                \
        XPC_WN_JSOp_ThisObject,                                               \
    }

#define XPC_WN_NoCall_ObjectOps                                               \
    {                                                                         \
        nullptr, /* lookupGeneric */                                          \
        nullptr, /* lookupProperty */                                         \
        nullptr, /* lookupElement */                                          \
        nullptr, /* lookupSpecial */                                          \
        nullptr, /* defineGeneric */                                          \
        nullptr, /* defineProperty */                                         \
        nullptr, /* defineElement */                                          \
        nullptr, /* defineSpecial */                                          \
        nullptr, /* getGeneric    */                                          \
        nullptr, /* getProperty    */                                         \
        nullptr, /* getElement    */                                          \
        nullptr, /* getElementIfPresent */                                    \
        nullptr, /* getSpecial    */                                          \
        nullptr, /* setGeneric    */                                          \
        nullptr, /* setProperty    */                                         \
        nullptr, /* setElement    */                                          \
        nullptr, /* setSpecial    */                                          \
        nullptr, /* getGenericAttributes  */                                  \
        nullptr, /* setGenericAttributes  */                                  \
        nullptr, /* deleteProperty */                                         \
        nullptr, /* deleteElement */                                          \
        nullptr, /* deleteSpecial */                                          \
        XPC_WN_JSOp_Enumerate,                                                \
        XPC_WN_JSOp_ThisObject,                                               \
    }




static inline bool IS_PROTO_CLASS(const js::Class *clazz)
{
    return clazz == &XPC_WN_NoMods_WithCall_Proto_JSClass ||
           clazz == &XPC_WN_NoMods_NoCall_Proto_JSClass ||
           clazz == &XPC_WN_ModsAllowed_WithCall_Proto_JSClass ||
           clazz == &XPC_WN_ModsAllowed_NoCall_Proto_JSClass;
}



namespace XPCWrapper {

enum WrapperType {
    UNKNOWN         = 0,
    NONE            = 0,
    XPCNW_IMPLICIT  = 1 << 0,
    XPCNW_EXPLICIT  = 1 << 1,
    XPCNW           = (XPCNW_IMPLICIT | XPCNW_EXPLICIT),
    SJOW            = 1 << 2,
    

    XOW             = 1 << 3,
    COW             = 1 << 4,
    SOW             = 1 << 5
};

}




class XPCWrappedNativeScope : public PRCList
{
public:

    static XPCWrappedNativeScope*
    GetNewOrUsed(JSContext *cx, JS::HandleObject aGlobal);

    XPCJSRuntime*
    GetRuntime() const {return XPCJSRuntime::Get();}

    Native2WrappedNativeMap*
    GetWrappedNativeMap() const {return mWrappedNativeMap;}

    ClassInfo2WrappedNativeProtoMap*
    GetWrappedNativeProtoMap(bool aMainThreadOnly) const
        {return aMainThreadOnly ?
                mMainThreadWrappedNativeProtoMap :
                mWrappedNativeProtoMap;}

    nsXPCComponents*
    GetComponents() const {return mComponents;}

    
    JSObject*
    GetComponentsJSObject();

    JSObject*
    GetGlobalJSObject() const {
        JS::ExposeObjectToActiveJS(mGlobalJSObject);
        return mGlobalJSObject;
    }

    JSObject*
    GetGlobalJSObjectPreserveColor() const {return mGlobalJSObject;}

    nsIPrincipal*
    GetPrincipal() const {
        JSCompartment *c = js::GetObjectCompartment(mGlobalJSObject);
        return nsJSPrincipals::get(JS_GetCompartmentPrincipals(c));
    }

    void RemoveWrappedNativeProtos();

    static void
    SystemIsBeingShutDown();

    static void
    TraceWrappedNativesInAllScopes(JSTracer* trc, XPCJSRuntime* rt);

    void TraceSelf(JSTracer *trc) {
        MOZ_ASSERT(mGlobalJSObject);
        mGlobalJSObject.trace(trc, "XPCWrappedNativeScope::mGlobalJSObject");
        if (mXBLScope)
            mXBLScope.trace(trc, "XPCWrappedNativeScope::mXBLScope");
    }

    static void
    SuspectAllWrappers(XPCJSRuntime* rt, nsCycleCollectionNoteRootCallback &cb);

    static void
    StartFinalizationPhaseOfGC(JSFreeOp *fop, XPCJSRuntime* rt);

    static void
    FinishedFinalizationPhaseOfGC();

    static void
    MarkAllWrappedNativesAndProtos();

    static nsresult
    ClearAllWrappedNativeSecurityPolicies();

#ifdef DEBUG
    static void
    ASSERT_NoInterfaceSetsAreMarked();
#endif

    static void
    SweepAllWrappedNativeTearOffs();

    static void
    DebugDumpAllScopes(int16_t depth);

    void
    DebugDump(int16_t depth);

    static size_t
    SizeOfAllScopesIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    size_t
    SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    bool
    IsValid() const {return mRuntime != nullptr;}

    static bool
    IsDyingScope(XPCWrappedNativeScope *scope);

    static void InitStatics() { gScopes = nullptr; gDyingScopes = nullptr; }

    XPCContext *GetContext() { return mContext; }
    void ClearContext() { mContext = nullptr; }

    typedef js::HashSet<JSObject *,
                        js::PointerHasher<JSObject *, 3>,
                        js::SystemAllocPolicy> DOMExpandoSet;

    bool RegisterDOMExpandoObject(JSObject *expando) {
        
        JS::AssertGCThingMustBeTenured(expando);
        if (!mDOMExpandoSet) {
            mDOMExpandoSet = new DOMExpandoSet();
            mDOMExpandoSet->init(8);
        }
        return mDOMExpandoSet->put(expando);
    }
    void RemoveDOMExpandoObject(JSObject *expando) {
        if (mDOMExpandoSet)
            mDOMExpandoSet->remove(expando);
    }

    
    
    
    JSObject *EnsureXBLScope(JSContext *cx);

    XPCWrappedNativeScope(JSContext *cx, JS::HandleObject aGlobal);

    nsAutoPtr<JSObject2JSObjectMap> mWaiverWrapperMap;

    bool IsXBLScope() { return mIsXBLScope; }
    bool AllowXBLScope();

protected:
    virtual ~XPCWrappedNativeScope();

    static void KillDyingScopes();

    XPCWrappedNativeScope(); 

private:
    static XPCWrappedNativeScope* gScopes;
    static XPCWrappedNativeScope* gDyingScopes;

    XPCJSRuntime*                    mRuntime;
    Native2WrappedNativeMap*         mWrappedNativeMap;
    ClassInfo2WrappedNativeProtoMap* mWrappedNativeProtoMap;
    ClassInfo2WrappedNativeProtoMap* mMainThreadWrappedNativeProtoMap;
    nsRefPtr<nsXPCComponents>        mComponents;
    XPCWrappedNativeScope*           mNext;
    
    
    
    
    JS::ObjectPtr                    mGlobalJSObject;

    
    
    
    JS::ObjectPtr                    mXBLScope;

    XPCContext*                      mContext;

    nsAutoPtr<DOMExpandoSet> mDOMExpandoSet;

    bool mIsXBLScope;

    
    
    
    
    
    
    
    
    
    
    
    bool mAllowXBLScope;
    bool mUseXBLScope;
};







class XPCNativeMember
{
public:
    static bool GetCallInfo(JSObject* funobj,
                            XPCNativeInterface** pInterface,
                            XPCNativeMember**    pMember);

    jsid   GetName() const {return mName;}

    uint16_t GetIndex() const {return mIndex;}

    bool GetConstantValue(XPCCallContext& ccx, XPCNativeInterface* iface,
                          jsval* pval)
        {MOZ_ASSERT(IsConstant(),
                    "Only call this if you're sure this is a constant!");
         return Resolve(ccx, iface, JS::NullPtr(), pval);}

    bool NewFunctionObject(XPCCallContext& ccx, XPCNativeInterface* iface,
                           JS::HandleObject parent, jsval* pval);

    bool IsMethod() const
        {return 0 != (mFlags & METHOD);}

    bool IsConstant() const
        {return 0 != (mFlags & CONSTANT);}

    bool IsAttribute() const
        {return 0 != (mFlags & GETTER);}

    bool IsWritableAttribute() const
        {return 0 != (mFlags & SETTER_TOO);}

    bool IsReadOnlyAttribute() const
        {return IsAttribute() && !IsWritableAttribute();}


    void SetName(jsid a) {mName = a;}

    void SetMethod(uint16_t index)
        {mFlags = METHOD; mIndex = index;}

    void SetConstant(uint16_t index)
        {mFlags = CONSTANT; mIndex = index;}

    void SetReadOnlyAttribute(uint16_t index)
        {mFlags = GETTER; mIndex = index;}

    void SetWritableAttribute()
        {MOZ_ASSERT(mFlags == GETTER,"bad"); mFlags = GETTER | SETTER_TOO;}

    
    XPCNativeMember()  {MOZ_COUNT_CTOR(XPCNativeMember);}
    ~XPCNativeMember() {MOZ_COUNT_DTOR(XPCNativeMember);}

private:
    bool Resolve(XPCCallContext& ccx, XPCNativeInterface* iface,
                 JS::HandleObject parent, jsval *vp);

    enum {
        METHOD      = 0x01,
        CONSTANT    = 0x02,
        GETTER      = 0x04,
        SETTER_TOO  = 0x08
    };

private:
    
    jsid     mName;
    uint16_t mIndex;
    uint16_t mFlags;
};







class XPCNativeInterface
{
  public:
    static XPCNativeInterface* GetNewOrUsed(const nsIID* iid);
    static XPCNativeInterface* GetNewOrUsed(nsIInterfaceInfo* info);
    static XPCNativeInterface* GetNewOrUsed(const char* name);
    static XPCNativeInterface* GetISupports();

    inline nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo.get();}
    inline jsid              GetName()          const {return mName;}

    inline const nsIID* GetIID() const;
    inline const char*  GetNameString() const;
    inline XPCNativeMember* FindMember(jsid name) const;

    inline bool HasAncestor(const nsIID* iid) const;

    uint16_t GetMemberCount() const {
        return mMemberCount;
    }
    XPCNativeMember* GetMemberAt(uint16_t i) {
        MOZ_ASSERT(i < mMemberCount, "bad index");
        return &mMembers[i];
    }

    void DebugDump(int16_t depth);

#define XPC_NATIVE_IFACE_MARK_FLAG ((uint16_t)JS_BIT(15)) // only high bit of 16 is set

    void Mark() {
        mMarked = 1;
    }

    void Unmark() {
        mMarked = 0;
    }

    bool IsMarked() const {
        return mMarked != 0;
    }

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

    static void DestroyInstance(XPCNativeInterface* inst);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

  protected:
    static XPCNativeInterface* NewInstance(nsIInterfaceInfo* aInfo);

    XPCNativeInterface();   
    XPCNativeInterface(nsIInterfaceInfo* aInfo, jsid aName)
      : mInfo(aInfo), mName(aName), mMemberCount(0), mMarked(0)
    {
        MOZ_COUNT_CTOR(XPCNativeInterface);
    }
    ~XPCNativeInterface() {
        MOZ_COUNT_DTOR(XPCNativeInterface);
    }

    void* operator new(size_t, void* p) CPP_THROW_NEW {return p;}

    XPCNativeInterface(const XPCNativeInterface& r); 
    XPCNativeInterface& operator= (const XPCNativeInterface& r); 

private:
    nsCOMPtr<nsIInterfaceInfo> mInfo;
    jsid                       mName;
    uint16_t                   mMemberCount : 15;
    uint16_t                   mMarked : 1;
    XPCNativeMember            mMembers[1]; 
};




class XPCNativeSetKey
{
public:
    XPCNativeSetKey(XPCNativeSet*       BaseSet  = nullptr,
                    XPCNativeInterface* Addition = nullptr,
                    uint16_t            Position = 0)
        : mIsAKey(IS_A_KEY), mPosition(Position), mBaseSet(BaseSet),
          mAddition(Addition) {}
    ~XPCNativeSetKey() {}

    XPCNativeSet*           GetBaseSet()  const {return mBaseSet;}
    XPCNativeInterface*     GetAddition() const {return mAddition;}
    uint16_t                GetPosition() const {return mPosition;}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    bool                    IsAKey() const {return mIsAKey == IS_A_KEY;}

    enum {IS_A_KEY = 0xffff};

    

private:
    uint16_t                mIsAKey;    
    uint16_t                mPosition;
    XPCNativeSet*           mBaseSet;
    XPCNativeInterface*     mAddition;
};




class XPCNativeSet
{
  public:
    static XPCNativeSet* GetNewOrUsed(const nsIID* iid);
    static XPCNativeSet* GetNewOrUsed(nsIClassInfo* classInfo);
    static XPCNativeSet* GetNewOrUsed(XPCNativeSet* otherSet,
                                      XPCNativeInterface* newInterface,
                                      uint16_t position);

    
    
    
    
    
    
    
    static XPCNativeSet* GetNewOrUsed(XPCNativeSet* firstSet,
                                      XPCNativeSet* secondSet,
                                      bool preserveFirstSetOrder);

    static void ClearCacheEntryForClassInfo(nsIClassInfo* classInfo);

    inline bool FindMember(jsid name, XPCNativeMember** pMember,
                             uint16_t* pInterfaceIndex) const;

    inline bool FindMember(jsid name, XPCNativeMember** pMember,
                             XPCNativeInterface** pInterface) const;

    inline bool FindMember(jsid name,
                             XPCNativeMember** pMember,
                             XPCNativeInterface** pInterface,
                             XPCNativeSet* protoSet,
                             bool* pIsLocal) const;

    inline bool HasInterface(XPCNativeInterface* aInterface) const;
    inline bool HasInterfaceWithAncestor(XPCNativeInterface* aInterface) const;
    inline bool HasInterfaceWithAncestor(const nsIID* iid) const;

    inline XPCNativeInterface* FindInterfaceWithIID(const nsIID& iid) const;

    inline XPCNativeInterface* FindNamedInterface(jsid name) const;

    uint16_t GetMemberCount() const {
        return mMemberCount;
    }
    uint16_t GetInterfaceCount() const {
        return mInterfaceCount;
    }
    XPCNativeInterface **GetInterfaceArray() {
        return mInterfaces;
    }

    XPCNativeInterface* GetInterfaceAt(uint16_t i)
        {MOZ_ASSERT(i < mInterfaceCount, "bad index"); return mInterfaces[i];}

    inline bool MatchesSetUpToInterface(const XPCNativeSet* other,
                                          XPCNativeInterface* iface) const;

#define XPC_NATIVE_SET_MARK_FLAG ((uint16_t)JS_BIT(15)) // only high bit of 16 is set

    inline void Mark();

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

  private:
    void MarkSelfOnly() {
        mMarked = 1;
    }

  public:
    void Unmark() {
        mMarked = 0;
    }
    bool IsMarked() const {
        return !!mMarked;
    }

#ifdef DEBUG
    inline void ASSERT_NotMarked();
#endif

    void DebugDump(int16_t depth);

    static void DestroyInstance(XPCNativeSet* inst);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

  protected:
    static XPCNativeSet* NewInstance(XPCNativeInterface** array,
                                     uint16_t count);
    static XPCNativeSet* NewInstanceMutate(XPCNativeSet*       otherSet,
                                           XPCNativeInterface* newInterface,
                                           uint16_t            position);
    XPCNativeSet()
      : mMemberCount(0), mInterfaceCount(0), mMarked(0)
    {
        MOZ_COUNT_CTOR(XPCNativeSet);
    }
    ~XPCNativeSet() {
        MOZ_COUNT_DTOR(XPCNativeSet);
    }
    void* operator new(size_t, void* p) CPP_THROW_NEW {return p;}

  private:
    uint16_t                mMemberCount;
    uint16_t                mInterfaceCount : 15;
    uint16_t                mMarked : 1;
    XPCNativeInterface*     mInterfaces[1];  
};








#define XPC_WN_SJSFLAGS_MARK_FLAG JS_BIT(31) // only high bit of 32 is set

class XPCNativeScriptableFlags
{
private:
    uint32_t mFlags;

public:

    XPCNativeScriptableFlags(uint32_t flags = 0) : mFlags(flags) {}

    uint32_t GetFlags() const {return mFlags & ~XPC_WN_SJSFLAGS_MARK_FLAG;}
    void     SetFlags(uint32_t flags) {mFlags = flags;}

    operator uint32_t() const {return GetFlags();}

    XPCNativeScriptableFlags(const XPCNativeScriptableFlags& r)
        {mFlags = r.GetFlags();}

    XPCNativeScriptableFlags& operator= (const XPCNativeScriptableFlags& r)
        {mFlags = r.GetFlags(); return *this;}

    void Mark()       {mFlags |= XPC_WN_SJSFLAGS_MARK_FLAG;}
    void Unmark()     {mFlags &= ~XPC_WN_SJSFLAGS_MARK_FLAG;}
    bool IsMarked() const {return 0 != (mFlags & XPC_WN_SJSFLAGS_MARK_FLAG);}

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return 0 != (mFlags & nsIXPCScriptable:: f_ );}

    bool WantPreCreate()                GET_IT(WANT_PRECREATE)
    bool WantCreate()                   GET_IT(WANT_CREATE)
    bool WantPostCreate()               GET_IT(WANT_POSTCREATE)
    bool WantAddProperty()              GET_IT(WANT_ADDPROPERTY)
    bool WantDelProperty()              GET_IT(WANT_DELPROPERTY)
    bool WantGetProperty()              GET_IT(WANT_GETPROPERTY)
    bool WantSetProperty()              GET_IT(WANT_SETPROPERTY)
    bool WantEnumerate()                GET_IT(WANT_ENUMERATE)
    bool WantNewEnumerate()             GET_IT(WANT_NEWENUMERATE)
    bool WantNewResolve()               GET_IT(WANT_NEWRESOLVE)
    bool WantConvert()                  GET_IT(WANT_CONVERT)
    bool WantFinalize()                 GET_IT(WANT_FINALIZE)
    bool WantCheckAccess()              GET_IT(WANT_CHECKACCESS)
    bool WantCall()                     GET_IT(WANT_CALL)
    bool WantConstruct()                GET_IT(WANT_CONSTRUCT)
    bool WantHasInstance()              GET_IT(WANT_HASINSTANCE)
    bool WantOuterObject()              GET_IT(WANT_OUTER_OBJECT)
    bool UseJSStubForAddProperty()      GET_IT(USE_JSSTUB_FOR_ADDPROPERTY)
    bool UseJSStubForDelProperty()      GET_IT(USE_JSSTUB_FOR_DELPROPERTY)
    bool UseJSStubForSetProperty()      GET_IT(USE_JSSTUB_FOR_SETPROPERTY)
    bool DontEnumStaticProps()          GET_IT(DONT_ENUM_STATIC_PROPS)
    bool DontEnumQueryInterface()       GET_IT(DONT_ENUM_QUERY_INTERFACE)
    bool DontAskInstanceForScriptable() GET_IT(DONT_ASK_INSTANCE_FOR_SCRIPTABLE)
    bool ClassInfoInterfacesOnly()      GET_IT(CLASSINFO_INTERFACES_ONLY)
    bool AllowPropModsDuringResolve()   GET_IT(ALLOW_PROP_MODS_DURING_RESOLVE)
    bool AllowPropModsToPrototype()     GET_IT(ALLOW_PROP_MODS_TO_PROTOTYPE)
    bool IsGlobalObject()               GET_IT(IS_GLOBAL_OBJECT)
    bool DontReflectInterfaceNames()    GET_IT(DONT_REFLECT_INTERFACE_NAMES)

#undef GET_IT
};














struct XPCWrappedNativeJSClass
{
    js::Class base;
    uint32_t interfacesBitmap;
};

class XPCNativeScriptableShared
{
public:
    const XPCNativeScriptableFlags& GetFlags() const {return mFlags;}
    uint32_t                        GetInterfacesBitmap() const
        {return mJSClass.interfacesBitmap;}
    const JSClass*                  GetJSClass()
        {return Jsvalify(&mJSClass.base);}

    XPCNativeScriptableShared(uint32_t aFlags, char* aName,
                              uint32_t interfacesBitmap)
        : mFlags(aFlags)
        {memset(&mJSClass, 0, sizeof(mJSClass));
         mJSClass.base.name = aName;  
         mJSClass.interfacesBitmap = interfacesBitmap;
         MOZ_COUNT_CTOR(XPCNativeScriptableShared);}

    ~XPCNativeScriptableShared()
        {if (mJSClass.base.name)nsMemory::Free((void*)mJSClass.base.name);
         MOZ_COUNT_DTOR(XPCNativeScriptableShared);}

    char* TransferNameOwnership()
        {char* name=(char*)mJSClass.base.name; mJSClass.base.name = nullptr;
        return name;}

    void PopulateJSClass();

    void Mark()       {mFlags.Mark();}
    void Unmark()     {mFlags.Unmark();}
    bool IsMarked() const {return mFlags.IsMarked();}

private:
    XPCNativeScriptableFlags mFlags;
    XPCWrappedNativeJSClass  mJSClass;
};





class XPCNativeScriptableInfo
{
public:
    static XPCNativeScriptableInfo*
    Construct(const XPCNativeScriptableCreateInfo* sci);

    nsIXPCScriptable*
    GetCallback() const {return mCallback;}

    const XPCNativeScriptableFlags&
    GetFlags() const      {return mShared->GetFlags();}

    uint32_t
    GetInterfacesBitmap() const {return mShared->GetInterfacesBitmap();}

    const JSClass*
    GetJSClass()          {return mShared->GetJSClass();}

    XPCNativeScriptableShared*
    GetScriptableShared() {return mShared;}

    void
    SetCallback(nsIXPCScriptable* s) {mCallback = s;}
    void
    SetCallback(already_AddRefed<nsIXPCScriptable> s) {mCallback = s;}

    void
    SetScriptableShared(XPCNativeScriptableShared* shared) {mShared = shared;}

    void Mark() {
        if (mShared)
            mShared->Mark();
    }

    void TraceJS(JSTracer *trc) {}
    void AutoTrace(JSTracer *trc) {}

protected:
    XPCNativeScriptableInfo(nsIXPCScriptable* scriptable = nullptr,
                            XPCNativeScriptableShared* shared = nullptr)
        : mCallback(scriptable), mShared(shared)
                               {MOZ_COUNT_CTOR(XPCNativeScriptableInfo);}
public:
    ~XPCNativeScriptableInfo() {MOZ_COUNT_DTOR(XPCNativeScriptableInfo);}
private:

    
    XPCNativeScriptableInfo(const XPCNativeScriptableInfo& r); 
    XPCNativeScriptableInfo& operator= (const XPCNativeScriptableInfo& r); 

private:
    nsCOMPtr<nsIXPCScriptable>  mCallback;
    XPCNativeScriptableShared*  mShared;
};






class MOZ_STACK_CLASS XPCNativeScriptableCreateInfo
{
public:

    XPCNativeScriptableCreateInfo(const XPCNativeScriptableInfo& si)
        : mCallback(si.GetCallback()), mFlags(si.GetFlags()),
          mInterfacesBitmap(si.GetInterfacesBitmap()) {}

    XPCNativeScriptableCreateInfo(already_AddRefed<nsIXPCScriptable> callback,
                                  XPCNativeScriptableFlags flags,
                                  uint32_t interfacesBitmap)
        : mCallback(callback), mFlags(flags),
          mInterfacesBitmap(interfacesBitmap) {}

    XPCNativeScriptableCreateInfo()
        : mFlags(0), mInterfacesBitmap(0) {}


    nsIXPCScriptable*
    GetCallback() const {return mCallback;}

    const XPCNativeScriptableFlags&
    GetFlags() const      {return mFlags;}

    uint32_t
    GetInterfacesBitmap() const     {return mInterfacesBitmap;}

    void
    SetCallback(already_AddRefed<nsIXPCScriptable> callback)
        {mCallback = callback;}

    void
    SetFlags(const XPCNativeScriptableFlags& flags)  {mFlags = flags;}

    void
    SetInterfacesBitmap(uint32_t interfacesBitmap)
        {mInterfacesBitmap = interfacesBitmap;}

private:
    nsCOMPtr<nsIXPCScriptable>  mCallback;
    XPCNativeScriptableFlags    mFlags;
    uint32_t                    mInterfacesBitmap;
};





class XPCWrappedNativeProto
{
public:
    static XPCWrappedNativeProto*
    GetNewOrUsed(XPCWrappedNativeScope* scope,
                 nsIClassInfo* classInfo,
                 const XPCNativeScriptableCreateInfo* scriptableCreateInfo,
                 bool callPostCreatePrototype = true);

    XPCWrappedNativeScope*
    GetScope()   const {return mScope;}

    XPCJSRuntime*
    GetRuntime() const {return mScope->GetRuntime();}

    JSObject*
    GetJSProtoObject() const {
        JS::ExposeObjectToActiveJS(mJSProtoObject);
        return mJSProtoObject;
    }

    nsIClassInfo*
    GetClassInfo()     const {return mClassInfo;}

    XPCNativeSet*
    GetSet()           const {return mSet;}

    XPCNativeScriptableInfo*
    GetScriptableInfo()   {return mScriptableInfo;}

    void**
    GetSecurityInfoAddr() {return &mSecurityInfo;}

    uint32_t
    GetClassInfoFlags() const {return mClassInfoFlags;}

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return !!(mClassInfoFlags & nsIClassInfo:: f_ );}

    bool ClassIsSingleton()           GET_IT(SINGLETON)
    bool ClassIsThreadSafe()          GET_IT(THREADSAFE)
    bool ClassIsMainThreadOnly()      GET_IT(MAIN_THREAD_ONLY)
    bool ClassIsDOMObject()           GET_IT(DOM_OBJECT)
    bool ClassIsPluginObject()        GET_IT(PLUGIN_OBJECT)

#undef GET_IT

    XPCLock* GetLock() const
        {return ClassIsThreadSafe() ? GetRuntime()->GetMapLock() : nullptr;}

    void SetScriptableInfo(XPCNativeScriptableInfo* si)
        {MOZ_ASSERT(!mScriptableInfo, "leak here!"); mScriptableInfo = si;}

    bool CallPostCreatePrototype();
    void JSProtoObjectFinalized(js::FreeOp *fop, JSObject *obj);

    void SystemIsBeingShutDown();

    void DebugDump(int16_t depth);

    void TraceSelf(JSTracer *trc) {
        if (mJSProtoObject)
            mJSProtoObject.trace(trc, "XPCWrappedNativeProto::mJSProtoObject");
    }

    void TraceInside(JSTracer *trc) {
        if (JS_IsGCMarkingTracer(trc)) {
            mSet->Mark();
            if (mScriptableInfo)
                mScriptableInfo->Mark();
        }

        GetScope()->TraceSelf(trc);
    }

    void TraceJS(JSTracer *trc) {
        TraceSelf(trc);
        TraceInside(trc);
    }

    void WriteBarrierPre(JSRuntime* rt)
    {
        if (JS::IsIncrementalBarrierNeeded(rt) && mJSProtoObject)
            mJSProtoObject.writeBarrierPre(rt);
    }

    
    inline void AutoTrace(JSTracer* trc) {}

    
    void Mark() const
        {mSet->Mark();
         if (mScriptableInfo) mScriptableInfo->Mark();}

#ifdef DEBUG
    void ASSERT_SetNotMarked() const {mSet->ASSERT_NotMarked();}
#endif

    ~XPCWrappedNativeProto();

protected:
    
    XPCWrappedNativeProto(const XPCWrappedNativeProto& r); 
    XPCWrappedNativeProto& operator= (const XPCWrappedNativeProto& r); 

    
    XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                          nsIClassInfo* ClassInfo,
                          uint32_t ClassInfoFlags,
                          XPCNativeSet* Set);

    bool Init(const XPCNativeScriptableCreateInfo* scriptableCreateInfo,
              bool callPostCreatePrototype);

private:
#if defined(DEBUG_xpc_hacker) || defined(DEBUG)
    static int32_t gDEBUG_LiveProtoCount;
#endif

private:
    XPCWrappedNativeScope*   mScope;
    JS::ObjectPtr            mJSProtoObject;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    uint32_t                 mClassInfoFlags;
    XPCNativeSet*            mSet;
    void*                    mSecurityInfo;
    XPCNativeScriptableInfo* mScriptableInfo;
};





class XPCWrappedNativeTearOff
{
public:
    bool IsAvailable() const {return mInterface == nullptr;}
    bool IsReserved()  const {return mInterface == (XPCNativeInterface*)1;}
    bool IsValid()     const {return !IsAvailable() && !IsReserved();}
    void   SetReserved()       {mInterface = (XPCNativeInterface*)1;}

    XPCNativeInterface* GetInterface() const {return mInterface;}
    nsISupports*        GetNative()    const {return mNative;}
    JSObject*           GetJSObject();
    JSObject*           GetJSObjectPreserveColor() const;
    void SetInterface(XPCNativeInterface*  Interface) {mInterface = Interface;}
    void SetNative(nsISupports*  Native)              {mNative = Native;}
    void SetJSObject(JSObject*  JSObj);

    void JSObjectFinalized() {SetJSObject(nullptr);}

    XPCWrappedNativeTearOff()
        : mInterface(nullptr), mNative(nullptr), mJSObject(nullptr) {}
    ~XPCWrappedNativeTearOff();

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

    void Mark()       {mJSObject = (JSObject*)(intptr_t(mJSObject) | 1);}
    void Unmark()     {mJSObject = (JSObject*)(intptr_t(mJSObject) & ~1);}
    bool IsMarked() const {return !!(intptr_t(mJSObject) & 1);}

private:
    XPCWrappedNativeTearOff(const XPCWrappedNativeTearOff& r) MOZ_DELETE;
    XPCWrappedNativeTearOff& operator= (const XPCWrappedNativeTearOff& r) MOZ_DELETE;

private:
    XPCNativeInterface* mInterface;
    nsISupports*        mNative;
    JSObject*           mJSObject;
};











#define XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK 1

class XPCWrappedNativeTearOffChunk
{
friend class XPCWrappedNative;
private:
    XPCWrappedNativeTearOffChunk() : mNextChunk(nullptr) {}
    ~XPCWrappedNativeTearOffChunk() {delete mNextChunk;}

private:
    XPCWrappedNativeTearOff mTearOffs[XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK];
    XPCWrappedNativeTearOffChunk* mNextChunk;
};

void *xpc_GetJSPrivate(JSObject *obj);





class XPCWrappedNative : public nsIXPConnectWrappedNative
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDNATIVE
    
    
    
    
    
    
    
    
    class NS_CYCLE_COLLECTION_INNERCLASS
     : public nsXPCOMCycleCollectionParticipant
    {
      NS_DECL_CYCLE_COLLECTION_CLASS_BODY(XPCWrappedNative, XPCWrappedNative)
      NS_IMETHOD Root(void *p) { return NS_OK; }
      NS_IMETHOD Unroot(void *p) { return NS_OK; }
      NS_IMPL_GET_XPCOM_CYCLE_COLLECTION_PARTICIPANT(XPCWrappedNative)
    };
    NS_CHECK_FOR_RIGHT_PARTICIPANT_IMPL(XPCWrappedNative);
    static NS_CYCLE_COLLECTION_INNERCLASS NS_CYCLE_COLLECTION_INNERNAME;

    void DeleteCycleCollectable() {}

    nsIPrincipal* GetObjectPrincipal() const;

    bool
    IsValid() const { return mFlatJSObject.hasFlag(FLAT_JS_OBJECT_VALID); }

#define XPC_SCOPE_WORD(s)   (intptr_t(s))
#define XPC_SCOPE_MASK      (intptr_t(0x3))
#define XPC_SCOPE_TAG       (intptr_t(0x1))
#define XPC_WRAPPER_EXPIRED (intptr_t(0x2))

    static inline bool
    IsTaggedScope(XPCWrappedNativeScope* s)
        {return XPC_SCOPE_WORD(s) & XPC_SCOPE_TAG;}

    static inline XPCWrappedNativeScope*
    TagScope(XPCWrappedNativeScope* s)
        {MOZ_ASSERT(!IsTaggedScope(s), "bad pointer!");
         return (XPCWrappedNativeScope*)(XPC_SCOPE_WORD(s) | XPC_SCOPE_TAG);}

    static inline XPCWrappedNativeScope*
    UnTagScope(XPCWrappedNativeScope* s)
        {return (XPCWrappedNativeScope*)(XPC_SCOPE_WORD(s) & ~XPC_SCOPE_TAG);}

    inline bool
    IsWrapperExpired() const
        {return XPC_SCOPE_WORD(mMaybeScope) & XPC_WRAPPER_EXPIRED;}

    bool
    HasProto() const {return !IsTaggedScope(mMaybeScope);}

    XPCWrappedNativeProto*
    GetProto() const
        {return HasProto() ?
         (XPCWrappedNativeProto*)
         (XPC_SCOPE_WORD(mMaybeProto) & ~XPC_SCOPE_MASK) : nullptr;}

    void SetProto(XPCWrappedNativeProto* p);

    XPCWrappedNativeScope*
    GetScope() const
        {return GetProto() ? GetProto()->GetScope() :
         (XPCWrappedNativeScope*)
         (XPC_SCOPE_WORD(mMaybeScope) & ~XPC_SCOPE_MASK);}

    nsISupports*
    GetIdentityObject() const {return mIdentity;}

    



    JSObject*
    GetFlatJSObject() const
    {
        JS::ExposeObjectToActiveJS(mFlatJSObject);
        return mFlatJSObject;
    }

    







    JSObject*
    GetFlatJSObjectPreserveColor() const {return mFlatJSObject;}

    XPCLock*
    GetLock() const {return IsValid() && HasProto() ?
                                GetProto()->GetLock() : nullptr;}

    XPCNativeSet*
    GetSet() const {XPCAutoLock al(GetLock()); return mSet;}

    void
    SetSet(XPCNativeSet* set) {XPCAutoLock al(GetLock()); mSet = set;}

    static XPCWrappedNative* Get(JSObject *obj) {
        MOZ_ASSERT(IS_WN_REFLECTOR(obj));
        return (XPCWrappedNative*)js::GetObjectPrivate(obj);
    }

private:
    inline void
    ExpireWrapper()
        {mMaybeScope = (XPCWrappedNativeScope*)
                       (XPC_SCOPE_WORD(mMaybeScope) | XPC_WRAPPER_EXPIRED);}

public:

    XPCNativeScriptableInfo*
    GetScriptableInfo() const {return mScriptableInfo;}

    nsIXPCScriptable*      
    GetScriptableCallback() const  {return mScriptableInfo->GetCallback();}

    void**
    GetSecurityInfoAddr() {return HasProto() ?
                                   GetProto()->GetSecurityInfoAddr() : nullptr;}

    nsIClassInfo*
    GetClassInfo() const {return IsValid() && HasProto() ?
                            GetProto()->GetClassInfo() : nullptr;}

    bool
    HasMutatedSet() const {return IsValid() &&
                                  (!HasProto() ||
                                   GetSet() != GetProto()->GetSet());}

    XPCJSRuntime*
    GetRuntime() const {XPCWrappedNativeScope* scope = GetScope();
                        return scope ? scope->GetRuntime() : nullptr;}

    static nsresult
    WrapNewGlobal(xpcObjectHelper &nativeHelper,
                  nsIPrincipal *principal, bool initStandardClasses,
                  JS::CompartmentOptions& aOptions,
                  XPCWrappedNative **wrappedGlobal);

    static nsresult
    GetNewOrUsed(xpcObjectHelper& helper,
                 XPCWrappedNativeScope* Scope,
                 XPCNativeInterface* Interface,
                 XPCWrappedNative** wrapper);

    static nsresult
    Morph(JS::HandleObject existingJSObject,
          XPCNativeInterface* Interface,
          nsWrapperCache *cache,
          XPCWrappedNative** resultWrapper);

public:
    static nsresult
    GetUsedOnly(nsISupports* Object,
                XPCWrappedNativeScope* Scope,
                XPCNativeInterface* Interface,
                XPCWrappedNative** wrapper);

    static nsresult
    ReparentWrapperIfFound(XPCWrappedNativeScope* aOldScope,
                           XPCWrappedNativeScope* aNewScope,
                           JS::HandleObject aNewParent,
                           nsISupports* aCOMObj);

    nsresult RescueOrphans();

    void FlatJSObjectFinalized();

    void SystemIsBeingShutDown();

    enum CallMode {CALL_METHOD, CALL_GETTER, CALL_SETTER};

    static bool CallMethod(XPCCallContext& ccx,
                           CallMode mode = CALL_METHOD);

    static bool GetAttribute(XPCCallContext& ccx)
        {return CallMethod(ccx, CALL_GETTER);}

    static bool SetAttribute(XPCCallContext& ccx)
        {return CallMethod(ccx, CALL_SETTER);}

    inline bool HasInterfaceNoQI(const nsIID& iid);

    XPCWrappedNativeTearOff* LocateTearOff(XPCNativeInterface* aInterface);
    XPCWrappedNativeTearOff* FindTearOff(XPCNativeInterface* aInterface,
                                         bool needJSObject = false,
                                         nsresult* pError = nullptr);
    void Mark() const
    {
        mSet->Mark();
        if (mScriptableInfo) mScriptableInfo->Mark();
        if (HasProto()) GetProto()->Mark();
    }

    
    inline void TraceInside(JSTracer *trc) {
        if (JS_IsGCMarkingTracer(trc)) {
            mSet->Mark();
            if (mScriptableInfo)
                mScriptableInfo->Mark();
        }
        if (HasProto())
            GetProto()->TraceSelf(trc);
        else
            GetScope()->TraceSelf(trc);
        TraceWrapper(trc);
        if (mFlatJSObject && JS_IsGlobalObject(mFlatJSObject))
        {
            TraceXPCGlobal(trc, mFlatJSObject);
        }
    }

    void TraceJS(JSTracer *trc) {
        TraceInside(trc);
    }

    void TraceSelf(JSTracer *trc) {
        
        
        
        
        
        if (mFlatJSObject) {
            JS_CallTenuredObjectTracer(trc, &mFlatJSObject,
                                       "XPCWrappedNative::mFlatJSObject");
        }
    }

    void AutoTrace(JSTracer *trc) {
        TraceSelf(trc);
    }

#ifdef DEBUG
    void ASSERT_SetsNotMarked() const
        {mSet->ASSERT_NotMarked();
         if (HasProto()){GetProto()->ASSERT_SetNotMarked();}}

    int DEBUG_CountOfTearoffChunks() const
        {int i = 0; const XPCWrappedNativeTearOffChunk* to;
         for (to = &mFirstChunk; to; to = to->mNextChunk) {i++;} return i;}
#endif

    inline void SweepTearOffs();

    
    char* ToString(XPCWrappedNativeTearOff* to = nullptr) const;

    static void GatherProtoScriptableCreateInfo(nsIClassInfo* classInfo,
                                                XPCNativeScriptableCreateInfo& sciProto);

    bool HasExternalReference() const {return mRefCnt > 1;}

    bool NeedsSOW() { return mWrapper.hasFlag(WRAPPER_NEEDS_SOW); }
    void SetNeedsSOW() { mWrapper.setFlags(WRAPPER_NEEDS_SOW); }
    bool NeedsCOW() { return mWrapper.hasFlag(WRAPPER_NEEDS_COW); }
    void SetNeedsCOW() { mWrapper.setFlags(WRAPPER_NEEDS_COW); }

    JSObject* GetWrapperPreserveColor() const { return mWrapper.getPtr(); }

    JSObject* GetWrapper()
    {
        JSObject* wrapper = GetWrapperPreserveColor();
        if (wrapper) {
            JS::ExposeObjectToActiveJS(wrapper);
            
            GetFlatJSObject();
        }
        return wrapper;
    }
    void SetWrapper(JSObject *obj)
    {
        JS::IncrementalObjectBarrier(GetWrapperPreserveColor());
        mWrapper.setPtr(obj);
    }

    void TraceWrapper(JSTracer *trc)
    {
        JS_CallTenuredObjectTracer(trc, &mWrapper, "XPCWrappedNative::mWrapper");
    }

    
    
    
    
    
    JSObject *GetSameCompartmentSecurityWrapper(JSContext *cx);

    void NoteTearoffs(nsCycleCollectionTraversalCallback& cb);

    
protected:
    XPCWrappedNative(); 

    
    XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                     XPCWrappedNativeProto* aProto);

    
    XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                     XPCWrappedNativeScope* aScope,
                     XPCNativeSet* aSet);

    virtual ~XPCWrappedNative();
    void Destroy();

    void UpdateScriptableInfo(XPCNativeScriptableInfo *si);

private:
    enum {
        
        WRAPPER_NEEDS_SOW = JS_BIT(0),
        WRAPPER_NEEDS_COW = JS_BIT(1),

        
        FLAT_JS_OBJECT_VALID = JS_BIT(0)
    };

private:

    bool Init(JS::HandleObject parent, const XPCNativeScriptableCreateInfo* sci);
    bool FinishInit();

    bool ExtendSet(XPCNativeInterface* aInterface);

    nsresult InitTearOff(XPCWrappedNativeTearOff* aTearOff,
                         XPCNativeInterface* aInterface,
                         bool needJSObject);

    bool InitTearOffJSObject(XPCWrappedNativeTearOff* to);

public:
    static const XPCNativeScriptableCreateInfo& GatherScriptableCreateInfo(nsISupports* obj,
                                                                           nsIClassInfo* classInfo,
                                                                           XPCNativeScriptableCreateInfo& sciProto,
                                                                           XPCNativeScriptableCreateInfo& sciWrapper);

private:
    union
    {
        XPCWrappedNativeScope*   mMaybeScope;
        XPCWrappedNativeProto*   mMaybeProto;
    };
    XPCNativeSet*                mSet;
    JS::TenuredHeap<JSObject*>   mFlatJSObject;
    XPCNativeScriptableInfo*     mScriptableInfo;
    XPCWrappedNativeTearOffChunk mFirstChunk;
    JS::TenuredHeap<JSObject*>   mWrapper;
};











#define NS_IXPCONNECT_WRAPPED_JS_CLASS_IID                                    \
{ 0x2453eba0, 0xa9b8, 0x11d2,                                                 \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedJSClass : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)
    NS_IMETHOD DebugDump(int16_t depth) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXPCWrappedJSClass,
                              NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)





class nsXPCWrappedJSClass : public nsIXPCWrappedJSClass
{
    
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_IMETHOD DebugDump(int16_t depth);
public:

    static nsresult
    GetNewOrUsed(JSContext* cx,
                 REFNSIID aIID,
                 nsXPCWrappedJSClass** clazz);

    REFNSIID GetIID() const {return mIID;}
    XPCJSRuntime* GetRuntime() const {return mRuntime;}
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}
    const char* GetInterfaceName();

    static bool IsWrappedJS(nsISupports* aPtr);

    NS_IMETHOD DelegatedQueryInterface(nsXPCWrappedJS* self, REFNSIID aIID,
                                       void** aInstancePtr);

    JSObject* GetRootJSObject(JSContext* cx, JSObject* aJSObj);

    NS_IMETHOD CallMethod(nsXPCWrappedJS* wrapper, uint16_t methodIndex,
                          const XPTMethodDescriptor* info,
                          nsXPTCMiniVariant* params);

    JSObject*  CallQueryInterfaceOnJSObject(JSContext* cx,
                                            JSObject* jsobj, REFNSIID aIID);

    static nsresult BuildPropertyEnumerator(XPCCallContext& ccx,
                                            JSObject* aJSObj,
                                            nsISimpleEnumerator** aEnumerate);

    static nsresult GetNamedPropertyAsVariant(XPCCallContext& ccx,
                                              JSObject* aJSObj,
                                              const nsAString& aName,
                                              nsIVariant** aResult);

    virtual ~nsXPCWrappedJSClass();

    static nsresult CheckForException(XPCCallContext & ccx,
                                      const char * aPropertyName,
                                      const char * anInterfaceName,
                                      bool aForceReport);
private:
    nsXPCWrappedJSClass();   
    nsXPCWrappedJSClass(JSContext* cx, REFNSIID aIID,
                        nsIInterfaceInfo* aInfo);

    bool IsReflectable(uint16_t i) const
        {return (bool)(mDescriptors[i/32] & (1 << (i%32)));}
    void SetReflectable(uint16_t i, bool b)
        {if (b) mDescriptors[i/32] |= (1 << (i%32));
         else mDescriptors[i/32] &= ~(1 << (i%32));}

    bool GetArraySizeFromParam(JSContext* cx,
                               const XPTMethodDescriptor* method,
                               const nsXPTParamInfo& param,
                               uint16_t methodIndex,
                               uint8_t paramIndex,
                               nsXPTCMiniVariant* params,
                               uint32_t* result);

    bool GetInterfaceTypeFromParam(JSContext* cx,
                                   const XPTMethodDescriptor* method,
                                   const nsXPTParamInfo& param,
                                   uint16_t methodIndex,
                                   const nsXPTType& type,
                                   nsXPTCMiniVariant* params,
                                   nsID* result);

    void CleanupPointerArray(const nsXPTType& datum_type,
                             uint32_t array_count,
                             void** arrayp);

    void CleanupPointerTypeObject(const nsXPTType& type,
                                  void** pp);

private:
    XPCJSRuntime* mRuntime;
    nsIInterfaceInfo* mInfo;
    char* mName;
    nsIID mIID;
    uint32_t* mDescriptors;
};






class nsXPCWrappedJS : protected nsAutoXPTCStub,
                       public nsIXPConnectWrappedJS,
                       public nsSupportsWeakReference,
                       public nsIPropertyBag,
                       public XPCRootSetElem
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDJS
    NS_DECL_NSISUPPORTSWEAKREFERENCE
    NS_DECL_NSIPROPERTYBAG

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXPCWrappedJS, nsIXPConnectWrappedJS)
    void DeleteCycleCollectable() {}

    NS_IMETHOD CallMethod(uint16_t methodIndex,
                          const XPTMethodDescriptor *info,
                          nsXPTCMiniVariant* params);

    





    static nsresult
    GetNewOrUsed(JS::HandleObject aJSObj,
                 REFNSIID aIID,
                 nsISupports* aOuter,
                 nsXPCWrappedJS** wrapper);

    nsISomeInterface* GetXPTCStub() { return mXPTCStub; }

    







    JSObject* GetJSObjectPreserveColor() const {return mJSObj;}

    nsXPCWrappedJSClass*  GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}
    nsXPCWrappedJS* GetRootWrapper() const {return mRoot;}
    nsXPCWrappedJS* GetNextWrapper() const {return mNext;}

    nsXPCWrappedJS* Find(REFNSIID aIID);
    nsXPCWrappedJS* FindInherited(REFNSIID aIID);

    bool IsValid() const {return mJSObj != nullptr;}
    void SystemIsBeingShutDown();

    
    
    
    
    
    
    bool IsSubjectToFinalization() const {return IsValid() && mRefCnt == 1;}
    bool IsObjectAboutToBeFinalized() {return JS_IsAboutToBeFinalized(&mJSObj);}

    bool IsAggregatedToNative() const {return mRoot->mOuter != nullptr;}
    nsISupports* GetAggregatedNativeObject() const {return mRoot->mOuter;}

    void SetIsMainThreadOnly() {
        MOZ_ASSERT(mMainThread);
        mMainThreadOnly = true;
    }
    bool IsMainThreadOnly() const {return mMainThreadOnly;}

    void TraceJS(JSTracer* trc);
    static void GetTraceName(JSTracer* trc, char *buf, size_t bufsize);

    virtual ~nsXPCWrappedJS();
protected:
    nsXPCWrappedJS();   
    nsXPCWrappedJS(JSContext* cx,
                   JSObject* aJSObj,
                   nsXPCWrappedJSClass* aClass,
                   nsXPCWrappedJS* root,
                   nsISupports* aOuter);

   void Unlink();

private:
    JS::Heap<JSObject*> mJSObj;
    nsXPCWrappedJSClass* mClass;
    nsXPCWrappedJS* mRoot;
    nsXPCWrappedJS* mNext;
    nsISupports* mOuter;    
    bool mMainThread;
    bool mMainThreadOnly;
};



class XPCJSObjectHolder : public nsIXPConnectJSObjectHolder,
                          public XPCRootSetElem
{
public:
    
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER

    

public:
    static XPCJSObjectHolder* newHolder(JSObject* obj);

    virtual ~XPCJSObjectHolder();

    void TraceJS(JSTracer *trc);
    static void GetTraceName(JSTracer* trc, char *buf, size_t bufsize);

private:
    XPCJSObjectHolder(JSObject* obj);
    XPCJSObjectHolder(); 

    JS::Heap<JSObject*> mJSObj;
};









class xpcProperty : public nsIProperty
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTY

  xpcProperty(const PRUnichar* aName, uint32_t aNameLen, nsIVariant* aValue);
  virtual ~xpcProperty() {}

private:
    nsString             mName;
    nsCOMPtr<nsIVariant> mValue;
};



class XPCConvert
{
public:
    static bool IsMethodReflectable(const XPTMethodDescriptor& info);

    











    static bool NativeData2JS(jsval* d,
                              const void* s, const nsXPTType& type,
                              const nsID* iid, nsresult* pErr);

    static bool JSData2Native(void* d, JS::HandleValue s,
                              const nsXPTType& type,
                              bool useAllocator, const nsID* iid,
                              nsresult* pErr);

    














    static bool NativeInterface2JSObject(jsval* d,
                                         nsIXPConnectJSObjectHolder** dest,
                                         xpcObjectHelper& aHelper,
                                         const nsID* iid,
                                         XPCNativeInterface** Interface,
                                         bool allowNativeWrapper,
                                         nsresult* pErr);

    static bool GetNativeInterfaceFromJSObject(void** dest, JSObject* src,
                                               const nsID* iid,
                                               nsresult* pErr);
    static bool JSObject2NativeInterface(void** dest, JS::HandleObject src,
                                         const nsID* iid,
                                         nsISupports* aOuter,
                                         nsresult* pErr);
    static bool GetISupportsFromJSObject(JSObject* obj, nsISupports** iface);

    










    static bool NativeArray2JS(jsval* d, const void** s,
                               const nsXPTType& type, const nsID* iid,
                               uint32_t count, nsresult* pErr);

    static bool JSArray2Native(void** d, JS::HandleValue s,
                               uint32_t count, const nsXPTType& type,
                               const nsID* iid, nsresult* pErr);

    static bool JSTypedArray2Native(void** d,
                                    JSObject* jsarray,
                                    uint32_t count,
                                    const nsXPTType& type,
                                    nsresult* pErr);

    static bool NativeStringWithSize2JS(jsval* d, const void* s,
                                        const nsXPTType& type,
                                        uint32_t count,
                                        nsresult* pErr);

    static bool JSStringWithSize2Native(void* d, JS::HandleValue s,
                                        uint32_t count, const nsXPTType& type,
                                        nsresult* pErr);

    static nsresult JSValToXPCException(JS::MutableHandleValue s,
                                        const char* ifaceName,
                                        const char* methodName,
                                        nsIException** exception);

    static nsresult JSErrorToXPCException(const char* message,
                                          const char* ifaceName,
                                          const char* methodName,
                                          const JSErrorReport* report,
                                          nsIException** exception);

    static nsresult ConstructException(nsresult rv, const char* message,
                                       const char* ifaceName,
                                       const char* methodName,
                                       nsISupports* data,
                                       nsIException** exception,
                                       JSContext* cx,
                                       jsval *jsExceptionPtr);

private:
    XPCConvert(); 

};




class nsXPCException;

class XPCThrower
{
public:
    static void Throw(nsresult rv, JSContext* cx);
    static void Throw(nsresult rv, XPCCallContext& ccx);
    static void ThrowBadResult(nsresult rv, nsresult result, XPCCallContext& ccx);
    static void ThrowBadParam(nsresult rv, unsigned paramNum, XPCCallContext& ccx);
    static bool SetVerbosity(bool state)
        {bool old = sVerbose; sVerbose = state; return old;}

    static bool CheckForPendingException(nsresult result, JSContext *cx);

private:
    static void Verbosify(XPCCallContext& ccx,
                          char** psz, bool own);

private:
    static bool sVerbose;
};



class nsXPCException
{
public:
    static bool NameAndFormatForNSResult(nsresult rv,
                                         const char** name,
                                         const char** format);

    static const void* IterateNSResults(nsresult* rv,
                                        const char** name,
                                        const char** format,
                                        const void** iterp);

    static uint32_t GetNSResultCount();
};









extern void xpc_DestroyJSxIDClassObjects();

class nsJSID : public nsIJSID
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_JS_ID_CID)

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIJSID

    bool InitWithName(const nsID& id, const char *nameString);
    bool SetName(const char* name);
    void   SetNameToNoString()
        {MOZ_ASSERT(!mName, "name already set"); mName = gNoString;}
    bool NameIsSet() const {return nullptr != mName;}
    const nsID& ID() const {return mID;}
    bool IsValid() const {return !mID.Equals(GetInvalidIID());}

    static nsJSID* NewID(const char* str);
    static nsJSID* NewID(const nsID& id);

    nsJSID();
    virtual ~nsJSID();
protected:

    void Reset();
    const nsID& GetInvalidIID() const;

protected:
    static char gNoString[];
    nsID    mID;
    char*   mNumber;
    char*   mName;
};



class nsJSIID : public nsIJSIID,
                public nsIXPCScriptable,
                public nsISecurityCheckedComponent
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_DECL_NSIJSID

    
    NS_DECL_NSIJSIID
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

    static nsJSIID* NewID(nsIInterfaceInfo* aInfo);

    nsJSIID(nsIInterfaceInfo* aInfo);
    nsJSIID(); 
    virtual ~nsJSIID();

private:
    nsCOMPtr<nsIInterfaceInfo> mInfo;
};



class nsJSCID : public nsIJSCID, public nsIXPCScriptable
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_DECL_NSIJSID

    
    NS_DECL_NSIJSCID
    NS_DECL_NSIXPCSCRIPTABLE

    static nsJSCID* NewID(const char* str);

    nsJSCID();
    virtual ~nsJSCID();

private:
    void ResolveName();

private:
    nsJSID mDetails;
};





struct XPCJSContextInfo {
    XPCJSContextInfo(JSContext* aCx) :
        cx(aCx),
        savedFrameChain(false)
    {}
    JSContext* cx;

    
    bool savedFrameChain;
};

namespace xpc {






bool PushJSContextNoScriptContext(JSContext *aCx);
void PopJSContextNoScriptContext();

} 

class XPCJSContextStack
{
public:
    XPCJSContextStack()
      : mSafeJSContext(NULL)
    { }

    virtual ~XPCJSContextStack();

    uint32_t Count()
    {
        return mStack.Length();
    }

    JSContext *Peek()
    {
        return mStack.IsEmpty() ? NULL : mStack[mStack.Length() - 1].cx;
    }

    JSContext *GetSafeJSContext();
    bool HasJSContext(JSContext *cx);

    const InfallibleTArray<XPCJSContextInfo>* GetStack()
    { return &mStack; }

private:
    friend class mozilla::AutoCxPusher;
    friend bool xpc::PushJSContextNoScriptContext(JSContext *aCx);;
    friend void xpc::PopJSContextNoScriptContext();

    
    
    JSContext *Pop();
    bool Push(JSContext *cx);

    AutoInfallibleTArray<XPCJSContextInfo, 16> mStack;
    JSContext*  mSafeJSContext;
};




class nsXPCComponents : public nsIXPCComponents,
                        public nsIXPCScriptable,
                        public nsIClassInfo,
                        public nsISecurityCheckedComponent
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

public:
    static bool
    AttachComponentsObject(JSContext* aCx, XPCWrappedNativeScope* aScope);

    void SystemIsBeingShutDown() {ClearMembers();}

    virtual ~nsXPCComponents();

private:
    nsXPCComponents(XPCWrappedNativeScope* aScope);
    void ClearMembers();

private:
    friend class XPCWrappedNativeScope;
    XPCWrappedNativeScope*          mScope;
    nsXPCComponents_Interfaces*     mInterfaces;
    nsXPCComponents_InterfacesByID* mInterfacesByID;
    nsXPCComponents_Classes*        mClasses;
    nsXPCComponents_ClassesByID*    mClassesByID;
    nsXPCComponents_Results*        mResults;
    nsXPCComponents_ID*             mID;
    nsXPCComponents_Exception*      mException;
    nsXPCComponents_Constructor*    mConstructor;
    nsXPCComponents_Utils*          mUtils;
};




extern JSObject*
xpc_NewIDObject(JSContext *cx, JS::HandleObject jsobj, const nsID& aID);

extern const nsID*
xpc_JSObjectToID(JSContext *cx, JSObject* obj);

extern bool
xpc_JSObjectIsID(JSContext *cx, JSObject* obj);




extern bool
xpc_DumpJSStack(JSContext* cx, bool showArgs, bool showLocals,
                bool showThisProps);




extern char*
xpc_PrintJSStack(JSContext* cx, bool showArgs, bool showLocals,
                 bool showThisProps);

extern bool
xpc_DumpEvalInJSStackFrame(JSContext* cx, uint32_t frameno, const char* text);

extern bool
xpc_InstallJSDebuggerKeywordHandler(JSRuntime* rt);





class nsScriptError : public nsIScriptError {
public:
    nsScriptError();

    virtual ~nsScriptError();

  

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSICONSOLEMESSAGE
    NS_DECL_NSISCRIPTERROR

private:
    nsString mMessage;
    nsString mSourceName;
    uint32_t mLineNumber;
    nsString mSourceLine;
    uint32_t mColumnNumber;
    uint32_t mFlags;
    nsCString mCategory;
    uint64_t mOuterWindowID;
    uint64_t mInnerWindowID;
    int64_t mTimeStamp;
    bool mIsFromPrivateWindow;
};





class MOZ_STACK_CLASS AutoScriptEvaluate
{
public:
    



    AutoScriptEvaluate(JSContext * cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
         : mJSContext(cx), mState(0), mErrorReporterSet(false),
           mEvaluated(false) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    






    bool StartEvaluating(JS::HandleObject scope, JSErrorReporter errorReporter = nullptr);

    


    ~AutoScriptEvaluate();
private:
    JSContext* mJSContext;
    JSExceptionState* mState;
    bool mErrorReporterSet;
    bool mEvaluated;
    mozilla::Maybe<JSAutoCompartment> mAutoCompartment;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoScriptEvaluate(const AutoScriptEvaluate &) MOZ_DELETE;
    AutoScriptEvaluate & operator =(const AutoScriptEvaluate &) MOZ_DELETE;
};


class MOZ_STACK_CLASS AutoResolveName
{
public:
    AutoResolveName(XPCCallContext& ccx, JS::HandleId name
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
          mOld(ccx, XPCJSRuntime::Get()->SetResolveName(name))
#ifdef DEBUG
          ,mCheck(ccx, name)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoResolveName()
        {
#ifdef DEBUG
            jsid old =
#endif
            XPCJSRuntime::Get()->SetResolveName(mOld);
            MOZ_ASSERT(old == mCheck, "Bad Nesting!");
        }

private:
    JS::RootedId mOld;
#ifdef DEBUG
    JS::RootedId mCheck;
#endif
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};








class AutoMarkingPtr
{
  public:
    AutoMarkingPtr(JSContext* cx) {
        mRoot = XPCJSRuntime::Get()->GetAutoRootsAdr();
        mNext = *mRoot;
        *mRoot = this;
    }

    virtual ~AutoMarkingPtr() {
        if (mRoot) {
            MOZ_ASSERT(*mRoot == this);
            *mRoot = mNext;
        }
    }

    void TraceJSAll(JSTracer* trc) {
        for (AutoMarkingPtr *cur = this; cur; cur = cur->mNext)
            cur->TraceJS(trc);
    }

    void MarkAfterJSFinalizeAll() {
        for (AutoMarkingPtr *cur = this; cur; cur = cur->mNext)
            cur->MarkAfterJSFinalize();
    }

  protected:
    virtual void TraceJS(JSTracer* trc) = 0;
    virtual void MarkAfterJSFinalize() = 0;

  private:
    AutoMarkingPtr** mRoot;
    AutoMarkingPtr* mNext;
};

template<class T>
class TypedAutoMarkingPtr : public AutoMarkingPtr
{
  public:
    TypedAutoMarkingPtr(JSContext* cx) : AutoMarkingPtr(cx), mPtr(nullptr) {}
    TypedAutoMarkingPtr(JSContext* cx, T* ptr) : AutoMarkingPtr(cx), mPtr(ptr) {}

    T* get() const { return mPtr; }
    operator T *() const { return mPtr; }
    T* operator->() const { return mPtr; }

    TypedAutoMarkingPtr<T>& operator =(T* ptr) { mPtr = ptr; return *this; }

  protected:
    virtual void TraceJS(JSTracer* trc)
    {
        if (mPtr) {
            mPtr->TraceJS(trc);
            mPtr->AutoTrace(trc);
        }
    }

    virtual void MarkAfterJSFinalize()
    {
        if (mPtr)
            mPtr->Mark();
    }

  private:
    T* mPtr;
};

typedef TypedAutoMarkingPtr<XPCNativeInterface> AutoMarkingNativeInterfacePtr;
typedef TypedAutoMarkingPtr<XPCNativeSet> AutoMarkingNativeSetPtr;
typedef TypedAutoMarkingPtr<XPCWrappedNative> AutoMarkingWrappedNativePtr;
typedef TypedAutoMarkingPtr<XPCWrappedNativeTearOff> AutoMarkingWrappedNativeTearOffPtr;
typedef TypedAutoMarkingPtr<XPCWrappedNativeProto> AutoMarkingWrappedNativeProtoPtr;
typedef TypedAutoMarkingPtr<XPCNativeScriptableInfo> AutoMarkingNativeScriptableInfoPtr;

template<class T>
class ArrayAutoMarkingPtr : public AutoMarkingPtr
{
  public:
    ArrayAutoMarkingPtr(JSContext* cx)
      : AutoMarkingPtr(cx), mPtr(nullptr), mCount(0) {}
    ArrayAutoMarkingPtr(JSContext* cx, T** ptr, uint32_t count, bool clear)
      : AutoMarkingPtr(cx), mPtr(ptr), mCount(count)
    {
        if (!mPtr) mCount = 0;
        else if (clear) memset(mPtr, 0, mCount*sizeof(T*));
    }

    T** get() const { return mPtr; }
    operator T **() const { return mPtr; }
    T** operator->() const { return mPtr; }

    ArrayAutoMarkingPtr<T>& operator =(const ArrayAutoMarkingPtr<T> &other)
    {
        mPtr = other.mPtr;
        mCount = other.mCount;
        return *this;
    }

  protected:
    virtual void TraceJS(JSTracer* trc)
    {
        for (uint32_t i = 0; i < mCount; i++) {
            if (mPtr[i]) {
                mPtr[i]->TraceJS(trc);
                mPtr[i]->AutoTrace(trc);
            }
        }
    }

    virtual void MarkAfterJSFinalize()
    {
        for (uint32_t i = 0; i < mCount; i++) {
            if (mPtr[i])
                mPtr[i]->Mark();
        }
    }

  private:
    T** mPtr;
    uint32_t mCount;
};

typedef ArrayAutoMarkingPtr<XPCNativeInterface> AutoMarkingNativeInterfacePtrArrayPtr;


namespace xpc {

char *
CloneAllAccess();


char *
CheckAccessList(const PRUnichar *wideName, const char *const list[]);
} 





#define XPCVARIANT_IID                                                        \
    {0x1809fd50, 0x91e8, 0x11d5,                                              \
      { 0x90, 0xf9, 0x0, 0x10, 0xa4, 0xe7, 0x3d, 0x9a } }


#define XPCVARIANT_CID                                                        \
    {0xdc524540, 0x487e, 0x4501,                                              \
      { 0x9a, 0xc7, 0xaa, 0xa7, 0x84, 0xb1, 0x7c, 0x1c } }

class XPCVariant : public nsIVariant
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIVARIANT
    NS_DECL_CYCLE_COLLECTION_CLASS(XPCVariant)

    
    
    

    
    
    NS_DECLARE_STATIC_IID_ACCESSOR(XPCVARIANT_IID)

    static XPCVariant* newVariant(JSContext* cx, jsval aJSVal);

    




    jsval GetJSVal() const {
        if (!JSVAL_IS_PRIMITIVE(mJSVal))
            JS::ExposeObjectToActiveJS(&mJSVal.toObject());
        return mJSVal;
    }

    








    jsval GetJSValPreserveColor() const {return mJSVal;}

    XPCVariant(JSContext* cx, jsval aJSVal);

    








    static bool VariantDataToJS(nsIVariant* variant,
                                nsresult* pErr, jsval* pJSVal);

    bool IsPurple()
    {
        return mRefCnt.IsPurple();
    }

    void RemovePurple()
    {
        mRefCnt.RemovePurple();
    }

    void SetCCGeneration(uint32_t aGen)
    {
        mCCGeneration = aGen;
    }

    uint32_t CCGeneration() { return mCCGeneration; }
protected:
    virtual ~XPCVariant() { }

    bool InitializeData(JSContext* cx);

protected:
    nsDiscriminatedUnion mData;
    JS::Heap<JS::Value>  mJSVal;
    bool                 mReturnRawObject : 1;
    uint32_t             mCCGeneration : 31;
};

NS_DEFINE_STATIC_IID_ACCESSOR(XPCVariant, XPCVARIANT_IID)

class XPCTraceableVariant: public XPCVariant,
                           public XPCRootSetElem
{
public:
    XPCTraceableVariant(JSContext* cx, jsval aJSVal)
        : XPCVariant(cx, aJSVal)
    {
         nsXPConnect::GetRuntimeInstance()->AddVariantRoot(this);
    }

    virtual ~XPCTraceableVariant();

    void TraceJS(JSTracer* trc);
    static void GetTraceName(JSTracer* trc, char *buf, size_t bufsize);
};




inline void *
xpc_GetJSPrivate(JSObject *obj)
{
    return js::GetObjectPrivate(obj);
}

inline JSContext *
xpc_GetSafeJSContext()
{
    return XPCJSRuntime::Get()->GetJSContextStack()->GetSafeJSContext();
}

namespace xpc {


bool
Atob(JSContext *cx, unsigned argc, jsval *vp);

bool
Btoa(JSContext *cx, unsigned argc, jsval *vp);





bool
NewFunctionForwarder(JSContext *cx, JS::HandleId id, JS::HandleObject callable,
                     bool doclone, JS::MutableHandleValue vp);


nsresult
ThrowAndFail(nsresult errNum, JSContext *cx, bool *retval);


already_AddRefed<nsIXPCComponents_utils_Sandbox>
NewSandboxConstructor();


bool
IsSandbox(JSObject *obj);

struct SandboxOptions {
    struct GlobalProperties {
        GlobalProperties() { mozilla::PodZero(this); }
        bool Parse(JSContext* cx, JS::HandleObject obj);
        bool Define(JSContext* cx, JS::HandleObject obj);
        bool XMLHttpRequest;
        bool TextDecoder;
        bool TextEncoder;
        bool atob;
        bool btoa;
    };

    SandboxOptions(JSContext *cx)
        : wantXrays(true)
        , wantComponents(true)
        , wantExportHelpers(false)
        , proto(xpc_GetSafeJSContext())
        , sameZoneAs(xpc_GetSafeJSContext())
        , metadata(xpc_GetSafeJSContext())
    { }

    bool wantXrays;
    bool wantComponents;
    bool wantExportHelpers;
    JS::RootedObject proto;
    nsCString sandboxName;
    JS::RootedObject sameZoneAs;
    GlobalProperties GlobalProperties;
    JS::RootedValue metadata;
};

JSObject *
CreateGlobalObject(JSContext *cx, const JSClass *clasp, nsIPrincipal *principal,
                   JS::CompartmentOptions& aOptions);










nsresult
CreateSandboxObject(JSContext *cx, jsval *vp, nsISupports *prinOrSop,
                    xpc::SandboxOptions& options);










nsresult
EvalInSandbox(JSContext *cx, JS::HandleObject sandbox, const nsAString& source,
              const char *filename, int32_t lineNo,
              JSVersion jsVersion, bool returnStringOnly,
              JS::MutableHandleValue rval);



nsresult
GetSandboxMetadata(JSContext *cx, JS::HandleObject sandboxArg,
                   JS::MutableHandleValue rval);

nsresult
SetSandboxMetadata(JSContext *cx, JS::HandleObject sandboxArg,
                   JS::HandleValue metadata);

} 





inline bool
xpc_ForcePropertyResolve(JSContext* cx, JSObject* obj, jsid id);

inline jsid
GetRTIdByIndex(JSContext *cx, unsigned index);

namespace xpc {

class CompartmentPrivate
{
public:
    CompartmentPrivate()
        : wantXrays(false)
        , universalXPConnectEnabled(false)
        , scope(nullptr)
        , locationWasParsed(false)
    {
        MOZ_COUNT_CTOR(xpc::CompartmentPrivate);
    }

    ~CompartmentPrivate();

    bool wantXrays;

    
    
    
    
    bool universalXPConnectEnabled;

    
    
    XPCWrappedNativeScope *scope;

    const nsACString& GetLocation() {
        if (location.IsEmpty() && locationURI) {
            if (NS_FAILED(locationURI->GetSpec(location)))
                location = NS_LITERAL_CSTRING("<unknown location>");
        }
        return location;
    }
    bool GetLocationURI(nsIURI **aURI) {
        if (!locationURI && !TryParseLocationURI())
            return false;
        NS_IF_ADDREF(*aURI = locationURI);
        return true;
    }
    void SetLocation(const nsACString& aLocation) {
        if (aLocation.IsEmpty())
            return;
        if (!location.IsEmpty() || locationURI)
            return;
        location = aLocation;
    }
    void SetLocationURI(nsIURI *aLocationURI) {
        if (!aLocationURI)
            return;
        if (locationURI)
            return;
        locationURI = aLocationURI;
    }

private:
    nsCString location;
    nsCOMPtr<nsIURI> locationURI;
    bool locationWasParsed;

    bool TryParseLocationURI();
    bool TryParseLocationURICandidate(const nsACString& uristr);
};

CompartmentPrivate*
EnsureCompartmentPrivate(JSObject *obj);

CompartmentPrivate*
EnsureCompartmentPrivate(JSCompartment *c);

inline CompartmentPrivate*
GetCompartmentPrivate(JSCompartment *compartment)
{
    MOZ_ASSERT(compartment);
    void *priv = JS_GetCompartmentPrivate(compartment);
    return static_cast<CompartmentPrivate*>(priv);
}

inline CompartmentPrivate*
GetCompartmentPrivate(JSObject *object)
{
    MOZ_ASSERT(object);
    JSCompartment *compartment = js::GetObjectCompartment(object);

    MOZ_ASSERT(compartment);
    return GetCompartmentPrivate(compartment);
}

bool IsUniversalXPConnectEnabled(JSCompartment *compartment);
bool IsUniversalXPConnectEnabled(JSContext *cx);
bool EnableUniversalXPConnect(JSContext *cx);



inline XPCWrappedNativeScope*
GetObjectScope(JSObject *obj)
{
    return EnsureCompartmentPrivate(obj)->scope;
}

extern bool gDebugMode;
extern bool gDesiredDebugMode;

extern const JSClass SafeJSContextGlobalClass;

JSObject* NewOutObject(JSContext* cx, JSObject* scope);
bool IsOutObject(JSContext* cx, JSObject* obj);

nsresult HasInstance(JSContext *cx, JS::HandleObject objArg, const nsID *iid, bool *bp);

} 




#include "XPCInlines.h"




#include "XPCMaps.h"



#endif 
