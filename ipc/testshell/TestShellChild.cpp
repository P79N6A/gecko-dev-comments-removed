



































#include "TestShellChild.h"

using mozilla::ipc::TestShellChild;
using mozilla::ipc::PTestShellCommandProtocolChild;
using mozilla::ipc::XPCShellEnvironment;

TestShellChild::TestShellChild()
: mXPCShell(XPCShellEnvironment::CreateEnvironment())
{
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

PTestShellCommandProtocolChild*
TestShellChild::PTestShellCommandConstructor(const nsString& aCommand)
{
  return new PTestShellCommandProtocolChild();
}

nsresult
TestShellChild::PTestShellCommandDestructor(PTestShellCommandProtocolChild* aCommand,
                                            const nsString& aResponse)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  delete aCommand;
  return NS_OK;
}

nsresult
TestShellChild::RecvPTestShellCommandConstructor(PTestShellCommandProtocolChild* aActor,
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

  nsresult rv = SendPTestShellCommandDestructor(aActor, response);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
