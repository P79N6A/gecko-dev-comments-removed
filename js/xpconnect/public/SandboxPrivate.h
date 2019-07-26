





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
    SandboxPrivate(nsIPrincipal *principal, JSObject *global)
        : mPrincipal(principal)
    {
        SetWrapper(global);
    }

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(SandboxPrivate,
                                                           nsIGlobalObject)

    nsIPrincipal *GetPrincipal()
    {
        return mPrincipal;
    }

    JSObject *GetGlobalJSObject()
    {
        return GetWrapper();
    }

    void ForgetGlobalObject()
    {
        ClearWrapper();
    }

private:
    virtual ~SandboxPrivate() { }

    nsCOMPtr<nsIPrincipal> mPrincipal;
};

#endif 
