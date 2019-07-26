



#ifndef __SANDBOXPRIVATE_H__
#define __SANDBOXPRIVATE_H__

#include "nsIGlobalObject.h"
#include "nsIPrincipal.h"
#include "nsWeakReference.h"




class SandboxPrivate : public nsIGlobalObject,
                       public nsSupportsWeakReference
{
public:
    SandboxPrivate(nsIPrincipal *principal, JSObject *global)
        : mPrincipal(principal)
        , mGlobalJSObject(global)
    {
    }
    virtual ~SandboxPrivate() { }

    NS_DECL_THREADSAFE_ISUPPORTS

    nsIPrincipal *GetPrincipal()
    {
        return mPrincipal;
    }

    JSObject *GetGlobalJSObject()
    {
        return mGlobalJSObject;
    }

    void ForgetGlobalObject()
    {
        mGlobalJSObject = NULL;
    }
private:
    nsCOMPtr<nsIPrincipal> mPrincipal;
    JSObject *mGlobalJSObject;
};

#endif 
