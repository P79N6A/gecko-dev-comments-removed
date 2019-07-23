









































#include "xpcprivate.h"

#if defined(DEBUG_xpc_hacker) || defined(DEBUG)
PRInt32 XPCWrappedNativeProto::gDEBUG_LiveProtoCount = 0;
#endif

XPCWrappedNativeProto::XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                                             nsIClassInfo* ClassInfo,
                                             PRUint32 ClassInfoFlags,
                                             XPCNativeSet* Set)
    : mScope(Scope),
      mJSProtoObject(nsnull),
      mClassInfo(ClassInfo),
      mClassInfoFlags(ClassInfoFlags),
      mSet(Set),
      mSecurityInfo(nsnull),
      mScriptableInfo(nsnull)
{
    
    

    MOZ_COUNT_CTOR(XPCWrappedNativeProto);

#ifdef DEBUG
    PR_AtomicIncrement(&gDEBUG_LiveProtoCount);
#endif
}

XPCWrappedNativeProto::~XPCWrappedNativeProto()
{
    NS_ASSERTION(!mJSProtoObject, "JSProtoObject still alive");

    MOZ_COUNT_DTOR(XPCWrappedNativeProto);

#ifdef DEBUG
    PR_AtomicDecrement(&gDEBUG_LiveProtoCount);
#endif
    
    

    XPCNativeSet::ClearCacheEntryForClassInfo(mClassInfo);

    delete mScriptableInfo;
}

JSBool
XPCWrappedNativeProto::Init(
                XPCCallContext& ccx,
                JSBool isGlobal,
                const XPCNativeScriptableCreateInfo* scriptableCreateInfo)
{
    if(scriptableCreateInfo && scriptableCreateInfo->GetCallback())
    {
        mScriptableInfo =
            XPCNativeScriptableInfo::Construct(ccx, isGlobal, scriptableCreateInfo);
        if(!mScriptableInfo)
            return JS_FALSE;
    }

    JSClass* jsclazz = mScriptableInfo &&
                       mScriptableInfo->GetFlags().AllowPropModsToPrototype() ?
                            &XPC_WN_ModsAllowed_Proto_JSClass :
                            &XPC_WN_NoMods_Proto_JSClass;

    JSObject *parent = mScope->GetGlobalJSObject();

    mJSProtoObject = JS_NewObject(ccx, jsclazz,
                                  mScope->GetPrototypeJSObject(),
                                  parent);

    JSBool ok = mJSProtoObject && JS_SetPrivate(ccx, mJSProtoObject, this);

    
    if(ok && JS_IsSystemObject(ccx, parent))
        JS_FlagSystemObject(ccx, mJSProtoObject);

    DEBUG_ReportShadowedMembers(mSet, nsnull, this);

    return ok;
}

void
XPCWrappedNativeProto::JSProtoObjectFinalized(JSContext *cx, JSObject *obj)
{
    NS_ASSERTION(obj == mJSProtoObject, "huh?");

    

    if(IsShared())
    {
        
        ClassInfo2WrappedNativeProtoMap* map = 
            GetScope()->GetWrappedNativeProtoMap();
        if(map->Find(mClassInfo) == this)
            map->Remove(mClassInfo);
    }

    GetRuntime()->GetDetachedWrappedNativeProtoMap()->Remove(this);
    GetRuntime()->GetDyingWrappedNativeProtoMap()->Add(this);

    mJSProtoObject = nsnull;
}

void
XPCWrappedNativeProto::SystemIsBeingShutDown(JSContext* cx)
{
    
    

#ifdef XPC_TRACK_PROTO_STATS
    static PRBool DEBUG_DumpedStats = PR_FALSE;
    if(!DEBUG_DumpedStats)
    {
        printf("%d XPCWrappedNativeProto(s) alive at shutdown\n",
               gDEBUG_LiveProtoCount);
        DEBUG_DumpedStats = PR_TRUE;
    }
#endif

    if(mJSProtoObject)
    {
        
        JS_SetPrivate(cx, mJSProtoObject, nsnull);
        mJSProtoObject = nsnull;
    }
}


XPCWrappedNativeProto*
XPCWrappedNativeProto::GetNewOrUsed(XPCCallContext& ccx,
                                    XPCWrappedNativeScope* Scope,
                                    nsIClassInfo* ClassInfo,
                                    const XPCNativeScriptableCreateInfo* ScriptableCreateInfo,
                                    JSBool ForceNoSharing,
                                    JSBool isGlobal)
{
    NS_ASSERTION(Scope, "bad param");
    NS_ASSERTION(ClassInfo, "bad param");

    AutoMarkingWrappedNativeProtoPtr proto(ccx);
    ClassInfo2WrappedNativeProtoMap* map;
    XPCLock* lock;
    JSBool shared;

    JSUint32 ciFlags;
    if(NS_FAILED(ClassInfo->GetFlags(&ciFlags)))
        ciFlags = 0;

    if(ciFlags & XPC_PROTO_DONT_SHARE)
    {
        NS_ERROR("reserved flag set!");
        ciFlags &= ~XPC_PROTO_DONT_SHARE;
    }

    if(ForceNoSharing || (ciFlags & nsIClassInfo::PLUGIN_OBJECT) ||
       (ScriptableCreateInfo &&
        ScriptableCreateInfo->GetFlags().DontSharePrototype()))
    {
        ciFlags |= XPC_PROTO_DONT_SHARE;
        shared = JS_FALSE;
    }
    else
    {
        shared = JS_TRUE;
    }

    if(shared)
    {
        map = Scope->GetWrappedNativeProtoMap();
        lock = Scope->GetRuntime()->GetMapLock();
        {   
            XPCAutoLock al(lock);
            proto = map->Find(ClassInfo);
            if(proto)
                return proto;
        }
    }

    AutoMarkingNativeSetPtr set(ccx);
    set = XPCNativeSet::GetNewOrUsed(ccx, ClassInfo);
    if(!set)
        return nsnull;

    proto = new XPCWrappedNativeProto(Scope, ClassInfo, ciFlags, set);

    if(!proto || !proto->Init(ccx, isGlobal, ScriptableCreateInfo))
    {
        delete proto.get();
        return nsnull;
    }

    if(shared)
    {   
        XPCAutoLock al(lock);
        map->Add(ClassInfo, proto);
    }

    return proto;
}

void
XPCWrappedNativeProto::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNativeProto @ %x", this));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gDEBUG_LiveProtoCount is %d", gDEBUG_LiveProtoCount));
        XPC_LOG_ALWAYS(("mScope @ %x", mScope));
        XPC_LOG_ALWAYS(("mJSProtoObject @ %x", mJSProtoObject));
        XPC_LOG_ALWAYS(("mSet @ %x", mSet));
        XPC_LOG_ALWAYS(("mSecurityInfo of %x", mSecurityInfo));
        XPC_LOG_ALWAYS(("mScriptableInfo @ %x", mScriptableInfo));
        if(depth && mScriptableInfo)
        {
            XPC_LOG_INDENT();
            XPC_LOG_ALWAYS(("mScriptable @ %x", mScriptableInfo->GetCallback()));
            XPC_LOG_ALWAYS(("mFlags of %x", (PRUint32)mScriptableInfo->GetFlags()));
            XPC_LOG_ALWAYS(("mJSClass @ %x", mScriptableInfo->GetJSClass()));
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
}


