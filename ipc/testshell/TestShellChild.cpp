



































#include "TestShellChild.h"

#include "XPCShellEnvironment.h"

using mozilla::ipc::TestShellChild;
using mozilla::ipc::XPCShellEnvironment;

TestShellChild::TestShellChild()
: mXPCShell(nsnull)
{

}

TestShellChild::~TestShellChild()
{

}

nsresult
TestShellChild::AnswerSendCommand(const String& aCommand)
{
  nsresult rv = mXPCShell->EvaluateString(aCommand) ? NS_OK : NS_ERROR_FAILURE;

  if (mXPCShell->IsQuitting()) {
    MessageLoop::current()->Quit();
  }

  return rv;
}
