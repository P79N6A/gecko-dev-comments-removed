







#ifndef mozilla_dom_ScriptSettings_h
#define mozilla_dom_ScriptSettings_h

#include "nsCxPusher.h"
#include "MainThreadUtils.h"
#include "nsIGlobalObject.h"

#include "mozilla/Maybe.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {





void InitScriptSettings();
void DestroyScriptSettings();

class ScriptSettingsStack;
struct ScriptSettingsStackEntry {
  nsCOMPtr<nsIGlobalObject> mGlobalObject;
  bool mIsCandidateEntryPoint;

  ScriptSettingsStackEntry(nsIGlobalObject *aGlobal, bool aCandidate)
    : mGlobalObject(aGlobal)
    , mIsCandidateEntryPoint(aCandidate)
  {
    MOZ_ASSERT(mGlobalObject);
    MOZ_ASSERT(mGlobalObject->GetGlobalJSObject(),
               "Must have an actual JS global for the duration on the stack");
    MOZ_ASSERT(JS_IsGlobalObject(mGlobalObject->GetGlobalJSObject()),
               "No outer windows allowed");
  }

  ~ScriptSettingsStackEntry() {
    
    MOZ_ASSERT_IF(mGlobalObject, mGlobalObject->GetGlobalJSObject());
  }

  bool IsSystemSingleton() { return this == &SystemSingleton; }
  static ScriptSettingsStackEntry SystemSingleton;

private:
  ScriptSettingsStackEntry() : mGlobalObject(nullptr)
                             , mIsCandidateEntryPoint(true)
  {}
};




class AutoEntryScript {
public:
  AutoEntryScript(nsIGlobalObject* aGlobalObject,
                  bool aIsMainThread = NS_IsMainThread(),
                  
                  JSContext* aCx = nullptr);
  ~AutoEntryScript();

private:
  dom::ScriptSettingsStack& mStack;
  dom::ScriptSettingsStackEntry mEntry;
  nsCxPusher mCxPusher;
  mozilla::Maybe<JSAutoCompartment> mAc; 
                                         
};




class AutoIncumbentScript {
public:
  AutoIncumbentScript(nsIGlobalObject* aGlobalObject);
  ~AutoIncumbentScript();
private:
  dom::ScriptSettingsStack& mStack;
  dom::ScriptSettingsStackEntry mEntry;
};






class AutoSystemCaller {
public:
  AutoSystemCaller(bool aIsMainThread = NS_IsMainThread());
  ~AutoSystemCaller();
private:
  dom::ScriptSettingsStack& mStack;
  nsCxPusher mCxPusher;
};

} 
} 

#endif 
