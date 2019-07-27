







#include "xpcprivate.h"
#include "jsprf.h"
#include "mozilla/DeferredFinalize.h"
#include "mozilla/jsipc/CrossProcessObjectWrappers.h"
#include "nsCCUncollectableMarker.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"

using namespace mozilla;








































bool
nsXPCWrappedJS::CanSkip()
{
    if (!nsCCUncollectableMarker::sGeneration)
        return false;

    if (IsSubjectToFinalization())
        return true;

    
    JSObject* obj = GetJSObjectPreserveColor();
    if (obj && JS::ObjectIsMarkedGray(obj))
        return false;

    
    
    if (!IsRootWrapper()) {
        
        NS_ENSURE_TRUE(mRoot, false);
        return mRoot->CanSkip();
    }

    
    
    if (!IsAggregatedToNative())
        return true;

    nsISupports* agg = GetAggregatedNativeObject();
    nsXPCOMCycleCollectionParticipant* cp = nullptr;
    CallQueryInterface(agg, &cp);
    nsISupports* canonical = nullptr;
    agg->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                        reinterpret_cast<void**>(&canonical));
    return cp && canonical && cp->CanSkipThis(canonical);
}

NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Traverse
   (void* p, nsCycleCollectionTraversalCallback& cb)
{
    nsISupports* s = static_cast<nsISupports*>(p);
    MOZ_ASSERT(CheckForRightISupports(s), "not the nsISupports pointer we expect");
    nsXPCWrappedJS* tmp = Downcast(s);

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

    
    if (tmp->IsSubjectToFinalization())
        return NS_OK;

    
    
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "self");
    cb.NoteXPCOMChild(s);

    if (tmp->IsValid()) {
        MOZ_ASSERT(refcnt > 1);
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mJSObj");
        cb.NoteJSObject(tmp->GetJSObjectPreserveColor());
    }

    if (tmp->IsRootWrapper()) {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "aggregated native");
        cb.NoteXPCOMChild(tmp->GetAggregatedNativeObject());
    } else {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "root");
        cb.NoteXPCOMChild(ToSupports(tmp->GetRootWrapper()));
    }

    return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXPCWrappedJS)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXPCWrappedJS)
    tmp->Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END



NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsXPCWrappedJS)
    return true;
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsXPCWrappedJS)
    return tmp->CanSkip();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsXPCWrappedJS)
    return tmp->CanSkip();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_IMETHODIMP
nsXPCWrappedJS::AggregatedQueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    MOZ_ASSERT(IsAggregatedToNative(), "bad AggregatedQueryInterface call");
    *aInstancePtr = nullptr;

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
        NS_PRECONDITION(false, "null pointer");
        return NS_ERROR_NULL_POINTER;
    }

    *aInstancePtr = nullptr;

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





MozExternalRefCountType
nsXPCWrappedJS::AddRef(void)
{
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();

    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");
    nsISupports* base = NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Upcast(this);
    nsrefcnt cnt = mRefCnt.incr(base);
    NS_LOG_ADDREF(this, cnt, "nsXPCWrappedJS", sizeof(*this));

    if (2 == cnt && IsValid()) {
        GetJSObject(); 
        mClass->GetRuntime()->AddWrappedJSRoot(this);
    }

    return cnt;
}

MozExternalRefCountType
nsXPCWrappedJS::Release(void)
{
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();
    MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");
    NS_ASSERT_OWNINGTHREAD(nsXPCWrappedJS);

    bool shouldDelete = false;
    nsISupports* base = NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Upcast(this);
    nsrefcnt cnt = mRefCnt.decr(base, &shouldDelete);
    NS_LOG_RELEASE(this, cnt, "nsXPCWrappedJS");

    if (0 == cnt) {
        if (MOZ_UNLIKELY(shouldDelete)) {
            mRefCnt.stabilizeForDeletion();
            DeleteCycleCollectable();
        } else {
            mRefCnt.incr(base);
            Destroy();
            mRefCnt.decr(base);
        }
    } else if (1 == cnt) {
        if (IsValid())
            RemoveFromRootSet();

        
        
        
        if (!HasWeakReferences())
            return Release();

        MOZ_ASSERT(IsRootWrapper(), "Only root wrappers should have weak references");
    }
    return cnt;
}

NS_IMETHODIMP_(void)
nsXPCWrappedJS::DeleteCycleCollectable(void)
{
    delete this;
}

void
nsXPCWrappedJS::TraceJS(JSTracer* trc)
{
    MOZ_ASSERT(mRefCnt >= 2 && IsValid(), "must be strongly referenced");
    JS_CallObjectTracer(trc, &mJSObj, "nsXPCWrappedJS::mJSObj");
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
                             nsXPCWrappedJS** wrapperResult)
{
    
    if (!MOZ_LIKELY(NS_IsMainThread()))
        MOZ_CRASH();

    AutoJSContext cx;
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    JSObject2WrappedJSMap* map = rt->GetWrappedJSMap();
    if (!map) {
        MOZ_ASSERT(map,"bad map");
        return NS_ERROR_FAILURE;
    }

    bool allowNonScriptable = mozilla::jsipc::IsWrappedCPOW(jsObj);
    nsRefPtr<nsXPCWrappedJSClass> clasp = nsXPCWrappedJSClass::GetNewOrUsed(cx, aIID,
                                                                            allowNonScriptable);
    if (!clasp)
        return NS_ERROR_FAILURE;

    JS::RootedObject rootJSObj(cx, clasp->GetRootJSObject(cx, jsObj));
    if (!rootJSObj)
        return NS_ERROR_FAILURE;

    nsresult rv = NS_ERROR_FAILURE;
    nsRefPtr<nsXPCWrappedJS> root = map->Find(rootJSObj);
    if (root) {
        nsRefPtr<nsXPCWrappedJS> wrapper = root->FindOrFindInherited(aIID);
        if (wrapper) {
            wrapper.forget(wrapperResult);
            return NS_OK;
        }
    } else if (rootJSObj != jsObj) {

        
        
        
        nsRefPtr<nsXPCWrappedJSClass> rootClasp = nsXPCWrappedJSClass::GetNewOrUsed(cx, NS_GET_IID(nsISupports));
        if (!rootClasp)
            return NS_ERROR_FAILURE;

        root = new nsXPCWrappedJS(cx, rootJSObj, rootClasp, nullptr, &rv);
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    nsRefPtr<nsXPCWrappedJS> wrapper = new nsXPCWrappedJS(cx, jsObj, clasp, root, &rv);
    if (NS_FAILED(rv)) {
        return rv;
    }
    wrapper.forget(wrapperResult);
    return NS_OK;
}

nsXPCWrappedJS::nsXPCWrappedJS(JSContext* cx,
                               JSObject* aJSObj,
                               nsXPCWrappedJSClass* aClass,
                               nsXPCWrappedJS* root,
                               nsresult* rv)
    : mJSObj(aJSObj),
      mClass(aClass),
      mRoot(root ? root : this),
      mNext(nullptr)
{
    *rv = InitStub(GetClass()->GetIID());
    
    

    
    
    
    NS_ADDREF_THIS();

    if (IsRootWrapper()) {
        nsXPConnect::GetRuntimeInstance()->GetWrappedJSMap()->Add(cx, this);
    } else {
        NS_ADDREF(mRoot);
        mNext = mRoot->mNext;
        mRoot->mNext = this;
    }
}

nsXPCWrappedJS::~nsXPCWrappedJS()
{
    Destroy();
}

void
nsXPCWrappedJS::Destroy()
{
    MOZ_ASSERT(1 == int32_t(mRefCnt), "should be stabilized for deletion");

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

    mClass = nullptr;
    if (mOuter) {
        XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
        if (rt->GCIsRunning()) {
            DeferredFinalize(mOuter.forget().take());
        } else {
            mOuter = nullptr;
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
nsXPCWrappedJS::GetInterfaceInfo(nsIInterfaceInfo** infoResult)
{
    MOZ_ASSERT(GetClass(), "wrapper without class");
    MOZ_ASSERT(GetClass()->GetInterfaceInfo(), "wrapper class without interface");

    
    

    nsCOMPtr<nsIInterfaceInfo> info = GetClass()->GetInterfaceInfo();
    if (!info)
        return NS_ERROR_UNEXPECTED;
    info.forget(infoResult);
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

size_t
nsXPCWrappedJS::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    
    
    
    
    size_t n = mallocSizeOf(this);
    n += nsAutoXPTCStub::SizeOfExcludingThis(mallocSizeOf);

    
    
    
    if (mNext)
        n += mNext->SizeOfIncludingThis(mallocSizeOf);

    return n;
}




NS_IMETHODIMP
nsXPCWrappedJS::GetEnumerator(nsISimpleEnumerator * *aEnumerate)
{
    AutoJSContext cx;
    XPCCallContext ccx(NATIVE_CALLER, cx);
    if (!ccx.IsValid())
        return NS_ERROR_UNEXPECTED;

    return nsXPCWrappedJSClass::BuildPropertyEnumerator(ccx, GetJSObject(),
                                                        aEnumerate);
}


NS_IMETHODIMP
nsXPCWrappedJS::GetProperty(const nsAString & name, nsIVariant** _retval)
{
    AutoJSContext cx;
    XPCCallContext ccx(NATIVE_CALLER, cx);
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
            free(name);
        char * iid = GetClass()->GetIID().ToString();
        XPC_LOG_ALWAYS(("IID number is %s", iid ? iid : "invalid"));
        if (iid)
            free(iid);
        XPC_LOG_ALWAYS(("nsXPCWrappedJSClass @ %x", mClass.get()));

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
