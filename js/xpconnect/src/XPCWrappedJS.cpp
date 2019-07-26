







#include "xpcprivate.h"
#include "jsprf.h"
#include "nsCxPusher.h"
#include "nsContentUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsTextFormatter.h"

using namespace mozilla;



NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Traverse
   (void *p, nsCycleCollectionTraversalCallback &cb)
{
    nsISupports *s = static_cast<nsISupports*>(p);
    MOZ_ASSERT(CheckForRightISupports(s), "not the nsISupports pointer we expect");
    nsXPCWrappedJS *tmp = Downcast(s);

    nsrefcnt refcnt = tmp->mRefCnt.get();
    if (cb.WantDebugInfo()) {
        char name[72];
        if (tmp->GetClass())
            JS_snprintf(name, sizeof(name), "nsXPCWrappedJS (%s)",
                        tmp->GetClass()->GetInterfaceName());
        else
            JS_snprintf(name, sizeof(name), "nsXPCWrappedJS");
        cb.DescribeRefCountedNode(refcnt, name);
    } else {
        NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsXPCWrappedJS, refcnt)
    }

    
    
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "self");
    cb.NoteXPCOMChild(s);

    if (refcnt > 1) {
        
        
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mJSObj");
        cb.NoteJSChild(tmp->GetJSObjectPreserveColor());
    }

    nsXPCWrappedJS* root = tmp->GetRootWrapper();
    if (root == tmp) {
        
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "aggregated native");
        cb.NoteXPCOMChild(tmp->GetAggregatedNativeObject());
    } else {
        
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "root");
        cb.NoteXPCOMChild(static_cast<nsIXPConnectWrappedJS*>(root));
    }

    return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXPCWrappedJS)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXPCWrappedJS)
    tmp->Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMETHODIMP
nsXPCWrappedJS::AggregatedQueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    MOZ_ASSERT(IsAggregatedToNative(), "bad AggregatedQueryInterface call");

    if (!IsValid())
        return NS_ERROR_UNEXPECTED;

    
    
    
    
    if (aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS))) {
        NS_ADDREF(this);
        *aInstancePtr = (void*) static_cast<nsIXPConnectWrappedJS*>(this);
        return NS_OK;
    }

    return mClass->DelegatedQueryInterface(this, aIID, aInstancePtr);
}

NS_IMETHODIMP
nsXPCWrappedJS::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (nullptr == aInstancePtr) {
        NS_PRECONDITION(0, "null pointer");
        return NS_ERROR_NULL_POINTER;
    }

    if ( aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant)) ) {
        *aInstancePtr = NS_CYCLE_COLLECTION_PARTICIPANT(nsXPCWrappedJS);
        return NS_OK;
    }

    if (aIID.Equals(NS_GET_IID(nsCycleCollectionISupports))) {
        *aInstancePtr =
            NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Upcast(this);
        return NS_OK;
    }

    if (!IsValid())
        return NS_ERROR_UNEXPECTED;

    
    
    if (aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS))) {
        NS_ADDREF(this);
        *aInstancePtr = (void*) static_cast<nsIXPConnectWrappedJS*>(this);
        return NS_OK;
    }

    nsISupports* outer = GetAggregatedNativeObject();
    if (outer)
        return outer->QueryInterface(aIID, aInstancePtr);

    

    return mClass->DelegatedQueryInterface(this, aIID, aInstancePtr);
}





















nsrefcnt
nsXPCWrappedJS::AddRef(void)
{
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();
    nsrefcnt cnt = ++mRefCnt;
    NS_LOG_ADDREF(this, cnt, "nsXPCWrappedJS", sizeof(*this));

    if (2 == cnt && IsValid()) {
        GetJSObject(); 
        XPCJSRuntime* rt = mClass->GetRuntime();
        rt->AddWrappedJSRoot(this);
    }

    return cnt;
}

nsrefcnt
nsXPCWrappedJS::Release(void)
{
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();
    NS_PRECONDITION(0 != mRefCnt, "dup release");

    if (mMainThreadOnly && !NS_IsMainThread()) {
        
        
        nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
        
        
        MOZ_ASSERT(mainThread, "Can't get main thread, leaking nsXPCWrappedJS!");
        if (mainThread) {
            NS_ProxyRelease(mainThread,
                            static_cast<nsIXPConnectWrappedJS*>(this));
        }
        return mRefCnt;
    }

    
    
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    XPCAutoLock lock(rt->GetMapLock());

do_decrement:

    nsrefcnt cnt = --mRefCnt;
    NS_LOG_RELEASE(this, cnt, "nsXPCWrappedJS");

    if (0 == cnt) {
        delete this;   
        return 0;
    }
    if (1 == cnt) {
        if (IsValid())
            RemoveFromRootSet(rt->GetMapLock());

        
        
        
        
        if (!HasWeakReferences())
            goto do_decrement;
    }
    return cnt;
}

void
nsXPCWrappedJS::TraceJS(JSTracer* trc)
{
    MOZ_ASSERT(mRefCnt >= 2 && IsValid(), "must be strongly referenced");
    JS_SET_TRACING_DETAILS(trc, GetTraceName, this, 0);
    JS_CallHeapObjectTracer(trc, &mJSObj, "nsXPCWrappedJS::mJSObj");
}


void
nsXPCWrappedJS::GetTraceName(JSTracer* trc, char *buf, size_t bufsize)
{
    const nsXPCWrappedJS* self = static_cast<const nsXPCWrappedJS*>
                                            (trc->debugPrintArg);
    JS_snprintf(buf, bufsize, "nsXPCWrappedJS[%s,0x%p:0x%p].mJSObj",
                self->GetClass()->GetInterfaceName(), self, self->mXPTCStub);
}

NS_IMETHODIMP
nsXPCWrappedJS::GetWeakReference(nsIWeakReference** aInstancePtr)
{
    if (mRoot != this)
        return mRoot->GetWeakReference(aInstancePtr);

    return nsSupportsWeakReference::GetWeakReference(aInstancePtr);
}

JSObject*
nsXPCWrappedJS::GetJSObject()
{
    if (mJSObj) {
        JS::ExposeObjectToActiveJS(mJSObj);
    }
    return mJSObj;
}

static bool
CheckMainThreadOnly(nsXPCWrappedJS *aWrapper)
{
    if(aWrapper->IsMainThreadOnly())
        return NS_IsMainThread();

    nsCOMPtr<nsIClassInfo> ci;
    CallQueryInterface(aWrapper, getter_AddRefs(ci));
    if (ci) {
        uint32_t flags;
        if (NS_SUCCEEDED(ci->GetFlags(&flags)) && !(flags & nsIClassInfo::MAIN_THREAD_ONLY))
            return true;

        if (!NS_IsMainThread())
            return false;
    }

    aWrapper->SetIsMainThreadOnly();

    return true;
}


nsresult
nsXPCWrappedJS::GetNewOrUsed(JS::HandleObject jsObj,
                             REFNSIID aIID,
                             nsISupports* aOuter,
                             nsXPCWrappedJS** wrapperResult)
{
    
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();

    AutoJSContext cx;
    JSObject2WrappedJSMap* map;
    nsXPCWrappedJS* root = nullptr;
    nsXPCWrappedJS* wrapper = nullptr;
    nsXPCWrappedJSClass* clazz = nullptr;
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    bool release_root = false;

    map = rt->GetWrappedJSMap();
    if (!map) {
        MOZ_ASSERT(map,"bad map");
        return NS_ERROR_FAILURE;
    }

    nsXPCWrappedJSClass::GetNewOrUsed(cx, aIID, &clazz);
    if (!clazz)
        return NS_ERROR_FAILURE;
    

    
    JS::RootedObject rootJSObj(cx, clazz->GetRootJSObject(cx, jsObj));
    if (!rootJSObj)
        goto return_wrapper;

    
    
    
    {   
        XPCAutoLock lock(rt->GetMapLock());
        root = map->Find(rootJSObj);
        if (root) {
            if ((nullptr != (wrapper = root->Find(aIID))) ||
                (nullptr != (wrapper = root->FindInherited(aIID)))) {
                NS_ADDREF(wrapper);
                goto return_wrapper;
            }
        }
    }

    if (!root) {
        
        if (rootJSObj == jsObj) {
            
            wrapper = root = new nsXPCWrappedJS(cx, jsObj, clazz, nullptr,
                                                aOuter);
            if (!root)
                goto return_wrapper;

            {   
#if DEBUG_xpc_leaks
                printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
                       (void*)wrapper, (void*)jsObj);
#endif
                XPCAutoLock lock(rt->GetMapLock());
                map->Add(cx, root);
            }

            if (!CheckMainThreadOnly(root)) {
                XPCAutoLock lock(rt->GetMapLock());
                map->Remove(root);

                wrapper = nullptr;
            }

            goto return_wrapper;
        } else {
            
            nsXPCWrappedJSClass* rootClazz = nullptr;
            nsXPCWrappedJSClass::GetNewOrUsed(cx, NS_GET_IID(nsISupports),
                                              &rootClazz);
            if (!rootClazz)
                goto return_wrapper;

            root = new nsXPCWrappedJS(cx, rootJSObj, rootClazz, nullptr, aOuter);
            NS_RELEASE(rootClazz);

            if (!root)
                goto return_wrapper;

            release_root = true;

            {   
#if DEBUG_xpc_leaks
                printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
                       (void*)root, (void*)rootJSObj);
#endif
                XPCAutoLock lock(rt->GetMapLock());
                map->Add(cx, root);
            }

            if (!CheckMainThreadOnly(root)) {
                XPCAutoLock lock(rt->GetMapLock());
                map->Remove(root);

                goto return_wrapper;
            }
        }
    }

    
    MOZ_ASSERT(root,"bad root");
    MOZ_ASSERT(clazz,"bad clazz");

    if (!wrapper) {
        wrapper = new nsXPCWrappedJS(cx, jsObj, clazz, root, aOuter);
        if (!wrapper)
            goto return_wrapper;
#if DEBUG_xpc_leaks
        printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
               (void*)wrapper, (void*)jsObj);
#endif
    }

    wrapper->mNext = root->mNext;
    root->mNext = wrapper;

return_wrapper:
    if (clazz)
        NS_RELEASE(clazz);

    if (release_root)
        NS_RELEASE(root);

    if (!wrapper)
        return NS_ERROR_FAILURE;

    *wrapperResult = wrapper;
    return NS_OK;
}

nsXPCWrappedJS::nsXPCWrappedJS(JSContext* cx,
                               JSObject* aJSObj,
                               nsXPCWrappedJSClass* aClass,
                               nsXPCWrappedJS* root,
                               nsISupports* aOuter)
    : mJSObj(aJSObj),
      mClass(aClass),
      mRoot(root ? root : this),
      mNext(nullptr),
      mOuter(root ? nullptr : aOuter),
      mMainThread(NS_IsMainThread()),
      mMainThreadOnly(root && root->mMainThreadOnly)
{
#ifdef DEBUG_stats_jband
    static int count = 0;
    static const int interval = 10;
    if (0 == (++count % interval))
        printf("//////// %d instances of nsXPCWrappedJS created\n", count);
#endif

    MOZ_ASSERT_IF(mMainThreadOnly, mMainThread);

    InitStub(GetClass()->GetIID());

    
    NS_ADDREF_THIS();
    NS_ADDREF_THIS();
    NS_ADDREF(aClass);
    NS_IF_ADDREF(mOuter);

    if (mRoot != this)
        NS_ADDREF(mRoot);

}

nsXPCWrappedJS::~nsXPCWrappedJS()
{
    NS_PRECONDITION(0 == mRefCnt, "refcounting error");

    if (mRoot == this) {
        
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        JSObject2WrappedJSMap* map = rt->GetWrappedJSMap();
        if (map) {
            XPCAutoLock lock(rt->GetMapLock());
            map->Remove(this);
        }
    }
    Unlink();
}

void
nsXPCWrappedJS::Unlink()
{
    if (IsValid()) {
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        if (rt) {
            if (mRoot == this) {
                
                JSObject2WrappedJSMap* map = rt->GetWrappedJSMap();
                if (map) {
                    XPCAutoLock lock(rt->GetMapLock());
                    map->Remove(this);
                }
            }

            if (mRefCnt > 1)
                RemoveFromRootSet(rt->GetMapLock());
        }

        mJSObj = nullptr;
    }

    if (mRoot == this) {
        ClearWeakReferences();
    } else if (mRoot) {
        
        nsXPCWrappedJS* cur = mRoot;
        while (1) {
            if (cur->mNext == this) {
                cur->mNext = mNext;
                break;
            }
            cur = cur->mNext;
            MOZ_ASSERT(cur, "failed to find wrapper in its own chain");
        }
        
        NS_RELEASE(mRoot);
    }

    NS_IF_RELEASE(mClass);
    if (mOuter) {
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        if (rt->GetThreadRunningGC()) {
            nsContentUtils::DeferredFinalize(mOuter);
            mOuter = nullptr;
        } else {
            NS_RELEASE(mOuter);
        }
    }
}

nsXPCWrappedJS*
nsXPCWrappedJS::Find(REFNSIID aIID)
{
    if (aIID.Equals(NS_GET_IID(nsISupports)))
        return mRoot;

    for (nsXPCWrappedJS* cur = mRoot; cur; cur = cur->mNext) {
        if (aIID.Equals(cur->GetIID()))
            return cur;
    }

    return nullptr;
}


nsXPCWrappedJS*
nsXPCWrappedJS::FindInherited(REFNSIID aIID)
{
    MOZ_ASSERT(!aIID.Equals(NS_GET_IID(nsISupports)), "bad call sequence");

    for (nsXPCWrappedJS* cur = mRoot; cur; cur = cur->mNext) {
        bool found;
        if (NS_SUCCEEDED(cur->GetClass()->GetInterfaceInfo()->
                         HasAncestor(&aIID, &found)) && found)
            return cur;
    }

    return nullptr;
}

NS_IMETHODIMP
nsXPCWrappedJS::GetInterfaceInfo(nsIInterfaceInfo** info)
{
    MOZ_ASSERT(GetClass(), "wrapper without class");
    MOZ_ASSERT(GetClass()->GetInterfaceInfo(), "wrapper class without interface");

    
    

    if (!(*info = GetClass()->GetInterfaceInfo()))
        return NS_ERROR_UNEXPECTED;
    NS_ADDREF(*info);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCWrappedJS::CallMethod(uint16_t methodIndex,
                           const XPTMethodDescriptor* info,
                           nsXPTCMiniVariant* params)
{
    
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();

    if (!IsValid())
        return NS_ERROR_UNEXPECTED;
    if (NS_IsMainThread() != mMainThread) {
        NS_NAMED_LITERAL_STRING(kFmt, "Attempt to use JS function on a different thread calling %s.%s. JS objects may not be shared across threads.");
        PRUnichar* msg =
            nsTextFormatter::smprintf(kFmt.get(),
                                      GetClass()->GetInterfaceName(),
                                      info->name);
        nsCOMPtr<nsIConsoleService> cs =
            do_GetService(NS_CONSOLESERVICE_CONTRACTID);
        if (cs)
            cs->LogStringMessage(msg);
        NS_Free(msg);

        return NS_ERROR_NOT_SAME_THREAD;
    }
    return GetClass()->CallMethod(this, methodIndex, info, params);
}

NS_IMETHODIMP
nsXPCWrappedJS::GetInterfaceIID(nsIID** iid)
{
    NS_PRECONDITION(iid, "bad param");

    *iid = (nsIID*) nsMemory::Clone(&(GetIID()), sizeof(nsIID));
    return *iid ? NS_OK : NS_ERROR_UNEXPECTED;
}

void
nsXPCWrappedJS::SystemIsBeingShutDown()
{
    
    
    

    
    
    

    
    

    
    
    
    *mJSObj.unsafeGet() = nullptr;

    
    if (mNext)
        mNext->SystemIsBeingShutDown();
}




NS_IMETHODIMP
nsXPCWrappedJS::GetEnumerator(nsISimpleEnumerator * *aEnumerate)
{
    XPCCallContext ccx(NATIVE_CALLER);
    if (!ccx.IsValid())
        return NS_ERROR_UNEXPECTED;

    return nsXPCWrappedJSClass::BuildPropertyEnumerator(ccx, GetJSObject(),
                                                        aEnumerate);
}


NS_IMETHODIMP
nsXPCWrappedJS::GetProperty(const nsAString & name, nsIVariant **_retval)
{
    XPCCallContext ccx(NATIVE_CALLER);
    if (!ccx.IsValid())
        return NS_ERROR_UNEXPECTED;

    return nsXPCWrappedJSClass::
        GetNamedPropertyAsVariant(ccx, GetJSObject(), name, _retval);
}



NS_IMETHODIMP
nsXPCWrappedJS::DebugDump(int16_t depth)
{
#ifdef DEBUG
    XPC_LOG_ALWAYS(("nsXPCWrappedJS @ %x with mRefCnt = %d", this, mRefCnt.get()));
        XPC_LOG_INDENT();

        bool isRoot = mRoot == this;
        XPC_LOG_ALWAYS(("%s wrapper around JSObject @ %x", \
                        isRoot ? "ROOT":"non-root", mJSObj.get()));
        char* name;
        GetClass()->GetInterfaceInfo()->GetName(&name);
        XPC_LOG_ALWAYS(("interface name is %s", name));
        if (name)
            nsMemory::Free(name);
        char * iid = GetClass()->GetIID().ToString();
        XPC_LOG_ALWAYS(("IID number is %s", iid ? iid : "invalid"));
        if (iid)
            NS_Free(iid);
        XPC_LOG_ALWAYS(("nsXPCWrappedJSClass @ %x", mClass));

        if (!isRoot)
            XPC_LOG_OUTDENT();
        if (mNext) {
            if (isRoot) {
                XPC_LOG_ALWAYS(("Additional wrappers for this object..."));
                XPC_LOG_INDENT();
            }
            mNext->DebugDump(depth);
            if (isRoot)
                XPC_LOG_OUTDENT();
        }
        if (isRoot)
            XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}
