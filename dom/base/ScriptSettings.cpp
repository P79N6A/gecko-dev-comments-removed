





#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/Assertions.h"

#include "jsapi.h"
#include "xpcpublic.h"
#include "nsIGlobalObject.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"
#include "nsTArray.h"
#include "nsJSUtils.h"

namespace mozilla {
namespace dom {

class ScriptSettingsStack;
static mozilla::ThreadLocal<ScriptSettingsStack*> sScriptSettingsTLS;

ScriptSettingsStackEntry ScriptSettingsStackEntry::NoJSAPISingleton;

class ScriptSettingsStack {
public:
  static ScriptSettingsStack& Ref() {
    return *sScriptSettingsTLS.get();
  }
  ScriptSettingsStack() {};

  void Push(ScriptSettingsStackEntry* aSettings) {
    
    MOZ_ASSERT_IF(mStack.Length() == 0 || mStack.LastElement()->NoJSAPI(),
                  aSettings->mIsCandidateEntryPoint);
    mStack.AppendElement(aSettings);
  }

  void PushNoJSAPI() {
    mStack.AppendElement(&ScriptSettingsStackEntry::NoJSAPISingleton);
  }

  void Pop() {
    MOZ_ASSERT(mStack.Length() > 0);
    mStack.RemoveElementAt(mStack.Length() - 1);
  }

  ScriptSettingsStackEntry* Incumbent() {
    if (!mStack.Length()) {
      return nullptr;
    }
    return mStack.LastElement();
  }

  nsIGlobalObject* IncumbentGlobal() {
    ScriptSettingsStackEntry *entry = Incumbent();
    return entry ? entry->mGlobalObject : nullptr;
  }

  ScriptSettingsStackEntry* EntryPoint() {
    if (!mStack.Length())
      return nullptr;
    for (int i = mStack.Length() - 1; i >= 0; --i) {
      if (mStack[i]->mIsCandidateEntryPoint) {
        return mStack[i];
      }
    }
    MOZ_CRASH("Non-empty stack should always have an entry point");
  }

  nsIGlobalObject* EntryGlobal() {
    ScriptSettingsStackEntry *entry = EntryPoint();
    return entry ? entry->mGlobalObject : nullptr;
  }

private:
  
  nsTArray<ScriptSettingsStackEntry*> mStack;
};

void
InitScriptSettings()
{
  if (!sScriptSettingsTLS.initialized()) {
    bool success = sScriptSettingsTLS.init();
    if (!success) {
      MOZ_CRASH();
    }
  }

  ScriptSettingsStack* ptr = new ScriptSettingsStack();
  sScriptSettingsTLS.set(ptr);
}

void DestroyScriptSettings()
{
  ScriptSettingsStack* ptr = sScriptSettingsTLS.get();
  MOZ_ASSERT(ptr);
  sScriptSettingsTLS.set(nullptr);
  delete ptr;
}




nsIGlobalObject*
BrokenGetEntryGlobal()
{
  
  
  
  
  JSContext *cx = nsContentUtils::GetCurrentJSContextForThread();
  if (!cx) {
    MOZ_ASSERT(ScriptSettingsStack::Ref().EntryGlobal() == nullptr);
    return nullptr;
  }

  return nsJSUtils::GetDynamicScriptGlobal(cx);
}




nsIGlobalObject*
GetIncumbentGlobal()
{
  
  
  
  
  
  JSContext *cx = nsContentUtils::GetCurrentJSContextForThread();
  if (!cx) {
    MOZ_ASSERT(ScriptSettingsStack::Ref().EntryGlobal() == nullptr);
    return nullptr;
  }

  
  
  
  
  if (JSObject *global = JS::GetScriptedCallerGlobal(cx)) {
    return xpc::GetNativeForGlobal(global);
  }

  
  
  return ScriptSettingsStack::Ref().IncumbentGlobal();
}

nsIPrincipal*
GetWebIDLCallerPrincipal()
{
  MOZ_ASSERT(NS_IsMainThread());
  ScriptSettingsStackEntry *entry = ScriptSettingsStack::Ref().EntryPoint();

  
  
  if (!entry || entry->NoJSAPI()) {
    return nullptr;
  }
  AutoEntryScript* aes = static_cast<AutoEntryScript*>(entry);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!aes->CxPusherIsStackTop()) {
    return nullptr;
  }

  return aes->mWebIDLCallerPrincipal;
}

static JSContext*
FindJSContext(nsIGlobalObject* aGlobalObject)
{
  MOZ_ASSERT(NS_IsMainThread());
  JSContext *cx = nullptr;
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aGlobalObject);
  if (sgo && sgo->GetScriptContext()) {
    cx = sgo->GetScriptContext()->GetNativeContext();
  }
  if (!cx) {
    cx = nsContentUtils::GetSafeJSContext();
  }
  return cx;
}

AutoJSAPI::AutoJSAPI()
  : mCx(nsContentUtils::GetDefaultJSContextForThread())
{
  if (NS_IsMainThread()) {
    mCxPusher.construct(mCx);
  }

  
  mNullAc.construct(mCx);
}

AutoJSAPI::AutoJSAPI(JSContext *aCx, bool aIsMainThread, bool aSkipNullAc)
  : mCx(aCx)
{
  MOZ_ASSERT_IF(aIsMainThread, NS_IsMainThread());
  if (aIsMainThread) {
    mCxPusher.construct(mCx);
  }

  
  
  if (!aSkipNullAc) {
    mNullAc.construct(mCx);
  }
}

AutoJSAPIWithErrorsReportedToWindow::AutoJSAPIWithErrorsReportedToWindow(nsIScriptContext* aScx)
  : AutoJSAPI(aScx->GetNativeContext(),  true)
{
}

AutoEntryScript::AutoEntryScript(nsIGlobalObject* aGlobalObject,
                                 bool aIsMainThread,
                                 JSContext* aCx)
  : AutoJSAPI(aCx ? aCx : FindJSContext(aGlobalObject), aIsMainThread,  true)
  , ScriptSettingsStackEntry(aGlobalObject,  true)
  , mAc(cx(), aGlobalObject->GetGlobalJSObject())
  , mStack(ScriptSettingsStack::Ref())
{
  MOZ_ASSERT(aGlobalObject);
  MOZ_ASSERT_IF(!aCx, aIsMainThread); 
  MOZ_ASSERT_IF(aCx && aIsMainThread, aCx == FindJSContext(aGlobalObject));
  mStack.Push(this);
}

AutoEntryScript::~AutoEntryScript()
{
  MOZ_ASSERT(mStack.Incumbent() == this);
  mStack.Pop();
}

AutoIncumbentScript::AutoIncumbentScript(nsIGlobalObject* aGlobalObject)
  : ScriptSettingsStackEntry(aGlobalObject,  false)
  , mStack(ScriptSettingsStack::Ref())
  , mCallerOverride(nsContentUtils::GetCurrentJSContextForThread())
{
  mStack.Push(this);
}

AutoIncumbentScript::~AutoIncumbentScript()
{
  MOZ_ASSERT(mStack.Incumbent() == this);
  mStack.Pop();
}

AutoNoJSAPI::AutoNoJSAPI(bool aIsMainThread)
  : mStack(ScriptSettingsStack::Ref())
{
  MOZ_ASSERT_IF(nsContentUtils::GetCurrentJSContextForThread(),
                !JS_IsExceptionPending(nsContentUtils::GetCurrentJSContextForThread()));
  if (aIsMainThread) {
    mCxPusher.construct(static_cast<JSContext*>(nullptr),
                         true);
  }
  mStack.PushNoJSAPI();
}

AutoNoJSAPI::~AutoNoJSAPI()
{
  mStack.Pop();
}

} 
} 
