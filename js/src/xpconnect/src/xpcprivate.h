












































#ifndef xpcprivate_h___
#define xpcprivate_h___

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "xpcpublic.h"
#include "jsapi.h"
#include "jsdhash.h"
#include "jsprf.h"
#include "prprf.h"
#include "jsinterp.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsgc.h"
#include "jscompartment.h"
#include "xpcpublic.h"
#include "nscore.h"
#include "nsXPCOM.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCycleCollector.h"
#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsIClassInfoImpl.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsISupportsPrimitives.h"
#include "nsMemory.h"
#include "nsIXPConnect.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "nsIXPCSecurityManager.h"
#include "nsIJSRuntimeService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsAutoLock.h"
#include "nsXPTCUtils.h"
#include "xptinfo.h"
#include "xpcforwards.h"
#include "xpclog.h"
#include "xpccomponents.h"
#include "xpcexception.h"
#include "xpcjsid.h"
#include "prlong.h"
#include "prmem.h"
#include "prenv.h"
#include "prclist.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsAutoJSValHolder.h"
#include "mozilla/AutoRestore.h"
#include "nsDataHashtable.h"

#include "nsThreadUtils.h"
#include "nsIJSContextStack.h"
#include "nsDeque.h"

#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIExceptionService.h"

#include "nsVariant.h"
#include "nsIPropertyBag.h"
#include "nsIProperty.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsWrapperCache.h"
#include "nsStringBuffer.h"

#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"

#include "nsIXPCScriptNotify.h"  

#ifndef XPCONNECT_STANDALONE
#define XPC_USE_SECURITY_CHECKED_COMPONENT
#include "nsIScriptObjectPrincipal.h"
#include "nsIPrincipal.h"
#endif

#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
#include "nsISecurityCheckedComponent.h"
#endif

#include "nsIThreadInternal.h"

#ifdef XPC_IDISPATCH_SUPPORT


#ifdef WINCE





#include <windows.h>
static FARPROC GetProcAddressA(HMODULE hMod, wchar_t *procName);
#endif 

#include <atlbase.h>
#include "oaidl.h"
#endif

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
#define XPC_CHECK_WRAPPER_THREADSAFETY

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

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
void DEBUG_ReportWrapperThreadSafetyError(XPCCallContext& ccx,
                                          const char* msg,
                                          const XPCWrappedNative* wrapper);
void DEBUG_CheckWrapperThreadSafety(const XPCWrappedNative* wrapper);
#else
#define DEBUG_CheckWrapperThreadSafety(w) ((void)0)
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



extern const char* XPC_ARG_FORMATTER_FORMAT_STRINGS[]; 
extern const char XPC_CONTEXT_STACK_CONTRACTID[];
extern const char XPC_RUNTIME_CONTRACTID[];
extern const char XPC_EXCEPTION_CONTRACTID[];
extern const char XPC_CONSOLE_CONTRACTID[];
extern const char XPC_SCRIPT_ERROR_CONTRACTID[];
extern const char XPC_ID_CONTRACTID[];
extern const char XPC_XPCONNECT_CONTRACTID[];

namespace xpc {

class PtrAndPrincipalHashKey : public PLDHashEntryHdr
{
  public:
    typedef PtrAndPrincipalHashKey *KeyType;
    typedef const PtrAndPrincipalHashKey *KeyTypePointer;

    PtrAndPrincipalHashKey(const PtrAndPrincipalHashKey *aKey)
      : mPtr(aKey->mPtr), mURI(aKey->mURI), mSavedHash(aKey->mSavedHash)
    {
        MOZ_COUNT_CTOR(PtrAndPrincipalHashKey);
    }

    PtrAndPrincipalHashKey(nsISupports *aPtr, nsIURI *aURI)
      : mPtr(aPtr), mURI(aURI)
    {
        MOZ_COUNT_CTOR(PtrAndPrincipalHashKey);
        mSavedHash = mURI
                     ? NS_SecurityHashURI(mURI)
                     : (NS_PTR_TO_UINT32(mPtr.get()) >> 2);
    }

    ~PtrAndPrincipalHashKey()
    {
        MOZ_COUNT_DTOR(PtrAndPrincipalHashKey);
    }

    PtrAndPrincipalHashKey* GetKey() const
    {
        return const_cast<PtrAndPrincipalHashKey*>(this);
    }
    const PtrAndPrincipalHashKey* GetKeyPointer() const { return this; }

    inline PRBool KeyEquals(const PtrAndPrincipalHashKey* aKey) const;

    static const PtrAndPrincipalHashKey*
    KeyToPointer(PtrAndPrincipalHashKey* aKey) { return aKey; }
    static PLDHashNumber HashKey(const PtrAndPrincipalHashKey* aKey)
    {
        return aKey->mSavedHash;
    }

    enum { ALLOW_MEMMOVE = PR_TRUE };

  protected:
    nsCOMPtr<nsISupports> mPtr;
    nsCOMPtr<nsIURI> mURI;

    
    
    
    
    
    PLDHashNumber mSavedHash;
};

}







typedef nsDataHashtableMT<nsISupportsHashKey, JSCompartment *> XPCMTCompartmentMap;


typedef nsDataHashtable<xpc::PtrAndPrincipalHashKey, JSCompartment *> XPCCompartmentMap;




#define XPC_STRING_GETTER_BODY(dest, src) \
    NS_ENSURE_ARG_POINTER(dest); \
    char* result; \
    if(src) \
        result = (char*) nsMemory::Clone(src, \
                                sizeof(char)*(strlen(src)+1)); \
    else \
        result = nsnull; \
    *dest = result; \
    return (result || !src) ? NS_OK : NS_ERROR_OUT_OF_MEMORY


#define WRAPPER_SLOTS (JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1))

#define INVALID_OBJECT ((JSObject *)1)





#ifdef _MSC_VER
#pragma warning(disable : 4355) // OK to pass "this" in member initializer
#endif

typedef PRMonitor XPCLock;

static inline void xpc_Wait(XPCLock* lock) 
    {
        NS_ASSERTION(lock, "xpc_Wait called with null lock!");
#ifdef DEBUG
        PRStatus result = 
#endif
        PR_Wait(lock, PR_INTERVAL_NO_TIMEOUT);
        NS_ASSERTION(PR_SUCCESS == result, "bad result from PR_Wait!");
    }

static inline void xpc_NotifyAll(XPCLock* lock) 
    {
        NS_ASSERTION(lock, "xpc_NotifyAll called with null lock!");
#ifdef DEBUG
        PRStatus result = 
#endif    
        PR_NotifyAll(lock);
        NS_ASSERTION(PR_SUCCESS == result, "bad result from PR_NotifyAll!");
    }











class NS_STACK_CLASS XPCAutoLock : public nsAutoLockBase {
public:

    static XPCLock* NewLock(const char* name)
                        {return nsAutoMonitor::NewMonitor(name);}
    static void     DestroyLock(XPCLock* lock)
                        {nsAutoMonitor::DestroyMonitor(lock);}

    XPCAutoLock(XPCLock* lock MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
#ifdef DEBUG_jband
        : nsAutoLockBase(lock ? (void*) lock : (void*) this, eAutoMonitor),
#else
        : nsAutoLockBase(lock, eAutoMonitor),
#endif
          mLock(lock)
    {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
        if(mLock)
            PR_EnterMonitor(mLock);
    }

    ~XPCAutoLock()
    {
        if(mLock)
        {
#ifdef DEBUG
            PRStatus status =
#endif
                PR_ExitMonitor(mLock);
            NS_ASSERTION(status == PR_SUCCESS, "PR_ExitMonitor failed");
        }
    }

private:
    XPCLock*  mLock;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
    XPCAutoLock(void) {}
    XPCAutoLock(XPCAutoLock& ) {}
    XPCAutoLock& operator =(XPCAutoLock& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nsnull;
    }
    static void operator delete(void* ) {}
};



class NS_STACK_CLASS XPCAutoUnlock : public nsAutoUnlockBase {
public:
    XPCAutoUnlock(XPCLock* lock MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
        : nsAutoUnlockBase(lock),
          mLock(lock)
    {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
        if(mLock)
        {
#ifdef DEBUG
            PRStatus status =
#endif
                PR_ExitMonitor(mLock);
            NS_ASSERTION(status == PR_SUCCESS, "PR_ExitMonitor failed");
        }
    }

    ~XPCAutoUnlock()
    {
        if(mLock)
            PR_EnterMonitor(mLock);
    }

private:
    XPCLock*  mLock;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
    XPCAutoUnlock(void) {}
    XPCAutoUnlock(XPCAutoUnlock& ) {}
    XPCAutoUnlock& operator =(XPCAutoUnlock& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nsnull;
    }
    static void operator delete(void* ) {}
};









static const uint32 XPC_GC_COLOR_BLACK = 0;
static const uint32 XPC_GC_COLOR_GRAY = 1;






const PRBool OBJ_IS_GLOBAL = PR_TRUE;
const PRBool OBJ_IS_NOT_GLOBAL = PR_FALSE;

class nsXPConnect : public nsIXPConnect,
                    public nsIThreadObserver,
                    public nsSupportsWeakReference,
                    public nsCycleCollectionJSRuntime,
                    public nsCycleCollectionParticipant,
                    public nsIJSRuntimeService,
                    public nsIThreadJSContextStack
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECT
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIJSRUNTIMESERVICE
    NS_DECL_NSIJSCONTEXTSTACK
    NS_DECL_NSITHREADJSCONTEXTSTACK

    
public:
    
    static nsXPConnect*  GetXPConnect();
    static nsXPConnect*  FastGetXPConnect() { return gSelf ? gSelf : GetXPConnect(); }
    static XPCJSRuntime* GetRuntimeInstance();
    XPCJSRuntime* GetRuntime() {return mRuntime;}

    
    static nsresult GetInterfaceInfoManager(nsIInterfaceInfoSuperManager** iim,
                                            nsXPConnect* xpc = nsnull);

    static JSBool IsISupportsDescendant(nsIInterfaceInfo* info);

    nsIXPCSecurityManager* GetDefaultSecurityManager() const
    {
        
        if (!NS_IsMainThread()) {
            return nsnull;
        }
        return mDefaultSecurityManager;
    }

    PRUint16 GetDefaultSecurityManagerFlags() const
        {return mDefaultSecurityManagerFlags;}

    
    
    
    static nsXPConnect* GetSingleton();

    
    static void InitStatics() { gSelf = nsnull; gOnceAliveNowDead = JS_FALSE; }
    
    static void ReleaseXPConnectSingleton();

    virtual ~nsXPConnect();

    JSBool IsShuttingDown() const {return mShuttingDown;}

    
    
    
    static JSBool IsGray(void *thing);

    nsresult GetInfoForIID(const nsIID * aIID, nsIInterfaceInfo** info);
    nsresult GetInfoForName(const char * name, nsIInterfaceInfo** info);

    static nsresult Base64Encode(const nsACString &aString,
                                 nsACString &aBinary);

    static nsresult Base64Encode(const nsAString &aString,
                                 nsAString &aBinaryData);

    
    static JSBool Base64Encode(JSContext *cx, jsval val, jsval *out);

    static nsresult Base64Decode(const nsACString &aBinaryData,
                                 nsACString &aString);

    static nsresult Base64Decode(const nsAString &aBinaryData,
                                 nsAString &aString);

    
    static JSBool Base64Decode(JSContext *cx, jsval val, jsval *out);

    
    NS_IMETHOD RootAndUnlinkJSObjects(void *p);
    NS_IMETHOD Unlink(void *p);
    NS_IMETHOD Unroot(void *p);
    NS_IMETHOD Traverse(void *p,
                        nsCycleCollectionTraversalCallback &cb);
    
    
    virtual nsresult BeginCycleCollection(nsCycleCollectionTraversalCallback &cb,
                                          bool explainExpectedLiveGarbage);
    virtual nsresult FinishTraverse();
    virtual nsresult FinishCycleCollection();
    virtual nsCycleCollectionParticipant *ToParticipant(void *p);
    virtual void Collect();
#ifdef DEBUG_CC
    virtual void PrintAllReferencesTo(void *p);
#endif

    unsigned GetOutstandingRequests(JSContext* cx);

    
    static nsCycleCollectionParticipant *JSContextParticipant();

#ifndef XPCONNECT_STANDALONE
    virtual nsIPrincipal* GetPrincipal(JSObject* obj,
                                       PRBool allowShortCircuit) const;

    void RecordTraversal(void *p, nsISupports *s);
#endif
    virtual char* DebugPrintJSStack(PRBool showArgs,
                                    PRBool showLocals,
                                    PRBool showThisProps);


    static PRBool ReportAllJSExceptions()
    {
      return gReportAllJSExceptions > 0;
    }

#ifdef XPC_IDISPATCH_SUPPORT
public:
    static PRBool IsIDispatchEnabled();
#endif
protected:
    nsXPConnect();

private:
    static PRThread* FindMainThread();

private:
    
    static nsXPConnect*      gSelf;
    static JSBool            gOnceAliveNowDead;

    XPCJSRuntime*            mRuntime;
    nsCOMPtr<nsIInterfaceInfoSuperManager> mInterfaceInfoManager;
    nsIXPCSecurityManager*   mDefaultSecurityManager;
    PRUint16                 mDefaultSecurityManagerFlags;
    JSBool                   mShuttingDown;
#ifdef DEBUG_CC
    PLDHashTable             mJSRoots;
#endif
    nsAutoPtr<XPCCallContext> mCycleCollectionContext;

#ifndef XPCONNECT_STANDALONE
    typedef nsBaseHashtable<nsVoidPtrHashKey, nsISupports*, nsISupports*> ScopeSet;
    ScopeSet mScopes;
#endif
    nsCOMPtr<nsIXPCScriptable> mBackstagePass;

    static PRUint32 gReportAllJSExceptions;
    static JSBool gDebugMode;
    static JSBool gDesiredDebugMode;
    static inline void CheckForDebugMode(JSRuntime *rt);

public:
    static nsIScriptSecurityManager *gScriptSecurityManager;
};



class XPCRootSetElem
{
public:
    XPCRootSetElem()
    {
#ifdef DEBUG
        mNext = nsnull;
        mSelfp = nsnull;
#endif
    }

    ~XPCRootSetElem()
    {
        NS_ASSERTION(!mNext, "Must be unlinked");
        NS_ASSERTION(!mSelfp, "Must be unlinked");
    }

    inline XPCRootSetElem* GetNextRoot() { return mNext; }
    void AddToRootSet(XPCLock *lock, XPCRootSetElem **listHead);
    void RemoveFromRootSet(XPCLock *lock);

private:
    XPCRootSetElem *mNext;
    XPCRootSetElem **mSelfp;
};







class XPCJSRuntime
{
public:
    static XPCJSRuntime* newXPCJSRuntime(nsXPConnect* aXPConnect);

    JSRuntime*     GetJSRuntime() const {return mJSRuntime;}
    nsXPConnect*   GetXPConnect() const {return mXPConnect;}

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

    XPCNativeWrapperMap* GetExplicitNativeWrapperMap() const
        {return mExplicitNativeWrapperMap;}

    XPCCompartmentMap& GetCompartmentMap()
        {return mCompartmentMap;}
    XPCMTCompartmentMap& GetMTCompartmentMap()
        {return mMTCompartmentMap;}

    XPCLock* GetMapLock() const {return mMapLock;}

    JSBool OnJSContextNew(JSContext* cx);

    JSBool DeferredRelease(nsISupports* obj);

    JSBool GetDoingFinalization() const {return mDoingFinalization;}

    
    
    
    
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
        IDX_SCRIPTONLY              ,
        IDX_TOTAL_COUNT 
    };

    jsid GetStringID(uintN index) const
    {
        NS_ASSERTION(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrIDs[index];
    }
    jsval GetStringJSVal(uintN index) const
    {
        NS_ASSERTION(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrJSVals[index];
    }
    const char* GetStringName(uintN index) const
    {
        NS_ASSERTION(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrings[index];
    }

    static void TraceJS(JSTracer* trc, void* data);
    void TraceXPConnectRoots(JSTracer *trc);
    void AddXPConnectRoots(JSContext* cx,
                           nsCycleCollectionTraversalCallback& cb);

    static JSBool GCCallback(JSContext *cx, JSGCStatus status);

    inline void AddVariantRoot(XPCTraceableVariant* variant);
    inline void AddWrappedJSRoot(nsXPCWrappedJS* wrappedJS);
    inline void AddObjectHolderRoot(XPCJSObjectHolder* holder);

    nsresult AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer);
    nsresult RemoveJSHolder(void* aHolder);

    void ClearWeakRoots();

    void DebugDump(PRInt16 depth);

    void SystemIsBeingShutDown(JSContext* cx);

    PRThread* GetThreadRunningGC() const {return mThreadRunningGC;}

    ~XPCJSRuntime();

#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
   void DEBUG_AddWrappedNative(nsIXPConnectWrappedNative* wrapper)
        {XPCAutoLock lock(GetMapLock());
         JSDHashEntryHdr *entry =
            JS_DHashTableOperate(DEBUG_WrappedNativeHashtable,
                                 wrapper, JS_DHASH_ADD);
         if(entry) ((JSDHashEntryStub *)entry)->key = wrapper;}

   void DEBUG_RemoveWrappedNative(nsIXPConnectWrappedNative* wrapper)
        {XPCAutoLock lock(GetMapLock());
         JS_DHashTableOperate(DEBUG_WrappedNativeHashtable,
                              wrapper, JS_DHASH_REMOVE);}
private:
   JSDHashTable* DEBUG_WrappedNativeHashtable;
public:
#endif

    void AddGCCallback(JSGCCallback cb);
    void RemoveGCCallback(JSGCCallback cb);

    static void ActivityCallback(void *arg, PRBool active);

private:
    XPCJSRuntime(); 
    XPCJSRuntime(nsXPConnect* aXPConnect);

    
    void RescheduleWatchdog(XPCContext* ccx);

    static void WatchdogMain(void *arg);

    static const char* mStrings[IDX_TOTAL_COUNT];
    jsid mStrIDs[IDX_TOTAL_COUNT];
    jsval mStrJSVals[IDX_TOTAL_COUNT];

    nsXPConnect* mXPConnect;
    JSRuntime*  mJSRuntime;
    JSObject2WrappedJSMap*   mWrappedJSMap;
    IID2WrappedJSClassMap*   mWrappedJSClassMap;
    IID2NativeInterfaceMap*  mIID2NativeInterfaceMap;
    ClassInfo2NativeSetMap*  mClassInfo2NativeSetMap;
    NativeSetMap*            mNativeSetMap;
    IID2ThisTranslatorMap*   mThisTranslatorMap;
    XPCNativeScriptableSharedMap* mNativeScriptableSharedMap;
    XPCWrappedNativeProtoMap* mDyingWrappedNativeProtoMap;
    XPCWrappedNativeProtoMap* mDetachedWrappedNativeProtoMap;
    XPCNativeWrapperMap*     mExplicitNativeWrapperMap;
    XPCCompartmentMap        mCompartmentMap;
    XPCMTCompartmentMap      mMTCompartmentMap;
    XPCLock* mMapLock;
    PRThread* mThreadRunningGC;
    nsTArray<nsXPCWrappedJS*> mWrappedJSToReleaseArray;
    nsTArray<nsISupports*> mNativesToReleaseArray;
    JSBool mDoingFinalization;
    XPCRootSetElem *mVariantRoots;
    XPCRootSetElem *mWrappedJSRoots;
    XPCRootSetElem *mObjectHolderRoots;
    JSDHashTable mJSHolders;
    PRCondVar *mWatchdogWakeup;
    PRThread *mWatchdogThread;
    nsTArray<JSGCCallback> extraGCCallbacks;
    PRBool mWatchdogHibernating;
    PRTime mLastActiveTime; 
};







class XPCContext
{
    friend class XPCJSRuntime;
public:
    static XPCContext* GetXPCContext(JSContext* aJSContext)
        {
            NS_ASSERTION(aJSContext->data2, "should already have XPCContext");
            return static_cast<XPCContext *>(aJSContext->data2);
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
    JSBool CallerTypeIsJavaScript() const 
        {
            return LANG_JS == mCallingLangType;
        }
    JSBool CallerTypeIsNative() const 
        {
            return LANG_NATIVE == mCallingLangType;
        }
    JSBool CallerTypeIsKnown() const 
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

    nsIXPCSecurityManager* GetSecurityManager() const
        {return mSecurityManager;}
    void SetSecurityManager(nsIXPCSecurityManager* aSecurityManager)
        {mSecurityManager = aSecurityManager;}

    PRUint16 GetSecurityManagerFlags() const
        {return mSecurityManagerFlags;}
    void SetSecurityManagerFlags(PRUint16 f)
        {mSecurityManagerFlags = f;}

    nsIXPCSecurityManager* GetAppropriateSecurityManager(PRUint16 flags) const
        {
            NS_ASSERTION(CallerTypeIsKnown(),"missing caller type set somewhere");
            if(!CallerTypeIsJavaScript())
                return nsnull;
            if(mSecurityManager)
            {
                if(flags & mSecurityManagerFlags)
                    return mSecurityManager;
            }
            else
            {
                nsIXPCSecurityManager* mgr;
                nsXPConnect* xpc = mRuntime->GetXPConnect();
                mgr = xpc->GetDefaultSecurityManager();
                if(mgr && (flags & xpc->GetDefaultSecurityManagerFlags()))
                    return mgr;
            }
            return nsnull;
        }

    void DebugDump(PRInt16 depth);
    void AddScope(PRCList *scope) { PR_INSERT_AFTER(scope, &mScopes); }
    void RemoveScope(PRCList *scope) { PR_REMOVE_LINK(scope); }

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
    nsIXPCSecurityManager* mSecurityManager;
    nsIException* mException;
    LangType mCallingLangType;
    PRUint16 mSecurityManagerFlags;

    
    PRCList mScopes;
};



#define NATIVE_CALLER  XPCContext::LANG_NATIVE
#define JS_CALLER      XPCContext::LANG_JS


class XPCReadableJSStringWrapper : public nsDependentString
{
public:
    typedef nsDependentString::char_traits char_traits;

    XPCReadableJSStringWrapper(const PRUnichar *chars, size_t length) :
        nsDependentString(chars, length)
    { }

    XPCReadableJSStringWrapper() :
        nsDependentString(char_traits::sEmptyBuffer, char_traits::sEmptyBuffer)
    { SetIsVoid(PR_TRUE); }

    explicit XPCReadableJSStringWrapper(JSString *str) :
        nsDependentString(reinterpret_cast<const PRUnichar *>(::JS_GetStringChars(str)),
                          str->length())
    { }
};
















class XPCCallContext : public nsAXPCNativeCallContext
{
public:
    NS_IMETHOD GetCallee(nsISupports **aResult);
    NS_IMETHOD GetCalleeMethodIndex(PRUint16 *aResult);
    NS_IMETHOD GetCalleeWrapper(nsIXPConnectWrappedNative **aResult);
    NS_IMETHOD GetJSContext(JSContext **aResult);
    NS_IMETHOD GetArgc(PRUint32 *aResult);
    NS_IMETHOD GetArgvPtr(jsval **aResult);
    NS_IMETHOD GetRetValPtr(jsval **aResult);
    NS_IMETHOD GetReturnValueWasSet(PRBool *aResult);
    NS_IMETHOD SetReturnValueWasSet(PRBool aValue);
    NS_IMETHOD GetCalleeInterface(nsIInterfaceInfo **aResult);
    NS_IMETHOD GetCalleeClassInfo(nsIClassInfo **aResult);
    NS_IMETHOD GetPreviousCallContext(nsAXPCNativeCallContext **aResult);
    NS_IMETHOD GetLanguage(PRUint16 *aResult);

    enum {NO_ARGS = (uintN) -1};

    XPCCallContext(XPCContext::LangType callerLanguage,
                   JSContext* cx    = nsnull,
                   JSObject* obj    = nsnull,
                   JSObject* funobj = nsnull,
                   jsid id          = JSID_VOID,
                   uintN argc       = NO_ARGS,
                   jsval *argv      = nsnull,
                   jsval *rval      = nsnull);

    virtual ~XPCCallContext();

    inline JSBool                       IsValid() const ;

    inline nsXPConnect*                 GetXPConnect() const ;
    inline XPCJSRuntime*                GetRuntime() const ;
    inline XPCPerThreadData*            GetThreadData() const ;
    inline XPCContext*                  GetXPCContext() const ;
    inline JSContext*                   GetJSContext() const ;
    inline JSContext*                   GetSafeJSContext() const ;
    inline JSBool                       GetContextPopRequired() const ;
    inline XPCContext::LangType         GetCallerLanguage() const ;
    inline XPCContext::LangType         GetPrevCallerLanguage() const ;
    inline XPCCallContext*              GetPrevCallContext() const ;

    inline JSObject*                    GetOperandJSObject() const ;
    inline JSObject*                    GetCurrentJSObject() const ;
    inline JSObject*                    GetFlattenedJSObject() const ;

    inline nsISupports*                 GetIdentityObject() const ;
    inline XPCWrappedNative*            GetWrapper() const ;
    inline XPCWrappedNativeProto*       GetProto() const ;

    inline JSBool                       CanGetTearOff() const ;
    inline XPCWrappedNativeTearOff*     GetTearOff() const ;

    inline XPCNativeScriptableInfo*     GetScriptableInfo() const ;
    inline JSBool                       CanGetSet() const ;
    inline XPCNativeSet*                GetSet() const ;
    inline JSBool                       CanGetInterface() const ;
    inline XPCNativeInterface*          GetInterface() const ;
    inline XPCNativeMember*             GetMember() const ;
    inline JSBool                       HasInterfaceAndMember() const ;
    inline jsid                         GetName() const ;
    inline JSBool                       GetStaticMemberIsLocal() const ;
    inline uintN                        GetArgc() const ;
    inline jsval*                       GetArgv() const ;
    inline jsval*                       GetRetVal() const ;
    inline JSBool                       GetReturnValueWasSet() const ;

    inline PRUint16                     GetMethodIndex() const ;
    inline void                         SetMethodIndex(PRUint16 index) ;

    inline JSBool   GetDestroyJSContextInDestructor() const;
    inline void     SetDestroyJSContextInDestructor(JSBool b);

    inline jsid GetResolveName() const;
    inline jsid SetResolveName(jsid name);

    inline XPCWrappedNative* GetResolvingWrapper() const;
    inline XPCWrappedNative* SetResolvingWrapper(XPCWrappedNative* w);

    inline void SetRetVal(jsval val);

    inline JSObject* GetCallee() const;
    inline void SetCallee(JSObject* callee);

    void SetName(jsid name);
    void SetArgsAndResultPtr(uintN argc, jsval *argv, jsval *rval);
    void SetCallInfo(XPCNativeInterface* iface, XPCNativeMember* member,
                     JSBool isSetter);

    nsresult  CanCallNow();

    void SystemIsBeingShutDown();

    operator JSContext*() const {return GetJSContext();}

    XPCReadableJSStringWrapper *NewStringWrapper(const PRUnichar *str,
                                                 PRUint32 len);
    void DeleteString(nsAString *string);

#ifdef XPC_IDISPATCH_SUPPORT
    




    void SetIDispatchInfo(XPCNativeInterface* iface, void * member);
    void* GetIDispatchMember() const { return mIDispatchMember; }
#endif
private:

    
    XPCCallContext(const XPCCallContext& r); 
    XPCCallContext& operator= (const XPCCallContext& r); 

    friend class XPCLazyCallContext;
    XPCCallContext(XPCContext::LangType callerLanguage,
                   JSContext* cx,
                   JSBool callBeginRequest,
                   JSObject* obj,
                   JSObject* currentJSObject,
                   XPCWrappedNative* wn,
                   XPCWrappedNativeTearOff* tearoff);

    void Init(XPCContext::LangType callerLanguage,
              JSBool callBeginRequest,
              JSObject* obj,
              JSObject* funobj,
              JSBool getWrappedNative,
              jsid name,
              uintN argc,
              jsval *argv,
              jsval *rval);

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
inline void CHECK_STATE(int s) const {NS_ASSERTION(mState >= s, "bad state");}
#else
#define CHECK_STATE(s) ((void)0)
#endif

private:
    State                           mState;

    nsXPConnect*                    mXPC;

    XPCPerThreadData*               mThreadData;
    XPCContext*                     mXPCContext;
    JSContext*                      mJSContext;
    JSBool                          mContextPopRequired;
    JSBool                          mDestroyJSContextInDestructor;

    XPCContext::LangType            mCallerLanguage;

    

    XPCContext::LangType            mPrevCallerLanguage;

    XPCCallContext*                 mPrevCallContext;

    JSObject*                       mOperandJSObject;
    JSObject*                       mCurrentJSObject;
    JSObject*                       mFlattenedJSObject;
    XPCWrappedNative*               mWrapper;
    XPCWrappedNativeTearOff*        mTearOff;

    XPCNativeScriptableInfo*        mScriptableInfo;

    XPCNativeSet*                   mSet;
    XPCNativeInterface*             mInterface;
    XPCNativeMember*                mMember;

    jsid                            mName;
    JSBool                          mStaticMemberIsLocal;

    uintN                           mArgc;
    jsval*                          mArgv;
    jsval*                          mRetVal;

    JSBool                          mReturnValueWasSet;
#ifdef XPC_IDISPATCH_SUPPORT
    void*                           mIDispatchMember;
#endif
    PRUint16                        mMethodIndex;

    
    
    
    
    JSObject*                       mCallee;

#define XPCCCX_STRING_CACHE_SIZE 2

    
    
    struct StringWrapperEntry
    {
        StringWrapperEntry()
            : mInUse(PR_FALSE)
        {
        }

        XPCReadableJSStringWrapper mString;
        PRBool mInUse;
    };

    
    
    
    
    
    char mStringWrapperData[sizeof(StringWrapperEntry) * XPCCCX_STRING_CACHE_SIZE];
};

class XPCLazyCallContext
{
public:
    XPCLazyCallContext(XPCCallContext& ccx)
        : mCallBeginRequest(DONT_CALL_BEGINREQUEST),
          mCcx(&ccx),
          mCcxToDestroy(nsnull)
#ifdef DEBUG
          , mCx(nsnull)
          , mCallerLanguage(JS_CALLER)
          , mObj(nsnull)
          , mCurrentJSObject(nsnull)
          , mWrapper(nsnull)
          , mTearOff(nsnull)
#endif
    {
    }
    XPCLazyCallContext(XPCContext::LangType callerLanguage, JSContext* cx,
                       JSObject* obj = nsnull,
                       JSObject* currentJSObject = nsnull,
                       XPCWrappedNative* wrapper = nsnull,
                       XPCWrappedNativeTearOff* tearoff = nsnull)
        : mCallBeginRequest(callerLanguage == NATIVE_CALLER ?
                            CALL_BEGINREQUEST : DONT_CALL_BEGINREQUEST),
          mCcx(nsnull),
          mCcxToDestroy(nsnull),
          mCx(cx),
          mCallerLanguage(callerLanguage),
          mObj(obj),
          mCurrentJSObject(currentJSObject),
          mWrapper(wrapper),
          mTearOff(tearoff)
    {
        NS_ASSERTION(cx, "Need a JS context!");
        NS_ASSERTION(callerLanguage == NATIVE_CALLER ||
                     callerLanguage == JS_CALLER,
                     "Can't deal with unknown caller language!");
#ifdef DEBUG
        AssertContextIsTopOfStack(cx);
#endif
    }
    ~XPCLazyCallContext()
    {
        if(mCcxToDestroy)
            mCcxToDestroy->~XPCCallContext();
        else if(mCallBeginRequest == CALLED_BEGINREQUEST)
            JS_EndRequest(mCx);
    }
    void SetWrapper(XPCWrappedNative* wrapper,
                    XPCWrappedNativeTearOff* tearoff);
    void SetWrapper(JSObject* currentJSObject);

    JSContext *GetJSContext()
    {
        if(mCcx)
            return mCcx->GetJSContext();

        if(mCallBeginRequest == CALL_BEGINREQUEST) {
            JS_BeginRequest(mCx);
            mCallBeginRequest = CALLED_BEGINREQUEST;
        }

        return mCx;
    }
    JSObject *GetCurrentJSObject() const
    {
        if(mCcx)
            return mCcx->GetCurrentJSObject();

        return mCurrentJSObject;
    }
    XPCCallContext &GetXPCCallContext()
    {
        if(!mCcx)
        {
            mCcxToDestroy = mCcx =
                new (mData) XPCCallContext(mCallerLanguage, mCx,
                                           mCallBeginRequest == CALL_BEGINREQUEST,
                                           mObj,
                                           mCurrentJSObject, mWrapper,
                                           mTearOff);
            if(!mCcx->IsValid())
            {
                NS_ERROR("This is not supposed to fail!");
            }
        }

        return *mCcx;
    }

private:
#ifdef DEBUG
    static void AssertContextIsTopOfStack(JSContext* cx);
#endif

    enum {
        DONT_CALL_BEGINREQUEST,
        CALL_BEGINREQUEST,
        CALLED_BEGINREQUEST
    } mCallBeginRequest;

    XPCCallContext *mCcx;
    XPCCallContext *mCcxToDestroy;
    JSContext *mCx;
    XPCContext::LangType mCallerLanguage;
    JSObject *mObj;
    JSObject *mCurrentJSObject;
    XPCWrappedNative *mWrapper;
    XPCWrappedNativeTearOff *mTearOff;
    char mData[sizeof(XPCCallContext)];
};












extern js::Class XPC_WN_NoHelper_JSClass;
extern js::Class XPC_WN_NoMods_WithCall_Proto_JSClass;
extern js::Class XPC_WN_NoMods_NoCall_Proto_JSClass;
extern js::Class XPC_WN_ModsAllowed_WithCall_Proto_JSClass;
extern js::Class XPC_WN_ModsAllowed_NoCall_Proto_JSClass;
extern js::Class XPC_WN_Tearoff_JSClass;
extern js::Class XPC_WN_NoHelper_Proto_JSClass;

extern JSBool
XPC_WN_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

extern JSBool
XPC_WN_CallMethod(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
XPC_WN_GetterSetter(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
XPC_WN_JSOp_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                      jsval *statep, jsid *idp);

extern JSType
XPC_WN_JSOp_TypeOf_Object(JSContext *cx, JSObject *obj);

extern JSType
XPC_WN_JSOp_TypeOf_Function(JSContext *cx, JSObject *obj);

extern void
XPC_WN_JSOp_Clear(JSContext *cx, JSObject *obj);

extern JSObject*
XPC_WN_JSOp_ThisObject(JSContext *cx, JSObject *obj);


#define XPC_WN_WithCall_ObjectOps                                             \
    {                                                                         \
        nsnull, /* lookupProperty */                                          \
        nsnull, /* defineProperty */                                          \
        nsnull, /* getProperty    */                                          \
        nsnull, /* setProperty    */                                          \
        nsnull, /* getAttributes  */                                          \
        nsnull, /* setAttributes  */                                          \
        nsnull, /* deleteProperty */                                          \
        js::Valueify(XPC_WN_JSOp_Enumerate),                                  \
        XPC_WN_JSOp_TypeOf_Function,                                          \
        nsnull, /* trace          */                                          \
        nsnull, /* fix            */                                          \
        XPC_WN_JSOp_ThisObject,                                               \
        XPC_WN_JSOp_Clear                                                     \
    }

#define XPC_WN_NoCall_ObjectOps                                               \
    {                                                                         \
        nsnull, /* lookupProperty */                                          \
        nsnull, /* defineProperty */                                          \
        nsnull, /* getProperty    */                                          \
        nsnull, /* setProperty    */                                          \
        nsnull, /* getAttributes  */                                          \
        nsnull, /* setAttributes  */                                          \
        nsnull, /* deleteProperty */                                          \
        js::Valueify(XPC_WN_JSOp_Enumerate),                                  \
        XPC_WN_JSOp_TypeOf_Object,                                            \
        nsnull, /* trace          */                                          \
        nsnull, /* fix            */                                          \
        XPC_WN_JSOp_ThisObject,                                               \
        XPC_WN_JSOp_Clear                                                     \
    }




#define IS_PROTO_CLASS(clazz)                                                 \
    ((clazz) == &XPC_WN_NoMods_WithCall_Proto_JSClass ||                      \
     (clazz) == &XPC_WN_NoMods_NoCall_Proto_JSClass ||                        \
     (clazz) == &XPC_WN_ModsAllowed_WithCall_Proto_JSClass ||                 \
     (clazz) == &XPC_WN_ModsAllowed_NoCall_Proto_JSClass)


extern void
xpc_TraceForValidWrapper(JSTracer *trc, XPCWrappedNative* wrapper);



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
    GetNewOrUsed(XPCCallContext& ccx, JSObject* aGlobal);

    XPCJSRuntime*
    GetRuntime() const {return mRuntime;}

    Native2WrappedNativeMap*
    GetWrappedNativeMap() const {return mWrappedNativeMap;}

    ClassInfo2WrappedNativeProtoMap*
    GetWrappedNativeProtoMap(JSBool aMainThreadOnly) const
        {return aMainThreadOnly ?
                mMainThreadWrappedNativeProtoMap :
                mWrappedNativeProtoMap;}

    nsXPCComponents*
    GetComponents() const {return mComponents;}

    JSObject*
    GetGlobalJSObject() const {return mGlobalJSObject;}

    JSObject*
    GetPrototypeJSObject() const {return mPrototypeJSObject;}

    
    
    JSObject*
    GetPrototypeNoHelper(XPCCallContext& ccx);

#ifndef XPCONNECT_STANDALONE
    nsIPrincipal*
    GetPrincipal() const
    {return mScriptObjectPrincipal ?
         mScriptObjectPrincipal->GetPrincipal() : nsnull;}
#endif
    
    JSObject*
    GetPrototypeJSFunction() const {return mPrototypeJSFunction;}

    void RemoveWrappedNativeProtos();

    static XPCWrappedNativeScope*
    FindInJSObjectScope(JSContext* cx, JSObject* obj,
                        JSBool OKIfNotInitialized = JS_FALSE,
                        XPCJSRuntime* runtime = nsnull);

    static XPCWrappedNativeScope*
    FindInJSObjectScope(XPCCallContext& ccx, JSObject* obj,
                        JSBool OKIfNotInitialized = JS_FALSE)
    {
        return FindInJSObjectScope(ccx, obj, OKIfNotInitialized,
                                   ccx.GetRuntime());
    }

    static void
    SystemIsBeingShutDown(JSContext* cx);

    static void
    TraceJS(JSTracer* trc, XPCJSRuntime* rt);

    static void
    SuspectAllWrappers(XPCJSRuntime* rt, JSContext* cx,
                       nsCycleCollectionTraversalCallback &cb);

    static void
    FinishedMarkPhaseOfGC(JSContext* cx, XPCJSRuntime* rt);

    static void
    FinishedFinalizationPhaseOfGC(JSContext* cx);

    static void
    MarkAllWrappedNativesAndProtos();

    static nsresult
    ClearAllWrappedNativeSecurityPolicies(XPCCallContext& ccx);

#ifdef DEBUG
    static void
    ASSERT_NoInterfaceSetsAreMarked();
#endif

    static void
    SweepAllWrappedNativeTearOffs();

    static void
    DebugDumpAllScopes(PRInt16 depth);

    void
    DebugDump(PRInt16 depth);

    JSBool
    IsValid() const {return mRuntime != nsnull;}

    static JSBool
    IsDyingScope(XPCWrappedNativeScope *scope);

    void SetComponents(nsXPCComponents* aComponents);
    void SetGlobal(XPCCallContext& ccx, JSObject* aGlobal);

    static void InitStatics() { gScopes = nsnull; gDyingScopes = nsnull; }

    XPCContext *GetContext() { return mContext; }
    void SetContext(XPCContext *xpcc) { mContext = nsnull; }

protected:
    XPCWrappedNativeScope(XPCCallContext& ccx, JSObject* aGlobal);
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
    nsXPCComponents*                 mComponents;
    XPCWrappedNativeScope*           mNext;
    
    
    
    
    JSObject*                        mGlobalJSObject;

    
    JSObject*                        mPrototypeJSObject;
    
    JSObject*                        mPrototypeJSFunction;
    
    JSObject*                        mPrototypeNoHelper;

    XPCContext*                      mContext;

#ifndef XPCONNECT_STANDALONE
    
    
    
    
    
    nsIScriptObjectPrincipal* mScriptObjectPrincipal;
#endif
};

JSObject* xpc_CloneJSFunction(XPCCallContext &ccx, JSObject *funobj,
                              JSObject *parent);







class XPCNativeMember
{
public:
    static JSBool GetCallInfo(XPCCallContext& ccx,
                              JSObject* funobj,
                              XPCNativeInterface** pInterface,
                              XPCNativeMember**    pMember);

    jsid   GetName() const {return mName;}

    PRUint16 GetIndex() const {return mIndex;}

    JSBool GetConstantValue(XPCCallContext& ccx, XPCNativeInterface* iface,
                            jsval* pval)
        {NS_ASSERTION(IsConstant(),
                      "Only call this if you're sure this is a constant!");
         return Resolve(ccx, iface, nsnull, pval);}

    JSBool NewFunctionObject(XPCCallContext& ccx, XPCNativeInterface* iface,
                             JSObject *parent, jsval* pval);

    JSBool IsMethod() const
        {return 0 != (mFlags & METHOD);}

    JSBool IsConstant() const
        {return 0 != (mFlags & CONSTANT);}

    JSBool IsAttribute() const
        {return 0 != (mFlags & GETTER);}

    JSBool IsWritableAttribute() const
        {return 0 != (mFlags & SETTER_TOO);}

    JSBool IsReadOnlyAttribute() const
        {return IsAttribute() && !IsWritableAttribute();}


    void SetName(jsid a) {mName = a;}

    void SetMethod(PRUint16 index)
        {mFlags = METHOD; mIndex = index;}

    void SetConstant(PRUint16 index)
        {mFlags = CONSTANT; mIndex = index;}

    void SetReadOnlyAttribute(PRUint16 index)
        {mFlags = GETTER; mIndex = index;}

    void SetWritableAttribute()
        {NS_ASSERTION(mFlags == GETTER,"bad"); mFlags = GETTER | SETTER_TOO;}

    
    XPCNativeMember()  {MOZ_COUNT_CTOR(XPCNativeMember);}
    ~XPCNativeMember() {MOZ_COUNT_DTOR(XPCNativeMember);}

private:
    JSBool Resolve(XPCCallContext& ccx, XPCNativeInterface* iface,
                   JSObject *parent, jsval *vp);

    enum {
        METHOD      = 0x01,
        CONSTANT    = 0x02,
        GETTER      = 0x04,
        SETTER_TOO  = 0x08
    };

private:
    
    jsid     mName;
    PRUint16 mIndex;
    PRUint16 mFlags;
};







class XPCNativeInterface
{
public:
    static XPCNativeInterface* GetNewOrUsed(XPCCallContext& ccx,
                                            const nsIID* iid);
    static XPCNativeInterface* GetNewOrUsed(XPCCallContext& ccx,
                                            nsIInterfaceInfo* info);
    static XPCNativeInterface* GetNewOrUsed(XPCCallContext& ccx,
                                            const char* name);
    static XPCNativeInterface* GetISupports(XPCCallContext& ccx);

    inline nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo.get();}
    inline jsid              GetName()          const {return mName;}

    inline const nsIID* GetIID() const;
    inline const char*  GetNameString() const;
    inline XPCNativeMember* FindMember(jsid name) const;

    inline JSBool HasAncestor(const nsIID* iid) const;

    PRUint16 GetMemberCount() const
        {NS_ASSERTION(!IsMarked(), "bad"); return mMemberCount;}
    XPCNativeMember* GetMemberAt(PRUint16 i)
        {NS_ASSERTION(i < mMemberCount, "bad index"); return &mMembers[i];}

    void DebugDump(PRInt16 depth);

#define XPC_NATIVE_IFACE_MARK_FLAG ((PRUint16)JS_BIT(15)) // only high bit of 16 is set

    void Mark()     {mMemberCount |= XPC_NATIVE_IFACE_MARK_FLAG;}
    void Unmark()   {mMemberCount &= ~XPC_NATIVE_IFACE_MARK_FLAG;}
    JSBool IsMarked() const
                    {return 0 != (mMemberCount & XPC_NATIVE_IFACE_MARK_FLAG);}

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

    static void DestroyInstance(XPCNativeInterface* inst);

protected:
    static XPCNativeInterface* NewInstance(XPCCallContext& ccx,
                                           nsIInterfaceInfo* aInfo);

    XPCNativeInterface();   
    XPCNativeInterface(nsIInterfaceInfo* aInfo, jsid aName)
        : mInfo(aInfo), mName(aName), mMemberCount(0)
                          {MOZ_COUNT_CTOR(XPCNativeInterface);}
    ~XPCNativeInterface() {MOZ_COUNT_DTOR(XPCNativeInterface);}

    void* operator new(size_t, void* p) CPP_THROW_NEW {return p;}

    XPCNativeInterface(const XPCNativeInterface& r); 
    XPCNativeInterface& operator= (const XPCNativeInterface& r); 

private:
    nsCOMPtr<nsIInterfaceInfo> mInfo;
    jsid                       mName;
    PRUint16          mMemberCount;
    XPCNativeMember   mMembers[1]; 
};




class XPCNativeSetKey
{
public:
    XPCNativeSetKey(XPCNativeSet*       BaseSet  = nsnull,
                    XPCNativeInterface* Addition = nsnull,
                    PRUint16            Position = 0)
        : mIsAKey(IS_A_KEY), mPosition(Position), mBaseSet(BaseSet),
          mAddition(Addition) {}
    ~XPCNativeSetKey() {}

    XPCNativeSet*           GetBaseSet()  const {return mBaseSet;}
    XPCNativeInterface*     GetAddition() const {return mAddition;}
    PRUint16                GetPosition() const {return mPosition;}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JSBool                  IsAKey() const {return mIsAKey == IS_A_KEY;}

    enum {IS_A_KEY = 0xffff};

    

private:
    PRUint16                mIsAKey;    
    PRUint16                mPosition;
    XPCNativeSet*           mBaseSet;
    XPCNativeInterface*     mAddition;
};




class XPCNativeSet
{
public:
    static XPCNativeSet* GetNewOrUsed(XPCCallContext& ccx, const nsIID* iid);
    static XPCNativeSet* GetNewOrUsed(XPCCallContext& ccx,
                                      nsIClassInfo* classInfo);
    static XPCNativeSet* GetNewOrUsed(XPCCallContext& ccx,
                                      XPCNativeSet* otherSet,
                                      XPCNativeInterface* newInterface,
                                      PRUint16 position);

    static void ClearCacheEntryForClassInfo(nsIClassInfo* classInfo);

    inline JSBool FindMember(jsid name, XPCNativeMember** pMember,
                             PRUint16* pInterfaceIndex) const;

    inline JSBool FindMember(jsid name, XPCNativeMember** pMember,
                             XPCNativeInterface** pInterface) const;

    inline JSBool FindMember(jsid name,
                             XPCNativeMember** pMember,
                             XPCNativeInterface** pInterface,
                             XPCNativeSet* protoSet,
                             JSBool* pIsLocal) const;

    inline JSBool HasInterface(XPCNativeInterface* aInterface) const;
    inline JSBool HasInterfaceWithAncestor(XPCNativeInterface* aInterface) const;
    inline JSBool HasInterfaceWithAncestor(const nsIID* iid) const;

    inline XPCNativeInterface* FindInterfaceWithIID(const nsIID& iid) const;

    inline XPCNativeInterface* FindNamedInterface(jsid name) const;

    PRUint16 GetMemberCount() const {return mMemberCount;}
    PRUint16 GetInterfaceCount() const
        {NS_ASSERTION(!IsMarked(), "bad"); return mInterfaceCount;}
    XPCNativeInterface** GetInterfaceArray() {return mInterfaces;}

    XPCNativeInterface* GetInterfaceAt(PRUint16 i)
        {NS_ASSERTION(i < mInterfaceCount, "bad index"); return mInterfaces[i];}

    inline JSBool MatchesSetUpToInterface(const XPCNativeSet* other,
                                          XPCNativeInterface* iface) const;

#define XPC_NATIVE_SET_MARK_FLAG ((PRUint16)JS_BIT(15)) // only high bit of 16 is set

    inline void Mark();

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

private:
    void MarkSelfOnly() {mInterfaceCount |= XPC_NATIVE_SET_MARK_FLAG;}
public:
    void Unmark()       {mInterfaceCount &= ~XPC_NATIVE_SET_MARK_FLAG;}
    JSBool IsMarked() const
                  {return 0 != (mInterfaceCount & XPC_NATIVE_SET_MARK_FLAG);}

#ifdef DEBUG
    inline void ASSERT_NotMarked();
#endif

    void DebugDump(PRInt16 depth);

    static void DestroyInstance(XPCNativeSet* inst);

protected:
    static XPCNativeSet* NewInstance(XPCCallContext& ccx,
                                     XPCNativeInterface** array,
                                     PRUint16 count);
    static XPCNativeSet* NewInstanceMutate(XPCNativeSet*       otherSet,
                                           XPCNativeInterface* newInterface,
                                           PRUint16            position);
    XPCNativeSet()  {MOZ_COUNT_CTOR(XPCNativeSet);}
    ~XPCNativeSet() {MOZ_COUNT_DTOR(XPCNativeSet);}
    void* operator new(size_t, void* p) CPP_THROW_NEW {return p;}

private:
    PRUint16                mMemberCount;
    PRUint16                mInterfaceCount;
    XPCNativeInterface*     mInterfaces[1];  
};








#define XPC_WN_SJSFLAGS_MARK_FLAG JS_BIT(31) // only high bit of 32 is set

class XPCNativeScriptableFlags
{
private:
    JSUint32 mFlags;

public:

    XPCNativeScriptableFlags(JSUint32 flags = 0) : mFlags(flags) {}

    JSUint32 GetFlags() const {return mFlags & ~XPC_WN_SJSFLAGS_MARK_FLAG;}
    void     SetFlags(JSUint32 flags) {mFlags = flags;}

    operator JSUint32() const {return GetFlags();}

    XPCNativeScriptableFlags(const XPCNativeScriptableFlags& r)
        {mFlags = r.GetFlags();}

    XPCNativeScriptableFlags& operator= (const XPCNativeScriptableFlags& r)
        {mFlags = r.GetFlags(); return *this;}

    void Mark()       {mFlags |= XPC_WN_SJSFLAGS_MARK_FLAG;}
    void Unmark()     {mFlags &= ~XPC_WN_SJSFLAGS_MARK_FLAG;}
    JSBool IsMarked() const {return 0 != (mFlags & XPC_WN_SJSFLAGS_MARK_FLAG);}

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return 0 != (mFlags & nsIXPCScriptable:: f_ );}

    JSBool WantPreCreate()                GET_IT(WANT_PRECREATE)
    JSBool WantCreate()                   GET_IT(WANT_CREATE)
    JSBool WantPostCreate()               GET_IT(WANT_POSTCREATE)
    JSBool WantAddProperty()              GET_IT(WANT_ADDPROPERTY)
    JSBool WantDelProperty()              GET_IT(WANT_DELPROPERTY)
    JSBool WantGetProperty()              GET_IT(WANT_GETPROPERTY)
    JSBool WantSetProperty()              GET_IT(WANT_SETPROPERTY)
    JSBool WantEnumerate()                GET_IT(WANT_ENUMERATE)
    JSBool WantNewEnumerate()             GET_IT(WANT_NEWENUMERATE)
    JSBool WantNewResolve()               GET_IT(WANT_NEWRESOLVE)
    JSBool WantConvert()                  GET_IT(WANT_CONVERT)
    JSBool WantFinalize()                 GET_IT(WANT_FINALIZE)
    JSBool WantCheckAccess()              GET_IT(WANT_CHECKACCESS)
    JSBool WantCall()                     GET_IT(WANT_CALL)
    JSBool WantConstruct()                GET_IT(WANT_CONSTRUCT)
    JSBool WantHasInstance()              GET_IT(WANT_HASINSTANCE)
    JSBool WantTrace()                    GET_IT(WANT_TRACE)
    JSBool WantEquality()                 GET_IT(WANT_EQUALITY)
    JSBool WantOuterObject()              GET_IT(WANT_OUTER_OBJECT)
    JSBool UseJSStubForAddProperty()      GET_IT(USE_JSSTUB_FOR_ADDPROPERTY)
    JSBool UseJSStubForDelProperty()      GET_IT(USE_JSSTUB_FOR_DELPROPERTY)
    JSBool UseJSStubForSetProperty()      GET_IT(USE_JSSTUB_FOR_SETPROPERTY)
    JSBool DontEnumStaticProps()          GET_IT(DONT_ENUM_STATIC_PROPS)
    JSBool DontEnumQueryInterface()       GET_IT(DONT_ENUM_QUERY_INTERFACE)
    JSBool DontAskInstanceForScriptable() GET_IT(DONT_ASK_INSTANCE_FOR_SCRIPTABLE)
    JSBool ClassInfoInterfacesOnly()      GET_IT(CLASSINFO_INTERFACES_ONLY)
    JSBool AllowPropModsDuringResolve()   GET_IT(ALLOW_PROP_MODS_DURING_RESOLVE)
    JSBool AllowPropModsToPrototype()     GET_IT(ALLOW_PROP_MODS_TO_PROTOTYPE)
    JSBool DontSharePrototype()           GET_IT(DONT_SHARE_PROTOTYPE)
    JSBool DontReflectInterfaceNames()    GET_IT(DONT_REFLECT_INTERFACE_NAMES)

#undef GET_IT
};











struct XPCNativeScriptableSharedJSClass
{
    js::Class base;
    PRUint32 interfacesBitmap;
};

class XPCNativeScriptableShared
{
public:
    const XPCNativeScriptableFlags& GetFlags() const {return mFlags;}
    PRUint32                        GetInterfacesBitmap() const
        {return mJSClass.interfacesBitmap;}
    JSClass*                        GetJSClass()
        {return js::Jsvalify(&mJSClass.base);}
    JSClass*                        GetSlimJSClass()
        {if(mCanBeSlim) return GetJSClass(); return nsnull;}

    XPCNativeScriptableShared(JSUint32 aFlags, char* aName,
                              PRUint32 interfacesBitmap)
        : mFlags(aFlags),
          mCanBeSlim(JS_FALSE)
        {memset(&mJSClass, 0, sizeof(mJSClass));
         mJSClass.base.name = aName;  
         mJSClass.interfacesBitmap = interfacesBitmap;
         MOZ_COUNT_CTOR(XPCNativeScriptableShared);}

    ~XPCNativeScriptableShared()
        {if(mJSClass.base.name)nsMemory::Free((void*)mJSClass.base.name);
         MOZ_COUNT_DTOR(XPCNativeScriptableShared);}

    char* TransferNameOwnership()
        {char* name=(char*)mJSClass.base.name; mJSClass.base.name = nsnull;
        return name;}

    void PopulateJSClass(JSBool isGlobal);

    void Mark()       {mFlags.Mark();}
    void Unmark()     {mFlags.Unmark();}
    JSBool IsMarked() const {return mFlags.IsMarked();}

private:
    XPCNativeScriptableFlags mFlags;
    XPCNativeScriptableSharedJSClass mJSClass;
    JSBool                   mCanBeSlim;
};





class XPCNativeScriptableInfo
{
public:
    static XPCNativeScriptableInfo*
    Construct(XPCCallContext& ccx, JSBool isGlobal,
              const XPCNativeScriptableCreateInfo* sci);

    nsIXPCScriptable*
    GetCallback() const {return mCallback;}

    const XPCNativeScriptableFlags&
    GetFlags() const      {return mShared->GetFlags();}

    PRUint32
    GetInterfacesBitmap() const {return mShared->GetInterfacesBitmap();}

    JSClass*
    GetJSClass()          {return mShared->GetJSClass();}

    JSClass*
    GetSlimJSClass()      {return mShared->GetSlimJSClass();}

    XPCNativeScriptableShared*
    GetScriptableShared() {return mShared;}

    void
    SetCallback(nsIXPCScriptable* s) {mCallback = s;}
    void
    SetCallback(already_AddRefed<nsIXPCScriptable> s) {mCallback = s;}

    void
    SetScriptableShared(XPCNativeScriptableShared* shared) {mShared = shared;}

    void Mark() {if(mShared) mShared->Mark();}

protected:
    XPCNativeScriptableInfo(nsIXPCScriptable* scriptable = nsnull,
                            XPCNativeScriptableShared* shared = nsnull)
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






class NS_STACK_CLASS XPCNativeScriptableCreateInfo
{
public:

    XPCNativeScriptableCreateInfo(const XPCNativeScriptableInfo& si)
        : mCallback(si.GetCallback()), mFlags(si.GetFlags()),
          mInterfacesBitmap(si.GetInterfacesBitmap()) {}

    XPCNativeScriptableCreateInfo(already_AddRefed<nsIXPCScriptable> callback,
                                  XPCNativeScriptableFlags flags,
                                  PRUint32 interfacesBitmap)
        : mCallback(callback), mFlags(flags),
          mInterfacesBitmap(interfacesBitmap) {}

    XPCNativeScriptableCreateInfo()
        : mFlags(0), mInterfacesBitmap(0) {}


    nsIXPCScriptable*
    GetCallback() const {return mCallback;}

    const XPCNativeScriptableFlags&
    GetFlags() const      {return mFlags;}

    PRUint32
    GetInterfacesBitmap() const     {return mInterfacesBitmap;}

    void
    SetCallback(already_AddRefed<nsIXPCScriptable> callback)
        {mCallback = callback;}

    void
    SetFlags(const XPCNativeScriptableFlags& flags)  {mFlags = flags;}

    void
    SetInterfacesBitmap(PRUint32 interfacesBitmap)
        {mInterfacesBitmap = interfacesBitmap;}

private:
    nsCOMPtr<nsIXPCScriptable>  mCallback;
    XPCNativeScriptableFlags    mFlags;
    PRUint32                    mInterfacesBitmap;
};





#define UNKNOWN_OFFSETS ((QITableEntry*)1)

class XPCWrappedNativeProto
{
public:
    static XPCWrappedNativeProto*
    GetNewOrUsed(XPCCallContext& ccx,
                 XPCWrappedNativeScope* Scope,
                 nsIClassInfo* ClassInfo,
                 const XPCNativeScriptableCreateInfo* ScriptableCreateInfo,
                 JSBool ForceNoSharing,
                 JSBool isGlobal,
                 QITableEntry* offsets = UNKNOWN_OFFSETS);

    XPCWrappedNativeScope*
    GetScope()   const {return mScope;}

    XPCJSRuntime*
    GetRuntime() const {return mScope->GetRuntime();}

    JSObject*
    GetJSProtoObject() const {return mJSProtoObject;}

    nsIClassInfo*
    GetClassInfo()     const {return mClassInfo;}

    XPCNativeSet*
    GetSet()           const {return mSet;}

    XPCNativeScriptableInfo*
    GetScriptableInfo()   {return mScriptableInfo;}

    void**
    GetSecurityInfoAddr() {return &mSecurityInfo;}

    JSUint32
    GetClassInfoFlags() const {return mClassInfoFlags;}

    QITableEntry*
    GetOffsets()
    {
        return InitedOffsets() ? mOffsets : nsnull;
    }
    QITableEntry*
    GetOffsetsMasked()
    {
        return mOffsets;
    }
    void
    CacheOffsets(nsISupports* identity)
    {
        static NS_DEFINE_IID(kThisPtrOffsetsSID, NS_THISPTROFFSETS_SID);

#ifdef DEBUG
        if(InitedOffsets() && mOffsets)
        {
            QITableEntry* offsets;
            identity->QueryInterface(kThisPtrOffsetsSID, (void**)&offsets);
            NS_ASSERTION(offsets == mOffsets,
                         "We can't deal with objects that have the same "
                         "classinfo but different offset tables.");
        }
#endif

        if(!InitedOffsets())
        {
            if(mClassInfoFlags & nsIClassInfo::CONTENT_NODE)
            {
                identity->QueryInterface(kThisPtrOffsetsSID, (void**)&mOffsets);
            }
            else
            {
                mOffsets = nsnull;
            }
        }
    }

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return !!(mClassInfoFlags & nsIClassInfo:: f_ );}

    JSBool ClassIsSingleton()           GET_IT(SINGLETON)
    JSBool ClassIsThreadSafe()          GET_IT(THREADSAFE)
    JSBool ClassIsMainThreadOnly()      GET_IT(MAIN_THREAD_ONLY)
    JSBool ClassIsDOMObject()           GET_IT(DOM_OBJECT)
    JSBool ClassIsPluginObject()        GET_IT(PLUGIN_OBJECT)

#undef GET_IT

#define XPC_PROTO_DONT_SHARE JS_BIT(31) // only high bit of 32 is set

    JSBool
    IsShared() const {return !(mClassInfoFlags & XPC_PROTO_DONT_SHARE);}

    XPCLock* GetLock() const
        {return ClassIsThreadSafe() ? GetRuntime()->GetMapLock() : nsnull;}

    void SetScriptableInfo(XPCNativeScriptableInfo* si)
        {NS_ASSERTION(!mScriptableInfo, "leak here!"); mScriptableInfo = si;}

    void JSProtoObjectFinalized(JSContext *cx, JSObject *obj);

    void SystemIsBeingShutDown(JSContext* cx);

    void DebugDump(PRInt16 depth);

    void TraceJS(JSTracer* trc)
    {
        if(mJSProtoObject)
        {
            JS_CALL_OBJECT_TRACER(trc, mJSProtoObject,
                                  "XPCWrappedNativeProto::mJSProtoObject");
        }
        if(mScriptableInfo && JS_IsGCMarkingTracer(trc))
            mScriptableInfo->Mark();
    }

    
    inline void AutoTrace(JSTracer* trc) {}

    
    void Mark() const
        {mSet->Mark(); 
         if(mScriptableInfo) mScriptableInfo->Mark();}

#ifdef DEBUG
    void ASSERT_SetNotMarked() const {mSet->ASSERT_NotMarked();}
#endif

    ~XPCWrappedNativeProto();

protected:
    
    XPCWrappedNativeProto(const XPCWrappedNativeProto& r); 
    XPCWrappedNativeProto& operator= (const XPCWrappedNativeProto& r); 

    
    XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                          nsIClassInfo* ClassInfo,
                          PRUint32 ClassInfoFlags,
                          XPCNativeSet* Set,
                          QITableEntry* offsets);

    JSBool Init(XPCCallContext& ccx, JSBool isGlobal,
                const XPCNativeScriptableCreateInfo* scriptableCreateInfo);

private:
#if defined(DEBUG_xpc_hacker) || defined(DEBUG)
    static PRInt32 gDEBUG_LiveProtoCount;
#endif

private:
    PRBool
    InitedOffsets()
    {
        return mOffsets != UNKNOWN_OFFSETS;
    }

    XPCWrappedNativeScope*   mScope;
    JSObject*                mJSProtoObject;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    PRUint32                 mClassInfoFlags;
    XPCNativeSet*            mSet;
    void*                    mSecurityInfo;
    XPCNativeScriptableInfo* mScriptableInfo;
    QITableEntry*            mOffsets;
};

class xpcObjectHelper;
JSObject *
ConstructProxyObject(XPCCallContext &ccx,
                     xpcObjectHelper &aHelper,
                     XPCWrappedNativeScope *xpcscope);
extern JSBool ConstructSlimWrapper(XPCCallContext &ccx,
                                   xpcObjectHelper &aHelper,
                                   XPCWrappedNativeScope* xpcScope,
                                   jsval *rval);
extern JSBool MorphSlimWrapper(JSContext *cx, JSObject *obj);

static inline XPCWrappedNativeProto*
GetSlimWrapperProto(JSObject *obj)
{
  const js::Value &v = obj->getSlot(0);
  return static_cast<XPCWrappedNativeProto*>(v.toPrivate());
}






class XPCWrappedNativeTearOff
{
public:
    JSBool IsAvailable() const {return mInterface == nsnull;}
    JSBool IsReserved()  const {return mInterface == (XPCNativeInterface*)1;}
    JSBool IsValid()     const {return !IsAvailable() && !IsReserved();}
    void   SetReserved()       {mInterface = (XPCNativeInterface*)1;}

    XPCNativeInterface* GetInterface() const {return mInterface;}
    nsISupports*        GetNative()    const {return mNative;}
    JSObject*           GetJSObject()  const;
    void SetInterface(XPCNativeInterface*  Interface) {mInterface = Interface;}
    void SetNative(nsISupports*  Native)              {mNative = Native;}
    void SetJSObject(JSObject*  JSObj);

    void JSObjectFinalized() {SetJSObject(nsnull);}

    XPCWrappedNativeTearOff()
        : mInterface(nsnull), mNative(nsnull), mJSObject(nsnull) {}
    ~XPCWrappedNativeTearOff();

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

    void Mark()       {mJSObject = (JSObject*)(((jsword)mJSObject) | 1);}
    void Unmark()     {mJSObject = (JSObject*)(((jsword)mJSObject) & ~1);}
    JSBool IsMarked() const {return (JSBool)(((jsword)mJSObject) & 1);}

#ifdef XPC_IDISPATCH_SUPPORT
    enum JSObject_flags
    {
        IDISPATCH_BIT = 2,
        JSOBJECT_MASK = 3
    };
    void                SetIDispatch(JSContext* cx);
    JSBool              IsIDispatch() const;
    XPCDispInterface*   GetIDispatchInfo() const;
#endif
private:
    XPCWrappedNativeTearOff(const XPCWrappedNativeTearOff& r); 
    XPCWrappedNativeTearOff& operator= (const XPCWrappedNativeTearOff& r); 

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
    XPCWrappedNativeTearOffChunk() : mNextChunk(nsnull) {}
    ~XPCWrappedNativeTearOffChunk() {delete mNextChunk;}

private:
    XPCWrappedNativeTearOff mTearOffs[XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK];
    XPCWrappedNativeTearOffChunk* mNextChunk;
};

void *xpc_GetJSPrivate(JSObject *obj);





class XPCWrappedNative : public nsIXPConnectWrappedNative
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDNATIVE
    
    
    
    
    
    
    
    
    class NS_CYCLE_COLLECTION_INNERCLASS
     : public nsXPCOMCycleCollectionParticipant
    {
      NS_DECL_CYCLE_COLLECTION_CLASS_BODY_NO_UNLINK(XPCWrappedNative,
                                                    XPCWrappedNative)
      NS_IMETHOD RootAndUnlinkJSObjects(void *p);
      NS_IMETHOD Unlink(void *p) { return NS_OK; }
      NS_IMETHOD Unroot(void *p) { return NS_OK; }
    };
    NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE
    NS_DECL_CYCLE_COLLECTION_UNMARK_PURPLE_STUB(XPCWrappedNative)

#ifndef XPCONNECT_STANDALONE
    nsIPrincipal* GetObjectPrincipal() const;
#endif

    JSBool
    IsValid() const {return nsnull != mFlatJSObject;}

#define XPC_SCOPE_WORD(s)   ((jsword)(s))
#define XPC_SCOPE_MASK      ((jsword)0x3)
#define XPC_SCOPE_TAG       ((jsword)0x1)
#define XPC_WRAPPER_EXPIRED ((jsword)0x2)

    static inline JSBool
    IsTaggedScope(XPCWrappedNativeScope* s)
        {return XPC_SCOPE_WORD(s) & XPC_SCOPE_TAG;}

    static inline XPCWrappedNativeScope*
    TagScope(XPCWrappedNativeScope* s)
        {NS_ASSERTION(!IsTaggedScope(s), "bad pointer!");
         return (XPCWrappedNativeScope*)(XPC_SCOPE_WORD(s) | XPC_SCOPE_TAG);}

    static inline XPCWrappedNativeScope*
    UnTagScope(XPCWrappedNativeScope* s)
        {return (XPCWrappedNativeScope*)(XPC_SCOPE_WORD(s) & ~XPC_SCOPE_TAG);}

    inline JSBool
    IsWrapperExpired() const
        {return XPC_SCOPE_WORD(mMaybeScope) & XPC_WRAPPER_EXPIRED;}

    JSBool
    HasProto() const {return !IsTaggedScope(mMaybeScope);}

    XPCWrappedNativeProto*
    GetProto() const
        {return HasProto() ?
         (XPCWrappedNativeProto*)
         (XPC_SCOPE_WORD(mMaybeProto) & ~XPC_SCOPE_MASK) : nsnull;}

    void
    SetProto(XPCWrappedNativeProto* p)
        {NS_ASSERTION(!IsWrapperExpired(), "bad ptr!");
         mMaybeProto = p;}

    XPCWrappedNativeScope*
    GetScope() const
        {return GetProto() ? GetProto()->GetScope() :
         (XPCWrappedNativeScope*)
         (XPC_SCOPE_WORD(mMaybeScope) & ~XPC_SCOPE_MASK);}

    nsISupports*
    GetIdentityObject() const {return mIdentity;}

    JSObject*
    GetFlatJSObjectAndMark() const
        {if(mFlatJSObject && mFlatJSObject != INVALID_OBJECT)
             mFlatJSObject->markIfUnmarked();
         return mFlatJSObject;}

    JSObject*
    GetFlatJSObjectNoMark() const {return mFlatJSObject;}

    XPCLock*
    GetLock() const {return IsValid() && HasProto() ?
                                GetProto()->GetLock() : nsnull;}

    XPCNativeSet*
    GetSet() const {XPCAutoLock al(GetLock()); return mSet;}

    void
    SetSet(XPCNativeSet* set) {XPCAutoLock al(GetLock()); mSet = set;}

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
                                   GetProto()->GetSecurityInfoAddr() : nsnull;}

    nsIClassInfo*
    GetClassInfo() const {return IsValid() && HasProto() ?
                            GetProto()->GetClassInfo() : nsnull;}

    JSBool
    HasSharedProto() const {return IsValid() && HasProto() &&
                            GetProto()->IsShared();}

    JSBool
    HasMutatedSet() const {return IsValid() &&
                                  (!HasProto() ||
                                   GetSet() != GetProto()->GetSet());}

    XPCJSRuntime*
    GetRuntime() const {XPCWrappedNativeScope* scope = GetScope();
                        return scope ? scope->GetRuntime() : nsnull;}

    static nsresult
    GetNewOrUsed(XPCCallContext& ccx,
                 xpcObjectHelper& helper,
                 XPCWrappedNativeScope* Scope,
                 XPCNativeInterface* Interface,
                 JSBool isGlobal,
                 XPCWrappedNative** wrapper);

    static nsresult
    Morph(XPCCallContext& ccx,
          JSObject* existingJSObject,
          XPCNativeInterface* Interface,
          nsWrapperCache *cache,
          XPCWrappedNative** resultWrapper);

public:
    static nsresult
    GetUsedOnly(XPCCallContext& ccx,
                nsISupports* Object,
                XPCWrappedNativeScope* Scope,
                XPCNativeInterface* Interface,
                XPCWrappedNative** wrapper);

    
    
    static XPCWrappedNative*
    GetWrappedNativeOfJSObject(JSContext* cx, JSObject* obj,
                               JSObject* funobj = nsnull,
                               JSObject** pobj2 = nsnull,
                               XPCWrappedNativeTearOff** pTearOff = nsnull);
    static XPCWrappedNative*
    GetAndMorphWrappedNativeOfJSObject(JSContext* cx, JSObject* obj)
    {
        JSObject *obj2 = nsnull;
        XPCWrappedNative* wrapper =
            GetWrappedNativeOfJSObject(cx, obj, nsnull, &obj2);
        if(wrapper || !obj2)
            return wrapper;

        NS_ASSERTION(IS_SLIM_WRAPPER(obj2),
                     "Hmm, someone changed GetWrappedNativeOfJSObject?");
        SLIM_LOG_WILL_MORPH(cx, obj2);
        return MorphSlimWrapper(cx, obj2) ?
               (XPCWrappedNative*)xpc_GetJSPrivate(obj2) :
               nsnull;
    }

    static nsresult
    ReparentWrapperIfFound(XPCCallContext& ccx,
                           XPCWrappedNativeScope* aOldScope,
                           XPCWrappedNativeScope* aNewScope,
                           JSObject* aNewParent,
                           nsISupports* aCOMObj,
                           XPCWrappedNative** aWrapper);

    void FlatJSObjectFinalized(JSContext *cx);

    void SystemIsBeingShutDown(JSContext* cx);

    enum CallMode {CALL_METHOD, CALL_GETTER, CALL_SETTER};

    static JSBool CallMethod(XPCCallContext& ccx,
                             CallMode mode = CALL_METHOD);

    static JSBool GetAttribute(XPCCallContext& ccx)
        {return CallMethod(ccx, CALL_GETTER);}

    static JSBool SetAttribute(XPCCallContext& ccx)
        {return CallMethod(ccx, CALL_SETTER);}

    inline JSBool HasInterfaceNoQI(const nsIID& iid);

    XPCWrappedNativeTearOff* LocateTearOff(XPCCallContext& ccx,
                                           XPCNativeInterface* aInterface);
    XPCWrappedNativeTearOff* FindTearOff(XPCCallContext& ccx,
                                         XPCNativeInterface* aInterface,
                                         JSBool needJSObject = JS_FALSE,
                                         nsresult* pError = nsnull);
    void Mark() const
    {
        mSet->Mark();
        if(mScriptableInfo) mScriptableInfo->Mark();
        if(HasProto()) GetProto()->Mark();
    }

    
    inline void TraceJS(JSTracer* trc)
    {
        if(mScriptableInfo && JS_IsGCMarkingTracer(trc))
            mScriptableInfo->Mark();
        if(HasProto()) GetProto()->TraceJS(trc);
        JSObject* wrapper = GetWrapper();
        if(wrapper)
            JS_CALL_OBJECT_TRACER(trc, wrapper, "XPCWrappedNative::mWrapper");
    }

    inline void AutoTrace(JSTracer* trc)
    {
        
        
        
        
        
        if(mFlatJSObject && mFlatJSObject != INVALID_OBJECT)
        {
            JS_CALL_OBJECT_TRACER(trc, mFlatJSObject,
                                  "XPCWrappedNative::mFlatJSObject");
        }
    }

#ifdef DEBUG
    void ASSERT_SetsNotMarked() const
        {mSet->ASSERT_NotMarked();
         if(HasProto()){GetProto()->ASSERT_SetNotMarked();}}

    int DEBUG_CountOfTearoffChunks() const
        {int i = 0; const XPCWrappedNativeTearOffChunk* to;
         for(to = &mFirstChunk; to; to = to->mNextChunk) {i++;} return i;}
#endif

    inline void SweepTearOffs();

    
    char* ToString(XPCCallContext& ccx,
                   XPCWrappedNativeTearOff* to = nsnull) const;

    static void GatherProtoScriptableCreateInfo(
                        nsIClassInfo* classInfo,
                        XPCNativeScriptableCreateInfo& sciProto);

    JSBool HasExternalReference() const {return mRefCnt > 1;}

    JSBool NeedsSOW() { return !!(mWrapperWord & NEEDS_SOW); }
    void SetNeedsSOW() { mWrapperWord |= NEEDS_SOW; }
    JSBool NeedsCOW() { return !!(mWrapperWord & NEEDS_COW); }
    void SetNeedsCOW() { mWrapperWord |= NEEDS_COW; }

    JSObject* GetWrapper()
    {
        return (JSObject *) (mWrapperWord & (size_t)~(size_t)FLAG_MASK);
    }
    void SetWrapper(JSObject *obj)
    {
        PRWord needsSOW = NeedsSOW() ? NEEDS_SOW : 0;
        PRWord needsCOW = NeedsCOW() ? NEEDS_COW : 0;
        mWrapperWord = PRWord(obj) |
                         needsSOW |
                         needsCOW;
    }

    void NoteTearoffs(nsCycleCollectionTraversalCallback& cb);

    QITableEntry* GetOffsets()
    {
        if(!HasProto() || !GetProto()->ClassIsDOMObject())
            return nsnull;

        XPCWrappedNativeProto* proto = GetProto();
        QITableEntry* offsets = proto->GetOffsets();
        if(!offsets)
        {
            static NS_DEFINE_IID(kThisPtrOffsetsSID, NS_THISPTROFFSETS_SID);
            mIdentity->QueryInterface(kThisPtrOffsetsSID, (void**)&offsets);
        }
        return offsets;
    }

    
protected:
    XPCWrappedNative(); 

    
    XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                     XPCWrappedNativeProto* aProto);

    
    XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                     XPCWrappedNativeScope* aScope,
                     XPCNativeSet* aSet);

    virtual ~XPCWrappedNative();

private:
    enum {
        NEEDS_SOW = JS_BIT(0),
        NEEDS_COW = JS_BIT(1),

        LAST_FLAG = NEEDS_COW,

        FLAG_MASK = 0x7
    };

private:

    JSBool Init(XPCCallContext& ccx, JSObject* parent, JSBool isGlobal,
                const XPCNativeScriptableCreateInfo* sci);
    JSBool Init(XPCCallContext &ccx, JSObject *existingJSObject);
    JSBool FinishInit(XPCCallContext &ccx);

    JSBool ExtendSet(XPCCallContext& ccx, XPCNativeInterface* aInterface);

    nsresult InitTearOff(XPCCallContext& ccx,
                         XPCWrappedNativeTearOff* aTearOff,
                         XPCNativeInterface* aInterface,
                         JSBool needJSObject);

    JSBool InitTearOffJSObject(XPCCallContext& ccx,
                                XPCWrappedNativeTearOff* to);

public:
    static const XPCNativeScriptableCreateInfo& GatherScriptableCreateInfo(
                        nsISupports* obj,
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
    JSObject*                    mFlatJSObject;
    XPCNativeScriptableInfo*     mScriptableInfo;
    XPCWrappedNativeTearOffChunk mFirstChunk;
    PRWord                       mWrapperWord;

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
public:
    nsCOMPtr<nsIThread>          mThread; 
#endif
};











#define NS_IXPCONNECT_WRAPPED_JS_CLASS_IID  \
{ 0x2453eba0, 0xa9b8, 0x11d2,               \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedJSClass : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)
    NS_IMETHOD DebugDump(PRInt16 depth) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXPCWrappedJSClass,
                              NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)





class nsXPCWrappedJSClass : public nsIXPCWrappedJSClass
{
    
    NS_DECL_ISUPPORTS
    NS_IMETHOD DebugDump(PRInt16 depth);
public:

    static nsresult
    GetNewOrUsed(XPCCallContext& ccx,
                 REFNSIID aIID,
                 nsXPCWrappedJSClass** clazz);

    REFNSIID GetIID() const {return mIID;}
    XPCJSRuntime* GetRuntime() const {return mRuntime;}
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}
    const char* GetInterfaceName();

    static JSBool IsWrappedJS(nsISupports* aPtr);

    NS_IMETHOD DelegatedQueryInterface(nsXPCWrappedJS* self, REFNSIID aIID,
                                       void** aInstancePtr);

    JSObject* GetRootJSObject(XPCCallContext& ccx, JSObject* aJSObj);

    NS_IMETHOD CallMethod(nsXPCWrappedJS* wrapper, uint16 methodIndex,
                          const XPTMethodDescriptor* info,
                          nsXPTCMiniVariant* params);

    JSObject*  CallQueryInterfaceOnJSObject(XPCCallContext& ccx,
                                            JSObject* jsobj, REFNSIID aIID);

    static nsresult BuildPropertyEnumerator(XPCCallContext& ccx,
                                            JSObject* aJSObj,
                                            nsISimpleEnumerator** aEnumerate);

    static nsresult GetNamedPropertyAsVariant(XPCCallContext& ccx, 
                                              JSObject* aJSObj,
                                              jsval aName, 
                                              nsIVariant** aResult);

    virtual ~nsXPCWrappedJSClass();

    static nsresult CheckForException(XPCCallContext & ccx,
                                      const char * aPropertyName,
                                      const char * anInterfaceName,
                                      PRBool aForceReport);
private:
    nsXPCWrappedJSClass();   
    nsXPCWrappedJSClass(XPCCallContext& ccx, REFNSIID aIID,
                        nsIInterfaceInfo* aInfo);

    JSObject*  NewOutObject(JSContext* cx, JSObject* scope);

    JSBool IsReflectable(uint16 i) const
        {return (JSBool)(mDescriptors[i/32] & (1 << (i%32)));}
    void SetReflectable(uint16 i, JSBool b)
        {if(b) mDescriptors[i/32] |= (1 << (i%32));
         else mDescriptors[i/32] &= ~(1 << (i%32));}

    enum SizeMode {GET_SIZE, GET_LENGTH};

    JSBool GetArraySizeFromParam(JSContext* cx,
                                 const XPTMethodDescriptor* method,
                                 const nsXPTParamInfo& param,
                                 uint16 methodIndex,
                                 uint8 paramIndex,
                                 SizeMode mode,
                                 nsXPTCMiniVariant* params,
                                 JSUint32* result);

    JSBool GetInterfaceTypeFromParam(JSContext* cx,
                                     const XPTMethodDescriptor* method,
                                     const nsXPTParamInfo& param,
                                     uint16 methodIndex,
                                     const nsXPTType& type,
                                     nsXPTCMiniVariant* params,
                                     nsID* result);

    void CleanupPointerArray(const nsXPTType& datum_type,
                             JSUint32 array_count,
                             void** arrayp);

    void CleanupPointerTypeObject(const nsXPTType& type,
                                  void** pp);

private:
    XPCJSRuntime* mRuntime;
    nsIInterfaceInfo* mInfo;
    char* mName;
    nsIID mIID;
    uint32* mDescriptors;
};






class nsXPCWrappedJS : protected nsAutoXPTCStub,
                       public nsIXPConnectWrappedJS,
                       public nsSupportsWeakReference,
                       public nsIPropertyBag,
                       public XPCRootSetElem
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDJS
    NS_DECL_NSISUPPORTSWEAKREFERENCE
    NS_DECL_NSIPROPERTYBAG

    class NS_CYCLE_COLLECTION_INNERCLASS
     : public nsXPCOMCycleCollectionParticipant
    {
      NS_IMETHOD RootAndUnlinkJSObjects(void *p);
      NS_DECL_CYCLE_COLLECTION_CLASS_BODY(nsXPCWrappedJS, nsIXPConnectWrappedJS)
    };
    NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE
    NS_DECL_CYCLE_COLLECTION_UNMARK_PURPLE_STUB(nsXPCWrappedJS)

    NS_IMETHOD CallMethod(PRUint16 methodIndex,
                          const XPTMethodDescriptor *info,
                          nsXPTCMiniVariant* params);

    





    static nsresult
    GetNewOrUsed(XPCCallContext& ccx,
                 JSObject* aJSObj,
                 REFNSIID aIID,
                 nsISupports* aOuter,
                 nsXPCWrappedJS** wrapper);

    nsISomeInterface* GetXPTCStub() { return mXPTCStub; }
    JSObject* GetJSObject() const {return mJSObj;}
    nsXPCWrappedJSClass*  GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}
    nsXPCWrappedJS* GetRootWrapper() const {return mRoot;}
    nsXPCWrappedJS* GetNextWrapper() const {return mNext;}

    nsXPCWrappedJS* Find(REFNSIID aIID);
    nsXPCWrappedJS* FindInherited(REFNSIID aIID);

    JSBool IsValid() const {return mJSObj != nsnull;}
    void SystemIsBeingShutDown(JSRuntime* rt);

    
    
    
    
    
    
    JSBool IsSubjectToFinalization() const {return IsValid() && mRefCnt == 1;}

    JSBool IsAggregatedToNative() const {return mRoot->mOuter != nsnull;}
    nsISupports* GetAggregatedNativeObject() const {return mRoot->mOuter;}

    void TraceJS(JSTracer* trc);
#ifdef DEBUG
    static void PrintTraceName(JSTracer* trc, char *buf, size_t bufsize);
#endif

    virtual ~nsXPCWrappedJS();
protected:
    nsXPCWrappedJS();   
    nsXPCWrappedJS(XPCCallContext& ccx,
                   JSObject* aJSObj,
                   nsXPCWrappedJSClass* aClass,
                   nsXPCWrappedJS* root,
                   nsISupports* aOuter);

   void Unlink();

private:
    JSObject* mJSObj;
    nsXPCWrappedJSClass* mClass;
    nsXPCWrappedJS* mRoot;
    nsXPCWrappedJS* mNext;
    nsISupports* mOuter;    
    bool mMainThread;
};



class XPCJSObjectHolder : public nsIXPConnectJSObjectHolder,
                          public XPCRootSetElem
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER

    

public:
    static XPCJSObjectHolder* newHolder(XPCCallContext& ccx, JSObject* obj);

    virtual ~XPCJSObjectHolder();

    void TraceJS(JSTracer *trc);
#ifdef DEBUG
    static void PrintTraceName(JSTracer* trc, char *buf, size_t bufsize);
#endif

private:
    XPCJSObjectHolder(XPCCallContext& ccx, JSObject* obj);
    XPCJSObjectHolder(); 

    JSObject* mJSObj;
};









class xpcProperty : public nsIProperty
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTY

  xpcProperty(const PRUnichar* aName, PRUint32 aNameLen, nsIVariant* aValue);
  virtual ~xpcProperty() {}

private:
    nsString             mName;
    nsCOMPtr<nsIVariant> mValue;
};




class xpcObjectHelper
{
public:
    xpcObjectHelper(nsISupports *aObject, nsWrapperCache *aCache = nsnull)
    : mCanonical(nsnull),
      mObject(aObject),
      mCache(aCache),
      mIsNode(PR_FALSE)
    {
        if(!mCache)
        {
            if(aObject)
                CallQueryInterface(aObject, &mCache);
            else
                mCache = nsnull;
        }
    }

    nsISupports* Object()
    {
        return mObject;
    }

    nsISupports* GetCanonical()
    {
        if (!mCanonical) {
            mCanonicalStrong = do_QueryInterface(mObject);
            mCanonical = mCanonicalStrong;
        } 
        return mCanonical;
    }

    already_AddRefed<nsISupports> forgetCanonical()
    {
        NS_ASSERTION(mCanonical, "Huh, no canonical to forget?");

        if (!mCanonicalStrong)
            mCanonicalStrong = mCanonical;
        mCanonical = nsnull;
        return mCanonicalStrong.forget();
    }

    nsIClassInfo *GetClassInfo()
    {
        if(mXPCClassInfo)
          return mXPCClassInfo;
        if(!mClassInfo)
            mClassInfo = do_QueryInterface(mObject);
        return mClassInfo;
    }
    nsXPCClassInfo *GetXPCClassInfo()
    {
        if(!mXPCClassInfo)
        {
            if(mIsNode)
                mXPCClassInfo = static_cast<nsINode*>(GetCanonical())->GetClassInfo();
            else
                CallQueryInterface(mObject, getter_AddRefs(mXPCClassInfo));
        }
        return mXPCClassInfo;
    }

    already_AddRefed<nsXPCClassInfo> forgetXPCClassInfo()
    {
        GetXPCClassInfo();

        return mXPCClassInfo.forget();
    }

    nsWrapperCache *GetWrapperCache()
    {
        return mCache;
    }

protected:
    xpcObjectHelper(nsISupports *aObject, nsISupports *aCanonical,
                    nsWrapperCache *aCache, PRBool aIsNode)
    : mCanonical(aCanonical),
      mObject(aObject),
      mCache(aCache),
      mIsNode(aIsNode)
    {
        if(!mCache && aObject)
            CallQueryInterface(aObject, &mCache);
    }

    nsCOMPtr<nsISupports>    mCanonicalStrong;
    nsISupports*             mCanonical;

private:
    xpcObjectHelper(xpcObjectHelper& aOther);

    nsISupports*             mObject;
    nsWrapperCache*          mCache;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    nsRefPtr<nsXPCClassInfo> mXPCClassInfo;
    PRBool                   mIsNode;
};


class XPCConvert
{
public:
    static JSBool IsMethodReflectable(const XPTMethodDescriptor& info);

    










    
    static JSBool NativeData2JS(XPCCallContext& ccx, jsval* d, const void* s,
                                const nsXPTType& type, const nsID* iid,
                                JSObject* scope, nsresult* pErr)
    {
        XPCLazyCallContext lccx(ccx);
        return NativeData2JS(lccx, d, s, type, iid, scope, pErr);
    }
    static JSBool NativeData2JS(XPCLazyCallContext& lccx, jsval* d,
                                const void* s, const nsXPTType& type,
                                const nsID* iid, JSObject* scope,
                                nsresult* pErr);

    static JSBool JSData2Native(XPCCallContext& ccx, void* d, jsval s,
                                const nsXPTType& type,
                                JSBool useAllocator, const nsID* iid,
                                nsresult* pErr);

    

















    static JSBool NativeInterface2JSObject(XPCCallContext& ccx,
                                           jsval* d,
                                           nsIXPConnectJSObjectHolder** dest,
                                           xpcObjectHelper& aHelper,
                                           const nsID* iid,
                                           XPCNativeInterface** Interface,
                                           JSObject* scope,
                                           PRBool allowNativeWrapper,
                                           PRBool isGlobal,
                                           nsresult* pErr)
    {
        XPCLazyCallContext lccx(ccx);
        return NativeInterface2JSObject(lccx, d, dest, aHelper, iid, Interface,
                                        scope, allowNativeWrapper, isGlobal,
                                        pErr);
    }
    static JSBool NativeInterface2JSObject(XPCLazyCallContext& lccx,
                                           jsval* d,
                                           nsIXPConnectJSObjectHolder** dest,
                                           xpcObjectHelper& aHelper,
                                           const nsID* iid,
                                           XPCNativeInterface** Interface,
                                           JSObject* scope,
                                           PRBool allowNativeWrapper,
                                           PRBool isGlobal,
                                           nsresult* pErr);

    static JSBool GetNativeInterfaceFromJSObject(XPCCallContext& ccx,
                                                 void** dest, JSObject* src,
                                                 const nsID* iid, 
                                                 nsresult* pErr);
    static JSBool JSObject2NativeInterface(XPCCallContext& ccx,
                                           void** dest, JSObject* src,
                                           const nsID* iid,
                                           nsISupports* aOuter,
                                           nsresult* pErr);
    static JSBool GetISupportsFromJSObject(JSObject* obj, nsISupports** iface);

    










    
    static JSBool NativeArray2JS(XPCLazyCallContext& ccx,
                                 jsval* d, const void** s,
                                 const nsXPTType& type, const nsID* iid,
                                 JSUint32 count, JSObject* scope,
                                 nsresult* pErr);

    static JSBool JSArray2Native(XPCCallContext& ccx, void** d, jsval s,
                                 JSUint32 count, JSUint32 capacity,
                                 const nsXPTType& type,
                                 JSBool useAllocator, const nsID* iid,
                                 uintN* pErr);

    static JSBool NativeStringWithSize2JS(JSContext* cx,
                                          jsval* d, const void* s,
                                          const nsXPTType& type,
                                          JSUint32 count,
                                          nsresult* pErr);

    static JSBool JSStringWithSize2Native(XPCCallContext& ccx, void* d, jsval s,
                                          JSUint32 count, JSUint32 capacity,
                                          const nsXPTType& type,
                                          JSBool useAllocator,
                                          uintN* pErr);

    static nsresult JSValToXPCException(XPCCallContext& ccx,
                                        jsval s,
                                        const char* ifaceName,
                                        const char* methodName,
                                        nsIException** exception);

    static nsresult JSErrorToXPCException(XPCCallContext& ccx,
                                          const char* message,
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

    static void RemoveXPCOMUCStringFinalizer();

private:
    XPCConvert(); 

};




class XPCStringConvert
{
public:

    
    
    
    static jsval ReadableToJSVal(JSContext *cx, const nsAString &readable,
                                 nsStringBuffer** sharedBuffer);

    static XPCReadableJSStringWrapper *JSStringToReadable(XPCCallContext& ccx,
                                                          JSString *str);

    static void ShutdownDOMStringFinalizer();

private:
    XPCStringConvert();         
};

extern JSBool
XPC_JSArgumentFormatter(JSContext *cx, const char *format,
                        JSBool fromJS, jsval **vpp, va_list *app);





class XPCThrower
{
public:
    static void Throw(nsresult rv, JSContext* cx);
    static void Throw(nsresult rv, XPCCallContext& ccx);
    static void ThrowBadResult(nsresult rv, nsresult result, XPCCallContext& ccx);
    static void ThrowBadParam(nsresult rv, uintN paramNum, XPCCallContext& ccx);
#ifdef XPC_IDISPATCH_SUPPORT
    static void ThrowCOMError(JSContext* cx, unsigned long COMErrorCode, 
                              nsresult rv = NS_ERROR_XPC_COM_ERROR,
                              const EXCEPINFO * exception = nsnull);
#endif
    static JSBool SetVerbosity(JSBool state)
        {JSBool old = sVerbose; sVerbose = state; return old;}

    static void BuildAndThrowException(JSContext* cx, nsresult rv, const char* sz);
    static JSBool CheckForPendingException(nsresult result, JSContext *cx);

private:
    static void Verbosify(XPCCallContext& ccx,
                          char** psz, PRBool own);

    static JSBool ThrowExceptionObject(JSContext* cx, nsIException* e);

private:
    static JSBool sVerbose;
};




class XPCJSStack
{
public:
    static nsresult
    CreateStack(JSContext* cx, nsIStackFrame** stack);

    static nsresult
    CreateStackFrameLocation(PRUint32 aLanguage,
                             const char* aFilename,
                             const char* aFunctionName,
                             PRInt32 aLineNumber,
                             nsIStackFrame* aCaller,
                             nsIStackFrame** stack);
private:
    XPCJSStack();   
};



class nsXPCException :
            public nsIXPCException
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_XPCEXCEPTION_CID)

    NS_DECL_ISUPPORTS
    NS_DECL_NSIEXCEPTION
    NS_DECL_NSIXPCEXCEPTION

    static nsresult NewException(const char *aMessage,
                                 nsresult aResult,
                                 nsIStackFrame *aLocation,
                                 nsISupports *aData,
                                 nsIException** exception);

    static JSBool NameAndFormatForNSResult(nsresult rv,
                                           const char** name,
                                           const char** format);

    static void* IterateNSResults(nsresult* rv,
                                  const char** name,
                                  const char** format,
                                  void** iterp);

    static PRUint32 GetNSResultCount();

    nsXPCException();
    virtual ~nsXPCException();

    static void InitStatics() { sEverMadeOneFromFactory = JS_FALSE; }

protected:
    void Reset();
private:
    char*           mMessage;
    nsresult        mResult;
    char*           mName;
    nsIStackFrame*  mLocation;
    nsISupports*    mData;
    char*           mFilename;
    int             mLineNumber;
    nsIException*   mInner;
    PRBool          mInitialized;

    nsAutoJSValHolder mThrownJSVal;

    static JSBool sEverMadeOneFromFactory;
};







extern void xpc_InitJSxIDClassObjects();
extern void xpc_DestroyJSxIDClassObjects();


class nsJSID : public nsIJSID
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_JS_ID_CID)

    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSID

    PRBool InitWithName(const nsID& id, const char *nameString);
    PRBool SetName(const char* name);
    void   SetNameToNoString()
        {NS_ASSERTION(!mName, "name already set"); mName = gNoString;}
    PRBool NameIsSet() const {return nsnull != mName;}
    const nsID& ID() const {return mID;}
    PRBool IsValid() const {return !mID.Equals(GetInvalidIID());}

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



class nsJSIID : public nsIJSIID, public nsIXPCScriptable
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
          , public nsISecurityCheckedComponent
#endif
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIJSID

    
    NS_DECL_NSIJSIID
    NS_DECL_NSIXPCSCRIPTABLE
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
#endif

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
    NS_DECL_ISUPPORTS

    
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
        frame(nsnull),
        suspendDepth(0)
    {}
    JSContext* cx;

    
    
    JSStackFrame* frame;

    
    jsrefcount suspendDepth;
};

class XPCJSContextStack
{
public:
    NS_DECL_NSIJSCONTEXTSTACK
    NS_DECL_NSITHREADJSCONTEXTSTACK

    XPCJSContextStack();
    virtual ~XPCJSContextStack();

#ifdef DEBUG
    JSBool DEBUG_StackHasJSContext(JSContext*  aJSContext);
#endif

    const nsTArray<XPCJSContextInfo>* GetStack()
    { return &mStack; }

private:
    nsAutoTArray<XPCJSContextInfo, 16> mStack;
    JSContext*  mSafeJSContext;

    
    
    JSContext*  mOwnSafeJSContext;
};



#define NS_XPC_JSCONTEXT_STACK_ITERATOR_CID \
{ 0x05bae29d, 0x8aef, 0x486d, \
  { 0x84, 0xaa, 0x53, 0xf4, 0x8f, 0x14, 0x68, 0x11 } }

class nsXPCJSContextStackIterator : public nsIJSContextStackIterator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSCONTEXTSTACKITERATOR

private:
    const nsTArray<XPCJSContextInfo> *mStack;
    PRUint32 mPosition;
};




#define BAD_TLS_INDEX ((PRUint32) -1)

class XPCPerThreadData
{
public:
    
    static inline XPCPerThreadData* GetData(JSContext *cx)
    {
        if(cx)
        {
            NS_ASSERTION(cx->thread, "Uh, JS context w/o a thread?");

            if(cx->thread == sMainJSThread)
                return sMainThreadData;
        }
        else if(sMainThreadData && sMainThreadData->mThread == PR_GetCurrentThread())
        {
            return sMainThreadData;
        }

        return GetDataImpl(cx);
    }

    static void CleanupAllThreads();

    ~XPCPerThreadData();

    nsresult GetException(nsIException** aException)
    {
        if(EnsureExceptionManager())
            return mExceptionManager->GetCurrentException(aException);

        NS_IF_ADDREF(mException);
        *aException = mException;
        return NS_OK;
    }

    nsresult SetException(nsIException* aException)
    {
        if(EnsureExceptionManager())
            return mExceptionManager->SetCurrentException(aException);

        NS_IF_ADDREF(aException);
        NS_IF_RELEASE(mException);
        mException = aException;
        return NS_OK;
    }

    nsIExceptionManager* GetExceptionManager()
    {
        if(EnsureExceptionManager())
            return mExceptionManager;
        return nsnull;
    }

    JSBool EnsureExceptionManager()
    {
        if(mExceptionManager)
            return JS_TRUE;

        if(mExceptionManagerNotAvailable)
            return JS_FALSE;

        nsCOMPtr<nsIExceptionService> xs =
            do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);
        if(xs)
            xs->GetCurrentExceptionManager(&mExceptionManager);
        if(mExceptionManager)
            return JS_TRUE;

        mExceptionManagerNotAvailable = JS_TRUE;
        return JS_FALSE;
    }

    XPCJSContextStack* GetJSContextStack() {return mJSContextStack;}

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

    void Cleanup();
    void ReleaseNatives();

    PRBool IsValid() const {return mJSContextStack != nsnull;}

    static PRLock* GetLock() {return gLock;}
    
    static XPCPerThreadData* IterateThreads(XPCPerThreadData** iteratorp);

    AutoMarkingPtr**  GetAutoRootsAdr() {return &mAutoRoots;}

    void TraceJS(JSTracer* trc);
    void MarkAutoRootsAfterJSFinalize();

    static void InitStatics()
        { gLock = nsnull; gThreads = nsnull; gTLSIndex = BAD_TLS_INDEX; }

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
    JSUint32  IncrementWrappedNativeThreadsafetyReportDepth()
        {return ++mWrappedNativeThreadsafetyReportDepth;}
    void      ClearWrappedNativeThreadsafetyReportDepth()
        {mWrappedNativeThreadsafetyReportDepth = 0;}
#endif

    static void ShutDown()
        {sMainJSThread = nsnull; sMainThreadData = nsnull;}

    static PRBool IsMainThread(JSContext *cx)
        { return cx->thread == sMainJSThread; }

private:
    XPCPerThreadData();
    static XPCPerThreadData* GetDataImpl(JSContext *cx);

private:
    XPCJSContextStack*   mJSContextStack;
    XPCPerThreadData*    mNextThread;
    XPCCallContext*      mCallContext;
    jsid                 mResolveName;
    XPCWrappedNative*    mResolvingWrapper;

    nsIExceptionManager* mExceptionManager;
    nsIException*        mException;
    JSBool               mExceptionManagerNotAvailable;
    AutoMarkingPtr*      mAutoRoots;

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
    JSUint32             mWrappedNativeThreadsafetyReportDepth;
#endif
    PRThread*            mThread;

    static PRLock*           gLock;
    static XPCPerThreadData* gThreads;
    static PRUintn           gTLSIndex;

    
    static void *sMainJSThread;

    
    
    static XPCPerThreadData *sMainThreadData;
};


#ifndef XPCONNECT_STANDALONE
#include "nsIScriptSecurityManager.h"

class BackstagePass : public nsIScriptObjectPrincipal,
                      public nsIXPCScriptable,
                      public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE
  NS_DECL_NSICLASSINFO

  virtual nsIPrincipal* GetPrincipal() {
    return mPrincipal;
  }

  BackstagePass(nsIPrincipal *prin) :
    mPrincipal(prin)
  {
  }

  virtual ~BackstagePass() { }

private:
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

#else

class BackstagePass : public nsIXPCScriptable, public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE
  NS_DECL_NSICLASSINFO

  BackstagePass()
  {
  }

  virtual ~BackstagePass() { }
};

#endif

class nsJSRuntimeServiceImpl : public nsIJSRuntimeService,
                               public nsSupportsWeakReference
{
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSRUNTIMESERVICE

    
    
    
    static nsJSRuntimeServiceImpl* GetSingleton();

    static void FreeSingleton();

    nsJSRuntimeServiceImpl();
    virtual ~nsJSRuntimeServiceImpl();

    static void InitStatics() { gJSRuntimeService = nsnull; }
 protected:
    static nsJSRuntimeServiceImpl* gJSRuntimeService;
    nsCOMPtr<nsIXPCScriptable> mBackstagePass;
};




class nsXPCComponents : public nsIXPCComponents,
                        public nsIXPCScriptable,
                        public nsIClassInfo
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
                      , public nsISecurityCheckedComponent
#endif
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
#endif

public:
    static JSBool
    AttachNewComponentsObject(XPCCallContext& ccx,
                              XPCWrappedNativeScope* aScope,
                              JSObject* aGlobal);

    void SystemIsBeingShutDown() {ClearMembers();}

    virtual ~nsXPCComponents();

private:
    nsXPCComponents();
    void ClearMembers();

private:
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



class nsXPCComponents_Interfaces :
            public nsIScriptableInterfaces,
            public nsIXPCScriptable,
            public nsIClassInfo
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
          , public nsISecurityCheckedComponent
#endif
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTABLEINTERFACES
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
#endif

public:
    nsXPCComponents_Interfaces();
    virtual ~nsXPCComponents_Interfaces();

private:
    nsCOMPtr<nsIInterfaceInfoManager> mManager;
};




extern JSObject*
xpc_NewIDObject(JSContext *cx, JSObject* jsobj, const nsID& aID);

extern const nsID*
xpc_JSObjectToID(JSContext *cx, JSObject* obj);

extern JSBool
xpc_JSObjectIsID(JSContext *cx, JSObject* obj);




extern JSBool
xpc_DumpJSStack(JSContext* cx, JSBool showArgs, JSBool showLocals,
                JSBool showThisProps);




extern char*
xpc_PrintJSStack(JSContext* cx, JSBool showArgs, JSBool showLocals,
                 JSBool showThisProps);

extern JSBool
xpc_DumpEvalInJSStackFrame(JSContext* cx, JSUint32 frameno, const char* text);

extern JSBool
xpc_DumpJSObject(JSObject* obj);

extern JSBool
xpc_InstallJSDebuggerKeywordHandler(JSRuntime* rt);





class nsScriptError : public nsIScriptError,
                      public nsIScriptError2 {
public:
    nsScriptError();

    virtual ~nsScriptError();

  

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLEMESSAGE
    NS_DECL_NSISCRIPTERROR
    NS_DECL_NSISCRIPTERROR2

private:
    nsString mMessage;
    nsString mSourceName;
    PRUint32 mLineNumber;
    nsString mSourceLine;
    PRUint32 mColumnNumber;
    PRUint32 mFlags;
    nsCString mCategory;
    PRUint64 mWindowID;
};



class NS_STACK_CLASS AutoJSErrorAndExceptionEater
{
public:
    AutoJSErrorAndExceptionEater(JSContext* aCX
                                 MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
        : mCX(aCX),
          mOldErrorReporter(JS_SetErrorReporter(mCX, nsnull)),
          mOldExceptionState(JS_SaveExceptionState(mCX)) {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoJSErrorAndExceptionEater()
    {
        JS_SetErrorReporter(mCX, mOldErrorReporter);
        JS_RestoreExceptionState(mCX, mOldExceptionState);
    }
private:
    JSContext*        mCX;
    JSErrorReporter   mOldErrorReporter;
    JSExceptionState* mOldExceptionState;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
};





class NS_STACK_CLASS AutoScriptEvaluate
{
public:
    



    AutoScriptEvaluate(JSContext * cx MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
         : mJSContext(cx), mState(0), mErrorReporterSet(PR_FALSE),
           mEvaluated(PR_FALSE), mContextHasThread(0) {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
    }

    






    void StartEvaluating(JSErrorReporter errorReporter = nsnull);
    


    ~AutoScriptEvaluate();
private:
    JSContext* mJSContext;
    JSExceptionState* mState;
    PRBool mErrorReporterSet;
    PRBool mEvaluated;
    jsword mContextHasThread;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoScriptEvaluate(const AutoScriptEvaluate &);
    AutoScriptEvaluate & operator =(const AutoScriptEvaluate &);
};


class NS_STACK_CLASS AutoResolveName
{
public:
    AutoResolveName(XPCCallContext& ccx, jsid name
                    MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
        : mTLS(ccx.GetThreadData()),
          mOld(mTLS->SetResolveName(name)),
          mCheck(name) {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoResolveName()
        {
#ifdef DEBUG
            jsid old = 
#endif
            mTLS->SetResolveName(mOld);
            NS_ASSERTION(old == mCheck, "Bad Nesting!");
        }

private:
    XPCPerThreadData* mTLS;
    jsid mOld;
    jsid mCheck;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
};


class XPCMarkableJSVal
{
public:
    XPCMarkableJSVal(jsval val) : mVal(val), mValPtr(&mVal) {}
    XPCMarkableJSVal(jsval *pval) : mVal(JSVAL_VOID), mValPtr(pval) {}
    ~XPCMarkableJSVal() {}
    void Mark() {}
    void TraceJS(JSTracer* trc)
    {
        JS_CALL_VALUE_TRACER(trc, *mValPtr, "XPCMarkableJSVal");
    }
    void AutoTrace(JSTracer* trc) {}
private:
    XPCMarkableJSVal(); 
    jsval  mVal;
    jsval* mValPtr;
}; 








class AutoMarkingPtr
{
public:
    AutoMarkingPtr(XPCCallContext& ccx)
        : mNext(nsnull), mTLS(ccx.GetThreadData()) {Link();}
    AutoMarkingPtr()
        : mNext(nsnull), mTLS(nsnull) {}

    virtual ~AutoMarkingPtr() {Unlink();}

    void Init(XPCCallContext& ccx)
        {NS_ASSERTION(!mTLS, "Already init'ed!");
         mTLS = ccx.GetThreadData();
         Link();}

    void Link() 
        {if(!mTLS) return;
         AutoMarkingPtr** list = mTLS->GetAutoRootsAdr(); 
         mNext = *list; *list = this;}

    void Unlink() 
        {if(!mTLS) return;
         AutoMarkingPtr** cur = mTLS->GetAutoRootsAdr(); 
         while(*cur != this) {
            NS_ASSERTION(*cur, "This object not in list!");
            cur = &(*cur)->mNext;
         }
         *cur = mNext;
         mTLS = nsnull;
        }

    AutoMarkingPtr* GetNext() {return mNext;}
    
    virtual void TraceJS(JSTracer* trc) = 0;
    virtual void MarkAfterJSFinalize() = 0;

protected:
    AutoMarkingPtr* mNext;
    XPCPerThreadData* mTLS;
};



#define DEFINE_AUTO_MARKING_PTR_TYPE(class_, type_)                          \
class class_ : public AutoMarkingPtr                                         \
{                                                                            \
public:                                                                      \
    class_ ()                                                                \
        : AutoMarkingPtr(), mPtr(nsnull) {}                                  \
    class_ (XPCCallContext& ccx, type_ * ptr = nsnull)                       \
        : AutoMarkingPtr(ccx), mPtr(ptr) {}                                  \
    virtual ~ class_ () {}                                                   \
                                                                             \
    virtual void TraceJS(JSTracer* trc)                                      \
        {if(mPtr) {                                                          \
           mPtr->TraceJS(trc);                                               \
           mPtr->AutoTrace(trc);                                             \
         }                                                                   \
         if(mNext) mNext->TraceJS(trc);}                                     \
                                                                             \
    virtual void MarkAfterJSFinalize()                                       \
        {if(mPtr) mPtr->Mark();                                              \
         if(mNext) mNext->MarkAfterJSFinalize();}                            \
                                                                             \
    type_ * get()        const  {return mPtr;}                               \
    operator type_ *()   const  {return mPtr;}                               \
    type_ * operator->() const  {return mPtr;}                               \
                                                                             \
    class_ & operator =(type_ * p)                                           \
        {NS_ASSERTION(mTLS, "Hasn't been init'ed!");                         \
         mPtr = p; return *this;}                                            \
                                                                             \
protected:                                                                   \
    type_ * mPtr;                                                            \
};



DEFINE_AUTO_MARKING_PTR_TYPE(AutoMarkingNativeInterfacePtr, XPCNativeInterface)
DEFINE_AUTO_MARKING_PTR_TYPE(AutoMarkingNativeSetPtr, XPCNativeSet)
DEFINE_AUTO_MARKING_PTR_TYPE(AutoMarkingWrappedNativePtr, XPCWrappedNative)
DEFINE_AUTO_MARKING_PTR_TYPE(AutoMarkingWrappedNativeTearOffPtr, XPCWrappedNativeTearOff)
DEFINE_AUTO_MARKING_PTR_TYPE(AutoMarkingWrappedNativeProtoPtr, XPCWrappedNativeProto)
DEFINE_AUTO_MARKING_PTR_TYPE(AutoMarkingJSVal, XPCMarkableJSVal)
                                    
#define DEFINE_AUTO_MARKING_ARRAY_PTR_TYPE(class_, type_)                    \
class class_ : public AutoMarkingPtr                                         \
{                                                                            \
public:                                                                      \
    class_ (XPCCallContext& ccx)                                             \
        : AutoMarkingPtr(ccx), mPtr(nsnull), mCount(0) {}                    \
    class_ (XPCCallContext& ccx, type_** aPtr, PRUint32 aCount,              \
            PRBool aClear = PR_FALSE)                                        \
        : AutoMarkingPtr(ccx), mPtr(aPtr), mCount(aCount)                    \
    {                                                                        \
        if(!mPtr) mCount = 0;                                                \
        else if(aClear) memset(mPtr, 0, mCount*sizeof(type_*));              \
    }                                                                        \
    virtual ~ class_ () {}                                                   \
                                                                             \
    virtual void TraceJS(JSTracer* trc)                                      \
    {                                                                        \
        for(PRUint32 i = 0; i < mCount; ++i)                                 \
        {                                                                    \
            type_* cur = mPtr[i];                                            \
            if(cur)                                                          \
            {                                                                \
                cur->TraceJS(trc);                                           \
                cur->AutoTrace(trc);                                         \
            }                                                                \
        }                                                                    \
        if(mNext) mNext->TraceJS(trc);                                       \
    }                                                                        \
                                                                             \
    virtual void MarkAfterJSFinalize()                                       \
    {                                                                        \
        for(PRUint32 i = 0; i < mCount; ++i)                                 \
        {                                                                    \
            type_* cur = mPtr[i];                                            \
            if(cur)                                                          \
                cur->Mark();                                                 \
        }                                                                    \
        if(mNext) mNext->MarkAfterJSFinalize();                              \
    }                                                                        \
                                                                             \
    type_ ** get()       const  {return mPtr;}                               \
    operator type_ **()  const  {return mPtr;}                               \
    type_ ** operator->() const  {return mPtr;}                              \
                                                                             \
    class_ & operator =(const class_ & inst)                                 \
        {mPtr = inst.mPtr; mCount = inst.mCount; return *this;}              \
                                                                             \
protected:                                                                   \
    type_ ** mPtr;                                                           \
    PRUint32 mCount;                                                         \
};

DEFINE_AUTO_MARKING_ARRAY_PTR_TYPE(AutoMarkingNativeInterfacePtrArrayPtr,
                                   XPCNativeInterface)
    






#define AUTO_MARK_JSVAL_HELPER2(tok, line) tok##line
#define AUTO_MARK_JSVAL_HELPER(tok, line) AUTO_MARK_JSVAL_HELPER2(tok, line)

#define AUTO_MARK_JSVAL(ccx, val)                                            \
    XPCMarkableJSVal AUTO_MARK_JSVAL_HELPER(_val_,__LINE__)(val);            \
    AutoMarkingJSVal AUTO_MARK_JSVAL_HELPER(_automarker_,__LINE__)           \
    (ccx, &AUTO_MARK_JSVAL_HELPER(_val_,__LINE__))

#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT



extern char* xpc_CloneAllAccess();



extern char * xpc_CheckAccessList(const PRUnichar* wideName, const char* list[]);
#endif





#define XPCVARIANT_IID \
    {0x1809fd50, 0x91e8, 0x11d5, \
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

    static XPCVariant* newVariant(XPCCallContext& ccx, jsval aJSVal);

    



    jsval GetJSVal() const {return mJSVal;}

    XPCVariant(XPCCallContext& ccx, jsval aJSVal);

    







    
    static JSBool VariantDataToJS(XPCLazyCallContext& lccx, 
                                  nsIVariant* variant,
                                  JSObject* scope, nsresult* pErr,
                                  jsval* pJSVal);

protected:
    virtual ~XPCVariant() { }

    JSBool InitializeData(XPCCallContext& ccx);

protected:
    nsDiscriminatedUnion mData;
    jsval                mJSVal;
    JSBool               mReturnRawObject;
};

NS_DEFINE_STATIC_IID_ACCESSOR(XPCVariant, XPCVARIANT_IID)

class XPCTraceableVariant: public XPCVariant,
                           public XPCRootSetElem
{
public:
    XPCTraceableVariant(XPCCallContext& ccx, jsval aJSVal)
        : XPCVariant(ccx, aJSVal)
    {
        ccx.GetRuntime()->AddVariantRoot(this);
    }

    virtual ~XPCTraceableVariant();

    void TraceJS(JSTracer* trc);
#ifdef DEBUG
    static void PrintTraceName(JSTracer* trc, char *buf, size_t bufsize);
#endif
};


#ifndef XPCONNECT_STANDALONE

#define PRINCIPALHOLDER_IID \
{0xbf109f49, 0xf94a, 0x43d8, {0x93, 0xdb, 0xe4, 0x66, 0x49, 0xc5, 0xd9, 0x7d}}

class PrincipalHolder : public nsIScriptObjectPrincipal
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(PRINCIPALHOLDER_IID)

    PrincipalHolder(nsIPrincipal *holdee)
        : mHoldee(holdee)
    {
    }
    virtual ~PrincipalHolder() { }

    NS_DECL_ISUPPORTS

    nsIPrincipal *GetPrincipal();

private:
    nsCOMPtr<nsIPrincipal> mHoldee;
};

NS_DEFINE_STATIC_IID_ACCESSOR(PrincipalHolder, PRINCIPALHOLDER_IID)

#endif 




inline void *
xpc_GetJSPrivate(JSObject *obj)
{
    return obj->getPrivate();
}


#ifndef XPCONNECT_STANDALONE










nsresult
xpc_CreateSandboxObject(JSContext * cx, jsval * vp, nsISupports *prinOrSop,
                        JSObject *proto, bool preferXray);











nsresult
xpc_EvalInSandbox(JSContext *cx, JSObject *sandbox, const nsAString& source,
                  const char *filename, PRInt32 lineNo,
                  JSVersion jsVersion, PRBool returnStringOnly, jsval *rval);
#endif 




inline JSBool
xpc_ForcePropertyResolve(JSContext* cx, JSObject* obj, jsid id);

inline jsid
GetRTIdByIndex(JSContext *cx, uintN index);

inline jsval
GetRTStringByIndex(JSContext *cx, uintN index);



inline JSObject*
xpc_NewSystemInheritingJSObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                                JSObject *parent);

inline JSBool
xpc_SameScope(XPCWrappedNativeScope *objectscope,
              XPCWrappedNativeScope *xpcscope,
              JSBool *sameOrigin);

nsISupports *
XPC_GetIdentityObject(JSContext *cx, JSObject *obj);

namespace xpc {

struct CompartmentPrivate
{
  CompartmentPrivate(PtrAndPrincipalHashKey *key, bool wantXrays, bool cycleCollectionEnabled)
    : key(key),
      ptr(nsnull),
      wantXrays(wantXrays),
      cycleCollectionEnabled(cycleCollectionEnabled),
      waiverWrapperMap(nsnull)
  {
  }

  CompartmentPrivate(nsISupports *ptr, bool wantXrays, bool cycleCollectionEnabled)
    : key(nsnull),
      ptr(ptr),
      wantXrays(wantXrays),
      cycleCollectionEnabled(cycleCollectionEnabled),
      waiverWrapperMap(nsnull)
  {
  }

  ~CompartmentPrivate();

  
  nsAutoPtr<PtrAndPrincipalHashKey> key;
  nsCOMPtr<nsISupports> ptr;
  bool wantXrays;
  bool cycleCollectionEnabled;
  JSObject2JSObjectMap *waiverWrapperMap;
};

inline bool
CompartmentParticipatesInCycleCollection(JSContext *cx, JSCompartment *compartment)
{
   CompartmentPrivate *priv =
       static_cast<CompartmentPrivate *>(JS_GetCompartmentPrivate(cx, compartment));
   NS_ASSERTION(priv, "This should never be null!");

   return priv->cycleCollectionEnabled;
}

inline bool
ParticipatesInCycleCollection(JSContext *cx, js::gc::Cell *cell)
{
   return CompartmentParticipatesInCycleCollection(cx, cell->compartment());
}

}

#ifdef XPC_IDISPATCH_SUPPORT

#ifdef WINCE

FARPROC GetProcAddressA(HMODULE hMod, wchar_t *procName) {
  FARPROC ret = NULL;
  int len = wcslen(procName);
  char *s = new char[len + 1];

  for (int i = 0; i < len; i++) {
    s[i] = (char) procName[i];
  }
  s[len-1] = 0;

  ret = ::GetProcAddress(hMod, s);
  delete [] s;

  return ret;
}
#endif 



#include "XPCDispPrivate.h"
#endif




#include "xpcinlines.h"




#include "xpcmaps.h"



#endif 
