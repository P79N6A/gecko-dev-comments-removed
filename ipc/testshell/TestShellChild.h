



































#ifndef ipc_testshell_TestShellChild_h
#define ipc_testshell_TestShellChild_h 1

#include "mozilla/ipc/PTestShellProtocolChild.h"
#include "mozilla/ipc/PTestShellCommandProtocolChild.h"
#include "mozilla/ipc/XPCShellEnvironment.h"

#include "nsAutoPtr.h"

namespace mozilla {
namespace ipc {

class XPCShellEnvironment;

class TestShellChild : public PTestShellProtocolChild
{
public:
  TestShellChild();

  nsresult
  RecvExecuteCommand(const nsString& aCommand);

  PTestShellCommandProtocolChild*
  PTestShellCommandConstructor(const nsString& aCommand);

  nsresult
  RecvPTestShellCommandConstructor(PTestShellCommandProtocolChild* aActor,
                                   const nsString& aCommand);

  nsresult
  PTestShellCommandDestructor(PTestShellCommandProtocolChild* aCommand,
                              const nsString& aResponse);

  void SetXPCShell(XPCShellEnvironment* aXPCShell) {
    mXPCShell = aXPCShell;
  }

private:
  nsAutoPtr<XPCShellEnvironment> mXPCShell;
};

} 
} 

#endif 
