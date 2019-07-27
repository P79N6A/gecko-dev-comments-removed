








































































#ifndef xpcprivate_h___
#define xpcprivate_h___

#include "mozilla/Alignment.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Preferences.h"
#include "mozilla/TimeStamp.h"

#include "mozilla/dom/ScriptSettings.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xpcpublic.h"
#include "js/TracingAPI.h"
#include "js/WeakMapPtr.h"
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
#include "nsIJSRuntimeService.h"
#include "nsIObserver.h"
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

#include "MainThreadUtils.h"

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
#include "xpcObjectHelper.h"
#include "nsIThreadInternal.h"

#include "SandboxPrivate.h"
#include "BackstagePass.h"
#include "nsAXPCNativeCallContext.h"

#ifdef XP_WIN

#ifdef GetClassInfo
#undef GetClassInfo
#endif
#ifdef GetClassName
#undef GetClassName
#endif
#endif 




#define XPC_CONTEXT_MAP_LENGTH                   8
#define XPC_JS_MAP_LENGTH                       32
#define XPC_JS_CLASS_MAP_LENGTH                 32

#define XPC_NATIVE_MAP_LENGTH                    8
#define XPC_NATIVE_PROTO_MAP_LENGTH              8
#define XPC_DYING_NATIVE_PROTO_MAP_LENGTH        8
#define XPC_DETACHED_NATIVE_PROTO_MAP_LENGTH    16
#define XPC_NATIVE_INTERFACE_MAP_LENGTH         32
#define XPC_NATIVE_SET_MAP_LENGTH               32
#define XPC_NATIVE_JSCLASS_MAP_LENGTH           16
#define XPC_THIS_TRANSLATOR_MAP_LENGTH           4
#define XPC_NATIVE_WRAPPER_MAP_LENGTH            8
#define XPC_WRAPPER_MAP_LENGTH                   8



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


#define WRAPPER_FLAGS (JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS )

#define INVALID_OBJECT ((JSObject*)1)



static inline bool IS_WN_CLASS(const js::Class* clazz)
{
    return clazz->ext.isWrappedNative;
}

static inline bool IS_WN_REFLECTOR(JSObject* obj)
{
    return IS_WN_CLASS(js::GetObjectClass(obj));
}














class nsXPConnect final : public nsIXPConnect,
                          public nsIThreadObserver,
                          public nsSupportsWeakReference,
                          public nsIJSRuntimeService
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECT
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIJSRUNTIMESERVICE

    
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

    static nsIScriptSecurityManager* SecurityManager()
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(gScriptSecurityManager);
        return gScriptSecurityManager;
    }

    static nsIPrincipal* SystemPrincipal()
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(gSystemPrincipal);
        return gSystemPrincipal;
    }

    
    
    
    static nsXPConnect* GetSingleton();

    
    static void InitStatics();
    
    static void ReleaseXPConnectSingleton();

    bool IsShuttingDown() const {return mShuttingDown;}

    nsresult GetInfoForIID(const nsIID * aIID, nsIInterfaceInfo** info);
    nsresult GetInfoForName(const char * name, nsIInterfaceInfo** info);

    virtual nsIPrincipal* GetPrincipal(JSObject* obj,
                                       bool allowShortCircuit) const override;

    void RecordTraversal(void* p, nsISupports* s);
    virtual char* DebugPrintJSStack(bool showArgs,
                                    bool showLocals,
                                    bool showThisProps) override;


    static bool ReportAllJSExceptions()
    {
      return gReportAllJSExceptions > 0;
    }

protected:
    virtual ~nsXPConnect();

    nsXPConnect();

private:
    
    static nsXPConnect*             gSelf;
    static bool                     gOnceAliveNowDead;

    XPCJSRuntime*                   mRuntime;
    bool                            mShuttingDown;

    
    
    
    
    
    uint16_t                 mEventDepth;

    static uint32_t gReportAllJSExceptions;

public:
    static nsIScriptSecurityManager* gScriptSecurityManager;
    static nsIPrincipal* gSystemPrincipal;
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
    void AddToRootSet(XPCRootSetElem** listHead);
    void RemoveFromRootSet();

private:
    XPCRootSetElem* mNext;
    XPCRootSetElem** mSelfp;
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

template <class StringType>
class ShortLivedStringBuffer
{
public:
    StringType* Create()
    {
        for (uint32_t i = 0; i < ArrayLength(mStrings); ++i) {
            if (!mStrings[i]) {
                mStrings[i].emplace();
                return mStrings[i].ptr();
            }
        }

        
        return new StringType();
    }

    void Destroy(StringType* string)
    {
        for (uint32_t i = 0; i < ArrayLength(mStrings); ++i) {
            if (mStrings[i] && mStrings[i].ptr() == string) {
                
                
                mStrings[i].reset();
                return;
            }
        }

        
        
        delete string;
    }

    ~ShortLivedStringBuffer()
    {
#ifdef DEBUG
        for (uint32_t i = 0; i < ArrayLength(mStrings); ++i) {
            MOZ_ASSERT(!mStrings[i], "Short lived string still in use");
        }
#endif
    }

private:
    mozilla::Maybe<StringType> mStrings[2];
};

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

    bool OnJSContextNew(JSContext* cx);

    virtual bool
    DescribeCustomObjects(JSObject* aObject, const js::Class* aClasp,
                          char (&aName)[72]) const override;
    virtual bool
    NoteCustomGCThingXPCOMChildren(const js::Class* aClasp, JSObject* aObj,
                                   nsCycleCollectionTraversalCallback& aCb) const override;

    





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
        IDX_EVAL                    ,
        IDX_CONTROLLERS             ,
        IDX_REALFRAMEELEMENT        ,
        IDX_LENGTH                  ,
        IDX_NAME                    ,
        IDX_UNDEFINED               ,
        IDX_EMPTYSTRING             ,
        IDX_FILENAME                ,
        IDX_LINENUMBER              ,
        IDX_COLUMNNUMBER            ,
        IDX_STACK                   ,
        IDX_MESSAGE                 ,
        IDX_LASTINDEX               ,
        IDX_TOTAL_COUNT 
    };

    JS::HandleId GetStringID(unsigned index) const
    {
        MOZ_ASSERT(index < IDX_TOTAL_COUNT, "index out of range");
        
        return JS::HandleId::fromMarkedLocation(&mStrIDs[index]);
    }
    JS::HandleValue GetStringJSVal(unsigned index) const
    {
        MOZ_ASSERT(index < IDX_TOTAL_COUNT, "index out of range");
        
        return JS::HandleValue::fromMarkedLocation(&mStrJSVals[index]);
    }
    const char* GetStringName(unsigned index) const
    {
        MOZ_ASSERT(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrings[index];
    }

    void TraceNativeBlackRoots(JSTracer* trc) override;
    void TraceAdditionalNativeGrayRoots(JSTracer* aTracer) override;
    void TraverseAdditionalNativeRoots(nsCycleCollectionNoteRootCallback& cb) override;
    void UnmarkSkippableJSHolders();
    void PrepareForForgetSkippable() override;
    void BeginCycleCollectionCallback() override;
    void EndCycleCollectionCallback(mozilla::CycleCollectorResults& aResults) override;
    void DispatchDeferredDeletion(bool continuation) override;

    void CustomGCCallback(JSGCStatus status) override;
    void CustomOutOfMemoryCallback() override;
    void CustomLargeAllocationFailureCallback() override;
    bool CustomContextCallback(JSContext* cx, unsigned operation) override;
    static void GCSliceCallback(JSRuntime* rt,
                                JS::GCProgress progress,
                                const JS::GCDescription& desc);
    static void FinalizeCallback(JSFreeOp* fop,
                                 JSFinalizeStatus status,
                                 bool isCompartmentGC,
                                 void* data);
    static void WeakPointerCallback(JSRuntime* rt, void* data);

    inline void AddVariantRoot(XPCTraceableVariant* variant);
    inline void AddWrappedJSRoot(nsXPCWrappedJS* wrappedJS);
    inline void AddObjectHolderRoot(XPCJSObjectHolder* holder);

    static void SuspectWrappedNative(XPCWrappedNative* wrapper,
                                     nsCycleCollectionNoteRootCallback& cb);

    void DebugDump(int16_t depth);

    void SystemIsBeingShutDown();

    bool GCIsRunning() const {return mGCIsRunning;}

    ~XPCJSRuntime();

    ShortLivedStringBuffer<nsString> mScratchStrings;
    ShortLivedStringBuffer<nsCString> mScratchCStrings;

    void AddGCCallback(xpcGCCallback cb);
    void RemoveGCCallback(xpcGCCallback cb);
    void AddContextCallback(xpcContextCallback cb);
    void RemoveContextCallback(xpcContextCallback cb);

    static JSContext* DefaultJSContextCallback(JSRuntime* rt);
    static void ActivityCallback(void* arg, bool active);
    static void CTypesActivityCallback(JSContext* cx,
                                       js::CTypesActivityType type);
    static bool InterruptCallback(JSContext* cx);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    AutoMarkingPtr**  GetAutoRootsAdr() {return &mAutoRoots;}

    JSObject* UnprivilegedJunkScope() { return mUnprivilegedJunkScope; }
    JSObject* PrivilegedJunkScope() { return mPrivilegedJunkScope; }
    JSObject* CompilationScope() { return mCompilationScope; }

    void InitSingletonScopes();
    void DeleteSingletonScopes();

    PRTime GetWatchdogTimestamp(WatchdogTimestampCategory aCategory);

    void OnProcessNextEvent() {
        mSlowScriptCheckpoint = mozilla::TimeStamp::NowLoRes();
        mSlowScriptSecondHalf = false;
        js::ResetStopwatches(Get()->Runtime());
    }
    void OnAfterProcessNextEvent() {
        mSlowScriptCheckpoint = mozilla::TimeStamp();
        mSlowScriptSecondHalf = false;
    }

    nsTArray<nsXPCWrappedJS*>& WrappedJSToReleaseArray() { return mWrappedJSToReleaseArray; }

private:
    XPCJSRuntime(); 
    explicit XPCJSRuntime(nsXPConnect* aXPConnect);

    void ReleaseIncrementally(nsTArray<nsISupports*>& array);

    static const char* const mStrings[IDX_TOTAL_COUNT];
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
    bool mGCIsRunning;
    nsTArray<nsXPCWrappedJS*> mWrappedJSToReleaseArray;
    nsTArray<nsISupports*> mNativesToReleaseArray;
    bool mDoingFinalization;
    XPCRootSetElem* mVariantRoots;
    XPCRootSetElem* mWrappedJSRoots;
    XPCRootSetElem* mObjectHolderRoots;
    nsTArray<xpcGCCallback> extraGCCallbacks;
    nsTArray<xpcContextCallback> extraContextCallbacks;
    nsRefPtr<WatchdogManager> mWatchdogManager;
    JS::GCSliceCallback mPrevGCSliceCallback;
    JS::PersistentRootedObject mUnprivilegedJunkScope;
    JS::PersistentRootedObject mPrivilegedJunkScope;
    JS::PersistentRootedObject mCompilationScope;
    nsRefPtr<AsyncFreeSnowWhite> mAsyncSnowWhiteFreer;

    
    
    
    
    
    
    
    
    
    
    bool mSlowScriptSecondHalf;

    
    
    
    
    
    mozilla::TimeStamp mSlowScriptCheckpoint;

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
            return static_cast<XPCContext*>(JS_GetSecondContextPrivate(aJSContext));
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
            nsCOMPtr<nsIException> rval = mException;
            rval.forget(e);
            return NS_OK;
        }
    void SetException(nsIException* e)
        {
            mException = e;
        }

    nsresult GetLastResult() {return mLastResult;}
    void SetLastResult(nsresult rc) {mLastResult = rc;}

    nsresult GetPendingResult() {return mPendingResult;}
    void SetPendingResult(nsresult rc) {mPendingResult = rc;}

    void DebugDump(int16_t depth);

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
    nsCOMPtr<nsIException> mException;
    LangType mCallingLangType;
    bool mErrorUnreported;
};



#define NATIVE_CALLER  XPCContext::LANG_NATIVE
#define JS_CALLER      XPCContext::LANG_JS
















class MOZ_STACK_CLASS XPCCallContext : public nsAXPCNativeCallContext
{
public:
    NS_IMETHOD GetCallee(nsISupports** aResult);
    NS_IMETHOD GetCalleeMethodIndex(uint16_t* aResult);
    NS_IMETHOD GetJSContext(JSContext** aResult);
    NS_IMETHOD GetArgc(uint32_t* aResult);
    NS_IMETHOD GetArgvPtr(jsval** aResult);
    NS_IMETHOD GetCalleeInterface(nsIInterfaceInfo** aResult);
    NS_IMETHOD GetCalleeClassInfo(nsIClassInfo** aResult);
    NS_IMETHOD GetPreviousCallContext(nsAXPCNativeCallContext** aResult);
    NS_IMETHOD GetLanguage(uint16_t* aResult);

    enum {NO_ARGS = (unsigned) -1};

    static JSContext* GetDefaultJSContext();

    XPCCallContext(XPCContext::LangType callerLanguage,
                   JSContext* cx,
                   JS::HandleObject obj    = JS::NullPtr(),
                   JS::HandleObject funobj = JS::NullPtr(),
                   JS::HandleId id         = JSID_VOIDHANDLE,
                   unsigned argc           = NO_ARGS,
                   jsval* argv             = nullptr,
                   jsval* rval             = nullptr);

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
    void SetArgsAndResultPtr(unsigned argc, jsval* argv, jsval* rval);
    void SetCallInfo(XPCNativeInterface* iface, XPCNativeMember* member,
                     bool isSetter);

    nsresult  CanCallNow();

    void SystemIsBeingShutDown();

    operator JSContext*() const {return GetJSContext();}

private:

    
    XPCCallContext(const XPCCallContext& r); 
    XPCCallContext& operator= (const XPCCallContext& r); 

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
    JSAutoRequest                   mAr;
    State                           mState;

    nsRefPtr<nsXPConnect>           mXPC;

    XPCContext*                     mXPCContext;
    JSContext*                      mJSContext;

    XPCContext::LangType            mCallerLanguage;

    

    XPCContext::LangType            mPrevCallerLanguage;

    XPCCallContext*                 mPrevCallContext;

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
#define XPC_WN_TEAROFF_RESERVED_SLOTS 1
#define XPC_WN_TEAROFF_FLAT_OBJECT_SLOT 0
extern const js::Class XPC_WN_NoHelper_Proto_JSClass;

extern bool
XPC_WN_CallMethod(JSContext* cx, unsigned argc, jsval* vp);

extern bool
XPC_WN_GetterSetter(JSContext* cx, unsigned argc, jsval* vp);

extern JSObject*
XPC_WN_JSOp_ThisObject(JSContext* cx, JS::HandleObject obj);


#define XPC_WN_WithCall_ObjectOps                                             \
    {                                                                         \
        nullptr, /* lookupProperty */                                         \
        nullptr, /* defineProperty */                                         \
        nullptr, /* hasProperty */                                            \
        nullptr, /* getProperty    */                                         \
        nullptr, /* setProperty    */                                         \
        nullptr, /* getOwnPropertyDescriptor */                               \
        nullptr, /* deleteProperty */                                         \
        nullptr, nullptr, /* watch/unwatch */                                 \
        nullptr, /* getElements */                                            \
        nullptr, /* enumerate */                                              \
        XPC_WN_JSOp_ThisObject,                                               \
    }

#define XPC_WN_NoCall_ObjectOps                                               \
    {                                                                         \
        nullptr, /* lookupProperty */                                         \
        nullptr, /* defineProperty */                                         \
        nullptr, /* hasProperty */                                            \
        nullptr, /* getProperty    */                                         \
        nullptr, /* setProperty    */                                         \
        nullptr, /* getOwnPropertyDescriptor */                               \
        nullptr, /* deleteProperty */                                         \
        nullptr, nullptr, /* watch/unwatch */                                 \
        nullptr, /* getElements */                                            \
        nullptr, /* enumerate */                                              \
        XPC_WN_JSOp_ThisObject,                                               \
    }




static inline bool IS_PROTO_CLASS(const js::Class* clazz)
{
    return clazz == &XPC_WN_NoMods_WithCall_Proto_JSClass ||
           clazz == &XPC_WN_NoMods_NoCall_Proto_JSClass ||
           clazz == &XPC_WN_ModsAllowed_WithCall_Proto_JSClass ||
           clazz == &XPC_WN_ModsAllowed_NoCall_Proto_JSClass;
}




class nsIAddonInterposition;
class nsXPCComponentsBase;
class XPCWrappedNativeScope : public PRCList
{
public:

    XPCJSRuntime*
    GetRuntime() const {return XPCJSRuntime::Get();}

    Native2WrappedNativeMap*
    GetWrappedNativeMap() const {return mWrappedNativeMap;}

    ClassInfo2WrappedNativeProtoMap*
    GetWrappedNativeProtoMap() const {return mWrappedNativeProtoMap;}

    nsXPCComponentsBase*
    GetComponents() const {return mComponents;}

    
    
    void
    ForcePrivilegedComponents();

    bool AttachComponentsObject(JSContext* aCx);

    
    bool
    GetComponentsJSObject(JS::MutableHandleObject obj);

    JSObject*
    GetGlobalJSObject() const {
        JS::ExposeObjectToActiveJS(mGlobalJSObject);
        return mGlobalJSObject;
    }

    JSObject*
    GetGlobalJSObjectPreserveColor() const {return mGlobalJSObject;}

    nsIPrincipal*
    GetPrincipal() const {
        JSCompartment* c = js::GetObjectCompartment(mGlobalJSObject);
        return nsJSPrincipals::get(JS_GetCompartmentPrincipals(c));
    }

    JSObject*
    GetExpandoChain(JS::HandleObject target);

    bool
    SetExpandoChain(JSContext* cx, JS::HandleObject target, JS::HandleObject chain);

    static void
    SystemIsBeingShutDown();

    static void
    TraceWrappedNativesInAllScopes(JSTracer* trc, XPCJSRuntime* rt);

    void TraceSelf(JSTracer* trc) {
        MOZ_ASSERT(mGlobalJSObject);
        mGlobalJSObject.trace(trc, "XPCWrappedNativeScope::mGlobalJSObject");
    }

    void TraceInside(JSTracer* trc) {
        if (mContentXBLScope)
            mContentXBLScope.trace(trc, "XPCWrappedNativeScope::mXBLScope");
        for (size_t i = 0; i < mAddonScopes.Length(); i++)
            mAddonScopes[i].trace(trc, "XPCWrappedNativeScope::mAddonScopes");
        if (mXrayExpandos.initialized())
            mXrayExpandos.trace(trc);
    }

    static void
    SuspectAllWrappers(XPCJSRuntime* rt, nsCycleCollectionNoteRootCallback& cb);

    static void
    MarkAllWrappedNativesAndProtos();

#ifdef DEBUG
    static void
    ASSERT_NoInterfaceSetsAreMarked();
#endif

    static void
    SweepAllWrappedNativeTearOffs();

    static void
    UpdateWeakPointersAfterGC(XPCJSRuntime* rt);

    static void
    KillDyingScopes();

    static void
    DebugDumpAllScopes(int16_t depth);

    void
    DebugDump(int16_t depth);

    struct ScopeSizeInfo {
        explicit ScopeSizeInfo(mozilla::MallocSizeOf mallocSizeOf)
            : mMallocSizeOf(mallocSizeOf),
              mScopeAndMapSize(0),
              mProtoAndIfaceCacheSize(0)
        {}

        mozilla::MallocSizeOf mMallocSizeOf;
        size_t mScopeAndMapSize;
        size_t mProtoAndIfaceCacheSize;
    };

    static void
    AddSizeOfAllScopesIncludingThis(ScopeSizeInfo* scopeSizeInfo);

    void
    AddSizeOfIncludingThis(ScopeSizeInfo* scopeSizeInfo);

    bool
    IsValid() const {return mRuntime != nullptr;}

    static bool
    IsDyingScope(XPCWrappedNativeScope* scope);

    typedef js::HashSet<JSObject*,
                        js::PointerHasher<JSObject*, 3>,
                        js::SystemAllocPolicy> DOMExpandoSet;

    bool RegisterDOMExpandoObject(JSObject* expando) {
        
        JS::AssertGCThingMustBeTenured(expando);
        if (!mDOMExpandoSet) {
            mDOMExpandoSet = new DOMExpandoSet();
            mDOMExpandoSet->init(8);
        }
        return mDOMExpandoSet->put(expando);
    }
    void RemoveDOMExpandoObject(JSObject* expando) {
        if (mDOMExpandoSet) {
            DOMExpandoSet::Ptr p = mDOMExpandoSet->lookup(expando);
            MOZ_ASSERT(p.found());
            mDOMExpandoSet->remove(p);
        }
    }

    typedef js::HashMap<JSAddonId*,
                        nsCOMPtr<nsIAddonInterposition>,
                        js::PointerHasher<JSAddonId*, 3>,
                        js::SystemAllocPolicy> InterpositionMap;

    static bool SetAddonInterposition(JSAddonId* addonId,
                                      nsIAddonInterposition* interp);

    
    
    
    JSObject* EnsureContentXBLScope(JSContext* cx);

    JSObject* EnsureAddonScope(JSContext* cx, JSAddonId* addonId);

    XPCWrappedNativeScope(JSContext* cx, JS::HandleObject aGlobal);

    nsAutoPtr<JSObject2JSObjectMap> mWaiverWrapperMap;

    bool IsContentXBLScope() { return mIsContentXBLScope; }
    bool AllowContentXBLScope();
    bool UseContentXBLScope() { return mUseContentXBLScope; }

    bool IsAddonScope() { return mIsAddonScope; }

    bool HasInterposition() { return mInterposition; }
    nsCOMPtr<nsIAddonInterposition> GetInterposition();

protected:
    virtual ~XPCWrappedNativeScope();

    XPCWrappedNativeScope(); 

private:
    class ClearInterpositionsObserver final : public nsIObserver {
        ~ClearInterpositionsObserver() {}

      public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOBSERVER
    };

    static XPCWrappedNativeScope* gScopes;
    static XPCWrappedNativeScope* gDyingScopes;

    static InterpositionMap*         gInterpositionMap;

    XPCJSRuntime*                    mRuntime;
    Native2WrappedNativeMap*         mWrappedNativeMap;
    ClassInfo2WrappedNativeProtoMap* mWrappedNativeProtoMap;
    nsRefPtr<nsXPCComponentsBase>    mComponents;
    XPCWrappedNativeScope*           mNext;
    
    
    
    
    JS::ObjectPtr                    mGlobalJSObject;

    
    
    
    JS::ObjectPtr                    mContentXBLScope;

    
    nsTArray<JS::ObjectPtr>          mAddonScopes;

    
    
    nsCOMPtr<nsIAddonInterposition>  mInterposition;

    nsAutoPtr<DOMExpandoSet> mDOMExpandoSet;

    JS::WeakMapPtr<JSObject*, JSObject*> mXrayExpandos;

    bool mIsContentXBLScope;
    bool mIsAddonScope;

    
    
    
    
    
    
    
    
    
    
    
    bool mAllowContentXBLScope;
    bool mUseContentXBLScope;
};



#define XPC_FUNCTION_NATIVE_MEMBER_SLOT 0
#define XPC_FUNCTION_PARENT_OBJECT_SLOT 1







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

    static uint16_t GetMaxIndexInInterface()
        {return (1<<12) - 1;}

    inline XPCNativeInterface* GetInterface() const;

    void SetIndexInInterface(uint16_t index)
        {mIndexInInterface = index;}

    
    XPCNativeMember()  {MOZ_COUNT_CTOR(XPCNativeMember);}
    ~XPCNativeMember() {MOZ_COUNT_DTOR(XPCNativeMember);}

private:
    bool Resolve(XPCCallContext& ccx, XPCNativeInterface* iface,
                 JS::HandleObject parent, jsval* vp);

    enum {
        METHOD      = 0x01,
        CONSTANT    = 0x02,
        GETTER      = 0x04,
        SETTER_TOO  = 0x08
        
        
        
        
    };

private:
    
    jsid     mName;
    uint16_t mIndex;
    
    uint16_t mFlags : 4;
    
    
    
    
    
    uint16_t mIndexInInterface : 12;
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
    static inline size_t OffsetOfMembers();

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
    explicit XPCNativeSetKey(XPCNativeSet*       BaseSet  = nullptr,
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
    XPCNativeInterface** GetInterfaceArray() {
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

    explicit XPCNativeScriptableFlags(uint32_t flags = 0) : mFlags(flags) {}

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
    bool WantAddProperty()              GET_IT(WANT_ADDPROPERTY)
    bool WantGetProperty()              GET_IT(WANT_GETPROPERTY)
    bool WantSetProperty()              GET_IT(WANT_SETPROPERTY)
    bool WantEnumerate()                GET_IT(WANT_ENUMERATE)
    bool WantNewEnumerate()             GET_IT(WANT_NEWENUMERATE)
    bool WantResolve()                  GET_IT(WANT_RESOLVE)
    bool WantFinalize()                 GET_IT(WANT_FINALIZE)
    bool WantCall()                     GET_IT(WANT_CALL)
    bool WantConstruct()                GET_IT(WANT_CONSTRUCT)
    bool WantHasInstance()              GET_IT(WANT_HASINSTANCE)
    bool UseJSStubForAddProperty()      GET_IT(USE_JSSTUB_FOR_ADDPROPERTY)
    bool UseJSStubForDelProperty()      GET_IT(USE_JSSTUB_FOR_DELPROPERTY)
    bool UseJSStubForSetProperty()      GET_IT(USE_JSSTUB_FOR_SETPROPERTY)
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
};

class XPCNativeScriptableShared
{
public:
    const XPCNativeScriptableFlags& GetFlags() const {return mFlags;}
    const JSClass*                  GetJSClass()
        {return Jsvalify(&mJSClass.base);}

    XPCNativeScriptableShared(uint32_t aFlags, char* aName)
        : mFlags(aFlags)
        {memset(&mJSClass, 0, sizeof(mJSClass));
         mJSClass.base.name = aName;  
         MOZ_COUNT_CTOR(XPCNativeScriptableShared);}

    ~XPCNativeScriptableShared()
        {if (mJSClass.base.name)free((void*)mJSClass.base.name);
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

    const JSClass*
    GetJSClass()          {return mShared->GetJSClass();}

    XPCNativeScriptableShared*
    GetScriptableShared() {return mShared;}

    void
    SetCallback(nsIXPCScriptable* s) {mCallback = s;}
    void
    SetCallback(already_AddRefed<nsIXPCScriptable>&& s) {mCallback = s;}

    void
    SetScriptableShared(XPCNativeScriptableShared* shared) {mShared = shared;}

    void Mark() {
        if (mShared)
            mShared->Mark();
    }

    void TraceJS(JSTracer* trc) {}
    void AutoTrace(JSTracer* trc) {}

protected:
    explicit XPCNativeScriptableInfo(nsIXPCScriptable* scriptable = nullptr,
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

    explicit XPCNativeScriptableCreateInfo(const XPCNativeScriptableInfo& si)
        : mCallback(si.GetCallback()), mFlags(si.GetFlags()) {}

    XPCNativeScriptableCreateInfo(already_AddRefed<nsIXPCScriptable>&& callback,
                                  XPCNativeScriptableFlags flags)
        : mCallback(callback), mFlags(flags) {}

    XPCNativeScriptableCreateInfo()
        : mFlags(0) {}


    nsIXPCScriptable*
    GetCallback() const {return mCallback;}

    const XPCNativeScriptableFlags&
    GetFlags() const      {return mFlags;}

    void
    SetCallback(already_AddRefed<nsIXPCScriptable>&& callback)
        {mCallback = callback;}

    void
    SetFlags(const XPCNativeScriptableFlags& flags)  {mFlags = flags;}

private:
    nsCOMPtr<nsIXPCScriptable>  mCallback;
    XPCNativeScriptableFlags    mFlags;
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

    uint32_t
    GetClassInfoFlags() const {return mClassInfoFlags;}

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return !!(mClassInfoFlags & nsIClassInfo:: f_ );}

    bool ClassIsSingleton()           GET_IT(SINGLETON)
    bool ClassIsDOMObject()           GET_IT(DOM_OBJECT)
    bool ClassIsPluginObject()        GET_IT(PLUGIN_OBJECT)

#undef GET_IT

    void SetScriptableInfo(XPCNativeScriptableInfo* si)
        {MOZ_ASSERT(!mScriptableInfo, "leak here!"); mScriptableInfo = si;}

    bool CallPostCreatePrototype();
    void JSProtoObjectFinalized(js::FreeOp* fop, JSObject* obj);
    void JSProtoObjectMoved(JSObject* obj, const JSObject* old);

    void SystemIsBeingShutDown();

    void DebugDump(int16_t depth);

    void TraceSelf(JSTracer* trc) {
        if (mJSProtoObject)
            mJSProtoObject.trace(trc, "XPCWrappedNativeProto::mJSProtoObject");
    }

    void TraceInside(JSTracer* trc) {
        if (trc->isMarkingTracer()) {
            mSet->Mark();
            if (mScriptableInfo)
                mScriptableInfo->Mark();
        }

        GetScope()->TraceSelf(trc);
    }

    void TraceJS(JSTracer* trc) {
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
#ifdef DEBUG
    static int32_t gDEBUG_LiveProtoCount;
#endif

private:
    XPCWrappedNativeScope*   mScope;
    JS::ObjectPtr            mJSProtoObject;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    uint32_t                 mClassInfoFlags;
    XPCNativeSet*            mSet;
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
    already_AddRefed<nsISupports> TakeNative() { return mNative.forget(); }
    void SetJSObject(JSObject*  JSObj);

    void JSObjectFinalized() {SetJSObject(nullptr);}
    void JSObjectMoved(JSObject* obj, const JSObject* old);

    XPCWrappedNativeTearOff()
        : mInterface(nullptr), mJSObject(nullptr)
    {
        MOZ_COUNT_CTOR(XPCWrappedNativeTearOff);
    }
    ~XPCWrappedNativeTearOff();

    
    inline void TraceJS(JSTracer* trc) {}
    inline void AutoTrace(JSTracer* trc) {}

    void Mark()       {mJSObject.setFlags(1);}
    void Unmark()     {mJSObject.unsetFlags(1);}
    bool IsMarked() const {return mJSObject.hasFlag(1);}

private:
    XPCWrappedNativeTearOff(const XPCWrappedNativeTearOff& r) = delete;
    XPCWrappedNativeTearOff& operator= (const XPCWrappedNativeTearOff& r) = delete;

private:
    XPCNativeInterface* mInterface;
    
    
    nsRefPtr<nsISupports> mNative;
    JS::TenuredHeap<JSObject*> mJSObject;
};







class XPCWrappedNativeTearOffChunk
{
friend class XPCWrappedNative;
private:
    XPCWrappedNativeTearOffChunk() : mNextChunk(nullptr) {}
    ~XPCWrappedNativeTearOffChunk() {delete mNextChunk;}

private:
    XPCWrappedNativeTearOff mTearOff;
    XPCWrappedNativeTearOffChunk* mNextChunk;
};





class XPCWrappedNative final : public nsIXPConnectWrappedNative
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDNATIVE

    NS_DECL_CYCLE_COLLECTION_CLASS(XPCWrappedNative)

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

    XPCNativeSet*
    GetSet() const {return mSet;}

    void
    SetSet(XPCNativeSet* set) {mSet = set;}

    static XPCWrappedNative* Get(JSObject* obj) {
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
    WrapNewGlobal(xpcObjectHelper& nativeHelper,
                  nsIPrincipal* principal, bool initStandardClasses,
                  JS::CompartmentOptions& aOptions,
                  XPCWrappedNative** wrappedGlobal);

    static nsresult
    GetNewOrUsed(xpcObjectHelper& helper,
                 XPCWrappedNativeScope* Scope,
                 XPCNativeInterface* Interface,
                 XPCWrappedNative** wrapper);

public:
    static nsresult
    GetUsedOnly(nsISupports* Object,
                XPCWrappedNativeScope* Scope,
                XPCNativeInterface* Interface,
                XPCWrappedNative** wrapper);

    void FlatJSObjectFinalized();
    void FlatJSObjectMoved(JSObject* obj, const JSObject* old);

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
    XPCWrappedNativeTearOff* FindTearOff(const nsIID& iid);

    void Mark() const
    {
        mSet->Mark();
        if (mScriptableInfo) mScriptableInfo->Mark();
        if (HasProto()) GetProto()->Mark();
    }

    
    inline void TraceInside(JSTracer* trc) {
        if (trc->isMarkingTracer()) {
            mSet->Mark();
            if (mScriptableInfo)
                mScriptableInfo->Mark();
        }
        if (HasProto())
            GetProto()->TraceSelf(trc);
        else
            GetScope()->TraceSelf(trc);
        if (mFlatJSObject && JS_IsGlobalObject(mFlatJSObject))
        {
            xpc::TraceXPCGlobal(trc, mFlatJSObject);
        }
    }

    void TraceJS(JSTracer* trc) {
        TraceInside(trc);
    }

    void TraceSelf(JSTracer* trc) {
        
        
        
        
        
        if (mFlatJSObject) {
            JS_CallTenuredObjectTracer(trc, &mFlatJSObject,
                                       "XPCWrappedNative::mFlatJSObject");
        }
    }

    static void Trace(JSTracer* trc, JSObject* obj);

    void AutoTrace(JSTracer* trc) {
        TraceSelf(trc);
    }

#ifdef DEBUG
    void ASSERT_SetsNotMarked() const
        {mSet->ASSERT_NotMarked();
         if (HasProto()){GetProto()->ASSERT_SetNotMarked();}}
#endif

    inline void SweepTearOffs();

    
    char* ToString(XPCWrappedNativeTearOff* to = nullptr) const;

    static void GatherProtoScriptableCreateInfo(nsIClassInfo* classInfo,
                                                XPCNativeScriptableCreateInfo& sciProto);

    bool HasExternalReference() const {return mRefCnt > 1;}

    void NoteTearoffs(nsCycleCollectionTraversalCallback& cb);

    
protected:
    XPCWrappedNative(); 

    
    XPCWrappedNative(already_AddRefed<nsISupports>&& aIdentity,
                     XPCWrappedNativeProto* aProto);

    
    XPCWrappedNative(already_AddRefed<nsISupports>&& aIdentity,
                     XPCWrappedNativeScope* aScope,
                     XPCNativeSet* aSet);

    virtual ~XPCWrappedNative();
    void Destroy();

    void UpdateScriptableInfo(XPCNativeScriptableInfo* si);

private:
    enum {
        
        FLAT_JS_OBJECT_VALID = JS_BIT(0)
    };

private:

    bool Init(const XPCNativeScriptableCreateInfo* sci);
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





class nsXPCWrappedJSClass final : public nsIXPCWrappedJSClass
{
    
    NS_DECL_ISUPPORTS
    NS_IMETHOD DebugDump(int16_t depth) override;
public:

    static already_AddRefed<nsXPCWrappedJSClass>
    GetNewOrUsed(JSContext* cx,
                 REFNSIID aIID,
                 bool allowNonScriptable = false);

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

    static nsresult CheckForException(XPCCallContext & ccx,
                                      const char * aPropertyName,
                                      const char * anInterfaceName,
                                      bool aForceReport);
private:
    virtual ~nsXPCWrappedJSClass();

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
    nsCOMPtr<nsIInterfaceInfo> mInfo;
    char* mName;
    nsIID mIID;
    uint32_t* mDescriptors;
};






class nsXPCWrappedJS final : protected nsAutoXPTCStub,
                             public nsIXPConnectWrappedJS,
                             public nsSupportsWeakReference,
                             public nsIPropertyBag,
                             public XPCRootSetElem
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDJS
    NS_DECL_NSISUPPORTSWEAKREFERENCE
    NS_DECL_NSIPROPERTYBAG

    NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS_AMBIGUOUS(nsXPCWrappedJS, nsIXPConnectWrappedJS)

    NS_IMETHOD CallMethod(uint16_t methodIndex,
                          const XPTMethodDescriptor* info,
                          nsXPTCMiniVariant* params) override;

    





    static nsresult
    GetNewOrUsed(JS::HandleObject aJSObj,
                 REFNSIID aIID,
                 nsXPCWrappedJS** wrapper);

    nsISomeInterface* GetXPTCStub() { return mXPTCStub; }

    







    JSObject* GetJSObjectPreserveColor() const {return mJSObj;}

    nsXPCWrappedJSClass*  GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}
    nsXPCWrappedJS* GetRootWrapper() const {return mRoot;}
    nsXPCWrappedJS* GetNextWrapper() const {return mNext;}

    nsXPCWrappedJS* Find(REFNSIID aIID);
    nsXPCWrappedJS* FindInherited(REFNSIID aIID);
    nsXPCWrappedJS* FindOrFindInherited(REFNSIID aIID) {
        nsXPCWrappedJS* wrapper = Find(aIID);
        if (wrapper)
            return wrapper;
        return FindInherited(aIID);
    }

    bool IsRootWrapper() const {return mRoot == this;}
    bool IsValid() const {return mJSObj != nullptr;}
    void SystemIsBeingShutDown();

    
    
    
    bool IsSubjectToFinalization() const {return IsValid() && mRefCnt == 1;}
    void UpdateObjectPointerAfterGC() {JS_UpdateWeakPointerAfterGC(&mJSObj);}

    bool IsAggregatedToNative() const {return mRoot->mOuter != nullptr;}
    nsISupports* GetAggregatedNativeObject() const {return mRoot->mOuter;}
    void SetAggregatedNativeObject(nsISupports* aNative) {
        MOZ_ASSERT(aNative);
        if (mRoot->mOuter) {
            MOZ_ASSERT(mRoot->mOuter == aNative,
                       "Only one aggregated native can be set");
            return;
        }
        mRoot->mOuter = aNative;
    }

    void TraceJS(JSTracer* trc);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    virtual ~nsXPCWrappedJS();
protected:
    nsXPCWrappedJS();   
    nsXPCWrappedJS(JSContext* cx,
                   JSObject* aJSObj,
                   nsXPCWrappedJSClass* aClass,
                   nsXPCWrappedJS* root,
                   nsresult* rv);

    bool CanSkip();
    void Destroy();
    void Unlink();

private:
    JS::Heap<JSObject*> mJSObj;
    nsRefPtr<nsXPCWrappedJSClass> mClass;
    nsXPCWrappedJS* mRoot;    
    nsXPCWrappedJS* mNext;
    nsCOMPtr<nsISupports> mOuter;    
};



class XPCJSObjectHolder : public nsIXPConnectJSObjectHolder,
                          public XPCRootSetElem
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER

    

public:
    void TraceJS(JSTracer* trc);

    explicit XPCJSObjectHolder(JSObject* obj);

private:
    virtual ~XPCJSObjectHolder();

    XPCJSObjectHolder(); 

    JS::Heap<JSObject*> mJSObj;
};









class xpcProperty : public nsIProperty
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTY

  xpcProperty(const char16_t* aName, uint32_t aNameLen, nsIVariant* aValue);

private:
    virtual ~xpcProperty() {}

    nsString             mName;
    nsCOMPtr<nsIVariant> mValue;
};



class XPCConvert
{
public:
    static bool IsMethodReflectable(const XPTMethodDescriptor& info);

    











    static bool NativeData2JS(JS::MutableHandleValue d,
                              const void* s, const nsXPTType& type,
                              const nsID* iid, nsresult* pErr);

    static bool JSData2Native(void* d, JS::HandleValue s,
                              const nsXPTType& type,
                              const nsID* iid,
                              nsresult* pErr);

    














    static bool NativeInterface2JSObject(JS::MutableHandleValue d,
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

    










    static bool NativeArray2JS(JS::MutableHandleValue d, const void** s,
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

    static bool NativeStringWithSize2JS(JS::MutableHandleValue d, const void* s,
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
                                       jsval* jsExceptionPtr);

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

    static bool CheckForPendingException(nsresult result, JSContext* cx);

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

class nsJSID final : public nsIJSID
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_JS_ID_CID)

    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSID

    bool InitWithName(const nsID& id, const char* nameString);
    bool SetName(const char* name);
    void   SetNameToNoString()
        {MOZ_ASSERT(!mName, "name already set"); mName = const_cast<char*>(gNoString);}
    bool NameIsSet() const {return nullptr != mName;}
    const nsID& ID() const {return mID;}
    bool IsValid() const {return !mID.Equals(GetInvalidIID());}

    static already_AddRefed<nsJSID> NewID(const char* str);
    static already_AddRefed<nsJSID> NewID(const nsID& id);

    nsJSID();

    void Reset();
    const nsID& GetInvalidIID() const;

protected:
    virtual ~nsJSID();
    static const char gNoString[];
    nsID    mID;
    char*   mNumber;
    char*   mName;
};




class nsJSIID : public nsIJSIID,
                public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIJSID

    
    NS_DECL_NSIJSIID
    NS_DECL_NSIXPCSCRIPTABLE

    static already_AddRefed<nsJSIID> NewID(nsIInterfaceInfo* aInfo);

    explicit nsJSIID(nsIInterfaceInfo* aInfo);
    nsJSIID(); 

private:
    virtual ~nsJSIID();

    nsCOMPtr<nsIInterfaceInfo> mInfo;
};



class nsJSCID : public nsIJSCID, public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIJSID

    
    NS_DECL_NSIJSCID
    NS_DECL_NSIXPCSCRIPTABLE

    static already_AddRefed<nsJSCID> NewID(const char* str);

    nsJSCID();

private:
    virtual ~nsJSCID();

    void ResolveName();

private:
    nsRefPtr<nsJSID> mDetails;
};





struct XPCJSContextInfo {
    explicit XPCJSContextInfo(JSContext* aCx) :
        cx(aCx),
        savedFrameChain(false)
    {}
    JSContext* cx;

    
    bool savedFrameChain;
};

namespace xpc {






bool PushJSContextNoScriptContext(JSContext* aCx);
void PopJSContextNoScriptContext();

} 

namespace mozilla {
namespace dom {
namespace danger {
class AutoCxPusher;
}
}
}

class XPCJSContextStack
{
public:
    explicit XPCJSContextStack(XPCJSRuntime* aRuntime)
      : mRuntime(aRuntime)
      , mSafeJSContext(nullptr)
    { }

    virtual ~XPCJSContextStack();

    uint32_t Count()
    {
        return mStack.Length();
    }

    JSContext* Peek()
    {
        return mStack.IsEmpty() ? nullptr : mStack[mStack.Length() - 1].cx;
    }

    JSContext* InitSafeJSContext();
    JSContext* GetSafeJSContext();
    bool HasJSContext(JSContext* cx);

    const InfallibleTArray<XPCJSContextInfo>* GetStack()
    { return &mStack; }

private:
    friend class mozilla::dom::danger::AutoCxPusher;
    friend bool xpc::PushJSContextNoScriptContext(JSContext* aCx);
    friend void xpc::PopJSContextNoScriptContext();

    
    
    JSContext* Pop();
    bool Push(JSContext* cx);

    AutoInfallibleTArray<XPCJSContextInfo, 16> mStack;
    XPCJSRuntime* mRuntime;
    JSContext*  mSafeJSContext;
};





class nsXPCComponentsBase : public nsIXPCComponentsBase
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTSBASE

public:
    void SystemIsBeingShutDown() { ClearMembers(); }

    XPCWrappedNativeScope* GetScope() { return mScope; }

protected:
    virtual ~nsXPCComponentsBase();

    explicit nsXPCComponentsBase(XPCWrappedNativeScope* aScope);
    virtual void ClearMembers();

    XPCWrappedNativeScope*                   mScope;

    
    nsRefPtr<nsXPCComponents_Interfaces>     mInterfaces;
    nsRefPtr<nsXPCComponents_InterfacesByID> mInterfacesByID;
    nsRefPtr<nsXPCComponents_Results>        mResults;

    friend class XPCWrappedNativeScope;
};

class nsXPCComponents : public nsXPCComponentsBase,
                        public nsIXPCComponents
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_FORWARD_NSIXPCCOMPONENTSBASE(nsXPCComponentsBase::)
    NS_DECL_NSIXPCCOMPONENTS

protected:
    explicit nsXPCComponents(XPCWrappedNativeScope* aScope);
    virtual ~nsXPCComponents();
    virtual void ClearMembers() override;

    
    nsRefPtr<nsXPCComponents_Classes>     mClasses;
    nsRefPtr<nsXPCComponents_ClassesByID> mClassesByID;
    nsRefPtr<nsXPCComponents_ID>          mID;
    nsRefPtr<nsXPCComponents_Exception>   mException;
    nsRefPtr<nsXPCComponents_Constructor> mConstructor;
    nsRefPtr<nsXPCComponents_Utils>       mUtils;

    friend class XPCWrappedNativeScope;
};




extern JSObject*
xpc_NewIDObject(JSContext* cx, JS::HandleObject jsobj, const nsID& aID);

extern const nsID*
xpc_JSObjectToID(JSContext* cx, JSObject* obj);

extern bool
xpc_JSObjectIsID(JSContext* cx, JSObject* obj);




extern bool
xpc_DumpJSStack(bool showArgs, bool showLocals, bool showThisProps);




extern char*
xpc_PrintJSStack(JSContext* cx, bool showArgs, bool showLocals,
                 bool showThisProps);





class nsScriptError final : public nsIScriptError {
public:
    nsScriptError();

  

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSICONSOLEMESSAGE
    NS_DECL_NSISCRIPTERROR

private:
    virtual ~nsScriptError();

    void
    InitializeOnMainThread();

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
    
    
    mozilla::Atomic<bool> mInitializedOnMainThread;
    bool mIsFromPrivateWindow;
};




class MOZ_STACK_CLASS AutoScriptEvaluate
{
public:
    



    explicit AutoScriptEvaluate(JSContext * cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
         : mJSContext(cx), mEvaluated(false) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    





    bool StartEvaluating(JS::HandleObject scope);

    


    ~AutoScriptEvaluate();
private:
    JSContext* mJSContext;
    mozilla::Maybe<JS::AutoSaveExceptionState> mState;
    bool mEvaluated;
    mozilla::Maybe<JSAutoCompartment> mAutoCompartment;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoScriptEvaluate(const AutoScriptEvaluate&) = delete;
    AutoScriptEvaluate & operator =(const AutoScriptEvaluate&) = delete;
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
    explicit AutoMarkingPtr(JSContext* cx) {
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
        for (AutoMarkingPtr* cur = this; cur; cur = cur->mNext)
            cur->TraceJS(trc);
    }

    void MarkAfterJSFinalizeAll() {
        for (AutoMarkingPtr* cur = this; cur; cur = cur->mNext)
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
    explicit TypedAutoMarkingPtr(JSContext* cx) : AutoMarkingPtr(cx), mPtr(nullptr) {}
    TypedAutoMarkingPtr(JSContext* cx, T* ptr) : AutoMarkingPtr(cx), mPtr(ptr) {}

    T* get() const { return mPtr; }
    operator T*() const { return mPtr; }
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
    explicit ArrayAutoMarkingPtr(JSContext* cx)
      : AutoMarkingPtr(cx), mPtr(nullptr), mCount(0) {}
    ArrayAutoMarkingPtr(JSContext* cx, T** ptr, uint32_t count, bool clear)
      : AutoMarkingPtr(cx), mPtr(ptr), mCount(count)
    {
        if (!mPtr) mCount = 0;
        else if (clear) memset(mPtr, 0, mCount*sizeof(T*));
    }

    T** get() const { return mPtr; }
    operator T**() const { return mPtr; }
    T** operator->() const { return mPtr; }

    ArrayAutoMarkingPtr<T>& operator =(const ArrayAutoMarkingPtr<T>& other)
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

char*
CloneAllAccess();


char*
CheckAccessList(const char16_t* wideName, const char* const list[]);
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

    static already_AddRefed<XPCVariant> newVariant(JSContext* cx, jsval aJSVal);

    




    jsval GetJSVal() const {
        if (!mJSVal.isPrimitive())
            JS::ExposeObjectToActiveJS(&mJSVal.toObject());
        return mJSVal;
    }

    








    jsval GetJSValPreserveColor() const {return mJSVal;}

    XPCVariant(JSContext* cx, jsval aJSVal);

    








    static bool VariantDataToJS(nsIVariant* variant,
                                nsresult* pErr, JS::MutableHandleValue pJSVal);

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
};




inline void*
xpc_GetJSPrivate(JSObject* obj)
{
    return js::GetObjectPrivate(obj);
}

inline JSContext*
xpc_GetSafeJSContext()
{
    return XPCJSRuntime::Get()->GetJSContextStack()->GetSafeJSContext();
}

namespace xpc {

JSAddonId*
NewAddonId(JSContext* cx, const nsACString& id);


bool
Atob(JSContext* cx, unsigned argc, jsval* vp);

bool
Btoa(JSContext* cx, unsigned argc, jsval* vp);



class FunctionForwarderOptions;
bool
NewFunctionForwarder(JSContext* cx, JS::HandleId id, JS::HandleObject callable,
                     FunctionForwarderOptions& options, JS::MutableHandleValue vp);


nsresult
ThrowAndFail(nsresult errNum, JSContext* cx, bool* retval);

struct GlobalProperties {
    GlobalProperties() {
      mozilla::PodZero(this);

    }
    bool Parse(JSContext* cx, JS::HandleObject obj);
    bool Define(JSContext* cx, JS::HandleObject obj);
    bool CSS : 1;
    bool indexedDB : 1;
    bool XMLHttpRequest : 1;
    bool TextDecoder : 1;
    bool TextEncoder : 1;
    bool URL : 1;
    bool URLSearchParams : 1;
    bool atob : 1;
    bool btoa : 1;
    bool Blob : 1;
    bool File : 1;
    bool crypto : 1;
    bool rtcIdentityProvider : 1;
};


already_AddRefed<nsIXPCComponents_utils_Sandbox>
NewSandboxConstructor();


bool
IsSandbox(JSObject* obj);

class MOZ_STACK_CLASS OptionsBase {
public:
    explicit OptionsBase(JSContext* cx = xpc_GetSafeJSContext(),
                         JSObject* options = nullptr)
        : mCx(cx)
        , mObject(cx, options)
    { }

    virtual bool Parse() = 0;

protected:
    bool ParseValue(const char* name, JS::MutableHandleValue prop, bool* found = nullptr);
    bool ParseBoolean(const char* name, bool* prop);
    bool ParseObject(const char* name, JS::MutableHandleObject prop);
    bool ParseJSString(const char* name, JS::MutableHandleString prop);
    bool ParseString(const char* name, nsCString& prop);
    bool ParseString(const char* name, nsString& prop);
    bool ParseId(const char* name, JS::MutableHandleId id);

    JSContext* mCx;
    JS::RootedObject mObject;
};

class MOZ_STACK_CLASS SandboxOptions : public OptionsBase {
public:
    explicit SandboxOptions(JSContext* cx = xpc_GetSafeJSContext(),
                            JSObject* options = nullptr)
        : OptionsBase(cx, options)
        , wantXrays(true)
        , wantComponents(true)
        , wantExportHelpers(false)
        , proto(cx)
        , addonId(cx)
        , writeToGlobalPrototype(false)
        , sameZoneAs(cx)
        , invisibleToDebugger(false)
        , discardSource(false)
        , metadata(cx)
    { }

    virtual bool Parse();

    bool wantXrays;
    bool wantComponents;
    bool wantExportHelpers;
    JS::RootedObject proto;
    nsCString sandboxName;
    JS::RootedString addonId;
    bool writeToGlobalPrototype;
    JS::RootedObject sameZoneAs;
    bool invisibleToDebugger;
    bool discardSource;
    GlobalProperties globalProperties;
    JS::RootedValue metadata;

protected:
    bool ParseGlobalProperties();
};

class MOZ_STACK_CLASS CreateObjectInOptions : public OptionsBase {
public:
    explicit CreateObjectInOptions(JSContext* cx = xpc_GetSafeJSContext(),
                                   JSObject* options = nullptr)
        : OptionsBase(cx, options)
        , defineAs(cx, JSID_VOID)
    { }

    virtual bool Parse() { return ParseId("defineAs", &defineAs); }

    JS::RootedId defineAs;
};

class MOZ_STACK_CLASS ExportFunctionOptions : public OptionsBase {
public:
    explicit ExportFunctionOptions(JSContext* cx = xpc_GetSafeJSContext(),
                                   JSObject* options = nullptr)
        : OptionsBase(cx, options)
        , defineAs(cx, JSID_VOID)
        , allowCrossOriginArguments(false)
    { }

    virtual bool Parse() {
        return ParseId("defineAs", &defineAs) &&
               ParseBoolean("allowCrossOriginArguments", &allowCrossOriginArguments);
    }

    JS::RootedId defineAs;
    bool allowCrossOriginArguments;
};

class MOZ_STACK_CLASS FunctionForwarderOptions : public OptionsBase {
public:
    explicit FunctionForwarderOptions(JSContext* cx = xpc_GetSafeJSContext(),
                                      JSObject* options = nullptr)
        : OptionsBase(cx, options)
        , allowCrossOriginArguments(false)
    { }

    JSObject* ToJSObject(JSContext* cx) {
        JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr()));
        if (!obj)
            return nullptr;

        JS::RootedValue val(cx);
        unsigned attrs = JSPROP_READONLY | JSPROP_PERMANENT;
        val = JS::BooleanValue(allowCrossOriginArguments);
        if (!JS_DefineProperty(cx, obj, "allowCrossOriginArguments", val, attrs))
            return nullptr;

        return obj;
    }

    virtual bool Parse() {
        return ParseBoolean("allowCrossOriginArguments", &allowCrossOriginArguments);
    }

    bool allowCrossOriginArguments;
};

class MOZ_STACK_CLASS StackScopedCloneOptions : public OptionsBase {
public:
    explicit StackScopedCloneOptions(JSContext* cx = xpc_GetSafeJSContext(),
                                     JSObject* options = nullptr)
        : OptionsBase(cx, options)
        , wrapReflectors(false)
        , cloneFunctions(false)
    { }

    virtual bool Parse() {
        return ParseBoolean("wrapReflectors", &wrapReflectors) &&
               ParseBoolean("cloneFunctions", &cloneFunctions);
    }

    
    bool wrapReflectors;

    
    
    bool cloneFunctions;
};

JSObject*
CreateGlobalObject(JSContext* cx, const JSClass* clasp, nsIPrincipal* principal,
                   JS::CompartmentOptions& aOptions);



bool
InitGlobalObject(JSContext* aJSContext, JS::Handle<JSObject*> aGlobal,
                 uint32_t aFlags);










nsresult
CreateSandboxObject(JSContext* cx, JS::MutableHandleValue vp, nsISupports* prinOrSop,
                    xpc::SandboxOptions& options);






nsresult
EvalInSandbox(JSContext* cx, JS::HandleObject sandbox, const nsAString& source,
              const nsACString& filename, int32_t lineNo,
              JSVersion jsVersion, JS::MutableHandleValue rval);

nsresult
GetSandboxAddonId(JSContext* cx, JS::HandleObject sandboxArg,
                  JS::MutableHandleValue rval);



nsresult
GetSandboxMetadata(JSContext* cx, JS::HandleObject sandboxArg,
                   JS::MutableHandleValue rval);

nsresult
SetSandboxMetadata(JSContext* cx, JS::HandleObject sandboxArg,
                   JS::HandleValue metadata);

bool
CreateObjectIn(JSContext* cx, JS::HandleValue vobj, CreateObjectInOptions& options,
               JS::MutableHandleValue rval);

bool
EvalInWindow(JSContext* cx, const nsAString& source, JS::HandleObject scope,
             JS::MutableHandleValue rval);

bool
ExportFunction(JSContext* cx, JS::HandleValue vscope, JS::HandleValue vfunction,
               JS::HandleValue voptions, JS::MutableHandleValue rval);

bool
CloneInto(JSContext* cx, JS::HandleValue vobj, JS::HandleValue vscope,
          JS::HandleValue voptions, JS::MutableHandleValue rval);

bool
StackScopedClone(JSContext* cx, StackScopedCloneOptions& options, JS::MutableHandleValue val);

} 





inline bool
xpc_ForcePropertyResolve(JSContext* cx, JS::HandleObject obj, jsid id);

inline jsid
GetRTIdByIndex(JSContext* cx, unsigned index);

namespace xpc {

enum WrapperDenialType {
    WrapperDenialForXray = 0,
    WrapperDenialForCOW,
    WrapperDenialTypeCount
};
bool ReportWrapperDenial(JSContext* cx, JS::HandleId id, WrapperDenialType type, const char* reason);

class CompartmentPrivate
{
public:
    enum LocationHint {
        LocationHintRegular,
        LocationHintAddon
    };

    explicit CompartmentPrivate(JSCompartment* c)
        : wantXrays(false)
        , writeToGlobalPrototype(false)
        , skipWriteToGlobalPrototype(false)
        , universalXPConnectEnabled(false)
        , forcePermissiveCOWs(false)
        , skipCOWCallableChecks(false)
        , scriptability(c)
        , scope(nullptr)
    {
        MOZ_COUNT_CTOR(xpc::CompartmentPrivate);
        mozilla::PodArrayZero(wrapperDenialWarnings);
    }

    ~CompartmentPrivate();

    static CompartmentPrivate* Get(JSCompartment* compartment)
    {
        MOZ_ASSERT(compartment);
        void* priv = JS_GetCompartmentPrivate(compartment);
        return static_cast<CompartmentPrivate*>(priv);
    }

    static CompartmentPrivate* Get(JSObject* object)
    {
        JSCompartment* compartment = js::GetObjectCompartment(object);
        return Get(compartment);
    }


    bool wantXrays;

    
    
    
    
    bool writeToGlobalPrototype;

    
    
    
    bool skipWriteToGlobalPrototype;

    
    
    
    
    
    
    bool universalXPConnectEnabled;

    
    
    
    
    
    
    bool forcePermissiveCOWs;

    
    
    bool skipCOWCallableChecks;

    
    
    bool wrapperDenialWarnings[WrapperDenialTypeCount];

    
    Scriptability scriptability;

    
    
    XPCWrappedNativeScope* scope;

    const nsACString& GetLocation() {
        if (location.IsEmpty() && locationURI) {
            if (NS_FAILED(locationURI->GetSpec(location)))
                location = NS_LITERAL_CSTRING("<unknown location>");
        }
        return location;
    }
    bool GetLocationURI(nsIURI** aURI) {
        return GetLocationURI(LocationHintRegular, aURI);
    }
    bool GetLocationURI(LocationHint aLocationHint, nsIURI** aURI) {
        if (locationURI) {
            nsCOMPtr<nsIURI> rval = locationURI;
            rval.forget(aURI);
            return true;
        }
        return TryParseLocationURI(aLocationHint, aURI);
    }
    void SetLocation(const nsACString& aLocation) {
        if (aLocation.IsEmpty())
            return;
        if (!location.IsEmpty() || locationURI)
            return;
        location = aLocation;
    }
    void SetLocationURI(nsIURI* aLocationURI) {
        if (!aLocationURI)
            return;
        if (locationURI)
            return;
        locationURI = aLocationURI;
    }

private:
    nsCString location;
    nsCOMPtr<nsIURI> locationURI;

    bool TryParseLocationURI(LocationHint aType, nsIURI** aURI);
};

bool IsUniversalXPConnectEnabled(JSCompartment* compartment);
bool IsUniversalXPConnectEnabled(JSContext* cx);
bool EnableUniversalXPConnect(JSContext* cx);

inline void
CrashIfNotInAutomation()
{
    const char* prefName =
      "security.turn_off_all_security_so_that_viruses_can_take_over_this_computer";
    MOZ_RELEASE_ASSERT(mozilla::Preferences::GetBool(prefName));
}

inline XPCWrappedNativeScope*
ObjectScope(JSObject* obj)
{
    return CompartmentPrivate::Get(obj)->scope;
}

JSObject* NewOutObject(JSContext* cx);
bool IsOutObject(JSContext* cx, JSObject* obj);

nsresult HasInstance(JSContext* cx, JS::HandleObject objArg, const nsID* iid, bool* bp);

nsIPrincipal* GetObjectPrincipal(JSObject* obj);

} 

namespace mozilla {
namespace dom {
extern bool
DefineStaticJSVals(JSContext* cx);
} 
} 

bool
xpc_LocalizeRuntime(JSRuntime* rt);
void
xpc_DelocalizeRuntime(JSRuntime* rt);




#include "XPCInlines.h"




#include "XPCMaps.h"



#endif
