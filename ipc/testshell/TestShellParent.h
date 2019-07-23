



































#ifndef _IPC_TESTSHELL_TESTSHELLPARENT_H_
#define _IPC_TESTSHELL_TESTSHELLPARENT_H_

#include "mozilla/ipc/TestShellProtocolParent.h"
#include "mozilla/ipc/TestShellCommandProtocolParent.h"

#include "jsapi.h"
#include "nsAutoJSValHolder.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace ipc {

class XPCShellEnvironment;

class TestShellCommandParent : public TestShellCommandProtocolParent
{
public:
  TestShellCommandParent() : mCx(NULL) { }

  JSBool SetCallback(JSContext* aCx,
                     jsval aCallback);

  JSBool RunCallback(const nsString& aResponse);

  void ReleaseCallback();

private:
  JSContext* mCx;
  nsAutoJSValHolder mCallback;
};

class TestShellParent : public TestShellProtocolParent
{
public:
  TestShellParent() : mXPCShell(nsnull) { }

  void
  SetXPCShell(XPCShellEnvironment* aXPCShell) {
    mXPCShell = aXPCShell;
  }

  TestShellCommandProtocolParent*
  TestShellCommandConstructor(const nsString& aCommand);

  nsresult
  TestShellCommandDestructor(TestShellCommandProtocolParent* aActor,
                             const nsString& aResponse);

  nsresult
  RecvTestShellCommandDestructor(TestShellCommandProtocolParent* aActor,
                                 const nsString& aResponse);

private:
  XPCShellEnvironment* mXPCShell;
};

} 
} 

#endif 
