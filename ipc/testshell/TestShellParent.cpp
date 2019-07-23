



































#include "TestShellParent.h"

#include "nsAutoPtr.h"
#include "XPCShellEnvironment.h"

using mozilla::ipc::TestShellParent;
using mozilla::ipc::TestShellCommandParent;
using mozilla::ipc::TestShellCommandProtocolParent;
using mozilla::ipc::XPCShellEnvironment;

TestShellCommandProtocolParent*
TestShellParent::TestShellCommandConstructor(const nsString& aCommand)
{
  return new TestShellCommandParent();
}

nsresult
TestShellParent::TestShellCommandDestructor(TestShellCommandProtocolParent* aActor,
                                            const nsString& aResponse)
{
  NS_ENSURE_ARG_POINTER(aActor);
  delete aActor;
  return NS_OK;
}

nsresult
TestShellParent::RecvTestShellCommandDestructor(TestShellCommandProtocolParent* aActor,
                                                const nsString& aResponse)
{
  NS_ENSURE_ARG_POINTER(aActor);

  TestShellCommandParent* command =
    static_cast<TestShellCommandParent*>(aActor);

  JSBool ok = command->RunCallback(aResponse);
  command->ReleaseCallback();

  if (!mXPCShell) {
    NS_WARNING("Processing a child message after exiting, need to spin events "
               "somehow to process this result");
    return NS_ERROR_UNEXPECTED;
  }

  NS_WARN_IF_FALSE(mXPCShell->EventLoopDepth(), "EventLoopDepth mismatch!");
  if (mXPCShell->EventLoopDepth()) {
    mXPCShell->DecrementEventLoopDepth();
  }

  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  return NS_OK;
}

JSBool
TestShellCommandParent::SetCallback(JSContext* aCx,
                                    jsval aCallback)
{
  if (!mCallback.Hold(aCx)) {
    return JS_FALSE;
  }

  mCallback = aCallback;
  mCx = aCx;

  return JS_TRUE;
}

JSBool
TestShellCommandParent::RunCallback(const nsString& aResponse)
{
  NS_ENSURE_TRUE(mCallback && mCx, JS_FALSE);

  JSAutoRequest ar(mCx);

  JSObject* global = JS_GetGlobalObject(mCx);
  NS_ENSURE_TRUE(global, JS_FALSE);

  JSString* str = JS_NewUCStringCopyN(mCx, aResponse.get(), aResponse.Length());
  NS_ENSURE_TRUE(str, JS_FALSE);

  jsval argv[] = { STRING_TO_JSVAL(str) };
  int argc = NS_ARRAY_LENGTH(argv);

  jsval rval;
  JSBool ok = JS_CallFunctionValue(mCx, global, mCallback, argc, argv, &rval);
  NS_ENSURE_TRUE(ok, JS_FALSE);

  return JS_TRUE;
}

void
TestShellCommandParent::ReleaseCallback()
{
  mCallback.Release();
}
