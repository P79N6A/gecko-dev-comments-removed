







#ifndef mozilla_dom_ScriptSettings_h
#define mozilla_dom_ScriptSettings_h

#include "nsCxPusher.h"
#include "MainThreadUtils.h"
#include "nsIGlobalObject.h"
#include "nsIPrincipal.h"

#include "mozilla/Maybe.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {





void InitScriptSettings();
void DestroyScriptSettings();




nsIGlobalObject* BrokenGetEntryGlobal();





nsIGlobalObject* GetIncumbentGlobal();

















nsIPrincipal* GetWebIDLCallerPrincipal();




inline JSObject& IncumbentJSGlobal()
{
  return *GetIncumbentGlobal()->GetGlobalJSObject();
}

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

  bool NoJSAPI() { return this == &NoJSAPISingleton; }
  static ScriptSettingsStackEntry NoJSAPISingleton;

private:
  ScriptSettingsStackEntry() : mGlobalObject(nullptr)
                             , mIsCandidateEntryPoint(true)
  {}
};

































class AutoJSAPI {
public:
  
  
  
  AutoJSAPI();
  JSContext* cx() const { return mCx; }

  bool CxPusherIsStackTop() { return mCxPusher.ref().IsStackTop(); }

protected:
  
  
  AutoJSAPI(JSContext *aCx, bool aIsMainThread, bool aSkipNullAC = false);

private:
  mozilla::Maybe<AutoCxPusher> mCxPusher;
  mozilla::Maybe<JSAutoNullCompartment> mNullAc;
  JSContext *mCx;
};








class AutoJSAPIWithErrorsReportedToWindow : public AutoJSAPI {
  public:
    AutoJSAPIWithErrorsReportedToWindow(nsIScriptContext* aScx);
    
    AutoJSAPIWithErrorsReportedToWindow(nsIGlobalObject* aGlobalObject);
};




class AutoEntryScript : public AutoJSAPI,
                        protected ScriptSettingsStackEntry {
public:
  AutoEntryScript(nsIGlobalObject* aGlobalObject,
                  bool aIsMainThread = NS_IsMainThread(),
                  
                  JSContext* aCx = nullptr);
  ~AutoEntryScript();

  void SetWebIDLCallerPrincipal(nsIPrincipal *aPrincipal) {
    mWebIDLCallerPrincipal = aPrincipal;
  }

private:
  JSAutoCompartment mAc;
  dom::ScriptSettingsStack& mStack;
  
  
  
  
  
  
  
  nsIPrincipal* mWebIDLCallerPrincipal;
  friend nsIPrincipal* GetWebIDLCallerPrincipal();
};




class AutoIncumbentScript : protected ScriptSettingsStackEntry {
public:
  AutoIncumbentScript(nsIGlobalObject* aGlobalObject);
  ~AutoIncumbentScript();
private:
  dom::ScriptSettingsStack& mStack;
  JS::AutoHideScriptedCaller mCallerOverride;
};









class AutoNoJSAPI {
public:
  AutoNoJSAPI(bool aIsMainThread = NS_IsMainThread());
  ~AutoNoJSAPI();
private:
  dom::ScriptSettingsStack& mStack;
  mozilla::Maybe<AutoCxPusher> mCxPusher;
};

} 
} 

#endif 
