







#ifndef mozilla_dom_ScriptSettings_h
#define mozilla_dom_ScriptSettings_h

#include "nsCxPusher.h"
#include "MainThreadUtils.h"
#include "nsIGlobalObject.h"
#include "nsIPrincipal.h"

#include "mozilla/Maybe.h"

class nsPIDOMWindow;
class nsGlobalWindow;

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
  ~ScriptSettingsStackEntry();

  bool NoJSAPI() { return !mGlobalObject; }

protected:
  ScriptSettingsStackEntry(nsIGlobalObject *aGlobal, bool aCandidate);

  nsCOMPtr<nsIGlobalObject> mGlobalObject;
  bool mIsCandidateEntryPoint;

private:
  
  friend class AutoNoJSAPI;
  ScriptSettingsStackEntry();

  ScriptSettingsStackEntry *mOlder;
};



































class MOZ_STACK_CLASS AutoJSAPI {
public:
  
  
  AutoJSAPI();

  
  
  
  void Init();

  
  
  
  
  bool Init(nsIGlobalObject* aGlobalObject);

  
  
  
  
  bool Init(nsIGlobalObject* aGlobalObject, JSContext* aCx);

  
  
  
  
  
  
  
  
  bool InitWithLegacyErrorReporting(nsIGlobalObject* aGlobalObject);

  
  
  bool Init(nsPIDOMWindow* aWindow);
  bool Init(nsPIDOMWindow* aWindow, JSContext* aCx);

  bool Init(nsGlobalWindow* aWindow);
  bool Init(nsGlobalWindow* aWindow, JSContext* aCx);

  bool InitWithLegacyErrorReporting(nsPIDOMWindow* aWindow);
  bool InitWithLegacyErrorReporting(nsGlobalWindow* aWindow);

  JSContext* cx() const {
    MOZ_ASSERT(mCx, "Must call Init before using an AutoJSAPI");
    MOZ_ASSERT_IF(NS_IsMainThread(), CxPusherIsStackTop());
    return mCx;
  }

  bool CxPusherIsStackTop() const { return mCxPusher->IsStackTop(); }

protected:
  
  
  
  
  
  AutoJSAPI(nsIGlobalObject* aGlobalObject, bool aIsMainThread, JSContext* aCx);

private:
  mozilla::Maybe<AutoCxPusher> mCxPusher;
  mozilla::Maybe<JSAutoNullableCompartment> mAutoNullableCompartment;
  JSContext *mCx;

  void InitInternal(JSObject* aGlobal, JSContext* aCx, bool aIsMainThread);
};




class AutoEntryScript : public AutoJSAPI,
                        protected ScriptSettingsStackEntry {
public:
  explicit AutoEntryScript(nsIGlobalObject* aGlobalObject,
                  bool aIsMainThread = NS_IsMainThread(),
                  
                  JSContext* aCx = nullptr);

  ~AutoEntryScript();

  void SetWebIDLCallerPrincipal(nsIPrincipal *aPrincipal) {
    mWebIDLCallerPrincipal = aPrincipal;
  }

private:
  
  
  
  
  
  
  
  nsIPrincipal* mWebIDLCallerPrincipal;
  friend nsIPrincipal* GetWebIDLCallerPrincipal();
};




class AutoIncumbentScript : protected ScriptSettingsStackEntry {
public:
  explicit AutoIncumbentScript(nsIGlobalObject* aGlobalObject);
private:
  JS::AutoHideScriptedCaller mCallerOverride;
};









class AutoNoJSAPI : protected ScriptSettingsStackEntry {
public:
  explicit AutoNoJSAPI(bool aIsMainThread = NS_IsMainThread());
private:
  mozilla::Maybe<AutoCxPusher> mCxPusher;
};

} 
} 

#endif 
