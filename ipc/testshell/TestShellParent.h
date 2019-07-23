



































#ifndef ipc_testshell_TestShellParent_h
#define ipc_testshell_TestShellParent_h 1

#include "mozilla/ipc/PTestShellProtocolParent.h"
#include "mozilla/ipc/PTestShellCommandProtocolParent.h"

#include "jsapi.h"
#include "nsAutoJSValHolder.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace ipc {

class TestShellCommandParent : public PTestShellCommandProtocolParent
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

class TestShellParent : public PTestShellProtocolParent
{
public:
  PTestShellCommandProtocolParent*
  PTestShellCommandConstructor(const nsString& aCommand);

  nsresult
  PTestShellCommandDestructor(PTestShellCommandProtocolParent* aActor,
                              const nsString& aResponse);

  nsresult
  RecvPTestShellCommandDestructor(PTestShellCommandProtocolParent* aActor,
                                  const nsString& aResponse);
};

} 
} 

#endif 
