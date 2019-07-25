



































#include "TestShellParent.h"
#include "mozilla/jsipc/ContextWrapperParent.h"

#include "nsAutoPtr.h"

using mozilla::ipc::TestShellParent;
using mozilla::ipc::TestShellCommandParent;
using mozilla::ipc::PTestShellCommandParent;
using mozilla::jsipc::PContextWrapperParent;
using mozilla::jsipc::ContextWrapperParent;

PTestShellCommandParent*
TestShellParent::AllocPTestShellCommand(const nsString& aCommand)
{
  return new TestShellCommandParent();
}

bool
TestShellParent::DeallocPTestShellCommand(PTestShellCommandParent* aActor)
{
  delete aActor;
  return true;
}

bool
TestShellParent::CommandDone(TestShellCommandParent* command,
                             const nsString& aResponse)
{
  
  command->RunCallback(aResponse);
  command->ReleaseCallback();

  return true;
}

PContextWrapperParent*
TestShellParent::AllocPContextWrapper()
{
    return new ContextWrapperParent();
}

bool
TestShellParent::DeallocPContextWrapper(PContextWrapperParent* actor)
{
    delete actor;
    return true;
}

bool
TestShellParent::GetGlobalJSObject(JSContext* cx, JSObject** globalp)
{
    
    nsTArray<PContextWrapperParent*> cwps(1);
    ManagedPContextWrapperParent(cwps);
    if (cwps.Length() < 1)
        return false;
    NS_ASSERTION(cwps.Length() == 1, "More than one PContextWrapper?");
    ContextWrapperParent* cwp = static_cast<ContextWrapperParent*>(cwps[0]);
    return (cwp->GetGlobalJSObject(cx, globalp));
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
