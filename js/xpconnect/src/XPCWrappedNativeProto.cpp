







#include "xpcprivate.h"
#include "nsCxPusher.h"
#include "pratom.h"

using namespace mozilla;

#if defined(DEBUG_xpc_hacker) || defined(DEBUG)
int32_t XPCWrappedNativeProto::gDEBUG_LiveProtoCount = 0;
#endif

XPCWrappedNativeProto::XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                                             nsIClassInfo* ClassInfo,
                                             uint32_t ClassInfoFlags,
                                             XPCNativeSet* Set)
    : mScope(Scope),
      mJSProtoObject(nullptr),
      mClassInfo(ClassInfo),
      mClassInfoFlags(ClassInfoFlags),
      mSet(Set),
      mSecurityInfo(nullptr),
      mScriptableInfo(nullptr)
{
    
    

    MOZ_COUNT_CTOR(XPCWrappedNativeProto);
    MOZ_ASSERT(mScope);

#ifdef DEBUG
    PR_ATOMIC_INCREMENT(&gDEBUG_LiveProtoCount);
#endif
}

XPCWrappedNativeProto::~XPCWrappedNativeProto()
{
    NS_ASSERTION(!mJSProtoObject, "JSProtoObject still alive");

    MOZ_COUNT_DTOR(XPCWrappedNativeProto);

#ifdef DEBUG
    PR_ATOMIC_DECREMENT(&gDEBUG_LiveProtoCount);
#endif

    

    XPCNativeSet::ClearCacheEntryForClassInfo(mClassInfo);

    delete mScriptableInfo;
}

bool
XPCWrappedNativeProto::Init(const XPCNativeScriptableCreateInfo* scriptableCreateInfo,
                            bool callPostCreatePrototype)
{
    AutoJSContext cx;
    nsIXPCScriptable *callback = scriptableCreateInfo ?
                                 scriptableCreateInfo->GetCallback() :
                                 nullptr;
    if (callback) {
        mScriptableInfo =
            XPCNativeScriptableInfo::Construct(scriptableCreateInfo);
        if (!mScriptableInfo)
            return false;
    }

    js::Class* jsclazz;

    if (mScriptableInfo) {
        const XPCNativeScriptableFlags& flags(mScriptableInfo->GetFlags());

        if (flags.AllowPropModsToPrototype()) {
            jsclazz = flags.WantCall() ?
                &XPC_WN_ModsAllowed_WithCall_Proto_JSClass :
                &XPC_WN_ModsAllowed_NoCall_Proto_JSClass;
        } else {
            jsclazz = flags.WantCall() ?
                &XPC_WN_NoMods_WithCall_Proto_JSClass :
                &XPC_WN_NoMods_NoCall_Proto_JSClass;
        }
    } else {
        jsclazz = &XPC_WN_NoMods_NoCall_Proto_JSClass;
    }

    JS::RootedObject parent(cx, mScope->GetGlobalJSObject());
    JS::RootedObject proto(cx, JS_GetObjectPrototype(cx, parent));
    mJSProtoObject = JS_NewObjectWithUniqueType(cx, js::Jsvalify(jsclazz),
                                                proto, parent);

    bool success = !!mJSProtoObject;
    if (success) {
        JS_SetPrivate(mJSProtoObject, this);
        if (callPostCreatePrototype)
            success = CallPostCreatePrototype();
    }

    DEBUG_ReportShadowedMembers(mSet, nullptr, this);

    return success;
}

bool
XPCWrappedNativeProto::CallPostCreatePrototype()
{
    AutoJSContext cx;

    
    nsIXPCScriptable *callback = mScriptableInfo ? mScriptableInfo->GetCallback()
                                                 : nullptr;
    if (!callback)
        return true;

    
    
    nsresult rv = callback->PostCreatePrototype(cx, mJSProtoObject);
    if (NS_FAILED(rv)) {
        JS_SetPrivate(mJSProtoObject, nullptr);
        mJSProtoObject = nullptr;
        XPCThrower::Throw(rv, cx);
        return false;
    }

    return true;
}

void
XPCWrappedNativeProto::JSProtoObjectFinalized(js::FreeOp *fop, JSObject *obj)
{
    NS_ASSERTION(obj == mJSProtoObject, "huh?");

    

    
    ClassInfo2WrappedNativeProtoMap* map =
        GetScope()->GetWrappedNativeProtoMap(ClassIsMainThreadOnly());
    if (map->Find(mClassInfo) == this)
        map->Remove(mClassInfo);

    GetRuntime()->GetDetachedWrappedNativeProtoMap()->Remove(this);
    GetRuntime()->GetDyingWrappedNativeProtoMap()->Add(this);

    mJSProtoObject.finalize(js::CastToJSFreeOp(fop)->runtime());
}

void
XPCWrappedNativeProto::SystemIsBeingShutDown()
{
    
    

#ifdef XPC_TRACK_PROTO_STATS
    static bool DEBUG_DumpedStats = false;
    if (!DEBUG_DumpedStats) {
        printf("%d XPCWrappedNativeProto(s) alive at shutdown\n",
               gDEBUG_LiveProtoCount);
        DEBUG_DumpedStats = true;
    }
#endif

    if (mJSProtoObject) {
        
        JS_SetPrivate(mJSProtoObject, nullptr);
        mJSProtoObject = nullptr;
    }
}


XPCWrappedNativeProto*
XPCWrappedNativeProto::GetNewOrUsed(XPCWrappedNativeScope* scope,
                                    nsIClassInfo* classInfo,
                                    const XPCNativeScriptableCreateInfo* scriptableCreateInfo,
                                    bool callPostCreatePrototype)
{
    AutoJSContext cx;
    NS_ASSERTION(scope, "bad param");
    NS_ASSERTION(classInfo, "bad param");

    AutoMarkingWrappedNativeProtoPtr proto(cx);
    ClassInfo2WrappedNativeProtoMap* map = nullptr;
    XPCLock* lock = nullptr;

    uint32_t ciFlags;
    if (NS_FAILED(classInfo->GetFlags(&ciFlags)))
        ciFlags = 0;

    bool mainThreadOnly = !!(ciFlags & nsIClassInfo::MAIN_THREAD_ONLY);
    map = scope->GetWrappedNativeProtoMap(mainThreadOnly);
    lock = mainThreadOnly ? nullptr : scope->GetRuntime()->GetMapLock();
    {   
        XPCAutoLock al(lock);
        proto = map->Find(classInfo);
        if (proto)
            return proto;
    }

    AutoMarkingNativeSetPtr set(cx);
    set = XPCNativeSet::GetNewOrUsed(classInfo);
    if (!set)
        return nullptr;

    proto = new XPCWrappedNativeProto(scope, classInfo, ciFlags, set);

    if (!proto || !proto->Init(scriptableCreateInfo, callPostCreatePrototype)) {
        delete proto.get();
        return nullptr;
    }

    {   
        XPCAutoLock al(lock);
        map->Add(classInfo, proto);
    }

    return proto;
}

void
XPCWrappedNativeProto::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNativeProto @ %x", this));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gDEBUG_LiveProtoCount is %d", gDEBUG_LiveProtoCount));
        XPC_LOG_ALWAYS(("mScope @ %x", mScope));
        XPC_LOG_ALWAYS(("mJSProtoObject @ %x", mJSProtoObject.get()));
        XPC_LOG_ALWAYS(("mSet @ %x", mSet));
        XPC_LOG_ALWAYS(("mSecurityInfo of %x", mSecurityInfo));
        XPC_LOG_ALWAYS(("mScriptableInfo @ %x", mScriptableInfo));
        if (depth && mScriptableInfo) {
            XPC_LOG_INDENT();
            XPC_LOG_ALWAYS(("mScriptable @ %x", mScriptableInfo->GetCallback()));
            XPC_LOG_ALWAYS(("mFlags of %x", (uint32_t)mScriptableInfo->GetFlags()));
            XPC_LOG_ALWAYS(("mJSClass @ %x", mScriptableInfo->GetJSClass()));
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
}


