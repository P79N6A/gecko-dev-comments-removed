





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

ScriptSettingsStackEntry ScriptSettingsStackEntry::SystemSingleton;

class ScriptSettingsStack {
public:
  static ScriptSettingsStack& Ref() {
    return *sScriptSettingsTLS.get();
  }
  ScriptSettingsStack() {};

  void Push(ScriptSettingsStackEntry* aSettings) {
    
    MOZ_ASSERT_IF(mStack.Length() == 0 || mStack.LastElement()->IsSystemSingleton(),
                  aSettings->mIsCandidateEntryPoint);
    mStack.AppendElement(aSettings);
  }

  void PushSystem() {
    mStack.AppendElement(&ScriptSettingsStackEntry::SystemSingleton);
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
    MOZ_ASSUME_UNREACHABLE("Non-empty stack should always have an entry point");
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

  
  
  
  
  JS::RootedScript script(cx);
  if (JS_DescribeScriptedCaller(cx, &script, nullptr)) {
    JS::RootedObject global(cx, JS_GetGlobalFromScript(script));
    MOZ_ASSERT(global);
    return xpc::GetNativeForGlobal(global);
  }

  
  
  return ScriptSettingsStack::Ref().IncumbentGlobal();
}

AutoEntryScript::AutoEntryScript(nsIGlobalObject* aGlobalObject,
                                 bool aIsMainThread,
                                 JSContext* aCx)
  : mStack(ScriptSettingsStack::Ref())
  , mEntry(aGlobalObject,  true)
{
  MOZ_ASSERT(aGlobalObject);
  if (!aCx) {
    
    
    
    
    MOZ_ASSERT(aIsMainThread, "cx is mandatory off-main-thread");
    nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aGlobalObject);
    if (sgo && sgo->GetScriptContext()) {
      aCx = sgo->GetScriptContext()->GetNativeContext();
    }
    if (!aCx) {
      aCx = nsContentUtils::GetSafeJSContext();
    }
  }
  if (aIsMainThread) {
    mCxPusher.construct(aCx);
  }
  mAc.construct(aCx, aGlobalObject->GetGlobalJSObject());
  mStack.Push(&mEntry);
}

AutoEntryScript::~AutoEntryScript()
{
  MOZ_ASSERT(mStack.Incumbent() == &mEntry);
  mStack.Pop();
}

AutoIncumbentScript::AutoIncumbentScript(nsIGlobalObject* aGlobalObject)
  : mStack(ScriptSettingsStack::Ref())
  , mEntry(aGlobalObject,  false)
  , mCallerOverride(nsContentUtils::GetCurrentJSContextForThread())
{
  mStack.Push(&mEntry);
}

AutoIncumbentScript::~AutoIncumbentScript()
{
  MOZ_ASSERT(mStack.Incumbent() == &mEntry);
  mStack.Pop();
}

AutoSystemCaller::AutoSystemCaller(bool aIsMainThread)
  : mStack(ScriptSettingsStack::Ref())
{
  if (aIsMainThread) {
    mCxPusher.construct(nullptr,  true);
  }
  mStack.PushSystem();
}

AutoSystemCaller::~AutoSystemCaller()
{
  mStack.Pop();
}

} 
} 
