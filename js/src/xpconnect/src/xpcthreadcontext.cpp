










































#include "xpcprivate.h"



XPCJSContextStack::XPCJSContextStack()
    : mStack(),
      mSafeJSContext(nsnull),
      mOwnSafeJSContext(nsnull)
{
    
}

XPCJSContextStack::~XPCJSContextStack()
{
    if(mOwnSafeJSContext)
    {
        JS_SetContextThread(mOwnSafeJSContext);
        JS_DestroyContext(mOwnSafeJSContext);
        mOwnSafeJSContext = nsnull;
        SyncJSContexts();
    }
}

void
XPCJSContextStack::SyncJSContexts()
{
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if(xpc)
        xpc->SyncJSContexts();
}


NS_IMETHODIMP
XPCJSContextStack::GetCount(PRInt32 *aCount)
{
    *aCount = mStack.Length();
    return NS_OK;
}


NS_IMETHODIMP
XPCJSContextStack::Peek(JSContext * *_retval)
{
    *_retval = mStack.IsEmpty() ? nsnull : mStack[mStack.Length() - 1].cx;
    return NS_OK;
}


NS_IMETHODIMP
XPCJSContextStack::Pop(JSContext * *_retval)
{
    NS_ASSERTION(!mStack.IsEmpty(), "ThreadJSContextStack underflow");

    PRUint32 idx = mStack.Length() - 1; 
    NS_ASSERTION(!mStack[idx].frame,
                 "Shouldn't have a pending frame to restore on the context "
                 "we're popping!");

    if(_retval)
        *_retval = mStack[idx].cx;

    mStack.RemoveElementAt(idx);
    if(idx > 0)
    {
        --idx; 
        JSContextAndFrame & e = mStack[idx];
        NS_ASSERTION(!e.frame || e.cx, "Shouldn't have frame without a cx!");
        if(e.cx && e.frame)
        {
            JS_RestoreFrameChain(e.cx, e.frame);
            e.frame = nsnull;
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
XPCJSContextStack::Push(JSContext * cx)
{
    if(!mStack.AppendElement(cx))
        return NS_ERROR_OUT_OF_MEMORY;
    if(mStack.Length() > 1)
    {
        JSContextAndFrame & e = mStack[mStack.Length() - 2];
        if(e.cx && e.cx != cx)
            e.frame = JS_SaveFrameChain(e.cx);
    }
    return NS_OK;
}

#ifdef DEBUG
JSBool 
XPCJSContextStack::DEBUG_StackHasJSContext(JSContext*  aJSContext)
{
    for(PRUint32 i = 0; i < mStack.Length(); i++)
        if(aJSContext == mStack[i].cx)
            return JS_TRUE;
    return JS_FALSE;
}
#endif

JS_STATIC_DLL_CALLBACK(JSBool)
SafeGlobalResolve(JSContext *cx, JSObject *obj, jsval id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

JS_STATIC_DLL_CALLBACK(void)
SafeFinalize(JSContext* cx, JSObject* obj)
{
#ifndef XPCONNECT_STANDALONE
    nsIScriptObjectPrincipal* sop =
        static_cast<nsIScriptObjectPrincipal*>(JS_GetPrivate(cx, obj));
    NS_IF_RELEASE(sop);
#endif
}

static JSClass global_class = {
    "global_for_XPCJSContextStack_SafeJSContext",
#ifndef XPCONNECT_STANDALONE
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_GLOBAL_FLAGS,
#else
    0,
#endif
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, SafeGlobalResolve, JS_ConvertStub, SafeFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};


NS_IMETHODIMP
XPCJSContextStack::GetSafeJSContext(JSContext * *aSafeJSContext)
{
    if(!mSafeJSContext)
    {
#ifndef XPCONNECT_STANDALONE
        
        
        nsCOMPtr<nsIPrincipal> principal =
            do_CreateInstance("@mozilla.org/nullprincipal;1");
        nsCOMPtr<nsIScriptObjectPrincipal> sop;
        if(principal)
        {
            sop = new PrincipalHolder(principal);
        }
        if(!sop)
        {
            *aSafeJSContext = nsnull;
            return NS_ERROR_FAILURE;
        }        
#endif 
        
        JSRuntime *rt;
        XPCJSRuntime* xpcrt;

        nsXPConnect* xpc = nsXPConnect::GetXPConnect();
        nsCOMPtr<nsIXPConnect> xpcholder(static_cast<nsIXPConnect*>(xpc));

        if(xpc && (xpcrt = xpc->GetRuntime()) && (rt = xpcrt->GetJSRuntime()))
        {
            mSafeJSContext = JS_NewContext(rt, 8192);
            if(mSafeJSContext)
            {
                
                AutoJSRequestWithNoCallContext req(mSafeJSContext);
                JSObject *glob;
                glob = JS_NewObject(mSafeJSContext, &global_class, NULL, NULL);

#ifndef XPCONNECT_STANDALONE
                if(glob)
                {
                    
                    
                    nsIScriptObjectPrincipal* priv = nsnull;
                    sop.swap(priv);
                    if(!JS_SetPrivate(mSafeJSContext, glob, priv))
                    {
                        
                        NS_RELEASE(priv);
                        glob = nsnull;
                    }
                }

                
                
                
                
#endif
                if(!glob || NS_FAILED(xpc->InitClasses(mSafeJSContext, glob)))
                {
                    
                    
                    
                    req.EndRequest();
                    JS_DestroyContext(mSafeJSContext);
                    mSafeJSContext = nsnull;
                }
                
                
                
                
                
                
                mOwnSafeJSContext = mSafeJSContext;
            }
        }
    }

    *aSafeJSContext = mSafeJSContext;
    return mSafeJSContext ? NS_OK : NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
XPCJSContextStack::SetSafeJSContext(JSContext * aSafeJSContext)
{
    if(mOwnSafeJSContext &&
       mOwnSafeJSContext == mSafeJSContext &&
       mOwnSafeJSContext != aSafeJSContext)
    {
        JS_DestroyContext(mOwnSafeJSContext);
        mOwnSafeJSContext = nsnull;
        SyncJSContexts();
    }

    mSafeJSContext = aSafeJSContext;
    return NS_OK;
}











NS_IMPL_THREADSAFE_ISUPPORTS3(nsXPCThreadJSContextStackImpl,
                              nsIThreadJSContextStack,
                              nsIJSContextStack,
                              nsISupportsWeakReference)

nsXPCThreadJSContextStackImpl*
nsXPCThreadJSContextStackImpl::gXPCThreadJSContextStack = nsnull;

nsXPCThreadJSContextStackImpl::nsXPCThreadJSContextStackImpl()
{
}

nsXPCThreadJSContextStackImpl::~nsXPCThreadJSContextStackImpl()
{
    gXPCThreadJSContextStack = nsnull;
}


nsXPCThreadJSContextStackImpl*
nsXPCThreadJSContextStackImpl::GetSingleton()
{
    if(!gXPCThreadJSContextStack)
    {
        gXPCThreadJSContextStack = new nsXPCThreadJSContextStackImpl();
        
        NS_IF_ADDREF(gXPCThreadJSContextStack);
    }
    NS_IF_ADDREF(gXPCThreadJSContextStack);

    return gXPCThreadJSContextStack;
}

void
nsXPCThreadJSContextStackImpl::FreeSingleton()
{
    nsXPCThreadJSContextStackImpl* tcs = gXPCThreadJSContextStack;
    if(tcs)
    {
        nsrefcnt cnt;
        NS_RELEASE2(tcs, cnt);
#ifdef XPC_DUMP_AT_SHUTDOWN
        if(0 != cnt)
            printf("*** dangling reference to nsXPCThreadJSContextStackImpl: refcnt=%d\n", cnt);
#endif
    }
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::GetCount(PRInt32 *aCount)
{
    if(!aCount)
        return NS_ERROR_NULL_POINTER;

    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        *aCount = 0;
        return NS_ERROR_FAILURE;
    }

    return myStack->GetCount(aCount);
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::Peek(JSContext * *_retval)
{
    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;
    }

    return myStack->Peek(_retval);
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::Pop(JSContext * *_retval)
{
    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        if(_retval)
            *_retval = nsnull;
        return NS_ERROR_FAILURE;
    }

    return myStack->Pop(_retval);
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::Push(JSContext * cx)
{
    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
        return NS_ERROR_FAILURE;

    return myStack->Push(cx);
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::GetSafeJSContext(JSContext * *aSafeJSContext)
{
    NS_ASSERTION(aSafeJSContext, "loser!");

    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
    {
        *aSafeJSContext = nsnull;
        return NS_ERROR_FAILURE;
    }

    return myStack->GetSafeJSContext(aSafeJSContext);
}


NS_IMETHODIMP
nsXPCThreadJSContextStackImpl::SetSafeJSContext(JSContext * aSafeJSContext)
{
    XPCJSContextStack* myStack = GetStackForCurrentThread();

    if(!myStack)
        return NS_ERROR_FAILURE;

    return myStack->SetSafeJSContext(aSafeJSContext);
}



PRUintn           XPCPerThreadData::gTLSIndex = BAD_TLS_INDEX;
PRLock*           XPCPerThreadData::gLock     = nsnull;
XPCPerThreadData* XPCPerThreadData::gThreads  = nsnull;

static jsuword
GetThreadStackLimit()
{
    int stackDummy;
    jsuword stackLimit, currentStackAddr = (jsuword)&stackDummy;

    const jsuword kStackSize = 0x80000;   

#if JS_STACK_GROWTH_DIRECTION < 0
    stackLimit = (currentStackAddr > kStackSize)
                 ? currentStackAddr - kStackSize
                 : 0;
#else
    stackLimit = (currentStackAddr + kStackSize > currentStackAddr)
                 ? currentStackAddr + kStackSize
                 : (jsuword) -1;
#endif

  return stackLimit;
}

MOZ_DECL_CTOR_COUNTER(xpcPerThreadData)

XPCPerThreadData::XPCPerThreadData()
    :   mJSContextStack(new XPCJSContextStack()),
        mNextThread(nsnull),
        mCallContext(nsnull),
        mResolveName(0),
        mResolvingWrapper(nsnull),
        mMostRecentJSContext(nsnull),
        mMostRecentXPCContext(nsnull),
        mExceptionManager(nsnull),
        mException(nsnull),
        mExceptionManagerNotAvailable(JS_FALSE),
        mAutoRoots(nsnull),
        mStackLimit(GetThreadStackLimit())
#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
      , mWrappedNativeThreadsafetyReportDepth(0)
#endif
{
    MOZ_COUNT_CTOR(xpcPerThreadData);
    if(gLock)
    {
        nsAutoLock lock(gLock);
        mNextThread = gThreads;
        gThreads = this;
    }
}

void
XPCPerThreadData::Cleanup()
{
    while(mAutoRoots)
        mAutoRoots->Unlink();
    NS_IF_RELEASE(mExceptionManager);
    NS_IF_RELEASE(mException);
    delete mJSContextStack;
    mJSContextStack = nsnull;

    if(mCallContext)
        mCallContext->SystemIsBeingShutDown();
}

XPCPerThreadData::~XPCPerThreadData()
{
    MOZ_COUNT_DTOR(xpcPerThreadData);

    Cleanup();

    
    if(gLock)
    {
        nsAutoLock lock(gLock);
        if(gThreads == this)
            gThreads = mNextThread;
        else
        {
            XPCPerThreadData* cur = gThreads;
            while(cur)
            {
                if(cur->mNextThread == this)
                {
                    cur->mNextThread = mNextThread;
                    break;
                }
                cur = cur->mNextThread;
            }
        }
    }

    if(gLock && !gThreads)
    {
        PR_DestroyLock(gLock);
        gLock = nsnull;
    }
}

PR_STATIC_CALLBACK(void)
xpc_ThreadDataDtorCB(void* ptr)
{
    XPCPerThreadData* data = (XPCPerThreadData*) ptr;
    if(data)
        delete data;
}

void XPCPerThreadData::TraceJS(JSTracer *trc)
{
#ifdef XPC_TRACK_AUTOMARKINGPTR_STATS
    {
        static int maxLength = 0;
        int length = 0;
        for(AutoMarkingPtr* p = mAutoRoots; p; p = p->GetNext())
            length++;
        if(length > maxLength)
            maxLength = length;
        printf("XPC gc on thread %x with %d AutoMarkingPtrs (%d max so far)\n",
               this, length, maxLength);
    }
#endif

    if(mAutoRoots)
        mAutoRoots->TraceJS(trc);
}

void XPCPerThreadData::MarkAutoRootsAfterJSFinalize()
{
    if(mAutoRoots)
        mAutoRoots->MarkAfterJSFinalize();
}


XPCPerThreadData*
XPCPerThreadData::GetData()
{
    XPCPerThreadData* data;

    if(!gLock)
    {
        gLock = PR_NewLock();
        if(!gLock)
            return nsnull;
    }

    if(gTLSIndex == BAD_TLS_INDEX)
    {
        nsAutoLock lock(gLock);
        
        if(gTLSIndex == BAD_TLS_INDEX)
        {
            if(PR_FAILURE ==
               PR_NewThreadPrivateIndex(&gTLSIndex, xpc_ThreadDataDtorCB))
            {
                NS_ASSERTION(0, "PR_NewThreadPrivateIndex failed!");
                gTLSIndex = BAD_TLS_INDEX;
                return nsnull;
            }
        }
    }

    data = (XPCPerThreadData*) PR_GetThreadPrivate(gTLSIndex);
    if(!data)
    {
        data = new XPCPerThreadData();
        if(!data || !data->IsValid())
        {
            NS_ASSERTION(0, "new XPCPerThreadData() failed!");
            if(data)
                delete data;
            return nsnull;
        }
        if(PR_FAILURE == PR_SetThreadPrivate(gTLSIndex, data))
        {
            NS_ASSERTION(0, "PR_SetThreadPrivate failed!");
            delete data;
            return nsnull;
        }
    }
    return data;
}


void
XPCPerThreadData::CleanupAllThreads()
{
    
    
    
    
    

    XPCJSContextStack** stacks = nsnull;
    int count = 0;
    int i;

    if(gLock)
    {
        nsAutoLock lock(gLock);

        for(XPCPerThreadData* cur = gThreads; cur; cur = cur->mNextThread)
            count++;

        stacks = (XPCJSContextStack**) new XPCJSContextStack*[count] ;
        if(stacks)
        {
            i = 0;
            for(XPCPerThreadData* cur = gThreads; cur; cur = cur->mNextThread)
            {
                stacks[i++] = cur->mJSContextStack;
                cur->mJSContextStack = nsnull;
                cur->Cleanup();
            }
        }
    }

    if(stacks)
    {
        for(i = 0; i < count; i++)
            delete stacks[i];
        delete [] stacks;
    }

    if(gTLSIndex != BAD_TLS_INDEX)
        PR_SetThreadPrivate(gTLSIndex, nsnull);
}


XPCPerThreadData*
XPCPerThreadData::IterateThreads(XPCPerThreadData** iteratorp)
{
    *iteratorp = (*iteratorp == nsnull) ? gThreads : (*iteratorp)->mNextThread;
    return *iteratorp;
}

NS_IMPL_ISUPPORTS1(nsXPCJSContextStackIterator, nsIJSContextStackIterator)

NS_IMETHODIMP
nsXPCJSContextStackIterator::Reset(nsIJSContextStack *aStack)
{
    
    nsXPCThreadJSContextStackImpl *impl =
        static_cast<nsXPCThreadJSContextStackImpl*>(aStack);
    XPCJSContextStack *stack = impl->GetStackForCurrentThread();
    if(!stack)
        return NS_ERROR_FAILURE;
    mStack = stack->GetStack();
    if(mStack->IsEmpty())
        mStack = nsnull;
    else
        mPosition = mStack->Length() - 1;

    return NS_OK;
}

NS_IMETHODIMP
nsXPCJSContextStackIterator::Done(PRBool *aDone)
{
    *aDone = !mStack;
    return NS_OK;
}

NS_IMETHODIMP
nsXPCJSContextStackIterator::Prev(JSContext **aContext)
{
    if(!mStack)
        return NS_ERROR_NOT_INITIALIZED;

    *aContext = mStack->ElementAt(mPosition).cx;

    if(mPosition == 0)
        mStack = nsnull;
    else
        --mPosition;
    
    return NS_OK;
}

