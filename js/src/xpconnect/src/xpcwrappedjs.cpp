










































#include "xpcprivate.h"



NS_IMPL_CYCLE_COLLECTION_CLASS(nsXPCWrappedJS)

NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Traverse
   (nsISupports *s, nsCycleCollectionTraversalCallback &cb)
{
    
    
    
    
    
    
    
    
    
    

    nsresult rv;
    nsIXPConnectWrappedJS *base;
    nsXPCWrappedJS *tmp;
    {
        
        
        nsCOMPtr<nsIXPConnectWrappedJS> owner = do_QueryInterface(s, &rv);
        if (NS_FAILED(rv))
            return rv;

        base = owner.get();
        tmp = NS_STATIC_CAST(nsXPCWrappedJS*, base);
        NS_ASSERTION(tmp->mRefCnt.get() > 2,
                     "How can this be, no one else holds a strong ref?");
    }

    NS_ASSERTION(tmp->IsValid(), "How did we get here?");

    nsrefcnt refcnt = tmp->mRefCnt.get();
#ifdef DEBUG
    char name[72];
    JS_snprintf(name, sizeof(name), "nsXPCWrappedJS (%s)",
                tmp->GetClass()->GetInterfaceName());
    cb.DescribeNode(refcnt, sizeof(nsXPCWrappedJS), name);
#else
    cb.DescribeNode(refcnt, sizeof(nsXPCWrappedJS), "nsXPCWrappedJS");
#endif

    
    
    cb.NoteXPCOMChild(base);

    if(refcnt > 1)
        
        
        cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT,
                           tmp->GetJSObject());

    nsXPCWrappedJS* root = tmp->GetRootWrapper();
    if(root == tmp)
        
        cb.NoteXPCOMChild(tmp->GetAggregatedNativeObject());
    else
        
        cb.NoteXPCOMChild(NS_STATIC_CAST(nsIXPConnectWrappedJS*, root));

    return NS_OK;
}

NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Unlink(nsISupports *s)
{
    
    
    
    
    
    return NS_OK;
}

NS_IMETHODIMP
nsXPCWrappedJS::AggregatedQueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    NS_ASSERTION(IsAggregatedToNative(), "bad AggregatedQueryInterface call");

    if(!IsValid())
        return NS_ERROR_UNEXPECTED;

    
    
    
    
    if(aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS)))
    {
        NS_ADDREF(this);
        *aInstancePtr = (void*) NS_STATIC_CAST(nsIXPConnectWrappedJS*,this);
        return NS_OK;
    }

    return mClass->DelegatedQueryInterface(this, aIID, aInstancePtr);
}

NS_IMETHODIMP
nsXPCWrappedJS::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if(!IsValid())
        return NS_ERROR_UNEXPECTED;

    if(nsnull == aInstancePtr)
    {
        NS_PRECONDITION(0, "null pointer");
        return NS_ERROR_NULL_POINTER;
    }

    if ( aIID.Equals(NS_GET_IID(nsCycleCollectionParticipant)) ) {
        *aInstancePtr = & NS_CYCLE_COLLECTION_NAME(nsXPCWrappedJS);
        return NS_OK;
    }

    if(aIID.Equals(NS_GET_IID(nsCycleCollectionISupports)))
    {
        NS_ADDREF(this);
        *aInstancePtr =
            NS_CYCLE_COLLECTION_CLASSNAME(nsXPCWrappedJS)::Upcast(this);
        return NS_OK;
    }

    
    
    if(aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS)))
    {
        NS_ADDREF(this);
        *aInstancePtr = (void*) NS_STATIC_CAST(nsIXPConnectWrappedJS*,this);
        return NS_OK;
    }

    nsISupports* outer = GetAggregatedNativeObject();
    if(outer)
        return outer->QueryInterface(aIID, aInstancePtr);

    

    return mClass->DelegatedQueryInterface(this, aIID, aInstancePtr);
}





















nsrefcnt
nsXPCWrappedJS::AddRef(void)
{
    NS_PRECONDITION(mRoot, "bad root");
    nsrefcnt cnt = (nsrefcnt) PR_AtomicIncrement((PRInt32*)&mRefCnt);
    NS_LOG_ADDREF(this, cnt, "nsXPCWrappedJS", sizeof(*this));

    if(2 == cnt && IsValid())
    {
        XPCCallContext ccx(NATIVE_CALLER);
        if(ccx.IsValid()) {
#ifdef GC_MARK_DEBUG
            mGCRootName = JS_smprintf("nsXPCWrappedJS::mJSObj[%s,0x%p,0x%p]",
                                      GetClass()->GetInterfaceName(),
                                      this, mJSObj);
            JS_AddNamedRoot(ccx.GetJSContext(), &mJSObj, mGCRootName);
#else
            JS_AddNamedRoot(ccx.GetJSContext(), &mJSObj, "nsXPCWrappedJS::mJSObj");
#endif
        }
    }

    return cnt;
}

nsrefcnt
nsXPCWrappedJS::Release(void)
{
    NS_PRECONDITION(mRoot, "bad root");
    NS_PRECONDITION(0 != mRefCnt, "dup release");

#ifdef DEBUG_jband
    NS_ASSERTION(IsValid(), "post xpconnect shutdown call of nsXPCWrappedJS::Release");
#endif

do_decrement:

    nsrefcnt cnt = (nsrefcnt) PR_AtomicDecrement((PRInt32*)&mRefCnt);
    NS_LOG_RELEASE(this, cnt, "nsXPCWrappedJS");

    if(0 == cnt)
    {
        NS_DELETEXPCOM(this);   
        return 0;
    }
    if(1 == cnt)
    {
        if(IsValid())
        {
            XPCJSRuntime* rt = mClass->GetRuntime();
            if(rt) {
                JS_RemoveRootRT(rt->GetJSRuntime(), &mJSObj);
#ifdef GC_MARK_DEBUG
                JS_smprintf_free(mGCRootName);
                mGCRootName = nsnull;
#endif
            }
        }

        
        
        
        
        if(!HasWeakReferences())
            goto do_decrement;
    }
    return cnt;
}

NS_IMETHODIMP
nsXPCWrappedJS::GetWeakReference(nsIWeakReference** aInstancePtr)
{
    if(mRoot != this)
        return mRoot->GetWeakReference(aInstancePtr);

    return nsSupportsWeakReference::GetWeakReference(aInstancePtr);
}

NS_IMETHODIMP
nsXPCWrappedJS::GetJSObject(JSObject** aJSObj)
{
    NS_PRECONDITION(aJSObj, "bad param");
    NS_PRECONDITION(mJSObj, "bad wrapper");
    if(!(*aJSObj = mJSObj))
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}


nsresult
nsXPCWrappedJS::GetNewOrUsed(XPCCallContext& ccx,
                             JSObject* aJSObj,
                             REFNSIID aIID,
                             nsISupports* aOuter,
                             nsXPCWrappedJS** wrapperResult)
{
    JSObject2WrappedJSMap* map;
    JSObject* rootJSObj;
    nsXPCWrappedJS* root;
    nsXPCWrappedJS* wrapper = nsnull;
    nsXPCWrappedJSClass* clazz = nsnull;
    XPCJSRuntime* rt = ccx.GetRuntime();

    map = rt->GetWrappedJSMap();
    if(!map)
    {
        NS_ASSERTION(map,"bad map");
        return NS_ERROR_FAILURE;
    }

    nsXPCWrappedJSClass::GetNewOrUsed(ccx, aIID, &clazz);
    if(!clazz)
        return NS_ERROR_FAILURE;
    

    
    rootJSObj = clazz->GetRootJSObject(ccx, aJSObj);
    if(!rootJSObj)
        goto return_wrapper;

    
    {   
        XPCAutoLock lock(rt->GetMapLock());
        root = map->Find(rootJSObj);
    }
    if(root)
    {
        if((nsnull != (wrapper = root->Find(aIID))) ||
           (nsnull != (wrapper = root->FindInherited(aIID))))
        {
            NS_ADDREF(wrapper);
            goto return_wrapper;
        }
    }
    else
    {
        
        if(rootJSObj == aJSObj)
        {
            
            wrapper = root = new nsXPCWrappedJS(ccx, aJSObj, clazz, nsnull,
                                                aOuter);
            if(root)
            {   
#if DEBUG_xpc_leaks
                printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
                       (void*)wrapper, (void*)aJSObj);
#endif
                XPCAutoLock lock(rt->GetMapLock());
                map->Add(root);
            }
            goto return_wrapper;
        }
        else
        {
            
            nsXPCWrappedJSClass* rootClazz = nsnull;
            nsXPCWrappedJSClass::GetNewOrUsed(ccx, NS_GET_IID(nsISupports),
                                              &rootClazz);
            if(!rootClazz)
                goto return_wrapper;

            root = new nsXPCWrappedJS(ccx, rootJSObj, rootClazz, nsnull, aOuter);
            NS_RELEASE(rootClazz);

            if(!root)
                goto return_wrapper;
            {   
#if DEBUG_xpc_leaks
                printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
                       (void*)root, (void*)rootJSObj);
#endif
                XPCAutoLock lock(rt->GetMapLock());
                map->Add(root);
            }
        }
    }

    
    NS_ASSERTION(root,"bad root");
    NS_ASSERTION(clazz,"bad clazz");

    if(!wrapper)
    {
        wrapper = new nsXPCWrappedJS(ccx, aJSObj, clazz, root, aOuter);
        if(!wrapper)
            goto return_wrapper;
#if DEBUG_xpc_leaks
        printf("Created nsXPCWrappedJS %p, JSObject is %p\n",
               (void*)wrapper, (void*)aJSObj);
#endif
    }

    wrapper->mNext = root->mNext;
    root->mNext = wrapper;

return_wrapper:
    if(clazz)
        NS_RELEASE(clazz);

    if(!wrapper)
        return NS_ERROR_FAILURE;

    *wrapperResult = wrapper;
    return NS_OK;
}

nsXPCWrappedJS::nsXPCWrappedJS(XPCCallContext& ccx,
                               JSObject* aJSObj,
                               nsXPCWrappedJSClass* aClass,
                               nsXPCWrappedJS* root,
                               nsISupports* aOuter)
    : mJSObj(aJSObj),
      mClass(aClass),
      mRoot(root ? root : this),
      mNext(nsnull),
      mOuter(root ? nsnull : aOuter)
#ifdef GC_MARK_DEBUG
      , mGCRootName(nsnull)
#endif
{
#ifdef DEBUG_stats_jband
    static int count = 0;
    static const int interval = 10;
    if(0 == (++count % interval))
        printf("//////// %d instances of nsXPCWrappedJS created\n", count);
#endif

    InitStub(GetClass()->GetIID());

    
    NS_ADDREF_THIS();
    NS_ADDREF_THIS();
    NS_ADDREF(aClass);
    NS_IF_ADDREF(mOuter);

    if(mRoot != this)
        NS_ADDREF(mRoot);

}

nsXPCWrappedJS::~nsXPCWrappedJS()
{
    NS_PRECONDITION(0 == mRefCnt, "refcounting error");

    XPCJSRuntime* rt = nsXPConnect::GetRuntime();

    if(mRoot != this)
    {
        
        nsXPCWrappedJS* cur = mRoot;
        while(1)
        {
            if(cur->mNext == this)
            {
                cur->mNext = mNext;
                break;
            }
            cur = cur->mNext;
            NS_ASSERTION(cur, "failed to find wrapper in its own chain");
        }
        
        NS_RELEASE(mRoot);
    }
    else
    {
        NS_ASSERTION(!mNext, "root wrapper with non-empty chain being deleted");

        
        ClearWeakReferences();

        
        if(rt)
        {
            JSObject2WrappedJSMap* map = rt->GetWrappedJSMap();
            if(map)
            {
                XPCAutoLock lock(rt->GetMapLock());
                map->Remove(this);
            }
        }
    }

    if(IsValid())
    {
        NS_IF_RELEASE(mClass);
        if (mOuter)
        {
            if (rt && rt->GetThreadRunningGC())
            {
                rt->DeferredRelease(mOuter);
                mOuter = nsnull;
            }
            else
            {
                NS_RELEASE(mOuter);
            }
        }
    }
}

nsXPCWrappedJS*
nsXPCWrappedJS::Find(REFNSIID aIID)
{
    if(aIID.Equals(NS_GET_IID(nsISupports)))
        return mRoot;

    for(nsXPCWrappedJS* cur = mRoot; cur; cur = cur->mNext)
    {
        if(aIID.Equals(cur->GetIID()))
            return cur;
    }

    return nsnull;
}


nsXPCWrappedJS*
nsXPCWrappedJS::FindInherited(REFNSIID aIID)
{
    NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsISupports)), "bad call sequence");

    for(nsXPCWrappedJS* cur = mRoot; cur; cur = cur->mNext)
    {
        PRBool found;
        if(NS_SUCCEEDED(cur->GetClass()->GetInterfaceInfo()->
                                HasAncestor(&aIID, &found)) && found)
            return cur;
    }

    return nsnull;
}

NS_IMETHODIMP
nsXPCWrappedJS::GetInterfaceInfo(nsIInterfaceInfo** info)
{
    NS_ASSERTION(GetClass(), "wrapper without class");
    NS_ASSERTION(GetClass()->GetInterfaceInfo(), "wrapper class without interface");

    
    

    if(!(*info = GetClass()->GetInterfaceInfo()))
        return NS_ERROR_UNEXPECTED;
    NS_ADDREF(*info);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCWrappedJS::CallMethod(PRUint16 methodIndex,
                           const XPTMethodDescriptor* info,
                           nsXPTCMiniVariant* params)
{
    if(!IsValid())
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
nsXPCWrappedJS::SystemIsBeingShutDown(JSRuntime* rt)
{
    
    
    

    
    
    

    
    
    mJSObj = nsnull;

    
    
    JS_RemoveRootRT(rt, &mJSObj);

    
    if(mNext)
        mNext->SystemIsBeingShutDown(rt);
}




NS_IMETHODIMP 
nsXPCWrappedJS::GetEnumerator(nsISimpleEnumerator * *aEnumerate)
{
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return NS_ERROR_UNEXPECTED;

    return nsXPCWrappedJSClass::BuildPropertyEnumerator(ccx, mJSObj, aEnumerate);
}


NS_IMETHODIMP 
nsXPCWrappedJS::GetProperty(const nsAString & name, nsIVariant **_retval)
{
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return NS_ERROR_UNEXPECTED;

    JSString* jsstr = XPCStringConvert::ReadableToJSString(ccx, name);
    if(!jsstr)
        return NS_ERROR_OUT_OF_MEMORY;

    return nsXPCWrappedJSClass::
        GetNamedPropertyAsVariant(ccx, mJSObj, STRING_TO_JSVAL(jsstr), _retval);
}



NS_IMETHODIMP
nsXPCWrappedJS::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    XPC_LOG_ALWAYS(("nsXPCWrappedJS @ %x with mRefCnt = %d", this, mRefCnt.get()));
        XPC_LOG_INDENT();

        PRBool isRoot = mRoot == this;
        XPC_LOG_ALWAYS(("%s wrapper around JSObject @ %x", \
                         isRoot ? "ROOT":"non-root", mJSObj));
        char* name;
        GetClass()->GetInterfaceInfo()->GetName(&name);
        XPC_LOG_ALWAYS(("interface name is %s", name));
        if(name)
            nsMemory::Free(name);
        char * iid = GetClass()->GetIID().ToString();
        XPC_LOG_ALWAYS(("IID number is %s", iid ? iid : "invalid"));
        if(iid)
            PR_Free(iid);
        XPC_LOG_ALWAYS(("nsXPCWrappedJSClass @ %x", mClass));

        if(!isRoot)
            XPC_LOG_OUTDENT();
        if(mNext)
        {
            if(isRoot)
            {
                XPC_LOG_ALWAYS(("Additional wrappers for this object..."));
                XPC_LOG_INDENT();
            }
            mNext->DebugDump(depth);
            if(isRoot)
                XPC_LOG_OUTDENT();
        }
        if(isRoot)
            XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}
