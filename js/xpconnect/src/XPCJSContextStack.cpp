








#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "nsDOMJSUtils.h"
#include "nsNullPrincipal.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace JS;
using namespace xpc;
using mozilla::dom::DestroyProtoAndIfaceCache;



XPCJSContextStack::~XPCJSContextStack()
{
    if (mSafeJSContext) {
        JS_DestroyContextNoGC(mSafeJSContext);
        mSafeJSContext = nullptr;
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

bool
XPCJSContextStack::Push(JSContext *cx)
{
    if (mStack.Length() == 0) {
        mStack.AppendElement(cx);
        return true;
    }

    XPCJSContextInfo &e = mStack[mStack.Length() - 1];
    if (e.cx) {
        
        
        
        nsIScriptSecurityManager* ssm = XPCWrapper::GetSecurityManager();
        if ((e.cx == cx) && ssm) {
            
            
            
            RootedObject defaultScope(cx, GetDefaultScopeFromJSContext(cx));

            nsIPrincipal *currentPrincipal =
              GetCompartmentPrincipal(js::GetContextCompartment(cx));
            nsIPrincipal *defaultPrincipal = GetObjectPrincipal(defaultScope);
            bool equal = false;
            currentPrincipal->Equals(defaultPrincipal, &equal);
            if (equal) {
                mStack.AppendElement(cx);
                return true;
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

bool
XPCJSContextStack::HasJSContext(JSContext *cx)
{
    for (uint32_t i = 0; i < mStack.Length(); i++)
        if (cx == mStack[i].cx)
            return true;
    return false;
}

static bool
SafeGlobalResolve(JSContext *cx, HandleObject obj, HandleId id)
{
    bool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static void
SafeFinalize(JSFreeOp *fop, JSObject* obj)
{
    SandboxPrivate* sop =
        static_cast<SandboxPrivate*>(xpc_GetJSPrivate(obj));
    sop->ForgetGlobalObject();
    NS_IF_RELEASE(sop);
    DestroyProtoAndIfaceCache(obj);
}

const JSClass xpc::SafeJSContextGlobalClass = {
    "global_for_XPCJSContextStack_SafeJSContext",
    XPCONNECT_GLOBAL_FLAGS,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, SafeGlobalResolve, JS_ConvertStub, SafeFinalize,
    nullptr, nullptr, nullptr, nullptr, TraceXPCGlobal
};

JSContext*
XPCJSContextStack::GetSafeJSContext()
{
    if (mSafeJSContext)
        return mSafeJSContext;

    
    
    nsRefPtr<nsNullPrincipal> principal = new nsNullPrincipal();
    nsresult rv = principal->Init();
    if (NS_FAILED(rv))
        MOZ_CRASH();

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    JSRuntime *rt = xpc->GetRuntime()->Runtime();
    if (!rt)
        MOZ_CRASH();

    mSafeJSContext = JS_NewContext(rt, 8192);
    if (!mSafeJSContext)
        MOZ_CRASH();
    JSAutoRequest req(mSafeJSContext);

    JS::RootedObject glob(mSafeJSContext);
    JS_SetErrorReporter(mSafeJSContext, xpc::SystemErrorReporter);

    JS::CompartmentOptions options;
    options.setZone(JS::SystemZone);
    glob = xpc::CreateGlobalObject(mSafeJSContext, &SafeJSContextGlobalClass, principal, options);
    if (!glob)
        MOZ_CRASH();

    
    
    js::SetDefaultObjectForContext(mSafeJSContext, glob);

    
    
    nsRefPtr<SandboxPrivate> sp = new SandboxPrivate(principal, glob);
    JS_SetPrivate(glob, sp.forget().get());

    
    
    
    
    if (NS_FAILED(xpc->InitClasses(mSafeJSContext, glob)))
        MOZ_CRASH();

    JS_FireOnNewGlobalObject(mSafeJSContext, glob);

    return mSafeJSContext;
}
