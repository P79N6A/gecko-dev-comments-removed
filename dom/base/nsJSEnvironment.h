



#ifndef nsJSEnvironment_h
#define nsJSEnvironment_h

#include "nsIScriptContext.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "nsIObserver.h"
#include "nsIXPCScriptNotify.h"
#include "prtime.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIXPConnect.h"
#include "nsIArray.h"
#include "mozilla/Attributes.h"

class nsIXPConnectJSObjectHolder;
class nsRootedJSValueArray;
class nsScriptNameSpaceManager;
namespace mozilla {
template <class> class Maybe;
}



#define NS_GC_DELAY                 4000 // ms

class nsJSContext : public nsIScriptContext,
                    public nsIXPCScriptNotify
{
public:
  nsJSContext(JSRuntime* aRuntime, bool aGCOnDestruction,
              nsIScriptGlobalObject* aGlobalObject);
  virtual ~nsJSContext();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSContext,
                                                         nsIScriptContext)

  virtual nsIScriptObjectPrincipal* GetObjectPrincipal();

  virtual nsresult EvaluateString(const nsAString& aScript,
                                  JSObject& aScopeObject,
                                  JS::CompileOptions &aOptions,
                                  bool aCoerceToString,
                                  JS::Value* aRetValue);

  virtual nsresult CompileScript(const PRUnichar* aText,
                                 int32_t aTextLength,
                                 nsIPrincipal *principal,
                                 const char *aURL,
                                 uint32_t aLineNo,
                                 uint32_t aVersion,
                                 JS::MutableHandle<JSScript*> aScriptObject,
                                 bool aSaveSource = false);
  virtual nsresult ExecuteScript(JSScript* aScriptObject,
                                 JSObject* aScopeObject);

  virtual nsresult CallEventHandler(nsISupports* aTarget, JSObject* aScope,
                                    JSObject* aHandler,
                                    nsIArray *argv, nsIVariant **rv);
  virtual nsresult BindCompiledEventHandler(nsISupports *aTarget,
                                            JSObject *aScope,
                                            JSObject* aHandler,
                                            JS::MutableHandle<JSObject*> aBoundHandler);

  virtual nsIScriptGlobalObject *GetGlobalObject();
  inline nsIScriptGlobalObject *GetGlobalObjectRef() { return mGlobalObjectRef; }

  virtual JSContext* GetNativeContext();
  virtual JSObject* GetNativeGlobal();
  virtual nsresult InitContext();
  virtual bool IsContextInitialized();

  virtual void ScriptEvaluated(bool aTerminated);
  virtual void SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                      nsIDOMWindow* aRef);
  virtual bool GetScriptsEnabled();
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts);

  virtual nsresult SetProperty(JSObject* aTarget, const char* aPropName, nsISupports* aVal);

  virtual bool GetProcessingScriptTag();
  virtual void SetProcessingScriptTag(bool aResult);

  virtual bool GetExecutingScript();

  virtual nsresult InitClasses(JSObject* aGlobalObj);

  virtual void WillInitializeContext();
  virtual void DidInitializeContext();

  virtual nsresult Serialize(nsIObjectOutputStream* aStream, JSScript* aScriptObject);
  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               JS::MutableHandle<JSScript*> aResult);

  virtual void EnterModalState();
  virtual void LeaveModalState();

  NS_DECL_NSIXPCSCRIPTNOTIFY

  static void LoadStart();
  static void LoadEnd();

  enum IsCompartment {
    CompartmentGC,
    NonCompartmentGC
  };

  enum IsShrinking {
    ShrinkingGC,
    NonShrinkingGC
  };

  enum IsIncremental {
    IncrementalGC,
    NonIncrementalGC
  };

  static void GarbageCollectNow(JS::gcreason::Reason reason,
                                IsIncremental aIncremental = NonIncrementalGC,
                                IsCompartment aCompartment = NonCompartmentGC,
                                IsShrinking aShrinking = NonShrinkingGC,
                                int64_t aSliceMillis = 0);
  static void ShrinkGCBuffersNow();
  
  
  static void CycleCollectNow(nsICycleCollectorListener *aListener = nullptr,
                              int32_t aExtraForgetSkippableCalls = 0,
                              bool aForced = true);

  static void PokeGC(JS::gcreason::Reason aReason, int aDelay = 0);
  static void KillGCTimer();

  static void PokeShrinkGCBuffers();
  static void KillShrinkGCBuffersTimer();

  static void MaybePokeCC();
  static void KillCCTimer();
  static void KillFullGCTimer();
  static void KillInterSliceGCTimer();

  
  static void LikelyShortLivingObjectCreated();

  virtual void GC(JS::gcreason::Reason aReason);

  static uint32_t CleanupsSinceLastGC();

  nsIScriptGlobalObject* GetCachedGlobalObject()
  {
    
    
    JSObject* global = JS_GetGlobalObject(mContext);
    return global ? mGlobalObjectRef.get() : nullptr;
  }
protected:
  nsresult InitializeExternalClasses();

  
  nsresult ConvertSupportsTojsvals(nsISupports *aArgs,
                                   JSObject *aScope,
                                   uint32_t *aArgc,
                                   jsval **aArgv,
                                   mozilla::Maybe<nsRootedJSValueArray> &aPoolRelease);

  nsresult AddSupportsPrimitiveTojsvals(nsISupports *aArg, jsval *aArgv);

  
  
  nsresult JSObjectFromInterface(nsISupports *aSup, JSObject *aScript,
                                 JSObject **aRet);

  
  
  
  void ReportPendingException();
private:
  void DestroyJSContext();

  nsrefcnt GetCCRefcnt();

  JSContext *mContext;
  bool mActive;

  
public:
  struct TerminationFuncHolder;
protected:
  friend struct TerminationFuncHolder;
  
  struct TerminationFuncClosure
  {
    TerminationFuncClosure(nsScriptTerminationFunc aFunc,
                           nsISupports* aArg,
                           TerminationFuncClosure* aNext) :
      mTerminationFunc(aFunc),
      mTerminationFuncArg(aArg),
      mNext(aNext)
    {
    }
    ~TerminationFuncClosure()
    {
      delete mNext;
    }
    
    nsScriptTerminationFunc mTerminationFunc;
    nsCOMPtr<nsISupports> mTerminationFuncArg;
    TerminationFuncClosure* mNext;
  };

  
public:
  struct TerminationFuncHolder
  {
    TerminationFuncHolder(nsJSContext* aContext)
      : mContext(aContext),
        mTerminations(aContext->mTerminations)
    {
      aContext->mTerminations = nullptr;
    }
    ~TerminationFuncHolder()
    {
      
      
      
      
      if (mTerminations) {
        TerminationFuncClosure* cur = mTerminations;
        while (cur->mNext) {
          cur = cur->mNext;
        }
        cur->mNext = mContext->mTerminations;
        mContext->mTerminations = mTerminations;
      }
    }

    nsJSContext* mContext;
    TerminationFuncClosure* mTerminations;
  };

protected:
  TerminationFuncClosure* mTerminations;

private:
  bool mIsInitialized;
  bool mScriptsEnabled;
  bool mGCOnDestruction;
  bool mProcessingScriptTag;

  uint32_t mExecuteDepth;
  uint32_t mDefaultJSOptions;
  PRTime mOperationCallbackTime;

  PRTime mModalStateTime;
  uint32_t mModalStateDepth;

  nsJSContext *mNext;
  nsJSContext **mPrev;

  
  
  nsCOMPtr<nsIScriptGlobalObject> mGlobalObjectRef;

  static int JSOptionChangedCallback(const char *pref, void *data);

  static JSBool DOMOperationCallback(JSContext *cx);
};

class nsIJSRuntimeService;

class nsJSRuntime MOZ_FINAL : public nsIScriptRuntime
{
public:
  
  static JSRuntime *sRuntime;

public:
  
  NS_DECL_ISUPPORTS

  virtual already_AddRefed<nsIScriptContext>
  CreateContext(bool aGCOnDestruction,
                nsIScriptGlobalObject* aGlobalObject);

  static void Startup();
  static void Shutdown();
  
  static nsresult Init();
  
  static nsScriptNameSpaceManager* GetNameSpaceManager();
};





#define NS_IJSARGARRAY_IID \
{ 0xb6acdac8, 0xf5c6, 0x432c, \
  { 0xa8, 0x6e, 0x33, 0xee, 0xb1, 0xb0, 0xcd, 0xdc } }

class nsIJSArgArray : public nsIArray
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSARGARRAY_IID)
  
  
  
  virtual nsresult GetArgs(uint32_t *argc, void **argv) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSArgArray, NS_IJSARGARRAY_IID)


nsresult NS_CreateJSRuntime(nsIScriptRuntime **aRuntime);


void NS_ScriptErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);

JSObject* NS_DOMReadStructuredClone(JSContext* cx,
                                    JSStructuredCloneReader* reader, uint32_t tag,
                                    uint32_t data, void* closure);

JSBool NS_DOMWriteStructuredClone(JSContext* cx,
                                  JSStructuredCloneWriter* writer,
                                  JSObject* obj, void *closure);

void NS_DOMStructuredCloneError(JSContext* cx, uint32_t errorid);

#endif 
