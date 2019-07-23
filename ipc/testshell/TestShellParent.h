



































#ifndef ipc_testshell_TestShellParent_h
#define ipc_testshell_TestShellParent_h 1

#include "mozilla/ipc/PTestShellParent.h"
#include "mozilla/ipc/PTestShellCommandParent.h"

#include "jsapi.h"
#include "nsAutoJSValHolder.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace ipc {

class TestShellCommandParent : public PTestShellCommandParent
{
public:
  TestShellCommandParent() : mCx(NULL) { }

  JSBool SetCallback(JSContext* aCx,
                     jsval aCallback);

  JSBool RunCallback(const nsString& aResponse);

  void ReleaseCallback();

private:
  JSContext* mCx;
  nsAutoJSValHolder mCallback;
};

class TestShellParent : public PTestShellParent
{
public:
  PTestShellCommandParent*
  PTestShellCommandConstructor(const nsString& aCommand);

  bool
  PTestShellCommandDestructor(PTestShellCommandParent* aActor,
                              const nsString& aResponse);

  bool
  RecvPTestShellCommandDestructor(PTestShellCommandParent* aActor,
                                  const nsString& aResponse);
};

} 
} 

#endif 
