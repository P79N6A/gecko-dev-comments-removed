



































#include "TestShellChild.h"

#include "XPCShellEnvironment.h"

using mozilla::ipc::TestShellChild;
using mozilla::ipc::TestShellCommandProtocolChild;
using mozilla::ipc::XPCShellEnvironment;

TestShellChild::TestShellChild()
: mXPCShell(nsnull)
{
  XPCShellEnvironment* env = XPCShellEnvironment::CreateEnvironment();
  if (env) {
    if (env->DefineIPCCommands(this)) {
      mXPCShell = env;
    }
    else {
      XPCShellEnvironment::DestroyEnvironment(env);
    }
  }
}

TestShellChild::~TestShellChild()
{
  if (mXPCShell) {
    XPCShellEnvironment::DestroyEnvironment(mXPCShell);
  }
}

nsresult
TestShellChild::RecvExecuteCommand(const nsString& aCommand)
{
  if (mXPCShell->IsQuitting()) {
    NS_WARNING("Commands sent after quit command issued!");
    return NS_ERROR_UNEXPECTED;
  }

  return mXPCShell->EvaluateString(aCommand) ? NS_OK : NS_ERROR_FAILURE;
}

TestShellCommandProtocolChild*
TestShellChild::TestShellCommandConstructor(const nsString& aCommand)
{
  return new TestShellCommandProtocolChild();
}

nsresult
TestShellChild::TestShellCommandDestructor(TestShellCommandProtocolChild* aCommand,
                                           const nsString& aResponse)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  delete aCommand;
  return NS_OK;
}

nsresult
TestShellChild::RecvTestShellCommandConstructor(TestShellCommandProtocolChild* aActor,
                                                const nsString& aCommand)
{
  NS_ASSERTION(aActor, "Shouldn't be null!");

  if (mXPCShell->IsQuitting()) {
    NS_WARNING("Commands sent after quit command issued!");
    return NS_ERROR_UNEXPECTED;
  }

  nsString response;
  if (!mXPCShell->EvaluateString(aCommand, &response)) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = SendTestShellCommandDestructor(aActor, response);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
