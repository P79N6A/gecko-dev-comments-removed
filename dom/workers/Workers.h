




#ifndef mozilla_dom_workers_workers_h__
#define mozilla_dom_workers_workers_h__

#include "jsapi.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include <stdint.h>
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsString.h"

#define BEGIN_WORKERS_NAMESPACE \
  namespace mozilla { namespace dom { namespace workers {
#define END_WORKERS_NAMESPACE \
  } /* namespace workers */ } /* namespace dom */ } /* namespace mozilla */
#define USING_WORKERS_NAMESPACE \
  using namespace mozilla::dom::workers;

#define WORKERS_SHUTDOWN_TOPIC "web-workers-shutdown"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_WORKERS_NAMESPACE

class WorkerPrivate;

struct PrivatizableBase
{ };

#ifdef DEBUG
void
AssertIsOnMainThread();
#else
inline void
AssertIsOnMainThread()
{ }
#endif

struct JSSettings
{
  enum {
    
    JSSettings_JSGC_MAX_BYTES = 0,
    JSSettings_JSGC_MAX_MALLOC_BYTES,
    JSSettings_JSGC_HIGH_FREQUENCY_TIME_LIMIT,
    JSSettings_JSGC_LOW_FREQUENCY_HEAP_GROWTH,
    JSSettings_JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN,
    JSSettings_JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX,
    JSSettings_JSGC_HIGH_FREQUENCY_LOW_LIMIT,
    JSSettings_JSGC_HIGH_FREQUENCY_HIGH_LIMIT,
    JSSettings_JSGC_ALLOCATION_THRESHOLD,
    JSSettings_JSGC_SLICE_TIME_BUDGET,
    JSSettings_JSGC_DYNAMIC_HEAP_GROWTH,
    JSSettings_JSGC_DYNAMIC_MARK_SLICE,
    

    
    kGCSettingsArraySize
  };

  struct JSGCSetting
  {
    JSGCParamKey key;
    uint32_t value;

    JSGCSetting()
    : key(static_cast<JSGCParamKey>(-1)), value(0)
    { }

    bool
    IsSet() const
    {
      return key != static_cast<JSGCParamKey>(-1);
    }

    void
    Unset()
    {
      key = static_cast<JSGCParamKey>(-1);
      value = 0;
    }
  };

  
  
  typedef JSGCSetting JSGCSettingsArray[kGCSettingsArraySize];

  
  struct JSContentChromeSettings
  {
    JS::CompartmentOptions compartmentOptions;
    int32_t maxScriptRuntime;

    JSContentChromeSettings()
    : compartmentOptions(), maxScriptRuntime(0)
    { }
  };

  JSContentChromeSettings chrome;
  JSContentChromeSettings content;
  JSGCSettingsArray gcSettings;
  JS::RuntimeOptions runtimeOptions;

#ifdef JS_GC_ZEAL
  uint8_t gcZeal;
  uint32_t gcZealFrequency;
#endif

  JSSettings()
#ifdef JS_GC_ZEAL
  : gcZeal(0), gcZealFrequency(0)
#endif
  {
    for (uint32_t index = 0; index < ArrayLength(gcSettings); index++) {
      new (gcSettings + index) JSGCSetting();
    }
  }

  bool
  ApplyGCSetting(JSGCParamKey aKey, uint32_t aValue)
  {
    JSSettings::JSGCSetting* firstEmptySetting = nullptr;
    JSSettings::JSGCSetting* foundSetting = nullptr;

    for (uint32_t index = 0; index < ArrayLength(gcSettings); index++) {
      JSSettings::JSGCSetting& setting = gcSettings[index];
      if (setting.key == aKey) {
        foundSetting = &setting;
        break;
      }
      if (!firstEmptySetting && !setting.IsSet()) {
        firstEmptySetting = &setting;
      }
    }

    if (aValue) {
      if (!foundSetting) {
        foundSetting = firstEmptySetting;
        if (!foundSetting) {
          NS_ERROR("Not enough space for this value!");
          return false;
        }
      }
      foundSetting->key = aKey;
      foundSetting->value = aValue;
      return true;
    }

    if (foundSetting) {
      foundSetting->Unset();
      return true;
    }

    return false;
  }
};

enum WorkerPreference
{
  WORKERPREF_DUMP = 0, 
  WORKERPREF_DOM_FETCH,
  WORKERPREF_COUNT
};





bool
ResolveWorkerClasses(JSContext* aCx, JS::Handle<JSObject*> aObj, JS::Handle<jsid> aId,
                     JS::MutableHandle<JSObject*> aObjp);

void
CancelWorkersForWindow(nsPIDOMWindow* aWindow);

void
SuspendWorkersForWindow(nsPIDOMWindow* aWindow);

void
ResumeWorkersForWindow(nsPIDOMWindow* aWindow);

class WorkerTask
{
protected:
  WorkerTask()
  { }

  virtual ~WorkerTask()
  { }

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WorkerTask)

  virtual bool
  RunTask(JSContext* aCx) = 0;
};

class WorkerCrossThreadDispatcher
{
   friend class WorkerPrivate;

  
  
  Mutex mMutex;
  WorkerPrivate* mWorkerPrivate;

private:
  
  explicit WorkerCrossThreadDispatcher(WorkerPrivate* aWorkerPrivate);

  
  void
  Forget()
  {
    MutexAutoLock lock(mMutex);
    mWorkerPrivate = nullptr;
  }

  ~WorkerCrossThreadDispatcher() {}

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WorkerCrossThreadDispatcher)

  
  
  bool
  PostTask(WorkerTask* aTask);
};

WorkerCrossThreadDispatcher*
GetWorkerCrossThreadDispatcher(JSContext* aCx, jsval aWorker);


const uint32_t kJSPrincipalsDebugToken = 0x7e2df9d2;

namespace exceptions {


void
ThrowDOMExceptionForNSResult(JSContext* aCx, nsresult aNSResult);

} 






extern bool
GetterOnlyJSNative(JSContext* aCx, unsigned aArgc, JS::Value* aVp);

END_WORKERS_NAMESPACE

#endif 
