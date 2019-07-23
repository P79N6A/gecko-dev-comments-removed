



































#ifndef ipc_testshell_TestShellChild_h
#define ipc_testshell_TestShellChild_h 1

#include "mozilla/ipc/PTestShellChild.h"
#include "mozilla/ipc/PTestShellCommandChild.h"
#include "mozilla/ipc/XPCShellEnvironment.h"

#include "nsAutoPtr.h"

namespace mozilla {
namespace ipc {

class XPCShellEnvironment;

class TestShellChild : public PTestShellChild
{
public:
  TestShellChild();

  nsresult
  RecvExecuteCommand(const nsString& aCommand);

  PTestShellCommandChild*
  PTestShellCommandConstructor(const nsString& aCommand);

  nsresult
  RecvPTestShellCommandConstructor(PTestShellCommandChild* aActor,
                                   const nsString& aCommand);

  nsresult
  PTestShellCommandDestructor(PTestShellCommandChild* aCommand,
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
