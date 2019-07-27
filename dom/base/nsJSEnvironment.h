



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
#include "nsPIDOMWindow.h"
#include "nsThreadUtils.h"
#include "xpcpublic.h"

class nsICycleCollectorListener;
class nsScriptNameSpaceManager;

namespace JS {
class AutoValueVector;
}

namespace mozilla {
template <class> class Maybe;
struct CycleCollectorResults;
}



#define NS_GC_DELAY                 4000 // ms

#define NS_MAJOR_FORGET_SKIPPABLE_CALLS 5

class nsJSContext : public nsIScriptContext
{
public:
  nsJSContext(bool aGCOnDestruction, nsIScriptGlobalObject* aGlobalObject);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSContext,
                                                         nsIScriptContext)

  virtual nsIScriptGlobalObject *GetGlobalObject() override;
  inline nsIScriptGlobalObject *GetGlobalObjectRef() { return mGlobalObjectRef; }

  virtual JSContext* GetNativeContext() override;
  virtual nsresult InitContext() override;
  virtual bool IsContextInitialized() override;

  virtual nsresult SetProperty(JS::Handle<JSObject*> aTarget, const char* aPropName, nsISupports* aVal) override;

  virtual bool GetProcessingScriptTag() override;
  virtual void SetProcessingScriptTag(bool aResult) override;

  virtual nsresult InitClasses(JS::Handle<JSObject*> aGlobalObj) override;

  virtual void WillInitializeContext() override;
  virtual void DidInitializeContext() override;

  virtual void SetWindowProxy(JS::Handle<JSObject*> aWindowProxy) override;
  virtual JSObject* GetWindowProxy() override;
  virtual JSObject* GetWindowProxyPreserveColor() override;

  static void LoadStart();
  static void LoadEnd();

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
                                IsShrinking aShrinking = NonShrinkingGC,
                                int64_t aSliceMillis = 0);
  static void ShrinkGCBuffersNow();

  
  
  static void CycleCollectNow(nsICycleCollectorListener *aListener = nullptr,
                              int32_t aExtraForgetSkippableCalls = 0);

  
  static void RunCycleCollectorSlice();

  
  static void RunCycleCollectorWorkSlice(int64_t aWorkBudget);

  static void BeginCycleCollectionCallback();
  static void EndCycleCollectionCallback(mozilla::CycleCollectorResults &aResults);

  
  static uint32_t GetMaxCCSliceTimeSinceClear();
  static void ClearMaxCCSliceTime();

  static void RunNextCollectorTimer();

  static void PokeGC(JS::gcreason::Reason aReason, int aDelay = 0);
  static void KillGCTimer();

  static void PokeShrinkGCBuffers();
  static void KillShrinkGCBuffersTimer();

  static void PokeShrinkingGC();
  static void KillShrinkingGCTimer();

  static void MaybePokeCC();
  static void KillCCTimer();
  static void KillICCTimer();
  static void KillFullGCTimer();
  static void KillInterSliceGCTimer();

  
  static void LikelyShortLivingObjectCreated();

  static uint32_t CleanupsSinceLastGC();

  nsIScriptGlobalObject* GetCachedGlobalObject()
  {
    
    
    JSObject* global = GetWindowProxy();
    return global ? mGlobalObjectRef.get() : nullptr;
  }

  static void NotifyDidPaint();
protected:
  virtual ~nsJSContext();

  
  nsresult ConvertSupportsTojsvals(nsISupports *aArgs,
                                   JS::Handle<JSObject*> aScope,
                                   JS::AutoValueVector &aArgsOut);

  nsresult AddSupportsPrimitiveTojsvals(nsISupports *aArg, JS::Value *aArgv);

private:
  void DestroyJSContext();

  nsrefcnt GetCCRefcnt();

  JSContext *mContext;
  JS::Heap<JSObject*> mWindowProxy;

  bool mIsInitialized;
  bool mGCOnDestruction;
  bool mProcessingScriptTag;

  PRTime mModalStateTime;
  uint32_t mModalStateDepth;

  
  
  nsCOMPtr<nsIScriptGlobalObject> mGlobalObjectRef;

  static void JSOptionChangedCallback(const char *pref, void *data);

  static bool DOMOperationCallback(JSContext *cx);
};

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

void StartupJSEnvironment();
void ShutdownJSEnvironment();


nsScriptNameSpaceManager* GetNameSpaceManager();


class AsyncErrorReporter : public nsRunnable
{
public:
  
  AsyncErrorReporter(JSRuntime* aRuntime, xpc::ErrorReport* aReport)
    : mReport(aReport)
  {}

  NS_IMETHOD Run() override
  {
    mReport->LogToConsole();
    return NS_OK;
  }

protected:
  nsRefPtr<xpc::ErrorReport> mReport;
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

JSObject* NS_DOMReadStructuredClone(JSContext* cx,
                                    JSStructuredCloneReader* reader, uint32_t tag,
                                    uint32_t data, void* closure);

bool NS_DOMWriteStructuredClone(JSContext* cx,
                                JSStructuredCloneWriter* writer,
                                JS::Handle<JSObject*> obj, void *closure);

void NS_DOMStructuredCloneError(JSContext* cx, uint32_t errorid);

#endif 
