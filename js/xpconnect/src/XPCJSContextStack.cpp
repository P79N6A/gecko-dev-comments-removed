







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

JSContext*
XPCJSContextStack::GetSafeJSContext()
{
    MOZ_ASSERT(mSafeJSContext);
    return mSafeJSContext;
}

JSContext*
XPCJSContextStack::InitSafeJSContext()
{
    MOZ_ASSERT(!mSafeJSContext);
    mSafeJSContext = JS_NewContext(XPCJSRuntime::Get()->Runtime(), 8192);
    if (!mSafeJSContext)
        MOZ_CRASH();
    return mSafeJSContext;
}
