










































#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "mozilla/Mutex.h"
#include "nsDOMJSUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsNullPrincipal.h"

using namespace mozilla;



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
    }
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

        XPCJSContextInfo & e = mStack[idx];
        NS_ASSERTION(!e.frame || e.cx, "Shouldn't have frame without a cx!");
        NS_ASSERTION(!e.suspendDepth || e.cx, "Shouldn't have suspendDepth without a cx!");
        if(e.cx)
        {
            if(e.suspendDepth)
            {
                JS_ResumeRequest(e.cx, e.suspendDepth);
                e.suspendDepth = 0;
            }

            if(e.frame)
            {
                
                JSAutoRequest ar(e.cx);
                JS_RestoreFrameChain(e.cx, e.frame);
                e.frame = nsnull;
            }
        }
    }
    return NS_OK;
}

static nsIPrincipal*
GetPrincipalFromCx(JSContext *cx)
{
    nsIScriptContextPrincipal* scp = GetScriptContextPrincipalFromJSContext(cx);
    if(scp)
    {
        nsIScriptObjectPrincipal* globalData = scp->GetObjectPrincipal();
        if(globalData)
            return globalData->GetPrincipal();
    }
    return nsnull;
}


NS_IMETHODIMP
XPCJSContextStack::Push(JSContext * cx)
{
    JS_ASSERT_IF(cx, JS_GetContextThread(cx));
    if(!mStack.AppendElement(cx))
        return NS_ERROR_OUT_OF_MEMORY;
    if(mStack.Length() > 1)
    {
        XPCJSContextInfo & e = mStack[mStack.Length() - 2];
        if(e.cx)
        {
            if(e.cx == cx)
            {
                nsIScriptSecurityManager* ssm = XPCWrapper::GetSecurityManager();
                if(ssm)
                {
                    nsIPrincipal* globalObjectPrincipal =
                        GetPrincipalFromCx(cx);
                    if(globalObjectPrincipal)
                    {
                        nsIPrincipal* subjectPrincipal = ssm->GetCxSubjectPrincipal(cx);
                        PRBool equals = PR_FALSE;
                        globalObjectPrincipal->Equals(subjectPrincipal, &equals);
                        if(equals)
                        {
                            return NS_OK;
                        }
                    }
                }
            }

            {
                
                JSAutoRequest ar(e.cx);
                e.frame = JS_SaveFrameChain(e.cx);
            }

            if(!cx)
                e.suspendDepth = JS_SuspendRequest(e.cx);
        }
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

static JSBool
SafeGlobalResolve(JSContext *cx, JSObject *obj, jsid id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static void
SafeFinalize(JSContext* cx, JSObject* obj)
{
    nsIScriptObjectPrincipal* sop =
        static_cast<nsIScriptObjectPrincipal*>(xpc_GetJSPrivate(obj));
    NS_IF_RELEASE(sop);
}

static JSClass global_class = {
    "global_for_XPCJSContextStack_SafeJSContext",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, SafeGlobalResolve, JS_ConvertStub, SafeFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};


NS_IMETHODIMP
XPCJSContextStack::GetSafeJSContext(JSContext * *aSafeJSContext)
{
    if(!mSafeJSContext)
    {
        
        
        nsRefPtr<nsNullPrincipal> principal = new nsNullPrincipal();
        nsCOMPtr<nsIScriptObjectPrincipal> sop;
        if(principal)
        {
            nsresult rv = principal->Init();
            if(NS_SUCCEEDED(rv))
              sop = new PrincipalHolder(principal);
        }
        if(!sop)
        {
            *aSafeJSContext = nsnull;
            return NS_ERROR_FAILURE;
        }        
        
        JSRuntime *rt;
        XPCJSRuntime* xpcrt;

        nsXPConnect* xpc = nsXPConnect::GetXPConnect();
        nsCOMPtr<nsIXPConnect> xpcholder(static_cast<nsIXPConnect*>(xpc));

        if(xpc && (xpcrt = xpc->GetRuntime()) && (rt = xpcrt->GetJSRuntime()))
        {
            JSObject *glob;
            mSafeJSContext = JS_NewContext(rt, 8192);
            if(mSafeJSContext)
            {
                
                JSAutoRequest req(mSafeJSContext);

                
                
                JSCompartment *compartment;
                nsresult rv = xpc_CreateMTGlobalObject(mSafeJSContext,
                                                       &global_class,
                                                       principal, &glob,
                                                       &compartment);
                if(NS_FAILED(rv))
                    glob = nsnull;

                if(glob)
                {
                    
                    
                    JS_SetGlobalObject(mSafeJSContext, glob);

                    
                    
                    nsIScriptObjectPrincipal* priv = nsnull;
                    sop.swap(priv);
                    if(!JS_SetPrivate(mSafeJSContext, glob, priv))
                    {
                        
                        NS_RELEASE(priv);
                        glob = nsnull;
                    }
                }

                
                
                
                
                if(glob && NS_FAILED(xpc->InitClasses(mSafeJSContext, glob)))
                {
                    glob = nsnull;
                }

            }
            if(mSafeJSContext && !glob)
            {
                
                
                JS_DestroyContext(mSafeJSContext);
                mSafeJSContext = nsnull;
            }
            
            
            
            
            
            
            mOwnSafeJSContext = mSafeJSContext;
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
        JS_DestroyContextNoGC(mOwnSafeJSContext);
        mOwnSafeJSContext = nsnull;
    }

    mSafeJSContext = aSafeJSContext;
    return NS_OK;
}



PRUintn           XPCPerThreadData::gTLSIndex       = BAD_TLS_INDEX;
Mutex*            XPCPerThreadData::gLock           = nsnull;
XPCPerThreadData* XPCPerThreadData::gThreads        = nsnull;
XPCPerThreadData *XPCPerThreadData::sMainThreadData = nsnull;
void *            XPCPerThreadData::sMainJSThread   = nsnull;

XPCPerThreadData::XPCPerThreadData()
    :   mJSContextStack(new XPCJSContextStack()),
        mNextThread(nsnull),
        mCallContext(nsnull),
        mResolveName(JSID_VOID),
        mResolvingWrapper(nsnull),
        mExceptionManager(nsnull),
        mException(nsnull),
        mExceptionManagerNotAvailable(JS_FALSE),
        mAutoRoots(nsnull)
#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
      , mWrappedNativeThreadsafetyReportDepth(0)
#endif
{
    MOZ_COUNT_CTOR(xpcPerThreadData);
    if(gLock)
    {
        MutexAutoLock lock(*gLock);
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
    


    PRBool doDestroyLock = PR_FALSE;

    MOZ_COUNT_DTOR(xpcPerThreadData);

    Cleanup();

    
    if(gLock)
    {
        MutexAutoLock lock(*gLock);
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
        if (!gThreads)
            doDestroyLock = PR_TRUE;
    }

    if(gLock && doDestroyLock)
    {
        delete gLock;
        gLock = nsnull;
    }
}

static void
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
XPCPerThreadData::GetDataImpl(JSContext *cx)
{
    XPCPerThreadData* data;

    if(!gLock)
    {
        gLock = new Mutex("XPCPerThreadData.gLock");
    }

    if(gTLSIndex == BAD_TLS_INDEX)
    {
        MutexAutoLock lock(*gLock);
        
        if(gTLSIndex == BAD_TLS_INDEX)
        {
            if(PR_FAILURE ==
               PR_NewThreadPrivateIndex(&gTLSIndex, xpc_ThreadDataDtorCB))
            {
                NS_ERROR("PR_NewThreadPrivateIndex failed!");
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
            NS_ERROR("new XPCPerThreadData() failed!");
            if(data)
                delete data;
            return nsnull;
        }
        if(PR_FAILURE == PR_SetThreadPrivate(gTLSIndex, data))
        {
            NS_ERROR("PR_SetThreadPrivate failed!");
            delete data;
            return nsnull;
        }
    }

    if(cx && !sMainJSThread && NS_IsMainThread())
    {
        sMainJSThread = cx->thread;

        sMainThreadData = data;

        sMainThreadData->mThread = PR_GetCurrentThread();
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
        MutexAutoLock lock(*gLock);

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
    NS_ASSERTION(aStack == nsXPConnect::GetXPConnect(),
                 "aStack must be implemented by XPConnect singleton");
    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);
    if(!data)
        return NS_ERROR_FAILURE;
    mStack = data->GetJSContextStack()->GetStack();
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

