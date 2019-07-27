





#ifndef __SANDBOXPRIVATE_H__
#define __SANDBOXPRIVATE_H__

#include "nsIGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIPrincipal.h"
#include "nsWeakReference.h"
#include "nsWrapperCache.h"

#include "js/RootingAPI.h"




class SandboxPrivate : public nsIGlobalObject,
                       public nsIScriptObjectPrincipal,
                       public nsSupportsWeakReference,
                       public nsWrapperCache
{
public:
    SandboxPrivate(nsIPrincipal* principal, JSObject* global)
        : mPrincipal(principal)
    {
        SetIsNotDOMBinding();
        SetWrapper(global);
    }

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(SandboxPrivate,
                                                           nsIGlobalObject)

    nsIPrincipal* GetPrincipal() override
    {
        return mPrincipal;
    }

    JSObject* GetGlobalJSObject() override
    {
        return GetWrapper();
    }

    void ForgetGlobalObject()
    {
        ClearWrapper();
    }

    virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override
    {
        MOZ_CRASH("SandboxPrivate doesn't use DOM bindings!");
    }

    void ObjectMoved(JSObject* obj, const JSObject* old)
    {
        UpdateWrapper(obj, old);
    }

private:
    virtual ~SandboxPrivate() { }

    nsCOMPtr<nsIPrincipal> mPrincipal;
};

#endif 
