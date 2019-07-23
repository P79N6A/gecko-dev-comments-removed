



































#include "TestShellChild.h"

#include "XPCShellEnvironment.h"

using mozilla::ipc::TestShellChild;
using mozilla::ipc::XPCShellEnvironment;

TestShellChild::TestShellChild()
  : mXPCShell(XPCShellEnvironment::CreateEnvironment())
{

}

TestShellChild::~TestShellChild()
{

}

nsresult
TestShellChild::RecvSendCommand(const nsString& aCommand)
{
  if (mXPCShell->IsQuitting()) {
    NS_WARNING("Commands sent after quit command issued!");
    return NS_ERROR_UNEXPECTED;
  }

  return mXPCShell->EvaluateString(aCommand) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
TestShellChild::RecvSendCommandWithResponse(const nsString& aCommand,
                                            nsString* aResponse)
{
  if (mXPCShell->IsQuitting()) {
    NS_WARNING("Commands sent after quit command issued!");
    return NS_ERROR_UNEXPECTED;
  }

  return mXPCShell->EvaluateString(aCommand, aResponse) ?
         NS_OK :
         NS_ERROR_FAILURE;
}
