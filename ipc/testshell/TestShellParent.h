






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
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

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
  TestShellCommandParent() {}

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
  nsAutoJSValHolder mCallback;
};


} 
} 

#endif 
