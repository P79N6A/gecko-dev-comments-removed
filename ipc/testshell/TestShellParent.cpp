



#include "TestShellParent.h"


#include "jsfriendapi.h"
#include "mozilla/ArrayUtils.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ScriptSettings.h"

#include "nsAutoPtr.h"
#include "xpcpublic.h"

using namespace mozilla;
using mozilla::ipc::TestShellParent;
using mozilla::ipc::TestShellCommandParent;
using mozilla::ipc::PTestShellCommandParent;
using mozilla::dom::ContentParent;

void
TestShellParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

PTestShellCommandParent*
TestShellParent::AllocPTestShellCommandParent(const nsString& aCommand)
{
  return new TestShellCommandParent();
}

bool
TestShellParent::DeallocPTestShellCommandParent(PTestShellCommandParent* aActor)
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

bool
TestShellCommandParent::SetCallback(JSContext* aCx,
                                    JS::Value aCallback)
{
  if (!mCallback.initialized()) {
    mCallback.init(aCx, aCallback);
    return true;
  }

  mCallback = aCallback;

  return true;
}

bool
TestShellCommandParent::RunCallback(const nsString& aResponse)
{
  NS_ENSURE_TRUE(mCallback.isObject(), false);

  
  
  dom::AutoEntryScript aes(
      xpc::NativeGlobal(js::GetGlobalForObjectCrossCompartment(&mCallback.toObject())),
      "TestShellCommand");
  JSContext* cx = aes.cx();
  JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));

  JSString* str = JS_NewUCStringCopyN(cx, aResponse.get(), aResponse.Length());
  NS_ENSURE_TRUE(str, false);

  JS::Rooted<JS::Value> strVal(cx, JS::StringValue(str));

  JS::Rooted<JS::Value> rval(cx);
  JS::Rooted<JS::Value> callback(cx, mCallback);
  bool ok = JS_CallFunctionValue(cx, global, callback, JS::HandleValueArray(strVal), &rval);
  NS_ENSURE_TRUE(ok, false);

  return true;
}

void
TestShellCommandParent::ReleaseCallback()
{
  mCallback.reset();
}

bool
TestShellCommandParent::ExecuteCallback(const nsString& aResponse)
{
  return static_cast<TestShellParent*>(Manager())->CommandDone(
      this, aResponse);
}

void
TestShellCommandParent::ActorDestroy(ActorDestroyReason why)
{
  if (why == AbnormalShutdown) {
    ExecuteCallback(EmptyString());
  }
}
