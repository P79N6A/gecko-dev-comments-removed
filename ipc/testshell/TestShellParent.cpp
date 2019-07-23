



































#include "TestShellParent.h"

#include "nsAutoPtr.h"

using mozilla::ipc::TestShellParent;
using mozilla::ipc::TestShellCommandParent;
using mozilla::ipc::PTestShellCommandParent;

PTestShellCommandParent*
TestShellParent::PTestShellCommandConstructor(const nsString& aCommand)
{
  return new TestShellCommandParent();
}

bool
TestShellParent::PTestShellCommandDestructor(PTestShellCommandParent* aActor,
                                             const nsString& aResponse)
{
  delete aActor;
  return true;
}

bool
TestShellParent::RecvPTestShellCommandDestructor(PTestShellCommandParent* aActor,
                                                 const nsString& aResponse)
{
  TestShellCommandParent* command =
    reinterpret_cast<TestShellCommandParent*>(aActor);

  JSBool ok = command->RunCallback(aResponse);
  command->ReleaseCallback();

  return true;
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
