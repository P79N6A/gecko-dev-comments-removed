







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
        mSafeJSContextGlobal = nullptr;
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
    if (idx == 0) {
        js::Debug_SetActiveJSContext(mRuntime->Runtime(), nullptr);
        return cx;
    }

    --idx; 

    XPCJSContextInfo &e = mStack[idx];
    if (e.cx && e.savedFrameChain) {
        
        JSAutoRequest ar(e.cx);
        JS_RestoreFrameChain(e.cx);
        e.savedFrameChain = false;
    }
    js::Debug_SetActiveJSContext(mRuntime->Runtime(), e.cx);
    return cx;
}

bool
XPCJSContextStack::Push(JSContext *cx)
{
    js::Debug_SetActiveJSContext(mRuntime->Runtime(), cx);
    if (mStack.Length() == 0) {
        mStack.AppendElement(cx);
        return true;
    }

    XPCJSContextInfo &e = mStack[mStack.Length() - 1];
    if (e.cx) {
        
        
        
        if (e.cx == cx) {
            
            
            
            
            RootedObject defaultScope(cx, GetDefaultScopeFromJSContext(cx));
            if (defaultScope) {
                nsIPrincipal *currentPrincipal =
                  GetCompartmentPrincipal(js::GetContextCompartment(cx));
                nsIPrincipal *defaultPrincipal = GetObjectPrincipal(defaultScope);
                if (currentPrincipal->Equals(defaultPrincipal)) {
                    mStack.AppendElement(cx);
                    return true;
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
    nullptr, nullptr, nullptr, JS_GlobalObjectTraceHook
};

JSContext*
XPCJSContextStack::GetSafeJSContext()
{
    MOZ_ASSERT(mSafeJSContext);
    return mSafeJSContext;
}

JSObject*
XPCJSContextStack::GetSafeJSContextGlobal()
{
    MOZ_ASSERT(mSafeJSContextGlobal);
    return mSafeJSContextGlobal;
}

JSContext*
XPCJSContextStack::InitSafeJSContext()
{
    MOZ_ASSERT(!mSafeJSContext);

    
    
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
    ContextOptionsRef(mSafeJSContext).setNoDefaultCompartmentObject(true);

    JS_SetErrorReporter(mSafeJSContext, xpc::SystemErrorReporter);

    JS::CompartmentOptions options;
    options.setZone(JS::SystemZone)
           .setTrace(TraceXPCGlobal);
    mSafeJSContextGlobal = CreateGlobalObject(mSafeJSContext,
                                              &SafeJSContextGlobalClass,
                                              principal, options);
    if (!mSafeJSContextGlobal)
        MOZ_CRASH();

    
    
    nsRefPtr<SandboxPrivate> sp = new SandboxPrivate(principal, mSafeJSContextGlobal);
    JS_SetPrivate(mSafeJSContextGlobal, sp.forget().take());

    
    
    
    
    if (NS_FAILED(xpc->InitClasses(mSafeJSContext, mSafeJSContextGlobal)))
        MOZ_CRASH();

    JS::RootedObject glob(mSafeJSContext, mSafeJSContextGlobal);
    JS_FireOnNewGlobalObject(mSafeJSContext, glob);

    return mSafeJSContext;
}
