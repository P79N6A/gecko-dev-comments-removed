







#include "xpcprivate.h"
#include "jsprf.h"
#include "nsCxPusher.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"

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

    if (tmp->IsRootWrapper()) {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "aggregated native");
        cb.NoteXPCOMChild(tmp->GetAggregatedNativeObject());
    } else {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "root");
        cb.NoteXPCOMChild(static_cast<nsIXPConnectWrappedJS*>(tmp->GetRootWrapper()));
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

do_decrement:

    nsrefcnt cnt = --mRefCnt;
    NS_LOG_RELEASE(this, cnt, "nsXPCWrappedJS");

    if (0 == cnt) {
        delete this;   
        return 0;
    }
    if (1 == cnt) {
        if (IsValid())
            RemoveFromRootSet();

        
        
        
        
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
    if (!IsRootWrapper())
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

    root = map->Find(rootJSObj);
    if (root) {
        if ((nullptr != (wrapper = root->Find(aIID))) ||
            (nullptr != (wrapper = root->FindInherited(aIID)))) {
            NS_ADDREF(wrapper);
            goto return_wrapper;
        }
    }

    if (!root) {
        
        if (rootJSObj == jsObj) {
            
            wrapper = root = new nsXPCWrappedJS(cx, jsObj, clazz, nullptr,
                                                aOuter);
            if (!root)
                goto return_wrapper;

            map->Add(cx, root);

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

            map->Add(cx, root);
        }
    }

    
    MOZ_ASSERT(root,"bad root");
    MOZ_ASSERT(clazz,"bad clazz");

    if (!wrapper) {
        wrapper = new nsXPCWrappedJS(cx, jsObj, clazz, root, aOuter);
        if (!wrapper)
            goto return_wrapper;
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
      mRoot(root ? root : MOZ_THIS_IN_INITIALIZER_LIST()),
      mNext(nullptr),
      mOuter(root ? nullptr : aOuter)
{
    InitStub(GetClass()->GetIID());

    
    NS_ADDREF_THIS();
    NS_ADDREF_THIS();
    NS_ADDREF(aClass);
    NS_IF_ADDREF(mOuter);

    if (!IsRootWrapper())
        NS_ADDREF(mRoot);

}

nsXPCWrappedJS::~nsXPCWrappedJS()
{
    NS_PRECONDITION(0 == mRefCnt, "refcounting error");

    if (IsRootWrapper()) {
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        JSObject2WrappedJSMap* map = rt->GetWrappedJSMap();
        if (map)
            map->Remove(this);
    }
    Unlink();
}

void
nsXPCWrappedJS::Unlink()
{
    if (IsValid()) {
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        if (rt) {
            if (IsRootWrapper()) {
                JSObject2WrappedJSMap* map = rt->GetWrappedJSMap();
                if (map)
                    map->Remove(this);
            }

            if (mRefCnt > 1)
                RemoveFromRootSet();
        }

        mJSObj = nullptr;
    }

    if (IsRootWrapper()) {
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
        if (rt->GCIsRunning()) {
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

        XPC_LOG_ALWAYS(("%s wrapper around JSObject @ %x", \
                        IsRootWrapper() ? "ROOT":"non-root", mJSObj.get()));
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

        if (!IsRootWrapper())
            XPC_LOG_OUTDENT();
        if (mNext) {
            if (IsRootWrapper()) {
                XPC_LOG_ALWAYS(("Additional wrappers for this object..."));
                XPC_LOG_INDENT();
            }
            mNext->DebugDump(depth);
            if (IsRootWrapper())
                XPC_LOG_OUTDENT();
        }
        if (IsRootWrapper())
            XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}
