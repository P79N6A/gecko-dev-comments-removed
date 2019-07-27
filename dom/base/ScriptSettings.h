







#ifndef mozilla_dom_ScriptSettings_h
#define mozilla_dom_ScriptSettings_h

#include "MainThreadUtils.h"
#include "nsIGlobalObject.h"
#include "nsIPrincipal.h"

#include "mozilla/Maybe.h"

#include "jsapi.h"

class nsPIDOMWindow;
class nsGlobalWindow;
class nsIScriptContext;

namespace mozilla {
namespace dom {


namespace danger {





class MOZ_STACK_CLASS AutoCxPusher
{
public:
  explicit AutoCxPusher(JSContext *aCx, bool aAllowNull = false);
  ~AutoCxPusher();

  nsIScriptContext* GetScriptContext() { return mScx; }

  
  
  bool IsStackTop() const;

private:
  mozilla::Maybe<JSAutoRequest> mAutoRequest;
  mozilla::Maybe<JSAutoCompartment> mAutoCompartment;
  nsCOMPtr<nsIScriptContext> mScx;
  uint32_t mStackDepthAfterPush;
#ifdef DEBUG
  JSContext* mPushedContext;
  unsigned mCompartmentDepthOnEntry;
#endif
};

} 





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
  mozilla::Maybe<danger::AutoCxPusher> mCxPusher;
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
  mozilla::Maybe<danger::AutoCxPusher> mCxPusher;
};

} 






class MOZ_STACK_CLASS AutoJSContext {
public:
  explicit AutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*() const;

protected:
  explicit AutoJSContext(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  
  
  
  void Init(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  JSContext* mCx;
  Maybe<dom::danger::AutoCxPusher> mPusher;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};





class MOZ_STACK_CLASS ThreadsafeAutoJSContext {
public:
  explicit ThreadsafeAutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*() const;

private:
  JSContext* mCx; 
  Maybe<JSAutoRequest> mRequest; 
  Maybe<AutoJSContext> mAutoJSContext; 
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};







class MOZ_STACK_CLASS AutoSafeJSContext : public AutoJSContext {
public:
  explicit AutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
private:
  JSAutoCompartment mAc;
};




class MOZ_STACK_CLASS ThreadsafeAutoSafeJSContext {
public:
  explicit ThreadsafeAutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*() const;

private:
  JSContext* mCx; 
  Maybe<JSAutoRequest> mRequest; 
  Maybe<AutoSafeJSContext> mAutoSafeJSContext; 
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


} 

#endif
