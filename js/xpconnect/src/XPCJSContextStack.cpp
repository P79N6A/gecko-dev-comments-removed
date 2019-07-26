








#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "mozilla/Mutex.h"
#include "nsDOMJSUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsNullPrincipal.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using mozilla::dom::DestroyProtoAndIfaceCache;



XPCJSContextStack::~XPCJSContextStack()
{
    if (mOwnSafeJSContext) {
        JS_DestroyContext(mOwnSafeJSContext);
        mOwnSafeJSContext = nullptr;
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
    if (e.cx && e.savedFrameChain) {
        
        JSAutoRequest ar(e.cx);
        JS_RestoreFrameChain(e.cx);
        e.savedFrameChain = false;
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
    return nullptr;
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
    }

    mStack.AppendElement(cx);
    return true;
}

#ifdef DEBUG
bool
XPCJSContextStack::DEBUG_StackHasJSContext(JSContext *cx)
{
    for (uint32_t i = 0; i < mStack.Length(); i++)
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
    DestroyProtoAndIfaceCache(obj);
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

        glob = xpc::CreateGlobalObject(mSafeJSContext, &global_class, principal, JS::SystemZone);

        if (glob) {
            
            
            JS_SetGlobalObject(mSafeJSContext, glob);

            
            
            nsIScriptObjectPrincipal* priv = nullptr;
            sop.swap(priv);
            JS_SetPrivate(glob, priv);
        }

        
        
        
        
        if (glob && NS_FAILED(xpc->InitClasses(mSafeJSContext, glob))) {
            glob = nullptr;
        }
    }
    if (mSafeJSContext && !glob) {
        
        
        JS_DestroyContext(mSafeJSContext);
        mSafeJSContext = nullptr;
    }

    
    mOwnSafeJSContext = mSafeJSContext;

    return mSafeJSContext;
}



NS_IMPL_ISUPPORTS1(nsXPCJSContextStackIterator, nsIJSContextStackIterator)

NS_IMETHODIMP
nsXPCJSContextStackIterator::Reset(nsIJSContextStack *aStack)
{
    NS_ASSERTION(aStack == nsXPConnect::GetXPConnect(),
                 "aStack must be implemented by XPConnect singleton");
    mStack = XPCJSRuntime::Get()->GetJSContextStack()->GetStack();
    if (mStack->IsEmpty())
        mStack = nullptr;
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
        mStack = nullptr;
    else
        --mPosition;

    return NS_OK;
}

