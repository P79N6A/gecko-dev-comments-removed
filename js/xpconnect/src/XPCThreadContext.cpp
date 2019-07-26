








#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "mozilla/Mutex.h"
#include "nsDOMJSUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsNullPrincipal.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using mozilla::dom::DestroyProtoOrIfaceCache;



XPCJSContextStack::~XPCJSContextStack()
{
    if (mOwnSafeJSContext) {
        JS_DestroyContext(mOwnSafeJSContext);
        mOwnSafeJSContext = nsnull;
    }
}

JSContext*
XPCJSContextStack::Pop()
{
    MOZ_ASSERT(!mStack.IsEmpty());

    uint32_t idx = mStack.Length() - 1; 

    JSContext *cx = mStack[idx].cx;

    mStack.RemoveElementAt(idx);
    if (idx == 0)
        return cx;

    --idx; 

    XPCJSContextInfo &e = mStack[idx];
    NS_ASSERTION(!e.suspendDepth || e.cx, "Shouldn't have suspendDepth without a cx!");
    if (e.cx) {
        if (e.suspendDepth) {
            JS_ResumeRequest(e.cx, e.suspendDepth);
            e.suspendDepth = 0;
        }

        if (e.savedFrameChain) {
            
            JSAutoRequest ar(e.cx);
            JS_RestoreFrameChain(e.cx);
            e.savedFrameChain = false;
        }
    }
    return cx;
}

static nsIPrincipal*
GetPrincipalFromCx(JSContext *cx)
{
    nsIScriptContextPrincipal* scp = GetScriptContextPrincipalFromJSContext(cx);
    if (scp) {
        nsIScriptObjectPrincipal* globalData = scp->GetObjectPrincipal();
        if (globalData)
            return globalData->GetPrincipal();
    }
    return nsnull;
}

bool
XPCJSContextStack::Push(JSContext *cx)
{
    if (mStack.Length() == 0) {
        mStack.AppendElement(cx);
        return true;
    }

    XPCJSContextInfo &e = mStack[mStack.Length() - 1];
    if (e.cx) {
        if (e.cx == cx) {
            nsIScriptSecurityManager* ssm = XPCWrapper::GetSecurityManager();
            if (ssm) {
                if (nsIPrincipal* globalObjectPrincipal = GetPrincipalFromCx(cx)) {
                    nsIPrincipal* subjectPrincipal = ssm->GetCxSubjectPrincipal(cx);
                    bool equals = false;
                    globalObjectPrincipal->Equals(subjectPrincipal, &equals);
                    if (equals) {
                        mStack.AppendElement(cx);
                        return true;
                    }
                }
            }
        }

        {
            
            JSAutoRequest ar(e.cx);
            if (!JS_SaveFrameChain(e.cx))
                return false;
            e.savedFrameChain = true;
        }

        if (!cx)
            e.suspendDepth = JS_SuspendRequest(e.cx);
    }

    mStack.AppendElement(cx);
    return true;
}

#ifdef DEBUG
bool
XPCJSContextStack::DEBUG_StackHasJSContext(JSContext *cx)
{
    for (PRUint32 i = 0; i < mStack.Length(); i++)
        if (cx == mStack[i].cx)
            return true;
    return false;
}
#endif

static JSBool
SafeGlobalResolve(JSContext *cx, JSHandleObject obj, JSHandleId id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static void
SafeFinalize(JSFreeOp *fop, JSObject* obj)
{
    nsIScriptObjectPrincipal* sop =
        static_cast<nsIScriptObjectPrincipal*>(xpc_GetJSPrivate(obj));
    NS_IF_RELEASE(sop);
    DestroyProtoOrIfaceCache(obj);
}

static JSClass global_class = {
    "global_for_XPCJSContextStack_SafeJSContext",
    XPCONNECT_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, SafeGlobalResolve, JS_ConvertStub, SafeFinalize,
    NULL, NULL, NULL, NULL, TraceXPCGlobal
};



extern void
mozJSLoaderErrorReporter(JSContext *cx, const char *message, JSErrorReport *rep);

JSContext*
XPCJSContextStack::GetSafeJSContext()
{
    if (mSafeJSContext)
        return mSafeJSContext;

    
    
    nsRefPtr<nsNullPrincipal> principal = new nsNullPrincipal();
    nsresult rv = principal->Init();
    if (NS_FAILED(rv))
        return NULL;

    nsCOMPtr<nsIScriptObjectPrincipal> sop = new PrincipalHolder(principal);

    nsRefPtr<nsXPConnect> xpc = nsXPConnect::GetXPConnect();
    if (!xpc)
        return NULL;

    XPCJSRuntime* xpcrt = xpc->GetRuntime();
    if (!xpcrt)
        return NULL;

    JSRuntime *rt = xpcrt->GetJSRuntime();
    if (!rt)
        return NULL;

    mSafeJSContext = JS_NewContext(rt, 8192);
    if (!mSafeJSContext)
        return NULL;

    JSObject *glob;
    {
        
        JSAutoRequest req(mSafeJSContext);

        JS_SetErrorReporter(mSafeJSContext, mozJSLoaderErrorReporter);

        JSCompartment *compartment;
        nsresult rv = xpc_CreateGlobalObject(mSafeJSContext, &global_class,
                                             principal, principal, false,
                                             &glob, &compartment);
        if (NS_FAILED(rv))
            glob = nsnull;

        if (glob) {
            
            
            JS_SetGlobalObject(mSafeJSContext, glob);

            
            
            nsIScriptObjectPrincipal* priv = nsnull;
            sop.swap(priv);
            JS_SetPrivate(glob, priv);
        }

        
        
        
        
        if (glob && NS_FAILED(xpc->InitClasses(mSafeJSContext, glob))) {
            glob = nsnull;
        }
    }
    if (mSafeJSContext && !glob) {
        
        
        JS_DestroyContext(mSafeJSContext);
        mSafeJSContext = nsnull;
    }

    
    mOwnSafeJSContext = mSafeJSContext;

    return mSafeJSContext;
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
        mExceptionManagerNotAvailable(false),
        mAutoRoots(nsnull)
#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
      , mWrappedNativeThreadsafetyReportDepth(0)
#endif
{
    MOZ_COUNT_CTOR(xpcPerThreadData);
    if (gLock) {
        MutexAutoLock lock(*gLock);
        mNextThread = gThreads;
        gThreads = this;
    }
}

void
XPCPerThreadData::Cleanup()
{
    MOZ_ASSERT(!mAutoRoots);
    NS_IF_RELEASE(mExceptionManager);
    NS_IF_RELEASE(mException);
    delete mJSContextStack;
    mJSContextStack = nsnull;

    if (mCallContext)
        mCallContext->SystemIsBeingShutDown();
}

XPCPerThreadData::~XPCPerThreadData()
{
    


    bool doDestroyLock = false;

    MOZ_COUNT_DTOR(xpcPerThreadData);

    Cleanup();

    
    if (gLock) {
        MutexAutoLock lock(*gLock);
        if (gThreads == this)
            gThreads = mNextThread;
        else {
            XPCPerThreadData* cur = gThreads;
            while (cur) {
                if (cur->mNextThread == this) {
                    cur->mNextThread = mNextThread;
                    break;
                }
                cur = cur->mNextThread;
            }
        }
        if (!gThreads)
            doDestroyLock = true;
    }

    if (gLock && doDestroyLock) {
        delete gLock;
        gLock = nsnull;
    }
}

static void
xpc_ThreadDataDtorCB(void* ptr)
{
    XPCPerThreadData* data = (XPCPerThreadData*) ptr;
    delete data;
}

void XPCPerThreadData::TraceJS(JSTracer *trc)
{
#ifdef XPC_TRACK_AUTOMARKINGPTR_STATS
    {
        static int maxLength = 0;
        int length = 0;
        for (AutoMarkingPtr* p = mAutoRoots; p; p = p->GetNext())
            length++;
        if (length > maxLength)
            maxLength = length;
        printf("XPC gc on thread %x with %d AutoMarkingPtrs (%d max so far)\n",
               this, length, maxLength);
    }
#endif

    if (mAutoRoots)
        mAutoRoots->TraceJSAll(trc);
}

void XPCPerThreadData::MarkAutoRootsAfterJSFinalize()
{
    if (mAutoRoots)
        mAutoRoots->MarkAfterJSFinalizeAll();
}


XPCPerThreadData*
XPCPerThreadData::GetDataImpl(JSContext *cx)
{
    XPCPerThreadData* data;

    if (!gLock) {
        gLock = new Mutex("XPCPerThreadData.gLock");
    }

    if (gTLSIndex == BAD_TLS_INDEX) {
        MutexAutoLock lock(*gLock);
        
        if (gTLSIndex == BAD_TLS_INDEX) {
            if (PR_FAILURE ==
                PR_NewThreadPrivateIndex(&gTLSIndex, xpc_ThreadDataDtorCB)) {
                NS_ERROR("PR_NewThreadPrivateIndex failed!");
                gTLSIndex = BAD_TLS_INDEX;
                return nsnull;
            }
        }
    }

    data = (XPCPerThreadData*) PR_GetThreadPrivate(gTLSIndex);
    if (!data) {
        data = new XPCPerThreadData();
        if (!data || !data->IsValid()) {
            NS_ERROR("new XPCPerThreadData() failed!");
            delete data;
            return nsnull;
        }
        if (PR_FAILURE == PR_SetThreadPrivate(gTLSIndex, data)) {
            NS_ERROR("PR_SetThreadPrivate failed!");
            delete data;
            return nsnull;
        }
    }

    if (cx && !sMainJSThread && NS_IsMainThread()) {
        sMainJSThread = js::GetOwnerThread(cx);

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

    if (gLock) {
        MutexAutoLock lock(*gLock);

        for (XPCPerThreadData* cur = gThreads; cur; cur = cur->mNextThread)
            count++;

        stacks = (XPCJSContextStack**) new XPCJSContextStack*[count] ;
        if (stacks) {
            i = 0;
            for (XPCPerThreadData* cur = gThreads; cur; cur = cur->mNextThread) {
                stacks[i++] = cur->mJSContextStack;
                cur->mJSContextStack = nsnull;
                cur->Cleanup();
            }
        }
    }

    if (stacks) {
        for (i = 0; i < count; i++)
            delete stacks[i];
        delete [] stacks;
    }

    if (gTLSIndex != BAD_TLS_INDEX)
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
    if (!data)
        return NS_ERROR_FAILURE;
    mStack = data->GetJSContextStack()->GetStack();
    if (mStack->IsEmpty())
        mStack = nsnull;
    else
        mPosition = mStack->Length() - 1;

    return NS_OK;
}

NS_IMETHODIMP
nsXPCJSContextStackIterator::Done(bool *aDone)
{
    *aDone = !mStack;
    return NS_OK;
}

NS_IMETHODIMP
nsXPCJSContextStackIterator::Prev(JSContext **aContext)
{
    if (!mStack)
        return NS_ERROR_NOT_INITIALIZED;

    *aContext = mStack->ElementAt(mPosition).cx;

    if (mPosition == 0)
        mStack = nsnull;
    else
        --mPosition;

    return NS_OK;
}

