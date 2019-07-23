



































#ifndef _IPC_TESTSHELL_TESTSHELLCHILD_H_
#define _IPC_TESTSHELL_TESTSHELLCHILD_H_

#include "mozilla/ipc/TestShellProtocolChild.h"
#include "mozilla/ipc/TestShellCommandProtocolChild.h"

namespace mozilla {
namespace ipc {

class XPCShellEnvironment;

class TestShellChild : public TestShellProtocolChild
{
public:
  TestShellChild();
  ~TestShellChild();

  nsresult
  RecvExecuteCommand(const nsString& aCommand);

  TestShellCommandProtocolChild*
  TestShellCommandConstructor(const nsString& aCommand);

  nsresult
  RecvTestShellCommandConstructor(TestShellCommandProtocolChild* aActor,
                                  const nsString& aCommand);

  nsresult
  TestShellCommandDestructor(TestShellCommandProtocolChild* aCommand,
                             const nsString& aResponse);

  void SetXPCShell(XPCShellEnvironment* aXPCShell) {
    mXPCShell = aXPCShell;
  }

private:
  XPCShellEnvironment* mXPCShell;
};

} 
} 

#endif 
