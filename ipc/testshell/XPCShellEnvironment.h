



#ifndef _IPC_TESTSHELL_XPCSHELLENVIRONMENT_H_
#define _IPC_TESTSHELL_XPCSHELLENVIRONMENT_H_

#include "base/basictypes.h"

#include <string>
#include <stdio.h>

#include "nsAutoJSValHolder.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsStringGlue.h"

struct JSContext;
class JSObject;
struct JSPrincipals;

namespace mozilla {
namespace ipc {

class XPCShellEnvironment
{
public:
    static XPCShellEnvironment* CreateEnvironment();
    ~XPCShellEnvironment();

    bool EvaluateString(const nsString& aString,
                        nsString* aResult = nullptr);

    JSPrincipals* GetPrincipal() {
        return mJSPrincipals;
    }

    JSObject* GetGlobalObject() {
        return mGlobalHolder.ToJSObject();
    }

    JSContext* GetContext() {
        return mCx;
    }

    void SetExitCode(int aExitCode) {
        mExitCode = aExitCode;
    }
    int ExitCode() {
        return mExitCode;
    }

    void SetIsQuitting() {
        mQuitting = JS_TRUE;
    }
    JSBool IsQuitting() {
        return mQuitting;
    }

protected:
    XPCShellEnvironment();
    bool Init();

private:
    JSContext* mCx;
    nsAutoJSValHolder mGlobalHolder;
    JSPrincipals* mJSPrincipals;

    int mExitCode;
    JSBool mQuitting;
};

} 
} 

#endif 
