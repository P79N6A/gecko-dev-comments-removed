



































#ifndef _IPC_TESTSHELL_XPCSHELLENVIRONMENT_H_
#define _IPC_TESTSHELL_XPCSHELLENVIRONMENT_H_

#include "base/basictypes.h"

#include <string>
#include <stdio.h>
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsAutoJSValHolder.h"

#include "mozilla/ipc/TestShellProtocol.h"

struct JSContext;
struct JSObject;
struct JSPrincipals;

class nsIJSContextStack;

namespace mozilla {
namespace ipc {

class TestShellChild;
class TestShellParent;

class XPCShellEnvironment
{
public:
    static XPCShellEnvironment* CreateEnvironment();
    static void DestroyEnvironment(XPCShellEnvironment* aEnv);

    void Process(const char* aFilename = nsnull,
                 JSBool aIsInteractive = JS_FALSE);

    bool DefineIPCCommands(TestShellChild* aChild);
    bool DefineIPCCommands(TestShellParent* aParent);

    JSBool DoSendCommand(const nsString& aCommand,
                         nsString* aResult = nsnull);

    bool EvaluateString(const nsString& aString,
                        nsString* aResult = nsnull);

    JSPrincipals* GetPrincipal() {
        return mJSPrincipals;
    }

    JSObject* GetGlobalObject() {
        return mGlobalHolder.ToJSObject();
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

    void SetShouldReportWarnings(JSBool aReportWarnings) {
        mReportWarnings = aReportWarnings;
    }
    JSBool ShouldReportWarnings() {
        return mReportWarnings;
    }

    void SetShouldCompoleOnly(JSBool aCompileOnly) {
        mCompileOnly = aCompileOnly;
    }
    JSBool ShouldCompileOnly() {
        return mCompileOnly;
    }

protected:
    XPCShellEnvironment();
    ~XPCShellEnvironment();

    bool Init();

private:
    JSContext* mCx;
    nsAutoJSValHolder mGlobalHolder;
    nsCOMPtr<nsIJSContextStack> mCxStack;
    JSPrincipals* mJSPrincipals;

    int mExitCode;
    JSBool mQuitting;
    JSBool mReportWarnings;
    JSBool mCompileOnly;

    TestShellParent* mParent;
};

} 
} 

#endif 