






#ifndef ipc_testshell_TestShellParent_h
#define ipc_testshell_TestShellParent_h 1

#include "mozilla/ipc/PTestShellParent.h"
#include "mozilla/ipc/PTestShellCommandParent.h"

#include "js/TypeDecls.h"
#include "nsAutoJSValHolder.h"
#include "nsString.h"

namespace mozilla {

namespace ipc {

class TestShellCommandParent;

class TestShellParent : public PTestShellParent
{
public:
  PTestShellCommandParent*
  AllocPTestShellCommandParent(const nsString& aCommand);

  bool
  DeallocPTestShellCommandParent(PTestShellCommandParent* aActor);

  bool
  CommandDone(TestShellCommandParent* aActor, const nsString& aResponse);
};


class TestShellCommandParent : public PTestShellCommandParent
{
public:
  TestShellCommandParent() : mCx(nullptr) { }

  bool SetCallback(JSContext* aCx, JS::Value aCallback);

  bool RunCallback(const nsString& aResponse);

  void ReleaseCallback();

protected:
  bool ExecuteCallback(const nsString& aResponse);

  void ActorDestroy(ActorDestroyReason why);

  bool Recv__delete__(const nsString& aResponse) {
    return ExecuteCallback(aResponse);
  }

private:
  JSContext* mCx;
  nsAutoJSValHolder mCallback;
};


} 
} 

#endif 
