







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
class ScriptSettingsStackEntry {
  friend class ScriptSettingsStack;

public:
  ScriptSettingsStackEntry(nsIGlobalObject *aGlobal, bool aCandidate);
  ~ScriptSettingsStackEntry();

  bool NoJSAPI() { return !mGlobalObject; }

protected:
  nsCOMPtr<nsIGlobalObject> mGlobalObject;
  bool mIsCandidateEntryPoint;

private:
  
  friend class AutoNoJSAPI;
  ScriptSettingsStackEntry();

  ScriptSettingsStackEntry *mOlder;
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

  void SetWebIDLCallerPrincipal(nsIPrincipal *aPrincipal) {
    mWebIDLCallerPrincipal = aPrincipal;
  }

private:
  JSAutoCompartment mAc;
  
  
  
  
  
  
  
  nsIPrincipal* mWebIDLCallerPrincipal;
  friend nsIPrincipal* GetWebIDLCallerPrincipal();
};




class AutoIncumbentScript : protected ScriptSettingsStackEntry {
public:
  AutoIncumbentScript(nsIGlobalObject* aGlobalObject);
private:
  JS::AutoHideScriptedCaller mCallerOverride;
};









class AutoNoJSAPI : protected ScriptSettingsStackEntry {
public:
  AutoNoJSAPI(bool aIsMainThread = NS_IsMainThread());
private:
  mozilla::Maybe<AutoCxPusher> mCxPusher;
};

} 
} 

#endif 
