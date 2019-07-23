



































#ifndef _IPC_TESTSHELL_TESTSHELLCHILD_H_
#define _IPC_TESTSHELL_TESTSHELLCHILD_H_

#include "mozilla/ipc/TestShellProtocolChild.h"

namespace mozilla {
namespace ipc {

class XPCShellEnvironment;

class TestShellChild : public TestShellProtocolChild
{
public:
  typedef mozilla::ipc::String String;

  TestShellChild();
  virtual ~TestShellChild();

  virtual nsresult RecvSendCommand(const String& aCommand);
  virtual nsresult RecvSendCommandWithResponse(const String& aCommand,
                                               String* aResponse);

  void SetXPCShell(XPCShellEnvironment* aXPCShell) {
    mXPCShell = aXPCShell;
  }

private:
  XPCShellEnvironment* mXPCShell;
};

} 
} 

#endif 
