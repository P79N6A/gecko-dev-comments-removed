







#include "xpcprivate.h"
#include "nsAtomicRefcnt.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsTextFormatter.h"



NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::TraverseImpl
   (NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS) *that, void *p,
    nsCycleCollectionTraversalCallback &cb)
{
    nsISupports *s = static_cast<nsISupports*>(p);
    NS_ASSERTION(CheckForRightISupports(s),
                 "not the nsISupports pointer we expect");
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

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXPCWrappedJS)
    tmp->Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMETHODIMP
nsXPCWrappedJS::AggregatedQueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    NS_ASSERTION(IsAggregatedToNative(), "bad AggregatedQueryInterface call");

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
    nsrefcnt cnt = NS_AtomicIncrementRefcnt(mRefCnt);
    NS_LOG_ADDREF(this, cnt, "nsXPCWrappedJS", sizeof(*this));

    if (2 == cnt && IsValid()) {
        XPCJSRuntime* rt = mClass->GetRuntime();
        rt->AddWrappedJSRoot(this);
    }

    return cnt;
}

nsrefcnt
nsXPCWrappedJS::Release(void)
{
    NS_PRECONDITION(0 != mRefCnt, "dup release");

    if (mMainThreadOnly && !NS_IsMainThread()) {
        
        
        nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
        
        
        NS_ASSERTION(mainThread,
                     "Can't get main thread, leaking nsXPCWrappedJS!");
        if (mainThread) {
            NS_ProxyRelease(mainThread,
                            static_cast<nsIXPConnectWrappedJS*>(this));
        }
        return mRefCnt;
    }

    
    
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    XPCAutoLock lock(rt->GetMapLock());

do_decrement:

    nsrefcnt cnt = NS_AtomicDecrementRefcnt(mRefCnt);
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
    NS_ASSERTION(mRefCnt >= 2 && IsValid(), "must be strongly referenced");
    JS_SET_TRACING_DETAILS(trc, GetTraceName, this, 0);
    JS_CallObjectTracer(trc, GetJSObjectPreserveColor(), "nsXPCWrappedJS::mJSObj");
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

NS_IMETHODIMP
nsXPCWrappedJS::GetJSObject(JSObject** aJSObj)
{
    NS_PRECONDITION(aJSObj, "bad param");
    NS_PRECONDITION(IsValid(), "bad wrapper");

    if (!(*aJSObj = GetJSObject()))
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
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
nsXPCWrappedJS::GetNewOrUsed(JSContext* cx,
                             JSObject* aJSObj,
                             REFNSIID aIID,
                             nsISupports* aOuter,
                             nsXPCWrappedJS** wrapperResult)
{
    JSObject2WrappedJSMap* map;
    JSObject* rootJSObj;
    nsXPCWrappedJS* root = nullptr;
    nsXPCWrappedJS* wrapper = nullptr;
    nsXPCWrappedJSClass* clazz = nullptr;
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    JSBool release_root = false;

    map = rt->GetWrappedJSMap();
    if (!map) {
        NS_ASSERTION(map,"bad map");
        return NS_ERROR_FAILURE;
    }

    nsXPCWrappedJSClass::GetNewOrUsed(cx, aIID, &clazz);
    if (!clazz)
        return NS_ERROR_FAILURE;
    

    
    rootJSObj = clazz->GetRootJSObject(cx, aJSObj);
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
        
        if (rootJSObj == aJSObj) {
            
            wrapper = root = new nsXPCWrappedJS(cx, aJSObj, clazz, nullptr,
                                                aOuter);
            if (!root)
                goto return_wrapper;

            {   
#if DEBUG_xpc_leaks
                printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
                       (void*)wrapper, (void*)aJSObj);
#endif
                XPCAutoLock lock(rt->GetMapLock());
                map->Add(root);
            }

            if (!CheckMainThreadOnly(root)) {
                XPCAutoLock lock(rt->GetMapLock());
                map->Remove(root);

                wrapper = NULL;
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
                map->Add(root);
            }

            if (!CheckMainThreadOnly(root)) {
                XPCAutoLock lock(rt->GetMapLock());
                map->Remove(root);

                goto return_wrapper;
            }
        }
    }

    
    NS_ASSERTION(root,"bad root");
    NS_ASSERTION(clazz,"bad clazz");

    if (!wrapper) {
        wrapper = new nsXPCWrappedJS(cx, aJSObj, clazz, root, aOuter);
        if (!wrapper)
            goto return_wrapper;
#if DEBUG_xpc_leaks
        printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
               (void*)wrapper, (void*)aJSObj);
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
            NS_ASSERTION(cur, "failed to find wrapper in its own chain");
        }
        
        NS_RELEASE(mRoot);
    }

    NS_IF_RELEASE(mClass);
    if (mOuter) {
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        if (rt->GetThreadRunningGC()) {
            rt->DeferredRelease(mOuter);
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
    NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsISupports)), "bad call sequence");

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
    NS_ASSERTION(GetClass(), "wrapper without class");
    NS_ASSERTION(GetClass()->GetInterfaceInfo(), "wrapper class without interface");

    
    

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
nsXPCWrappedJS::SystemIsBeingShutDown(JSRuntime* rt)
{
    
    
    

    
    
    

    
    
    mJSObj = nullptr;

    
    if (mNext)
        mNext->SystemIsBeingShutDown(rt);
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
                        isRoot ? "ROOT":"non-root", mJSObj));
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
