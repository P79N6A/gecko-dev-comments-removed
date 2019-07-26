





#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/Assertions.h"

#include "jsapi.h"
#include "nsIGlobalObject.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"
#include "nsTArray.h"

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

  nsIGlobalObject* Incumbent() {
    if (!mStack.Length()) {
      return nullptr;
    }
    return mStack.LastElement()->mGlobalObject;
  }

  nsIGlobalObject* EntryPoint() {
    if (!mStack.Length())
      return nullptr;
    for (int i = mStack.Length() - 1; i >= 0; --i) {
      if (mStack[i]->mIsCandidateEntryPoint) {
        return mStack[i]->mGlobalObject;
      }
    }
    MOZ_ASSUME_UNREACHABLE("Non-empty stack should always have an entry point");
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
    mCxPusher.Push(aCx);
  }
  mAc.construct(aCx, aGlobalObject->GetGlobalJSObject());
  mStack.Push(&mEntry);
}

AutoEntryScript::~AutoEntryScript()
{
  MOZ_ASSERT(mStack.Incumbent() == mEntry.mGlobalObject);
  mStack.Pop();
}

AutoIncumbentScript::AutoIncumbentScript(nsIGlobalObject* aGlobalObject)
  : mStack(ScriptSettingsStack::Ref())
  , mEntry(aGlobalObject,  false)
{
  mStack.Push(&mEntry);
}

AutoIncumbentScript::~AutoIncumbentScript()
{
  MOZ_ASSERT(mStack.Incumbent() == mEntry.mGlobalObject);
  mStack.Pop();
}

AutoSystemCaller::AutoSystemCaller(bool aIsMainThread)
  : mStack(ScriptSettingsStack::Ref())
{
  if (aIsMainThread) {
    mCxPusher.PushNull();
  }
  mStack.PushSystem();
}

AutoSystemCaller::~AutoSystemCaller()
{
  mStack.Pop();
}

} 
} 
