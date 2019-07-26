



#ifndef nsJSEnvironment_h
#define nsJSEnvironment_h

#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "prtime.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIXPConnect.h"
#include "nsIArray.h"
#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"

class nsICycleCollectorListener;
class nsIXPConnectJSObjectHolder;
class nsRootedJSValueArray;
class nsScriptNameSpaceManager;
class nsCycleCollectionNoteRootCallback;

namespace mozilla {
template <class> class Maybe;
}



#define NS_GC_DELAY                 4000 // ms

class nsJSContext : public nsIScriptContext
{
public:
  nsJSContext(bool aGCOnDestruction, nsIScriptGlobalObject* aGlobalObject);
  virtual ~nsJSContext();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSContext,
                                                         nsIScriptContext)

  virtual nsresult EvaluateString(const nsAString& aScript,
                                  JS::Handle<JSObject*> aScopeObject,
                                  JS::CompileOptions &aOptions,
                                  bool aCoerceToString,
                                  JS::Value* aRetValue,
                                  void **aOffThreadToken = nullptr) MOZ_OVERRIDE;

  virtual nsresult BindCompiledEventHandler(nsISupports *aTarget,
                                            JS::Handle<JSObject*> aScope,
                                            JS::Handle<JSObject*> aHandler,
                                            JS::MutableHandle<JSObject*> aBoundHandler) MOZ_OVERRIDE;

  virtual nsIScriptGlobalObject *GetGlobalObject() MOZ_OVERRIDE;
  inline nsIScriptGlobalObject *GetGlobalObjectRef() { return mGlobalObjectRef; }

  virtual JSContext* GetNativeContext() MOZ_OVERRIDE;
  virtual nsresult InitContext() MOZ_OVERRIDE;
  virtual bool IsContextInitialized() MOZ_OVERRIDE;

  virtual bool GetScriptsEnabled() MOZ_OVERRIDE;
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) MOZ_OVERRIDE;

  virtual nsresult SetProperty(JS::Handle<JSObject*> aTarget, const char* aPropName, nsISupports* aVal) MOZ_OVERRIDE;

  virtual bool GetProcessingScriptTag() MOZ_OVERRIDE;
  virtual void SetProcessingScriptTag(bool aResult) MOZ_OVERRIDE;

  virtual nsresult InitClasses(JS::Handle<JSObject*> aGlobalObj) MOZ_OVERRIDE;

  virtual void WillInitializeContext() MOZ_OVERRIDE;
  virtual void DidInitializeContext() MOZ_OVERRIDE;

  virtual void SetWindowProxy(JS::Handle<JSObject*> aWindowProxy) MOZ_OVERRIDE;
  virtual JSObject* GetWindowProxy() MOZ_OVERRIDE;
  virtual JSObject* GetWindowProxyPreserveColor() MOZ_OVERRIDE;

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

  
  void EnsureStatics();

  static void GarbageCollectNow(JS::gcreason::Reason reason,
                                IsIncremental aIncremental = NonIncrementalGC,
                                IsCompartment aCompartment = NonCompartmentGC,
                                IsShrinking aShrinking = NonShrinkingGC,
                                int64_t aSliceMillis = 0);
  static void ShrinkGCBuffersNow();
  
  
  static void CycleCollectNow(nsICycleCollectorListener *aListener = nullptr,
                              int32_t aExtraForgetSkippableCalls = 0,
                              bool aManuallyTriggered = true);

  static void PokeGC(JS::gcreason::Reason aReason, int aDelay = 0);
  static void KillGCTimer();

  static void PokeShrinkGCBuffers();
  static void KillShrinkGCBuffersTimer();

  static void MaybePokeCC();
  static void KillCCTimer();
  static void KillFullGCTimer();
  static void KillInterSliceGCTimer();

  
  static void LikelyShortLivingObjectCreated();

  virtual void GC(JS::gcreason::Reason aReason) MOZ_OVERRIDE;

  static uint32_t CleanupsSinceLastGC();

  nsIScriptGlobalObject* GetCachedGlobalObject()
  {
    
    
    JSObject* global = GetWindowProxy();
    return global ? mGlobalObjectRef.get() : nullptr;
  }
protected:
  nsresult InitializeExternalClasses();

  
  nsresult ConvertSupportsTojsvals(nsISupports *aArgs,
                                   JS::Handle<JSObject*> aScope,
                                   uint32_t *aArgc,
                                   JS::Value **aArgv,
                                   mozilla::Maybe<nsRootedJSValueArray> &aPoolRelease);

  nsresult AddSupportsPrimitiveTojsvals(nsISupports *aArg, JS::Value *aArgv);

  
  
  nsresult JSObjectFromInterface(nsISupports *aSup,
                                 JS::Handle<JSObject*> aScript,
                                 JSObject **aRet);

  
  
  
  void ReportPendingException();

private:
  void DestroyJSContext();

  nsrefcnt GetCCRefcnt();

  JSContext *mContext;
  JS::Heap<JSObject*> mWindowProxy;

  bool mIsInitialized;
  bool mScriptsEnabled;
  bool mGCOnDestruction;
  bool mProcessingScriptTag;

  uint32_t mDefaultJSOptions;
  PRTime mOperationCallbackTime;

  PRTime mModalStateTime;
  uint32_t mModalStateDepth;

  nsJSContext *mNext;
  nsJSContext **mPrev;

  
  
  nsCOMPtr<nsIScriptGlobalObject> mGlobalObjectRef;

  static int JSOptionChangedCallback(const char *pref, void *data);

  static bool DOMOperationCallback(JSContext *cx);
};

class nsIJSRuntimeService;
class nsIPrincipal;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

void StartupJSEnvironment();
void ShutdownJSEnvironment();


nsScriptNameSpaceManager* GetNameSpaceManager();


class AsyncErrorReporter : public nsRunnable
{
public:
  
  AsyncErrorReporter(JSRuntime* aRuntime,
                     JSErrorReport* aErrorReport,
                     const char* aFallbackMessage,
                     nsIPrincipal* aGlobalPrincipal, 
                     nsPIDOMWindow* aWindow);

  NS_IMETHOD Run()
  {
    ReportError();
    return NS_OK;
  }

protected:
  
  void ReportError();

  nsString mErrorMsg;
  nsString mFileName;
  nsString mSourceLine;
  nsCString mCategory;
  uint32_t mLineNumber;
  uint32_t mColumn;
  uint32_t mFlags;
  uint64_t mInnerWindowID;
};

} 
} 





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


void NS_ScriptErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);

JSObject* NS_DOMReadStructuredClone(JSContext* cx,
                                    JSStructuredCloneReader* reader, uint32_t tag,
                                    uint32_t data, void* closure);

bool NS_DOMWriteStructuredClone(JSContext* cx,
                                JSStructuredCloneWriter* writer,
                                JS::Handle<JSObject*> obj, void *closure);

void NS_DOMStructuredCloneError(JSContext* cx, uint32_t errorid);

#endif 
