





#include "WorkerPrivate.h"

#include "amIAddonManager.h"
#include "nsIClassInfo.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIConsoleService.h"
#include "nsIDOMDOMException.h"
#include "nsIDOMFile.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsIMemoryReporter.h"
#include "nsIPermissionManager.h"
#include "nsIScriptError.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"
#include "nsITextToSubURI.h"
#include "nsITimer.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIXPConnect.h"
#include "nsPrintfCString.h"
#include "nsHostObjectProtocolHandler.h"

#include <algorithm>
#include "jsfriendapi.h"
#include "js/OldDebugAPI.h"
#include "js/MemoryMetrics.h"
#include "mozilla/Attributes.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/Likely.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/ImageDataBinding.h"
#include "nsAlgorithm.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsError.h"
#include "nsDOMJSUtils.h"
#include "nsJSEnvironment.h"
#include "nsJSUtils.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsSandboxFlags.h"
#include "nsThreadUtils.h"
#include "xpcpublic.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#include "Events.h"
#include "mozilla/dom/Exceptions.h"
#include "File.h"
#include "Principal.h"
#include "RuntimeService.h"
#include "ScriptLoader.h"
#include "Worker.h"
#include "WorkerFeature.h"
#include "WorkerScope.h"


#define NORMAL_GC_TIMER_DELAY_MS 30000


#define IDLE_GC_TIMER_DELAY_MS 5000

using mozilla::MutexAutoLock;
using mozilla::TimeDuration;
using mozilla::TimeStamp;
using mozilla::dom::Throw;
using mozilla::AutoPushJSContext;
using mozilla::AutoSafeJSContext;

USING_WORKERS_NAMESPACE
using namespace mozilla::dom::workers::events;
using namespace mozilla::dom;

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(JsWorkerMallocSizeOf)

namespace {

template <class T>
class AutoPtrComparator
{
  typedef nsAutoPtr<T> A;
  typedef T* B;

public:
  bool Equals(const A& a, const B& b) const {
    return a && b ? *a == *b : !a && !b ? true : false;
  }
  bool LessThan(const A& a, const B& b) const {
    return a && b ? *a < *b : b ? true : false;
  }
};

template <class T>
inline AutoPtrComparator<T>
GetAutoPtrComparator(const nsTArray<nsAutoPtr<T> >&)
{
  return AutoPtrComparator<T>();
}


template <class T>
struct ISupportsBaseInfo
{
  typedef T ISupportsBase;
};

template <template <class> class SmartPtr, class T>
inline void
SwapToISupportsArray(SmartPtr<T>& aSrc,
                     nsTArray<nsCOMPtr<nsISupports> >& aDest)
{
  nsCOMPtr<nsISupports>* dest = aDest.AppendElement();

  T* raw = nullptr;
  aSrc.swap(raw);

  nsISupports* rawSupports =
    static_cast<typename ISupportsBaseInfo<T>::ISupportsBase*>(raw);
  dest->swap(rawSupports);
}

struct WorkerStructuredCloneCallbacks
{
  static JSObject*
  Read(JSContext* aCx, JSStructuredCloneReader* aReader, uint32_t aTag,
       uint32_t aData, void* aClosure)
  {
    
    if (aTag == DOMWORKER_SCTAG_FILE) {
      JS_ASSERT(!aData);

      nsIDOMFile* file;
      if (JS_ReadBytes(aReader, &file, sizeof(file))) {
        JS_ASSERT(file);

#ifdef DEBUG
        {
          
          nsCOMPtr<nsIMutable> mutableFile = do_QueryInterface(file);
          bool isMutable;
          NS_ASSERTION(NS_SUCCEEDED(mutableFile->GetMutable(&isMutable)) &&
                       !isMutable,
                       "Only immutable file should be passed to worker");
        }
#endif

        
        
        JSObject* jsFile = file::CreateFile(aCx, file);
        return jsFile;
      }
    }
    
    else if (aTag == DOMWORKER_SCTAG_BLOB) {
      JS_ASSERT(!aData);

      nsIDOMBlob* blob;
      if (JS_ReadBytes(aReader, &blob, sizeof(blob))) {
        JS_ASSERT(blob);

#ifdef DEBUG
        {
          
          nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(blob);
          bool isMutable;
          NS_ASSERTION(NS_SUCCEEDED(mutableBlob->GetMutable(&isMutable)) &&
                       !isMutable,
                       "Only immutable blob should be passed to worker");
        }
#endif

        
        
        JSObject* jsBlob = file::CreateBlob(aCx, blob);
        return jsBlob;
      }
    }
    
    else if (aTag == SCTAG_DOM_IMAGEDATA) {
      JS_ASSERT(!aData);

      
      uint32_t width, height;
      JS::Rooted<JS::Value> dataArray(aCx);
      if (!JS_ReadUint32Pair(aReader, &width, &height) ||
          !JS_ReadTypedArray(aReader, dataArray.address()))
      {
        return nullptr;
      }
      MOZ_ASSERT(dataArray.isObject());

      
      nsRefPtr<ImageData> imageData = new ImageData(width, height,
                                                    dataArray.toObject());
      
      JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));
      if (!global) {
        return nullptr;
      }
      return imageData->WrapObject(aCx, global);
    }

    Error(aCx, 0);
    return nullptr;
  }

  static bool
  Write(JSContext* aCx, JSStructuredCloneWriter* aWriter,
        JS::Handle<JSObject*> aObj, void* aClosure)
  {
    NS_ASSERTION(aClosure, "Null pointer!");

    
    nsTArray<nsCOMPtr<nsISupports> >* clonedObjects =
      static_cast<nsTArray<nsCOMPtr<nsISupports> >*>(aClosure);

    
    {
      nsIDOMFile* file = file::GetDOMFileFromJSObject(aObj);
      if (file) {
        if (JS_WriteUint32Pair(aWriter, DOMWORKER_SCTAG_FILE, 0) &&
            JS_WriteBytes(aWriter, &file, sizeof(file))) {
          clonedObjects->AppendElement(file);
          return true;
        }
      }
    }

    
    {
      nsIDOMBlob* blob = file::GetDOMBlobFromJSObject(aObj);
      if (blob) {
        nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(blob);
        if (mutableBlob && NS_SUCCEEDED(mutableBlob->SetMutable(false)) &&
            JS_WriteUint32Pair(aWriter, DOMWORKER_SCTAG_BLOB, 0) &&
            JS_WriteBytes(aWriter, &blob, sizeof(blob))) {
          clonedObjects->AppendElement(blob);
          return true;
        }
      }
    }

    
    {
      ImageData* imageData = nullptr;
      if (NS_SUCCEEDED(UNWRAP_OBJECT(ImageData, aCx, aObj, imageData))) {
        
        uint32_t width = imageData->Width();
        uint32_t height = imageData->Height();
        JS::Rooted<JSObject*> dataArray(aCx, imageData->GetDataObject());

        
        JSAutoCompartment ac(aCx, dataArray);
        return JS_WriteUint32Pair(aWriter, SCTAG_DOM_IMAGEDATA, 0) &&
               JS_WriteUint32Pair(aWriter, width, height) &&
               JS_WriteTypedArray(aWriter, JS::ObjectValue(*dataArray));
      }
    }

    Error(aCx, 0);
    return false;
  }

  static void
  Error(JSContext* aCx, uint32_t )
  {
    Throw(aCx, NS_ERROR_DOM_DATA_CLONE_ERR);
  }
};

JSStructuredCloneCallbacks gWorkerStructuredCloneCallbacks = {
  WorkerStructuredCloneCallbacks::Read,
  WorkerStructuredCloneCallbacks::Write,
  WorkerStructuredCloneCallbacks::Error
};

struct MainThreadWorkerStructuredCloneCallbacks
{
  static JSObject*
  Read(JSContext* aCx, JSStructuredCloneReader* aReader, uint32_t aTag,
       uint32_t aData, void* aClosure)
  {
    AssertIsOnMainThread();

    
    if (aTag == DOMWORKER_SCTAG_FILE) {
      JS_ASSERT(!aData);

      nsIDOMFile* file;
      if (JS_ReadBytes(aReader, &file, sizeof(file))) {
        JS_ASSERT(file);

#ifdef DEBUG
        {
          
          nsCOMPtr<nsIMutable> mutableFile = do_QueryInterface(file);
          bool isMutable;
          NS_ASSERTION(NS_SUCCEEDED(mutableFile->GetMutable(&isMutable)) &&
                       !isMutable,
                       "Only immutable file should be passed to worker");
        }
#endif

        
        
        JS::Rooted<JS::Value> wrappedFile(aCx);
        JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));
        nsresult rv = nsContentUtils::WrapNative(aCx, global, file,
                                                 &NS_GET_IID(nsIDOMFile),
                                                 wrappedFile.address());
        if (NS_FAILED(rv)) {
          Error(aCx, nsIDOMDOMException::DATA_CLONE_ERR);
          return nullptr;
        }

        return &wrappedFile.toObject();
      }
    }
    
    else if (aTag == DOMWORKER_SCTAG_BLOB) {
      JS_ASSERT(!aData);

      nsIDOMBlob* blob;
      if (JS_ReadBytes(aReader, &blob, sizeof(blob))) {
        JS_ASSERT(blob);

#ifdef DEBUG
        {
          
          nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(blob);
          bool isMutable;
          NS_ASSERTION(NS_SUCCEEDED(mutableBlob->GetMutable(&isMutable)) &&
                       !isMutable,
                       "Only immutable blob should be passed to worker");
        }
#endif

        
        
        JS::Rooted<JS::Value> wrappedBlob(aCx);
        JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));
        nsresult rv = nsContentUtils::WrapNative(aCx, global, blob,
                                                 &NS_GET_IID(nsIDOMBlob),
                                                 wrappedBlob.address());
        if (NS_FAILED(rv)) {
          Error(aCx, nsIDOMDOMException::DATA_CLONE_ERR);
          return nullptr;
        }

        return &wrappedBlob.toObject();
      }
    }

    JS_ClearPendingException(aCx);
    return NS_DOMReadStructuredClone(aCx, aReader, aTag, aData, nullptr);
  }

  static bool
  Write(JSContext* aCx, JSStructuredCloneWriter* aWriter,
        JS::Handle<JSObject*> aObj, void* aClosure)
  {
    AssertIsOnMainThread();

    NS_ASSERTION(aClosure, "Null pointer!");

    
    nsTArray<nsCOMPtr<nsISupports> >* clonedObjects =
      static_cast<nsTArray<nsCOMPtr<nsISupports> >*>(aClosure);

    
    nsCOMPtr<nsIXPConnectWrappedNative> wrappedNative;
    nsContentUtils::XPConnect()->
      GetWrappedNativeOfJSObject(aCx, aObj, getter_AddRefs(wrappedNative));

    if (wrappedNative) {
      
      nsISupports* wrappedObject = wrappedNative->Native();
      NS_ASSERTION(wrappedObject, "Null pointer?!");

      nsISupports* ccISupports = nullptr;
      wrappedObject->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                                    reinterpret_cast<void**>(&ccISupports));
      if (ccISupports) {
        NS_WARNING("Cycle collected objects are not supported!");
      }
      else {
        
        nsCOMPtr<nsIDOMFile> file = do_QueryInterface(wrappedObject);
        if (file) {
          nsCOMPtr<nsIMutable> mutableFile = do_QueryInterface(file);
          if (mutableFile && NS_SUCCEEDED(mutableFile->SetMutable(false))) {
            nsIDOMFile* filePtr = file;
            if (JS_WriteUint32Pair(aWriter, DOMWORKER_SCTAG_FILE, 0) &&
                JS_WriteBytes(aWriter, &filePtr, sizeof(filePtr))) {
              clonedObjects->AppendElement(file);
              return true;
            }
          }
        }

        
        nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(wrappedObject);
        if (blob) {
          nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(blob);
          if (mutableBlob && NS_SUCCEEDED(mutableBlob->SetMutable(false))) {
            nsIDOMBlob* blobPtr = blob;
            if (JS_WriteUint32Pair(aWriter, DOMWORKER_SCTAG_BLOB, 0) &&
                JS_WriteBytes(aWriter, &blobPtr, sizeof(blobPtr))) {
              clonedObjects->AppendElement(blob);
              return true;
            }
          }
        }
      }
    }

    JS_ClearPendingException(aCx);
    return NS_DOMWriteStructuredClone(aCx, aWriter, aObj, nullptr);
  }

  static void
  Error(JSContext* aCx, uint32_t aErrorId)
  {
    AssertIsOnMainThread();

    NS_DOMStructuredCloneError(aCx, aErrorId);
  }
};

JSStructuredCloneCallbacks gMainThreadWorkerStructuredCloneCallbacks = {
  MainThreadWorkerStructuredCloneCallbacks::Read,
  MainThreadWorkerStructuredCloneCallbacks::Write,
  MainThreadWorkerStructuredCloneCallbacks::Error
};

struct ChromeWorkerStructuredCloneCallbacks
{
  static JSObject*
  Read(JSContext* aCx, JSStructuredCloneReader* aReader, uint32_t aTag,
       uint32_t aData, void* aClosure)
  {
    return WorkerStructuredCloneCallbacks::Read(aCx, aReader, aTag, aData,
                                                aClosure);
  }

  static bool
  Write(JSContext* aCx, JSStructuredCloneWriter* aWriter,
        JS::Handle<JSObject*> aObj, void* aClosure)
  {
    return WorkerStructuredCloneCallbacks::Write(aCx, aWriter, aObj, aClosure);
  }

  static void
  Error(JSContext* aCx, uint32_t aErrorId)
  {
    return WorkerStructuredCloneCallbacks::Error(aCx, aErrorId);
  }
};

JSStructuredCloneCallbacks gChromeWorkerStructuredCloneCallbacks = {
  ChromeWorkerStructuredCloneCallbacks::Read,
  ChromeWorkerStructuredCloneCallbacks::Write,
  ChromeWorkerStructuredCloneCallbacks::Error
};

struct MainThreadChromeWorkerStructuredCloneCallbacks
{
  static JSObject*
  Read(JSContext* aCx, JSStructuredCloneReader* aReader, uint32_t aTag,
       uint32_t aData, void* aClosure)
  {
    AssertIsOnMainThread();

    JSObject* clone =
      MainThreadWorkerStructuredCloneCallbacks::Read(aCx, aReader, aTag, aData,
                                                     aClosure);
    if (clone) {
      return clone;
    }

    clone =
      ChromeWorkerStructuredCloneCallbacks::Read(aCx, aReader, aTag, aData,
                                                 aClosure);
    if (clone) {
      return clone;
    }

    JS_ClearPendingException(aCx);
    return NS_DOMReadStructuredClone(aCx, aReader, aTag, aData, nullptr);
  }

  static bool
  Write(JSContext* aCx, JSStructuredCloneWriter* aWriter,
        JS::Handle<JSObject*> aObj, void* aClosure)
  {
    AssertIsOnMainThread();

    if (MainThreadWorkerStructuredCloneCallbacks::Write(aCx, aWriter, aObj,
                                                        aClosure) ||
        ChromeWorkerStructuredCloneCallbacks::Write(aCx, aWriter, aObj,
                                                    aClosure) ||
        NS_DOMWriteStructuredClone(aCx, aWriter, aObj, nullptr)) {
      return true;
    }

    return false;
  }

  static void
  Error(JSContext* aCx, uint32_t aErrorId)
  {
    AssertIsOnMainThread();

    NS_DOMStructuredCloneError(aCx, aErrorId);
  }
};

JSStructuredCloneCallbacks gMainThreadChromeWorkerStructuredCloneCallbacks = {
  MainThreadChromeWorkerStructuredCloneCallbacks::Read,
  MainThreadChromeWorkerStructuredCloneCallbacks::Write,
  MainThreadChromeWorkerStructuredCloneCallbacks::Error
};

class MainThreadReleaseRunnable : public nsRunnable
{
  nsCOMPtr<nsIThread> mThread;
  nsTArray<nsCOMPtr<nsISupports> > mDoomed;
  nsTArray<nsCString> mHostObjectURIs;

public:
  MainThreadReleaseRunnable(nsCOMPtr<nsIThread>& aThread,
                            nsTArray<nsCOMPtr<nsISupports> >& aDoomed,
                            nsTArray<nsCString>& aHostObjectURIs)
  {
    mThread.swap(aThread);
    mDoomed.SwapElements(aDoomed);
    mHostObjectURIs.SwapElements(aHostObjectURIs);
  }

  MainThreadReleaseRunnable(nsTArray<nsCOMPtr<nsISupports> >& aDoomed,
                            nsTArray<nsCString>& aHostObjectURIs)
  {
    mDoomed.SwapElements(aDoomed);
    mHostObjectURIs.SwapElements(aHostObjectURIs);
  }

  NS_IMETHOD
  Run()
  {
    mDoomed.Clear();

    if (mThread) {
      RuntimeService* runtime = RuntimeService::GetService();
      NS_ASSERTION(runtime, "This should never be null!");

      runtime->NoteIdleThread(mThread);
    }

    for (uint32_t i = 0, len = mHostObjectURIs.Length(); i < len; ++i) {
      nsHostObjectProtocolHandler::RemoveDataEntry(mHostObjectURIs[i]);
    }

    return NS_OK;
  }
};

class WorkerFinishedRunnable : public WorkerControlRunnable
{
  WorkerPrivate* mFinishedWorker;
  nsCOMPtr<nsIThread> mThread;

public:
  WorkerFinishedRunnable(WorkerPrivate* aWorkerPrivate,
                         WorkerPrivate* aFinishedWorker,
                         nsIThread* aFinishedThread)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mFinishedWorker(aFinishedWorker), mThread(aFinishedThread)
  { }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    
    return true;
  }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult)
  {
    
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    nsTArray<nsCOMPtr<nsISupports> > doomed;
    mFinishedWorker->ForgetMainThreadObjects(doomed);

    nsTArray<nsCString> hostObjectURIs;
    mFinishedWorker->StealHostObjectURIs(hostObjectURIs);

    nsRefPtr<MainThreadReleaseRunnable> runnable =
      new MainThreadReleaseRunnable(mThread, doomed, hostObjectURIs);
    if (NS_FAILED(NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch, going to leak!");
    }

    mFinishedWorker->Finish(aCx);

    RuntimeService* runtime = RuntimeService::GetService();
    NS_ASSERTION(runtime, "This should never be null!");

    runtime->UnregisterWorker(aCx, mFinishedWorker);

    mFinishedWorker->Release();
    return true;
  }
};

class TopLevelWorkerFinishedRunnable : public nsRunnable
{
  WorkerPrivate* mFinishedWorker;
  nsCOMPtr<nsIThread> mThread;

public:
  TopLevelWorkerFinishedRunnable(WorkerPrivate* aFinishedWorker,
                                 nsIThread* aFinishedThread)
  : mFinishedWorker(aFinishedWorker), mThread(aFinishedThread)
  {
    aFinishedWorker->AssertIsOnWorkerThread();
  }

  NS_IMETHOD
  Run()
  {
    AssertIsOnMainThread();

    AutoSafeJSContext cx;
    JSAutoRequest ar(cx);

    mFinishedWorker->Finish(cx);

    RuntimeService* runtime = RuntimeService::GetService();
    NS_ASSERTION(runtime, "This should never be null!");

    runtime->UnregisterWorker(cx, mFinishedWorker);

    nsTArray<nsCOMPtr<nsISupports> > doomed;
    mFinishedWorker->ForgetMainThreadObjects(doomed);

    nsTArray<nsCString> hostObjectURIs;
    mFinishedWorker->StealHostObjectURIs(hostObjectURIs);

    nsRefPtr<MainThreadReleaseRunnable> runnable =
      new MainThreadReleaseRunnable(doomed, hostObjectURIs);
    if (NS_FAILED(NS_DispatchToCurrentThread(runnable))) {
      NS_WARNING("Failed to dispatch, going to leak!");
    }

    if (mThread) {
      runtime->NoteIdleThread(mThread);
    }

    mFinishedWorker->Release();

    return NS_OK;
  }
};

class ModifyBusyCountRunnable : public WorkerControlRunnable
{
  bool mIncrease;

public:
  ModifyBusyCountRunnable(WorkerPrivate* aWorkerPrivate, bool aIncrease)
  : WorkerControlRunnable(aWorkerPrivate, ParentThread, UnchangedBusyCount),
    mIncrease(aIncrease)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    return aWorkerPrivate->ModifyBusyCount(aCx, mIncrease);
  }

  void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult)
  {
    if (mIncrease) {
      WorkerControlRunnable::PostRun(aCx, aWorkerPrivate, aRunResult);
      return;
    }
    
    
  }
};

class CompileScriptRunnable : public WorkerRunnable
{
public:
  CompileScriptRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThread, ModifyBusyCount,
                   SkipWhenClearing)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    JS::Rooted<JSObject*> global(aCx, CreateDedicatedWorkerGlobalScope(aCx));
    if (!global) {
      NS_WARNING("Failed to make global!");
      return false;
    }

    JSAutoCompartment ac(aCx, global);
    return scriptloader::LoadWorkerScript(aCx);
  }
};

class CloseEventRunnable : public WorkerRunnable
{
public:
  CloseEventRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount,
                   SkipWhenClearing)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    JS::Rooted<JSObject*> target(aCx, JS::CurrentGlobalOrNull(aCx));
    NS_ASSERTION(target, "This must never be null!");

    aWorkerPrivate->CloseHandlerStarted();

    JS::Rooted<JSString*> type(aCx, JS_InternString(aCx, "close"));
    if (!type) {
      return false;
    }

    JS::Rooted<JSObject*> event(aCx, CreateGenericEvent(aCx, type, false, false, false));
    if (!event) {
      return false;
    }

    bool ignored;
    return DispatchEventToTarget(aCx, target, event, &ignored);
  }

  void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult)
  {
    
    WorkerRunnable::PostRun(aCx, aWorkerPrivate, aRunResult);

    
    if (!aWorkerPrivate->ModifyBusyCountFromWorker(aCx, false)) {
      JS_ReportPendingException(aCx);
    }

    aWorkerPrivate->CloseHandlerFinished();
  }
};

class MessageEventRunnable : public WorkerRunnable
{
  JSAutoStructuredCloneBuffer mBuffer;
  nsTArray<nsCOMPtr<nsISupports> > mClonedObjects;

public:
  MessageEventRunnable(WorkerPrivate* aWorkerPrivate, Target aTarget,
                       JSAutoStructuredCloneBuffer& aData,
                       nsTArray<nsCOMPtr<nsISupports> >& aClonedObjects)
  : WorkerRunnable(aWorkerPrivate, aTarget, aTarget == WorkerThread ?
                                                       ModifyBusyCount :
                                                       UnchangedBusyCount,
                   SkipWhenClearing)
  {
    mBuffer.swap(aData);
    mClonedObjects.SwapElements(aClonedObjects);
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    bool mainRuntime;
    JS::Rooted<JSObject*> target(aCx);
    if (mTarget == ParentThread) {
      
      
      if (!aWorkerPrivate->IsAcceptingEvents()) {
        return true;
      }

      mainRuntime = !aWorkerPrivate->GetParent();

      target = aWorkerPrivate->GetJSObject();
      NS_ASSERTION(target, "Must have a target!");

      if (aWorkerPrivate->IsSuspended()) {
        aWorkerPrivate->QueueRunnable(this);
        return true;
      }

      aWorkerPrivate->AssertInnerWindowIsCorrect();
    }
    else {
      NS_ASSERTION(aWorkerPrivate == GetWorkerPrivateFromContext(aCx),
                   "Badness!");
      mainRuntime = false;
      target = JS::CurrentGlobalOrNull(aCx);
    }

    NS_ASSERTION(target, "This should never be null!");

    JS::Rooted<JSObject*> event(aCx,
      CreateMessageEvent(aCx, mBuffer, mClonedObjects, mainRuntime));
    if (!event) {
      return false;
    }

    bool dummy;
    return DispatchEventToTarget(aCx, target, event, &dummy);
  }

  void PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult)
  {
    WorkerRunnable::PostRun(aCx, aWorkerPrivate, aRunResult);
  }
};

class NotifyRunnable : public WorkerControlRunnable
{
  Status mStatus;

public:
  NotifyRunnable(WorkerPrivate* aWorkerPrivate, Status aStatus)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mStatus(aStatus)
  {
    NS_ASSERTION(aStatus == Terminating || aStatus == Canceling ||
                 aStatus == Killing, "Bad status!");
  }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    
    
    return aWorkerPrivate->ModifyBusyCount(aCx, true);
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    return aWorkerPrivate->NotifyInternal(aCx, mStatus);
  }
};

class CloseRunnable MOZ_FINAL : public WorkerControlRunnable
{
public:
  CloseRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerControlRunnable(aWorkerPrivate, ParentThread, UnchangedBusyCount)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    
    return aWorkerPrivate->ModifyBusyCount(aCx, true) &&
           aWorkerPrivate->Close(aCx);
  }
};

class SuspendRunnable : public WorkerControlRunnable
{
public:
  SuspendRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    return aWorkerPrivate->SuspendInternal(aCx);
  }
};

class ResumeRunnable : public WorkerControlRunnable
{
public:
  ResumeRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    return aWorkerPrivate->ResumeInternal(aCx);
  }
};

class ReportErrorRunnable : public WorkerRunnable
{
  nsString mMessage;
  nsString mFilename;
  nsString mLine;
  uint32_t mLineNumber;
  uint32_t mColumnNumber;
  uint32_t mFlags;
  uint32_t mErrorNumber;

public:
  ReportErrorRunnable(WorkerPrivate* aWorkerPrivate, const nsString& aMessage,
                      const nsString& aFilename, const nsString& aLine,
                      uint32_t aLineNumber, uint32_t aColumnNumber,
                      uint32_t aFlags, uint32_t aErrorNumber)
  : WorkerRunnable(aWorkerPrivate, ParentThread, UnchangedBusyCount,
                   SkipWhenClearing),
    mMessage(aMessage), mFilename(aFilename), mLine(aLine),
    mLineNumber(aLineNumber), mColumnNumber(aColumnNumber), mFlags(aFlags),
    mErrorNumber(aErrorNumber)
  { }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult)
  {
    aWorkerPrivate->AssertIsOnWorkerThread();

    
    
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    
    
    if (!aWorkerPrivate->IsAcceptingEvents()) {
      return true;
    }

    JS::Rooted<JSObject*> target(aCx, aWorkerPrivate->GetJSObject());

    uint64_t innerWindowId;

    WorkerPrivate* parent = aWorkerPrivate->GetParent();
    if (parent) {
      innerWindowId = 0;
    }
    else {
      AssertIsOnMainThread();

      if (aWorkerPrivate->IsSuspended()) {
        aWorkerPrivate->QueueRunnable(this);
        return true;
      }

      aWorkerPrivate->AssertInnerWindowIsCorrect();

      innerWindowId = aWorkerPrivate->GetInnerWindowId();
    }

    return ReportErrorRunnable::ReportError(aCx, parent, true, target, mMessage,
                                            mFilename, mLine, mLineNumber,
                                            mColumnNumber, mFlags,
                                            mErrorNumber, innerWindowId);
  }

  void PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate, bool aRunResult)
  {
    WorkerRunnable::PostRun(aCx, aWorkerPrivate, aRunResult);
  }

  static bool
  ReportError(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
              bool aFireAtScope, JSObject* target, const nsString& aMessage,
              const nsString& aFilename, const nsString& aLine,
              uint32_t aLineNumber, uint32_t aColumnNumber, uint32_t aFlags,
              uint32_t aErrorNumber, uint64_t aInnerWindowId)
  {
    JS::Rooted<JSObject*> aTarget(aCx, target);
    if (aWorkerPrivate) {
      aWorkerPrivate->AssertIsOnWorkerThread();
    }
    else {
      AssertIsOnMainThread();
    }

    JS::Rooted<JSString*> message(aCx, JS_NewUCStringCopyN(aCx, aMessage.get(),
                                                           aMessage.Length()));
    if (!message) {
      return false;
    }

    JS::Rooted<JSString*> filename(aCx, JS_NewUCStringCopyN(aCx, aFilename.get(),
                                                            aFilename.Length()));
    if (!filename) {
      return false;
    }

    
    
    if (!JSREPORT_IS_WARNING(aFlags)) {
      
      if (aTarget) {
        JS::Rooted<JSObject*> event(aCx,
          CreateErrorEvent(aCx, message, filename, aLineNumber, !aWorkerPrivate));
        if (!event) {
          return false;
        }

        bool preventDefaultCalled;
        if (!DispatchEventToTarget(aCx, aTarget, event, &preventDefaultCalled)) {
          return false;
        }

        if (preventDefaultCalled) {
          return true;
        }
      }

      
      
      if (aFireAtScope && (aTarget || aErrorNumber != JSMSG_OVER_RECURSED)) {
        aTarget = JS::CurrentGlobalOrNull(aCx);
        NS_ASSERTION(aTarget, "This should never be null!");

        bool preventDefaultCalled;
        nsIScriptGlobalObject* sgo;

        if (aWorkerPrivate ||
            !(sgo = nsJSUtils::GetStaticScriptGlobal(aTarget))) {
          
          JS::Rooted<JSObject*> event(aCx,
            CreateErrorEvent(aCx, message, filename, aLineNumber, false));
          if (!event) {
            return false;
          }

          if (!DispatchEventToTarget(aCx, aTarget, event,
                                     &preventDefaultCalled)) {
            return false;
          }
        }
        else {
          
          nsScriptErrorEvent event(true, NS_LOAD_ERROR);
          event.lineNr = aLineNumber;
          event.errorMsg = aMessage.get();
          event.fileName = aFilename.get();

          nsEventStatus status = nsEventStatus_eIgnore;
          if (NS_FAILED(sgo->HandleScriptError(&event, &status))) {
            NS_WARNING("Failed to dispatch main thread error event!");
            status = nsEventStatus_eIgnore;
          }

          preventDefaultCalled = status == nsEventStatus_eConsumeNoDefault;
        }

        if (preventDefaultCalled) {
          return true;
        }
      }
    }

    
    if (aWorkerPrivate) {
      nsRefPtr<ReportErrorRunnable> runnable =
        new ReportErrorRunnable(aWorkerPrivate, aMessage, aFilename, aLine,
                                aLineNumber, aColumnNumber, aFlags,
                                aErrorNumber);
      return runnable->Dispatch(aCx);
    }

    
    nsCOMPtr<nsIScriptError> scriptError =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
    NS_WARN_IF_FALSE(scriptError, "Failed to create script error!");

    if (scriptError) {
      if (NS_FAILED(scriptError->InitWithWindowID(aMessage,
                                                  aFilename,
                                                  aLine, aLineNumber,
                                                  aColumnNumber, aFlags,
                                                  "Web Worker",
                                                  aInnerWindowId))) {
        NS_WARNING("Failed to init script error!");
        scriptError = nullptr;
      }
    }

    nsCOMPtr<nsIConsoleService> consoleService =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    NS_WARN_IF_FALSE(consoleService, "Failed to get console service!");

    bool logged = false;

    if (consoleService) {
      if (scriptError) {
        if (NS_SUCCEEDED(consoleService->LogMessage(scriptError))) {
          logged = true;
        }
        else {
          NS_WARNING("Failed to log script error!");
        }
      }
      else if (NS_SUCCEEDED(consoleService->LogStringMessage(aMessage.get()))) {
        logged = true;
      }
      else {
        NS_WARNING("Failed to log script error!");
      }
    }

    if (!logged || nsContentUtils::DOMWindowDumpEnabled()) {
      NS_ConvertUTF16toUTF8 msg(aMessage);
#ifdef ANDROID
      __android_log_print(ANDROID_LOG_INFO, "Gecko", "%s", msg.get());
#endif
      fputs(msg.get(), stderr);
      fflush(stderr);
    }

    return true;
  }
};

class TimerRunnable : public WorkerRunnable
{
public:
  TimerRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount,
                   SkipWhenClearing)
  { }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    
    return true;
  }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult)
  {
    
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    return aWorkerPrivate->RunExpiredTimeouts(aCx);
  }
};

void
DummyCallback(nsITimer* aTimer, void* aClosure)
{
  
}

class WorkerRunnableEventTarget MOZ_FINAL : public nsIEventTarget
{
protected:
  nsRefPtr<WorkerRunnable> mWorkerRunnable;

public:
  WorkerRunnableEventTarget(WorkerRunnable* aWorkerRunnable)
  : mWorkerRunnable(aWorkerRunnable)
  { }

  NS_DECL_THREADSAFE_ISUPPORTS

  NS_IMETHOD
  Dispatch(nsIRunnable* aRunnable, uint32_t aFlags)
  {
    NS_ASSERTION(aFlags == nsIEventTarget::DISPATCH_NORMAL, "Don't call me!");

    nsRefPtr<WorkerRunnableEventTarget> kungFuDeathGrip = this;

    
    
    
    aRunnable->Run();

    
    
    mWorkerRunnable->Dispatch(nullptr);

    return NS_OK;
  }

  NS_IMETHOD
  IsOnCurrentThread(bool* aIsOnCurrentThread)
  {
    *aIsOnCurrentThread = false;
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(WorkerRunnableEventTarget, nsIEventTarget)

class KillCloseEventRunnable : public WorkerRunnable
{
  nsCOMPtr<nsITimer> mTimer;

  class KillScriptRunnable : public WorkerControlRunnable
  {
  public:
    KillScriptRunnable(WorkerPrivate* aWorkerPrivate)
    : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount)
    { }

    bool
    PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
    {
      
      return true;
    }

    void
    PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                 bool aDispatchResult)
    {
      
    }

    bool
    WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
    {
      
      return false;
    }
  };

public:
  KillCloseEventRunnable(WorkerPrivate* aWorkerPrivate)
  : WorkerRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount,
                   SkipWhenClearing)
  { }

  ~KillCloseEventRunnable()
  {
    if (mTimer) {
      mTimer->Cancel();
    }
  }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    NS_NOTREACHED("Not meant to be dispatched!");
    return false;
  }

  bool
  SetTimeout(JSContext* aCx, uint32_t aDelayMS)
  {
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!timer) {
      JS_ReportError(aCx, "Failed to create timer!");
      return false;
    }

    nsRefPtr<KillScriptRunnable> runnable =
      new KillScriptRunnable(mWorkerPrivate);

    nsRefPtr<WorkerRunnableEventTarget> target =
      new WorkerRunnableEventTarget(runnable);

    if (NS_FAILED(timer->SetTarget(target))) {
      JS_ReportError(aCx, "Failed to set timer's target!");
      return false;
    }

    if (NS_FAILED(timer->InitWithFuncCallback(DummyCallback, nullptr, aDelayMS,
                                              nsITimer::TYPE_ONE_SHOT))) {
      JS_ReportError(aCx, "Failed to start timer!");
      return false;
    }

    mTimer.swap(timer);
    return true;
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nullptr;
    }

    return true;
  }
};

class UpdateJSContextOptionsRunnable : public WorkerControlRunnable
{
  uint32_t mContentOptions;
  uint32_t mChromeOptions;

public:
  UpdateJSContextOptionsRunnable(WorkerPrivate* aWorkerPrivate,
                                 uint32_t aContentOptions,
                                 uint32_t aChromeOptions)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mContentOptions(aContentOptions), mChromeOptions(aChromeOptions)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->UpdateJSContextOptionsInternal(aCx, mContentOptions,
                                                   mChromeOptions);
    return true;
  }
};

class UpdateJSWorkerMemoryParameterRunnable : public WorkerControlRunnable
{
  uint32_t mValue;
  JSGCParamKey mKey;

public:
  UpdateJSWorkerMemoryParameterRunnable(WorkerPrivate* aWorkerPrivate,
                                        JSGCParamKey aKey,
                                        uint32_t aValue)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mValue(aValue), mKey(aKey)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->UpdateJSWorkerMemoryParameterInternal(aCx, mKey, mValue);
    return true;
  }
};

#ifdef JS_GC_ZEAL
class UpdateGCZealRunnable : public WorkerControlRunnable
{
  uint8_t mGCZeal;
  uint32_t mFrequency;

public:
  UpdateGCZealRunnable(WorkerPrivate* aWorkerPrivate,
                       uint8_t aGCZeal,
                       uint32_t aFrequency)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mGCZeal(aGCZeal), mFrequency(aFrequency)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->UpdateGCZealInternal(aCx, mGCZeal, mFrequency);
    return true;
  }
};
#endif

class UpdateJITHardeningRunnable : public WorkerControlRunnable
{
  bool mJITHardening;

public:
  UpdateJITHardeningRunnable(WorkerPrivate* aWorkerPrivate, bool aJITHardening)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mJITHardening(aJITHardening)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->UpdateJITHardeningInternal(aCx, mJITHardening);
    return true;
  }
};

class GarbageCollectRunnable : public WorkerControlRunnable
{
protected:
  bool mShrinking;
  bool mCollectChildren;

public:
  GarbageCollectRunnable(WorkerPrivate* aWorkerPrivate, bool aShrinking,
                         bool aCollectChildren)
  : WorkerControlRunnable(aWorkerPrivate, WorkerThread, UnchangedBusyCount),
    mShrinking(aShrinking), mCollectChildren(aCollectChildren)
  { }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    
    
    return true;
  }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                bool aDispatchResult)
  {
    
    
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    aWorkerPrivate->GarbageCollectInternal(aCx, mShrinking, mCollectChildren);
    return true;
  }
};

class SynchronizeAndResumeRunnable : public nsRunnable
{
protected:
  WorkerPrivate* mWorkerPrivate;
  nsCOMPtr<nsIScriptContext> mCx;

public:
  SynchronizeAndResumeRunnable(WorkerPrivate* aWorkerPrivate,
                               nsIScriptContext* aCx)
  : mWorkerPrivate(aWorkerPrivate), mCx(aCx)
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    AutoPushJSContext cx(mCx ? mCx->GetNativeContext() :
                         nsContentUtils::GetSafeJSContext());
    JSAutoRequest ar(cx);
    mWorkerPrivate->Resume(cx);

    return NS_OK;
  }
};

class WorkerJSRuntimeStats : public JS::RuntimeStats
{
  const nsACString& mRtPath;

public:
  WorkerJSRuntimeStats(const nsACString& aRtPath)
  : JS::RuntimeStats(JsWorkerMallocSizeOf), mRtPath(aRtPath)
  { }

  ~WorkerJSRuntimeStats()
  {
    for (size_t i = 0; i != zoneStatsVector.length(); i++) {
      delete static_cast<xpc::ZoneStatsExtras*>(zoneStatsVector[i].extra);
    }

    for (size_t i = 0; i != compartmentStatsVector.length(); i++) {
      delete static_cast<xpc::CompartmentStatsExtras*>(compartmentStatsVector[i].extra);
    }
  }

  virtual void
  initExtraZoneStats(JS::Zone* aZone,
                     JS::ZoneStats* aZoneStats)
                     MOZ_OVERRIDE
  {
    MOZ_ASSERT(!aZoneStats->extra);

    
    
    xpc::ZoneStatsExtras* extras = new xpc::ZoneStatsExtras;
    extras->pathPrefix = mRtPath;
    extras->pathPrefix += nsPrintfCString("zone(0x%p)/", (void *)aZone);
    aZoneStats->extra = extras;
  }

  virtual void
  initExtraCompartmentStats(JSCompartment* aCompartment,
                            JS::CompartmentStats* aCompartmentStats)
                            MOZ_OVERRIDE
  {
    MOZ_ASSERT(!aCompartmentStats->extra);

    
    
    xpc::CompartmentStatsExtras* extras = new xpc::CompartmentStatsExtras;

    
    
    extras->jsPathPrefix.Assign(mRtPath);
    extras->jsPathPrefix += nsPrintfCString("zone(0x%p)/",
                                            (void *)js::GetCompartmentZone(aCompartment));
    extras->jsPathPrefix += js::IsAtomsCompartment(aCompartment)
                            ? NS_LITERAL_CSTRING("compartment(web-worker-atoms)/")
                            : NS_LITERAL_CSTRING("compartment(web-worker)/");

    
    extras->domPathPrefix.AssignLiteral("explicit/workers/?!/");

    aCompartmentStats->extra = extras;
  }
};

} 

#ifdef DEBUG
void
mozilla::dom::workers::AssertIsOnMainThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

WorkerRunnable::WorkerRunnable(WorkerPrivate* aWorkerPrivate, Target aTarget,
                               BusyBehavior aBusyBehavior,
                               ClearingBehavior aClearingBehavior)
: mWorkerPrivate(aWorkerPrivate), mTarget(aTarget),
  mBusyBehavior(aBusyBehavior), mClearingBehavior(aClearingBehavior)
{
  NS_ASSERTION(aWorkerPrivate, "Null worker private!");
}
#endif

NS_IMPL_ISUPPORTS1(WorkerRunnable, nsIRunnable)

bool
WorkerRunnable::PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
{
#ifdef DEBUG
  if (mBusyBehavior == ModifyBusyCount) {
    NS_ASSERTION(mTarget == WorkerThread,
                 "Don't set this option unless targeting the worker thread!");
  }
  if (mTarget == ParentThread) {
    aWorkerPrivate->AssertIsOnWorkerThread();
  }
  else {
    aWorkerPrivate->AssertIsOnParentThread();
  }
#endif

  if (mBusyBehavior == ModifyBusyCount && aCx) {
    return aWorkerPrivate->ModifyBusyCount(aCx, true);
  }

  return true;
}

bool
WorkerRunnable::Dispatch(JSContext* aCx)
{
  bool ok;

  if (!aCx) {
    ok = PreDispatch(nullptr, mWorkerPrivate);
    if (ok) {
      ok = DispatchInternal();
    }
    PostDispatch(nullptr, mWorkerPrivate, ok);
    return ok;
  }

  JSAutoRequest ar(aCx);

  JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));

  Maybe<JSAutoCompartment> ac;
  if (global) {
    ac.construct(aCx, global);
  }

  ok = PreDispatch(aCx, mWorkerPrivate);

  if (ok && !DispatchInternal()) {
    ok = false;
  }

  PostDispatch(aCx, mWorkerPrivate, ok);

  return ok;
}


bool
WorkerRunnable::DispatchToMainThread(nsIRunnable* aRunnable)
{
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  NS_ASSERTION(mainThread, "This should never fail!");

  return NS_SUCCEEDED(mainThread->Dispatch(aRunnable, NS_DISPATCH_NORMAL));
}




#define IMPL_DISPATCH_INTERNAL(_class)                                         \
  bool                                                                         \
  _class ::DispatchInternal()                                                  \
  {                                                                            \
    if (mTarget == WorkerThread) {                                             \
      return mWorkerPrivate->Dispatch(this);                                   \
    }                                                                          \
                                                                               \
    if (mWorkerPrivate->GetParent()) {                                         \
      return mWorkerPrivate->GetParent()->Dispatch(this);                      \
    }                                                                          \
                                                                               \
    return DispatchToMainThread(this);                                         \
  }

IMPL_DISPATCH_INTERNAL(WorkerRunnable)
IMPL_DISPATCH_INTERNAL(WorkerSyncRunnable)
IMPL_DISPATCH_INTERNAL(WorkerControlRunnable)

#undef IMPL_DISPATCH_INTERNAL

void
WorkerRunnable::PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                             bool aDispatchResult)
{
#ifdef DEBUG
  if (mTarget == ParentThread) {
    aWorkerPrivate->AssertIsOnWorkerThread();
  }
  else {
    aWorkerPrivate->AssertIsOnParentThread();
  }
#endif

  if (!aDispatchResult && aCx) {
    if (mBusyBehavior == ModifyBusyCount) {
      aWorkerPrivate->ModifyBusyCount(aCx, false);
    }
    JS_ReportPendingException(aCx);
  }
}

NS_IMETHODIMP
WorkerRunnable::Run()
{
  JSContext* cx;
  nsRefPtr<WorkerPrivate> kungFuDeathGrip;
  nsCxPusher pusher;

  if (mTarget == WorkerThread) {
    mWorkerPrivate->AssertIsOnWorkerThread();
    cx = mWorkerPrivate->GetJSContext();
  } else {
    kungFuDeathGrip = mWorkerPrivate;
    mWorkerPrivate->AssertIsOnParentThread();
    cx = mWorkerPrivate->ParentJSContext();

    if (!mWorkerPrivate->GetParent()) {
      AssertIsOnMainThread();
      pusher.Push(cx);
    }
  }

  JS::Rooted<JSObject*> targetCompartmentObject(cx);

  if (mTarget == WorkerThread) {
    targetCompartmentObject = JS::CurrentGlobalOrNull(cx);
  } else {
    targetCompartmentObject = mWorkerPrivate->GetJSObject();
  }

  NS_ASSERTION(cx, "Must have a context!");

  JSAutoRequest ar(cx);

  Maybe<JSAutoCompartment> ac;
  if (targetCompartmentObject) {
    ac.construct(cx, targetCompartmentObject);
  }

  bool result = WorkerRun(cx, mWorkerPrivate);
  
  
  
  if (mTarget == WorkerThread && ac.empty() &&
      js::DefaultObjectForContextOrNull(cx)) {
    ac.construct(cx, js::DefaultObjectForContextOrNull(cx));
  }
  PostRun(cx, mWorkerPrivate, result);
  return result ? NS_OK : NS_ERROR_FAILURE;
}

void
WorkerRunnable::PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                        bool aRunResult)
{
#ifdef DEBUG
  if (mTarget == ParentThread) {
    mWorkerPrivate->AssertIsOnParentThread();
  }
  else {
    mWorkerPrivate->AssertIsOnWorkerThread();
  }
#endif

  if (mBusyBehavior == ModifyBusyCount) {
    if (!aWorkerPrivate->ModifyBusyCountFromWorker(aCx, false)) {
      aRunResult = false;
    }
  }

  if (!aRunResult) {
    JS_ReportPendingException(aCx);
  }
}

struct WorkerPrivate::TimeoutInfo
{
  TimeoutInfo()
  : mTimeoutVal(JS::UndefinedValue()), mLineNumber(0), mId(0), mIsInterval(false),
    mCanceled(false)
  {
    MOZ_COUNT_CTOR(mozilla::dom::workers::WorkerPrivate::TimeoutInfo);
  }

  ~TimeoutInfo()
  {
    MOZ_COUNT_DTOR(mozilla::dom::workers::WorkerPrivate::TimeoutInfo);
  }

  bool operator==(const TimeoutInfo& aOther)
  {
    return mTargetTime == aOther.mTargetTime;
  }

  bool operator<(const TimeoutInfo& aOther)
  {
    return mTargetTime < aOther.mTargetTime;
  }

  JS::Heap<JS::Value> mTimeoutVal;
  nsTArray<JS::Heap<JS::Value> > mExtraArgVals;
  mozilla::TimeStamp mTargetTime;
  mozilla::TimeDuration mInterval;
  nsCString mFilename;
  uint32_t mLineNumber;
  uint32_t mId;
  bool mIsInterval;
  bool mCanceled;
};

class WorkerPrivate::MemoryReporter MOZ_FINAL : public nsIMemoryReporter
{
  friend class WorkerPrivate;

  SharedMutex mMutex;
  WorkerPrivate* mWorkerPrivate;
  nsCString mRtPath;
  bool mAlreadyMappedToAddon;

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  MemoryReporter(WorkerPrivate* aWorkerPrivate)
  : mMutex(aWorkerPrivate->mMutex), mWorkerPrivate(aWorkerPrivate),
    mAlreadyMappedToAddon(false)
  {
    aWorkerPrivate->AssertIsOnWorkerThread();

    nsCString escapedDomain(aWorkerPrivate->Domain());
    escapedDomain.ReplaceChar('/', '\\');

    NS_ConvertUTF16toUTF8 escapedURL(aWorkerPrivate->ScriptURL());
    escapedURL.ReplaceChar('/', '\\');

    nsAutoCString addressString;
    addressString.AppendPrintf("0x%p", static_cast<void*>(aWorkerPrivate));

    mRtPath = NS_LITERAL_CSTRING("explicit/workers/workers(") +
              escapedDomain + NS_LITERAL_CSTRING(")/worker(") +
              escapedURL + NS_LITERAL_CSTRING(", ") + addressString +
              NS_LITERAL_CSTRING(")/");
  }

  NS_IMETHOD
  GetName(nsACString& aName)
  {
    aName.AssignLiteral("workers");
    return NS_OK;
  }

  NS_IMETHOD
  CollectReports(nsIMemoryReporterCallback* aCallback,
                 nsISupports* aClosure)
  {
    AssertIsOnMainThread();

    
    
    WorkerJSRuntimeStats rtStats(mRtPath);

    {
      MutexAutoLock lock(mMutex);

      TryToMapAddon();

      if (!mWorkerPrivate ||
          !mWorkerPrivate->BlockAndCollectRuntimeStats(&rtStats)) {
        
        return NS_OK;
      }
    }

    return xpc::ReportJSRuntimeExplicitTreeStats(rtStats, mRtPath,
                                                 aCallback, aClosure);
  }

private:
  ~MemoryReporter()
  { }

  void
  Disable()
  {
    
    mMutex.AssertCurrentThreadOwns();

    NS_ASSERTION(mWorkerPrivate, "Disabled more than once!");
    mWorkerPrivate = nullptr;
  }

  
  void
  TryToMapAddon()
  {
    AssertIsOnMainThread();
    mMutex.AssertCurrentThreadOwns();

    if (mAlreadyMappedToAddon || !mWorkerPrivate) {
      return;
    }

    nsCOMPtr<nsIURI> scriptURI;
    if (NS_FAILED(NS_NewURI(getter_AddRefs(scriptURI),
                            mWorkerPrivate->ScriptURL()))) {
      return;
    }

    mAlreadyMappedToAddon = true;

    nsAutoCString addonId;
    bool ok;
    nsCOMPtr<amIAddonManager> addonManager =
      do_GetService("@mozilla.org/addons/integration;1");

    if (!addonManager ||
        NS_FAILED(addonManager->MapURIToAddonID(scriptURI, addonId, &ok)) ||
        !ok) {
      return;
    }

    static const size_t explicitLength = strlen("explicit/");
    addonId.Insert(NS_LITERAL_CSTRING("add-ons/"), 0);
    addonId += "/";
    mRtPath.Insert(addonId, explicitLength);
  }
};

NS_IMPL_ISUPPORTS1(WorkerPrivate::MemoryReporter, nsIMemoryReporter)

template <class Derived>
WorkerPrivateParent<Derived>::WorkerPrivateParent(
                                     JSContext* aCx,
                                     JS::Handle<JSObject*> aObject,
                                     WorkerPrivate* aParent,
                                     JSContext* aParentJSContext,
                                     const nsAString& aScriptURL,
                                     bool aIsChromeWorker,
                                     const nsACString& aDomain,
                                     nsCOMPtr<nsPIDOMWindow>& aWindow,
                                     nsCOMPtr<nsIScriptContext>& aScriptContext,
                                     nsCOMPtr<nsIURI>& aBaseURI,
                                     nsCOMPtr<nsIPrincipal>& aPrincipal,
                                     nsCOMPtr<nsIChannel>& aChannel,
                                     nsCOMPtr<nsIContentSecurityPolicy>& aCSP,
                                     bool aEvalAllowed,
                                     bool aReportCSPViolations)
: EventTarget(aParent ? aCx : NULL), mMutex("WorkerPrivateParent Mutex"),
  mCondVar(mMutex, "WorkerPrivateParent CondVar"),
  mMemoryReportCondVar(mMutex, "WorkerPrivateParent Memory Report CondVar"),
  mJSObject(aObject), mParent(aParent), mParentJSContext(aParentJSContext),
  mScriptURL(aScriptURL), mDomain(aDomain), mBusyCount(0),
  mParentStatus(Pending), mJSObjectRooted(false), mParentSuspended(false),
  mIsChromeWorker(aIsChromeWorker), mPrincipalIsSystem(false),
  mMainThreadObjectsForgotten(false), mEvalAllowed(aEvalAllowed),
  mReportCSPViolations(aReportCSPViolations)
{
  MOZ_COUNT_CTOR(mozilla::dom::workers::WorkerPrivateParent);

  if (aWindow) {
    NS_ASSERTION(aWindow->IsInnerWindow(), "Should have inner window here!");
  }

  mWindow.swap(aWindow);
  mScriptContext.swap(aScriptContext);
  mBaseURI.swap(aBaseURI);
  mPrincipal.swap(aPrincipal);
  mChannel.swap(aChannel);
  mCSP.swap(aCSP);

  if (aParent) {
    aParent->AssertIsOnWorkerThread();

    aParent->CopyJSSettings(mJSSettings);

    NS_ASSERTION(JS_GetOptions(aCx) == (aParent->IsChromeWorker() ?
                                        mJSSettings.chrome.options :
                                        mJSSettings.content.options),
                 "Options mismatch!");
  }
  else {
    AssertIsOnMainThread();

    RuntimeService::GetDefaultJSSettings(mJSSettings);
  }
}

template <class Derived>
WorkerPrivateParent<Derived>::~WorkerPrivateParent()
{
  MOZ_COUNT_DTOR(mozilla::dom::workers::WorkerPrivateParent);
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::Start()
{
  
  {
    MutexAutoLock lock(mMutex);

    NS_ASSERTION(mParentStatus != Running, "How can this be?!");

    if (mParentStatus == Pending) {
      mParentStatus = Running;
      return true;
    }
  }

  return false;
}


template <class Derived>
bool
WorkerPrivateParent<Derived>::NotifyPrivate(JSContext* aCx, Status aStatus)
{
  AssertIsOnParentThread();

  bool pending;
  {
    MutexAutoLock lock(mMutex);

    if (mParentStatus >= aStatus) {
      return true;
    }

    pending = mParentStatus == Pending;
    mParentStatus = aStatus;
  }

  if (pending) {
    WorkerPrivate* self = ParentAsWorkerPrivate();
#ifdef DEBUG
    {
      
      nsIThread* currentThread = NS_GetCurrentThread();
      NS_ASSERTION(currentThread, "This should never be null!");

      self->SetThread(currentThread);
    }
#endif
    
    self->ScheduleDeletion(true);
    return true;
  }

  NS_ASSERTION(aStatus != Terminating || mQueuedRunnables.IsEmpty(),
               "Shouldn't have anything queued!");

  
  mQueuedRunnables.Clear();

  nsRefPtr<NotifyRunnable> runnable =
    new NotifyRunnable(ParentAsWorkerPrivate(), aStatus);
  return runnable->Dispatch(aCx);
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::Suspend(JSContext* aCx)
{
  AssertIsOnParentThread();
  NS_ASSERTION(!mParentSuspended, "Suspended more than once!");

  mParentSuspended = true;

  {
    MutexAutoLock lock(mMutex);

    if (mParentStatus >= Terminating) {
      return true;
    }
  }

  nsRefPtr<SuspendRunnable> runnable =
    new SuspendRunnable(ParentAsWorkerPrivate());
  return runnable->Dispatch(aCx);
}

template <class Derived>
void
WorkerPrivateParent<Derived>::Resume(JSContext* aCx)
{
  AssertIsOnParentThread();
  NS_ASSERTION(mParentSuspended, "Not yet suspended!");

  mParentSuspended = false;

  {
    MutexAutoLock lock(mMutex);

    if (mParentStatus >= Terminating) {
      return;
    }
  }

  
  
  if (!mQueuedRunnables.IsEmpty()) {
    AssertIsOnMainThread();

    nsTArray<nsRefPtr<WorkerRunnable> > runnables;
    mQueuedRunnables.SwapElements(runnables);

    for (uint32_t index = 0; index < runnables.Length(); index++) {
      nsRefPtr<WorkerRunnable>& runnable = runnables[index];
      runnable->Run();
    }
  }

  nsRefPtr<ResumeRunnable> runnable =
    new ResumeRunnable(ParentAsWorkerPrivate());
  runnable->Dispatch(aCx);
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::SynchronizeAndResume(nsIScriptContext* aCx)
{
  AssertIsOnParentThread();
  NS_ASSERTION(mParentSuspended, "Not yet suspended!");

  
  
  
  
  

  nsRefPtr<SynchronizeAndResumeRunnable> runnable =
    new SynchronizeAndResumeRunnable(ParentAsWorkerPrivate(), aCx);
  return NS_SUCCEEDED(NS_DispatchToCurrentThread(runnable));
}

template <class Derived>
void
WorkerPrivateParent<Derived>::_trace(JSTracer* aTrc)
{
  
  
  
  EventTarget::_trace(aTrc);
}

template <class Derived>
void
WorkerPrivateParent<Derived>::_finalize(JSFreeOp* aFop)
{
  AssertIsOnParentThread();

  MOZ_ASSERT(mJSObject);
  MOZ_ASSERT(!mJSObjectRooted);

  
  mJSObject = nullptr;

  if (!TerminatePrivate(nullptr)) {
    NS_WARNING("Failed to terminate!");
  }

  
  
  
  
  
  WorkerPrivateParent<Derived>* extraSelfRef = NULL;

  if (!mParent && !mMainThreadObjectsForgotten) {
    AssertIsOnMainThread();
    NS_ADDREF(extraSelfRef = this);
  }

  EventTarget::_finalize(aFop);

  if (extraSelfRef) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewNonOwningRunnableMethod(extraSelfRef,
                                    &WorkerPrivateParent<Derived>::Release);
    if (NS_FAILED(NS_DispatchToCurrentThread(runnable))) {
      NS_WARNING("Failed to proxy release, this will leak!");
    }
  }
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::Close(JSContext* aCx)
{
  AssertIsOnParentThread();

  {
    MutexAutoLock lock(mMutex);

    if (mParentStatus < Closing) {
      mParentStatus = Closing;
    }
  }

  return true;
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::ModifyBusyCount(JSContext* aCx, bool aIncrease)
{
  AssertIsOnParentThread();

  NS_ASSERTION(aIncrease || mBusyCount, "Mismatched busy count mods!");

  if (aIncrease) {
    if (mBusyCount++ == 0 && mJSObject) {
      if (!RootJSObject(aCx, true)) {
        return false;
      }
    }
    return true;
  }

  if (--mBusyCount == 0 && mJSObject) {
    if (!RootJSObject(aCx, false)) {
      return false;
    }

    bool shouldCancel;
    {
      MutexAutoLock lock(mMutex);
      shouldCancel = mParentStatus == Terminating;
    }

    if (shouldCancel && !Cancel(aCx)) {
      return false;
    }
  }

  return true;
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::RootJSObject(JSContext* aCx, bool aRoot)
{
  AssertIsOnParentThread();

  if (aRoot != mJSObjectRooted) {
    if (aRoot) {
      NS_ASSERTION(mJSObject, "Nothing to root?");
      if (!JS_AddNamedObjectRoot(aCx, &mJSObject, "Worker root")) {
        NS_WARNING("JS_AddNamedObjectRoot failed!");
        return false;
      }
    }
    else {
      JS_RemoveObjectRoot(aCx, &mJSObject);
    }

    mJSObjectRooted = aRoot;
  }

  return true;
}

template <class Derived>
void
WorkerPrivateParent<Derived>::ForgetMainThreadObjects(
                                      nsTArray<nsCOMPtr<nsISupports> >& aDoomed)
{
  AssertIsOnParentThread();
  MOZ_ASSERT(!mMainThreadObjectsForgotten);

  aDoomed.SetCapacity(7);

  SwapToISupportsArray(mWindow, aDoomed);
  SwapToISupportsArray(mScriptContext, aDoomed);
  SwapToISupportsArray(mBaseURI, aDoomed);
  SwapToISupportsArray(mScriptURI, aDoomed);
  SwapToISupportsArray(mPrincipal, aDoomed);
  SwapToISupportsArray(mChannel, aDoomed);
  SwapToISupportsArray(mCSP, aDoomed);

  mMainThreadObjectsForgotten = true;
}

template <class Derived>
bool
WorkerPrivateParent<Derived>::PostMessage(JSContext* aCx,
                                          JS::Handle<JS::Value> aMessage,
                                          JS::Handle<JS::Value> aTransferable)
{
  AssertIsOnParentThread();

  {
    MutexAutoLock lock(mMutex);
    if (mParentStatus > Running) {
      return true;
    }
  }

  JSStructuredCloneCallbacks* callbacks;
  if (GetParent()) {
    if (IsChromeWorker()) {
      callbacks = &gChromeWorkerStructuredCloneCallbacks;
    }
    else {
      callbacks = &gWorkerStructuredCloneCallbacks;
    }
  }
  else {
    AssertIsOnMainThread();

    if (IsChromeWorker()) {
      callbacks = &gMainThreadChromeWorkerStructuredCloneCallbacks;
    }
    else {
      callbacks = &gMainThreadWorkerStructuredCloneCallbacks;
    }
  }

  nsTArray<nsCOMPtr<nsISupports> > clonedObjects;

  JSAutoStructuredCloneBuffer buffer;
  if (!buffer.write(aCx, aMessage, aTransferable, callbacks, &clonedObjects)) {
    return false;
  }

  nsRefPtr<MessageEventRunnable> runnable =
    new MessageEventRunnable(ParentAsWorkerPrivate(),
                             WorkerRunnable::WorkerThread, buffer,
                             clonedObjects);
  return runnable->Dispatch(aCx);
}

template <class Derived>
uint64_t
WorkerPrivateParent<Derived>::GetInnerWindowId()
{
  AssertIsOnMainThread();
  NS_ASSERTION(!mWindow || mWindow->IsInnerWindow(), "Outer window?");
  return mWindow ? mWindow->WindowID() : 0;
}

template <class Derived>
void
WorkerPrivateParent<Derived>::UpdateJSContextOptions(JSContext* aCx,
                                                     uint32_t aContentOptions,
                                                     uint32_t aChromeOptions)
{
  AssertIsOnParentThread();

  {
    MutexAutoLock lock(mMutex);
    mJSSettings.content.options = aContentOptions;
    mJSSettings.chrome.options = aChromeOptions;
  }

  nsRefPtr<UpdateJSContextOptionsRunnable> runnable =
    new UpdateJSContextOptionsRunnable(ParentAsWorkerPrivate(), aContentOptions,
                                       aChromeOptions);
  if (!runnable->Dispatch(aCx)) {
    NS_WARNING("Failed to update worker context options!");
    JS_ClearPendingException(aCx);
  }
}

template <class Derived>
void
WorkerPrivateParent<Derived>::UpdateJSWorkerMemoryParameter(JSContext* aCx,
                                                            JSGCParamKey aKey,
                                                            uint32_t aValue)
{
  AssertIsOnParentThread();

  bool found = false;

  {
    MutexAutoLock lock(mMutex);
    found = mJSSettings.ApplyGCSetting(aKey, aValue);
  }

  if (found) {
    nsRefPtr<UpdateJSWorkerMemoryParameterRunnable> runnable =
      new UpdateJSWorkerMemoryParameterRunnable(ParentAsWorkerPrivate(), aKey,
                                                aValue);
    if (!runnable->Dispatch(aCx)) {
      NS_WARNING("Failed to update memory parameter!");
      JS_ClearPendingException(aCx);
    }
  }
}

#ifdef JS_GC_ZEAL
template <class Derived>
void
WorkerPrivateParent<Derived>::UpdateGCZeal(JSContext* aCx, uint8_t aGCZeal,
                                           uint32_t aFrequency)
{
  AssertIsOnParentThread();

  {
    MutexAutoLock lock(mMutex);
    mJSSettings.gcZeal = aGCZeal;
    mJSSettings.gcZealFrequency = aFrequency;
  }

  nsRefPtr<UpdateGCZealRunnable> runnable =
    new UpdateGCZealRunnable(ParentAsWorkerPrivate(), aGCZeal, aFrequency);
  if (!runnable->Dispatch(aCx)) {
    NS_WARNING("Failed to update worker gczeal!");
    JS_ClearPendingException(aCx);
  }
}
#endif

template <class Derived>
void
WorkerPrivateParent<Derived>::UpdateJITHardening(JSContext* aCx,
                                                 bool aJITHardening)
{
  AssertIsOnParentThread();

  {
    MutexAutoLock lock(mMutex);
    mJSSettings.jitHardening = aJITHardening;
  }

  nsRefPtr<UpdateJITHardeningRunnable> runnable =
    new UpdateJITHardeningRunnable(ParentAsWorkerPrivate(), aJITHardening);
  if (!runnable->Dispatch(aCx)) {
    NS_WARNING("Failed to update worker jit hardening!");
    JS_ClearPendingException(aCx);
  }
}

template <class Derived>
void
WorkerPrivateParent<Derived>::GarbageCollect(JSContext* aCx, bool aShrinking)
{
  nsRefPtr<GarbageCollectRunnable> runnable =
    new GarbageCollectRunnable(ParentAsWorkerPrivate(), aShrinking, true);
  if (!runnable->Dispatch(aCx)) {
    NS_WARNING("Failed to update worker heap size!");
    JS_ClearPendingException(aCx);
  }
}

template <class Derived>
void
WorkerPrivateParent<Derived>::SetBaseURI(nsIURI* aBaseURI)
{
  AssertIsOnMainThread();

  mBaseURI = aBaseURI;

  if (NS_FAILED(aBaseURI->GetSpec(mLocationInfo.mHref))) {
    mLocationInfo.mHref.Truncate();
  }

  if (NS_FAILED(aBaseURI->GetHost(mLocationInfo.mHostname))) {
    mLocationInfo.mHostname.Truncate();
  }

  if (NS_FAILED(aBaseURI->GetPath(mLocationInfo.mPathname))) {
    mLocationInfo.mPathname.Truncate();
  }

  nsCString temp;

  nsCOMPtr<nsIURL> url(do_QueryInterface(aBaseURI));
  if (url && NS_SUCCEEDED(url->GetQuery(temp)) && !temp.IsEmpty()) {
    mLocationInfo.mSearch.AssignLiteral("?");
    mLocationInfo.mSearch.Append(temp);
  }

  if (NS_SUCCEEDED(aBaseURI->GetRef(temp)) && !temp.IsEmpty()) {
    nsCOMPtr<nsITextToSubURI> converter =
      do_GetService(NS_ITEXTTOSUBURI_CONTRACTID);
    if (converter) {
      nsCString charset;
      nsAutoString unicodeRef;
      if (NS_SUCCEEDED(aBaseURI->GetOriginCharset(charset)) &&
          NS_SUCCEEDED(converter->UnEscapeURIForUI(charset, temp,
                                                   unicodeRef))) {
        mLocationInfo.mHash.AssignLiteral("#");
        mLocationInfo.mHash.Append(NS_ConvertUTF16toUTF8(unicodeRef));
      }
    }

    if (mLocationInfo.mHash.IsEmpty()) {
      mLocationInfo.mHash.AssignLiteral("#");
      mLocationInfo.mHash.Append(temp);
    }
  }

  if (NS_SUCCEEDED(aBaseURI->GetScheme(mLocationInfo.mProtocol))) {
    mLocationInfo.mProtocol.AppendLiteral(":");
  }
  else {
    mLocationInfo.mProtocol.Truncate();
  }

  int32_t port;
  if (NS_SUCCEEDED(aBaseURI->GetPort(&port)) && port != -1) {
    mLocationInfo.mPort.AppendInt(port);

    nsAutoCString host(mLocationInfo.mHostname);
    host.AppendLiteral(":");
    host.Append(mLocationInfo.mPort);

    mLocationInfo.mHost.Assign(host);
  }
  else {
    mLocationInfo.mHost.Assign(mLocationInfo.mHostname);
  }
}

template <class Derived>
void
WorkerPrivateParent<Derived>::SetPrincipal(nsIPrincipal* aPrincipal)
{
  AssertIsOnMainThread();

  mPrincipal = aPrincipal;
  mPrincipalIsSystem = nsContentUtils::IsSystemPrincipal(aPrincipal);
}

template <class Derived>
JSContext*
WorkerPrivateParent<Derived>::ParentJSContext() const
{
  AssertIsOnParentThread();

  if (!mParent) {
    AssertIsOnMainThread();

    if (!mScriptContext) {
      NS_ASSERTION(!mParentJSContext, "Shouldn't have a parent context!");
      return nsContentUtils::GetSafeJSContext();
    }

    NS_ASSERTION(mParentJSContext == mScriptContext->GetNativeContext(),
                 "Native context has changed!");
  }

  return mParentJSContext;
}

WorkerPrivate::WorkerPrivate(JSContext* aCx, JS::Handle<JSObject*> aObject,
                             WorkerPrivate* aParent,
                             JSContext* aParentJSContext,
                             const nsAString& aScriptURL, bool aIsChromeWorker,
                             const nsACString& aDomain,
                             nsCOMPtr<nsPIDOMWindow>& aWindow,
                             nsCOMPtr<nsIScriptContext>& aParentScriptContext,
                             nsCOMPtr<nsIURI>& aBaseURI,
                             nsCOMPtr<nsIPrincipal>& aPrincipal,
                             nsCOMPtr<nsIChannel>& aChannel,
                             nsCOMPtr<nsIContentSecurityPolicy>& aCSP,
                             bool aEvalAllowed,
                             bool aReportCSPViolations,
                             bool aXHRParamsAllowed)
: WorkerPrivateParent<WorkerPrivate>(aCx, aObject, aParent, aParentJSContext,
                                     aScriptURL, aIsChromeWorker, aDomain,
                                     aWindow, aParentScriptContext, aBaseURI,
                                     aPrincipal, aChannel, aCSP, aEvalAllowed,
                                     aReportCSPViolations),
  mJSContext(nullptr), mErrorHandlerRecursionCount(0), mNextTimeoutId(1),
  mStatus(Pending), mSuspended(false), mTimerRunning(false),
  mRunningExpiredTimeouts(false), mCloseHandlerStarted(false),
  mCloseHandlerFinished(false), mMemoryReporterRunning(false),
  mBlockedForMemoryReporter(false), mXHRParamsAllowed(aXHRParamsAllowed)
{
  MOZ_COUNT_CTOR(mozilla::dom::workers::WorkerPrivate);
}

WorkerPrivate::~WorkerPrivate()
{
  MOZ_COUNT_DTOR(mozilla::dom::workers::WorkerPrivate);
}


already_AddRefed<WorkerPrivate>
WorkerPrivate::Create(JSContext* aCx, JS::Handle<JSObject*> aObj, WorkerPrivate* aParent,
                      JS::Handle<JSString*> aScriptURL, bool aIsChromeWorker)
{
  nsCString domain;
  nsCOMPtr<nsIURI> baseURI;
  nsCOMPtr<nsIPrincipal> principal;
  nsCOMPtr<nsIScriptContext> scriptContext;
  nsCOMPtr<nsIDocument> document;
  nsCOMPtr<nsPIDOMWindow> window;
  nsCOMPtr<nsIContentSecurityPolicy> csp;

  bool evalAllowed = true;
  bool reportEvalViolations = false;

  JSContext* parentContext;

  bool xhrParamsAllowed = false;

  if (aParent) {
    aParent->AssertIsOnWorkerThread();

    
    Status currentStatus;
    {
      MutexAutoLock lock(aParent->mMutex);
      currentStatus = aParent->mStatus;
    }

    if (currentStatus > Running) {
      JS_ReportError(aCx, "Cannot create child workers from the close handler!");
      return nullptr;
    }

    parentContext = aCx;

    
    
    domain = aParent->Domain();
  }
  else {
    AssertIsOnMainThread();

    nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
    NS_ASSERTION(ssm, "This should never be null!");

    bool isChrome = nsContentUtils::IsCallerChrome();

    
    
    if (aIsChromeWorker && !isChrome) {
      xpc::Throw(aCx, NS_ERROR_DOM_SECURITY_ERR);
      return nullptr;
    }

    
    
    
    if (isChrome &&
        NS_FAILED(ssm->GetSystemPrincipal(getter_AddRefs(principal)))) {
      JS_ReportError(aCx, "Could not get system principal!");
      return nullptr;
    }

    
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal =
      nsJSUtils::GetStaticScriptGlobal(JS::CurrentGlobalOrNull(aCx));
    if (scriptGlobal) {
      
      nsCOMPtr<nsPIDOMWindow> globalWindow = do_QueryInterface(scriptGlobal);

      
      
      nsPIDOMWindow* outerWindow = globalWindow ?
                                   globalWindow->GetOuterWindow() :
                                   nullptr;
      window = outerWindow ? outerWindow->GetCurrentInnerWindow() : nullptr;
      if (!window ||
          (globalWindow != window &&
           !nsContentUtils::CanCallerAccess(window))) {
        xpc::Throw(aCx, NS_ERROR_DOM_SECURITY_ERR);
        return nullptr;
      }

      scriptContext = scriptGlobal->GetContext();
      if (!scriptContext) {
        JS_ReportError(aCx, "Couldn't get script context for this worker!");
        return nullptr;
      }

      parentContext = scriptContext->GetNativeContext();

      
      
      document = window->GetExtantDoc();
      if (!document) {
        JS_ReportError(aCx, "No document in this window!");
        return nullptr;
      }

      baseURI = document->GetDocBaseURI();

      
      
      if (!principal) {
        if (!(principal = document->NodePrincipal())) {
          JS_ReportError(aCx, "Could not get document principal!");
          return nullptr;
        }

        
        
        
        
        
        if (document->GetSandboxFlags() & SANDBOXED_ORIGIN) {
          nsCOMPtr<nsIDocument> tmpDoc = document;
          do {
            tmpDoc = tmpDoc->GetParentDocument();
          } while (tmpDoc && tmpDoc->GetSandboxFlags() & SANDBOXED_ORIGIN);

          if (tmpDoc) {
            
            nsCOMPtr<nsIPrincipal> tmpPrincipal = tmpDoc->NodePrincipal();

            if (NS_FAILED(tmpPrincipal->GetBaseDomain(domain))) {
              JS_ReportError(aCx, "Could not determine base domain!");
              return nullptr;
            }
          } else {
            
            if (NS_FAILED(principal->GetBaseDomain(domain))) {
              JS_ReportError(aCx, "Could not determine base domain!");
             return nullptr;
            }
          }
        } else {
          
          if (NS_FAILED(principal->GetBaseDomain(domain))) {
            JS_ReportError(aCx, "Could not determine base domain!");
            return nullptr;
          }
        }
      }

      xhrParamsAllowed = CheckXHRParamsAllowed(window);
    }
    else {
      
      NS_ASSERTION(isChrome, "Should be chrome only!");

      parentContext = nullptr;

      
      
      JS::RootedScript script(aCx);
      if (JS_DescribeScriptedCaller(aCx, &script, nullptr)) {
        if (NS_FAILED(NS_NewURI(getter_AddRefs(baseURI),
                                JS_GetScriptFilename(aCx, script)))) {
          JS_ReportError(aCx, "Failed to construct base URI!");
          return nullptr;
        }
      }

      xhrParamsAllowed = true;
    }

    NS_ASSERTION(principal, "Must have a principal now!");

    if (!isChrome && domain.IsEmpty()) {
      NS_ERROR("Must be chrome or have an domain!");
      return nullptr;
    }

    if (!nsContentUtils::GetContentSecurityPolicy(aCx, getter_AddRefs(csp))) {
      return nullptr;
    }

    if (csp && NS_FAILED(csp->GetAllowsEval(&reportEvalViolations, &evalAllowed))) {
      NS_ERROR("CSP: failed to get allowsEval");
      return nullptr;
    }
  }

  size_t urlLength;
  const jschar* urlChars = JS_GetStringCharsZAndLength(aCx, aScriptURL,
                                                       &urlLength);
  if (!urlChars) {
    return nullptr;
  }

  nsDependentString scriptURL(urlChars, urlLength);
  nsCOMPtr<nsIChannel> channel;
  nsresult rv;
  if (aParent) {
    rv =
      scriptloader::ChannelFromScriptURLWorkerThread(aCx, aParent, scriptURL,
                                                     getter_AddRefs(channel));

    
    

    Status currentStatus;
    {
      MutexAutoLock lock(aParent->mMutex);
      currentStatus = aParent->mStatus;
    }

    if (currentStatus > Running) {
      nsCOMPtr<nsIThread> mainThread;
      NS_GetMainThread(getter_AddRefs(mainThread));
      if (!mainThread) {
        MOZ_CRASH();
      }

      nsIChannel* rawChannel;
      channel.forget(&rawChannel);
      
      NS_ProxyRelease(mainThread, rawChannel);

      return nullptr;
    }
  }
  else {
    rv =
      scriptloader::ChannelFromScriptURLMainThread(principal, baseURI,
                                                   document, scriptURL,
                                                   getter_AddRefs(channel));
  }
  if (NS_FAILED(rv)) {
    scriptloader::ReportLoadError(aCx, scriptURL, rv, !aParent);
    return nullptr;
  }

  nsRefPtr<WorkerPrivate> worker =
    new WorkerPrivate(aCx, aObj, aParent, parentContext, scriptURL,
                      aIsChromeWorker, domain, window, scriptContext, baseURI,
                      principal, channel, csp, evalAllowed, reportEvalViolations,
                      xhrParamsAllowed);

  worker->SetIsDOMBinding();
  worker->SetWrapper(aObj);

  nsRefPtr<CompileScriptRunnable> compiler = new CompileScriptRunnable(worker);
  if (!compiler->Dispatch(aCx)) {
    return nullptr;
  }

  return worker.forget();
}

void
WorkerPrivate::DoRunLoop(JSContext* aCx)
{
  AssertIsOnWorkerThread();

  {
    MutexAutoLock lock(mMutex);
    mJSContext = aCx;

    NS_ASSERTION(mStatus == Pending, "Huh?!");
    mStatus = Running;
  }

  
  
  
  
  
  nsCOMPtr<nsITimer> gcTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!gcTimer) {
    JS_ReportError(aCx, "Failed to create GC timer!");
    return;
  }

  bool normalGCTimerRunning = false;

  
  nsCOMPtr<nsIEventTarget> normalGCEventTarget;
  nsCOMPtr<nsIEventTarget> idleGCEventTarget;

  
  
  nsCOMPtr<nsIRunnable> idleGCEvent;
  {
    nsRefPtr<GarbageCollectRunnable> runnable =
      new GarbageCollectRunnable(this, false, false);
    normalGCEventTarget = new WorkerRunnableEventTarget(runnable);

    runnable = new GarbageCollectRunnable(this, true, false);
    idleGCEventTarget = new WorkerRunnableEventTarget(runnable);

    idleGCEvent = runnable;
  }

  EnableMemoryReporter();

  Maybe<JSAutoCompartment> maybeAC;
  for (;;) {
    Status currentStatus;
    bool scheduleIdleGC;

    WorkerRunnable* event;
    {
      MutexAutoLock lock(mMutex);

      while (!mControlQueue.Pop(event) && !mQueue.Pop(event)) {
        WaitForWorkerEvents();
      }

      bool eventIsNotIdleGCEvent;
      currentStatus = mStatus;

      {
        MutexAutoUnlock unlock(mMutex);

        
        
        
        
        
        
        
        if (maybeAC.empty() && js::DefaultObjectForContextOrNull(aCx)) {
          maybeAC.construct(aCx, js::DefaultObjectForContextOrNull(aCx));
        }

        if (!normalGCTimerRunning &&
            event != idleGCEvent &&
            currentStatus <= Terminating) {
          
          if (NS_FAILED(gcTimer->Cancel())) {
            NS_WARNING("Failed to cancel GC timer!");
          }

          if (NS_SUCCEEDED(gcTimer->SetTarget(normalGCEventTarget)) &&
              NS_SUCCEEDED(gcTimer->InitWithFuncCallback(
                                             DummyCallback, nullptr,
                                             NORMAL_GC_TIMER_DELAY_MS,
                                             nsITimer::TYPE_REPEATING_SLACK))) {
            normalGCTimerRunning = true;
          }
          else {
            JS_ReportError(aCx, "Failed to start normal GC timer!");
          }
        }

        
        eventIsNotIdleGCEvent = event != idleGCEvent;

        static_cast<nsIRunnable*>(event)->Run();
        NS_RELEASE(event);
      }

      currentStatus = mStatus;
      scheduleIdleGC = mControlQueue.IsEmpty() &&
                       mQueue.IsEmpty() &&
                       eventIsNotIdleGCEvent &&
                       JS::CurrentGlobalOrNull(aCx);
    }

    
    
    
    if (currentStatus > Terminating || scheduleIdleGC) {
      if (NS_SUCCEEDED(gcTimer->Cancel())) {
        normalGCTimerRunning = false;
      }
      else {
        NS_WARNING("Failed to cancel GC timer!");
      }
    }

    if (scheduleIdleGC) {
      NS_ASSERTION(JS::CurrentGlobalOrNull(aCx), "Should have global here!");

      
      JSAutoCompartment ac(aCx, JS::CurrentGlobalOrNull(aCx));
      JS_MaybeGC(aCx);

      if (NS_SUCCEEDED(gcTimer->SetTarget(idleGCEventTarget)) &&
          NS_SUCCEEDED(gcTimer->InitWithFuncCallback(
                                                    DummyCallback, nullptr,
                                                    IDLE_GC_TIMER_DELAY_MS,
                                                    nsITimer::TYPE_ONE_SHOT))) {
      }
      else {
        JS_ReportError(aCx, "Failed to start idle GC timer!");
      }
    }

    if (currentStatus != Running && !HasActiveFeatures()) {
      
      
      if (mCloseHandlerFinished && currentStatus != Killing) {
        if (!NotifyInternal(aCx, Killing)) {
          JS_ReportPendingException(aCx);
        }
#ifdef DEBUG
        {
          MutexAutoLock lock(mMutex);
          currentStatus = mStatus;
        }
        NS_ASSERTION(currentStatus == Killing, "Should have changed status!");
#else
        currentStatus = Killing;
#endif
      }

      
      if (currentStatus == Killing) {
        
        if (NS_FAILED(gcTimer->Cancel())) {
          NS_WARNING("Failed to cancel the GC timer!");
        }

        DisableMemoryReporter();

        StopAcceptingEvents();
        return;
      }
    }
  }

  NS_NOTREACHED("Shouldn't get here!");
}

bool
WorkerPrivate::OperationCallback(JSContext* aCx)
{
  AssertIsOnWorkerThread();

  bool mayContinue = true;

  for (;;) {
    
    mayContinue = ProcessAllControlRunnables();

    if (!mayContinue || !mSuspended) {
      break;
    }

    
    JS_GC(JS_GetRuntime(aCx));

    while ((mayContinue = MayContinueRunning())) {
      MutexAutoLock lock(mMutex);
      if (!mControlQueue.IsEmpty()) {
        break;
      }

      WaitForWorkerEvents(PR_MillisecondsToInterval(RemainingRunTimeMS()));
    }
  }

  if (!mayContinue) {
    
    NS_ASSERTION(!JS_IsExceptionPending(aCx),
                 "Should not have an exception set here!");
    return false;
  }

  return true;
}

void
WorkerPrivate::ScheduleDeletion(bool aWasPending)
{
  AssertIsOnWorkerThread();
  NS_ASSERTION(mChildWorkers.IsEmpty(), "Live child workers!");
  NS_ASSERTION(mSyncQueues.IsEmpty(), "Should have no sync queues here!");

  StopAcceptingEvents();

  nsIThread* currentThread;
  if (aWasPending) {
    
    currentThread = nullptr;
  }
  else {
    currentThread = NS_GetCurrentThread();
    NS_ASSERTION(currentThread, "This should never be null!");
  }

  WorkerPrivate* parent = GetParent();
  if (parent) {
    nsRefPtr<WorkerFinishedRunnable> runnable =
      new WorkerFinishedRunnable(parent, this, currentThread);
    if (!runnable->Dispatch(nullptr)) {
      NS_WARNING("Failed to dispatch runnable!");
    }
  }
  else {
    nsRefPtr<TopLevelWorkerFinishedRunnable> runnable =
      new TopLevelWorkerFinishedRunnable(this, currentThread);
    if (NS_FAILED(NS_DispatchToMainThread(runnable, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch runnable!");
    }
  }
}

bool
WorkerPrivate::BlockAndCollectRuntimeStats(JS::RuntimeStats* aRtStats)
{
  AssertIsOnMainThread();
  mMutex.AssertCurrentThreadOwns();
  NS_ASSERTION(aRtStats, "Null RuntimeStats!");

  NS_ASSERTION(!mMemoryReporterRunning, "How can we get reentered here?!");

  
  mMemoryReporterRunning = true;

  NS_ASSERTION(mJSContext, "This must never be null!");
  JSRuntime* rt = JS_GetRuntime(mJSContext);

  
  
  
  if (!mBlockedForMemoryReporter) {
    JS_TriggerOperationCallback(rt);

    
    while (!mBlockedForMemoryReporter) {
      mMemoryReportCondVar.Wait();
    }
  }

  bool succeeded = false;

  
  
  if (mMemoryReporter) {
    
    MutexAutoUnlock unlock(mMutex);
    succeeded = JS::CollectRuntimeStats(rt, aRtStats, nullptr);
  }

  NS_ASSERTION(mMemoryReporterRunning, "This isn't possible!");
  NS_ASSERTION(mBlockedForMemoryReporter, "Somehow we got unblocked!");

  
  mMemoryReporterRunning = false;

  
  mMemoryReportCondVar.Notify();

  return succeeded;
}

void
WorkerPrivate::EnableMemoryReporter()
{
  AssertIsOnWorkerThread();

  
  
  mMemoryReporter = new MemoryReporter(this);

  if (NS_FAILED(NS_RegisterMemoryReporter(mMemoryReporter))) {
    NS_WARNING("Failed to register memory reporter!");
    
    
    mMemoryReporter = nullptr;

    return;
  }
}

void
WorkerPrivate::DisableMemoryReporter()
{
  AssertIsOnWorkerThread();

  nsRefPtr<MemoryReporter> memoryReporter;
  {
    MutexAutoLock lock(mMutex);

    
    
    if (!mMemoryReporter) {
      return;
    }

    
    
    mMemoryReporter.swap(memoryReporter);

    
    
    memoryReporter->Disable();

    
    
    if (mMemoryReporterRunning) {
      NS_ASSERTION(!mBlockedForMemoryReporter,
                   "Can't be blocked in more than one place at the same time!");
      mBlockedForMemoryReporter = true;

      
      mMemoryReportCondVar.Notify();

      
      
      while (mMemoryReporterRunning) {
        mMemoryReportCondVar.Wait();
      }

      NS_ASSERTION(mBlockedForMemoryReporter, "Somehow we got unblocked!");
      mBlockedForMemoryReporter = false;
    }
  }

  
  if (NS_FAILED(NS_UnregisterMemoryReporter(memoryReporter))) {
    NS_WARNING("Failed to unregister memory reporter!");
  }
}

void
WorkerPrivate::WaitForWorkerEvents(PRIntervalTime aInterval)
{
  AssertIsOnWorkerThread();
  mMutex.AssertCurrentThreadOwns();

  NS_ASSERTION(!mBlockedForMemoryReporter,
                "Can't be blocked in more than one place at the same time!");

  
  
  mBlockedForMemoryReporter = true;

  
  mMemoryReportCondVar.Notify();

  
  mCondVar.Wait(aInterval);

  
  
  while (mMemoryReporterRunning) {
    mMemoryReportCondVar.Wait();
  }

  NS_ASSERTION(mBlockedForMemoryReporter, "Somehow we got unblocked!");

  
  mBlockedForMemoryReporter = false;
}

bool
WorkerPrivate::ProcessAllControlRunnables()
{
  AssertIsOnWorkerThread();

  bool result = true;

  for (;;) {
    WorkerRunnable* event;
    {
      MutexAutoLock lock(mMutex);

      
      if (mMemoryReporterRunning) {
        NS_ASSERTION(!mBlockedForMemoryReporter,
                     "Can't be blocked in more than one place at the same "
                     "time!");

        
        
        mBlockedForMemoryReporter = true;

        
        mMemoryReportCondVar.Notify();

        
        while (mMemoryReporterRunning) {
          mMemoryReportCondVar.Wait();
        }

        NS_ASSERTION(mBlockedForMemoryReporter, "Somehow we got unblocked!");

        
        
        mBlockedForMemoryReporter = false;
      }

      if (!mControlQueue.Pop(event)) {
        break;
      }
    }

    if (NS_FAILED(static_cast<nsIRunnable*>(event)->Run())) {
      result = false;
    }

    NS_RELEASE(event);
  }

  return result;
}

bool
WorkerPrivate::CheckXHRParamsAllowed(nsPIDOMWindow* aWindow)
{
  AssertIsOnMainThread();
  NS_ASSERTION(aWindow, "Wrong cannot be null");

  if (!aWindow->GetDocShell()) {
    return false;
  }

  nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
  if (!doc) {
    return false;
  }

  nsCOMPtr<nsIPermissionManager> permMgr = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!permMgr) {
    return false;
  }

  uint32_t permission;
  nsresult rv = permMgr->TestPermissionFromPrincipal(doc->NodePrincipal(),
                                                     "systemXHR", &permission);
  if (NS_FAILED(rv) || permission != nsIPermissionManager::ALLOW_ACTION) {
    return false;
  }

  return true;
}

bool
WorkerPrivate::Dispatch(WorkerRunnable* aEvent, EventQueue* aQueue)
{
  nsRefPtr<WorkerRunnable> event(aEvent);

  {
    MutexAutoLock lock(mMutex);

    if (mStatus == Dead) {
      
      return false;
    }

    if (aQueue == &mQueue) {
      
      Status parentStatus = ParentStatus();
      if (parentStatus >= Terminating) {
        
        return false;
      }

      
      if (parentStatus >= Closing || mStatus >= Closing) {
        
        return true;
      }
    }

    if (!aQueue->Push(event)) {
      return false;
    }

    if (aQueue == &mControlQueue && mJSContext) {
      JS_TriggerOperationCallback(JS_GetRuntime(mJSContext));
    }

    mCondVar.Notify();
  }

  event.forget();
  return true;
}

bool
WorkerPrivate::DispatchToSyncQueue(WorkerSyncRunnable* aEvent)
{
  nsRefPtr<WorkerRunnable> event(aEvent);

  {
    MutexAutoLock lock(mMutex);

    NS_ASSERTION(mSyncQueues.Length() > aEvent->mSyncQueueKey, "Bad event!");

    if (!mSyncQueues[aEvent->mSyncQueueKey]->mQueue.Push(event)) {
      return false;
    }

    mCondVar.Notify();
  }

  event.forget();
  return true;
}

void
WorkerPrivate::ClearQueue(EventQueue* aQueue)
{
  AssertIsOnWorkerThread();
  mMutex.AssertCurrentThreadOwns();

  WorkerRunnable* event;
  while (aQueue->Pop(event)) {
    if (event->WantsToRunDuringClear()) {
      MutexAutoUnlock unlock(mMutex);

      static_cast<nsIRunnable*>(event)->Run();
    }
    event->Release();
  }
}

uint32_t
WorkerPrivate::RemainingRunTimeMS() const
{
  if (mKillTime.IsNull()) {
    return UINT32_MAX;
  }
  TimeDuration runtime = mKillTime - TimeStamp::Now();
  double ms = runtime > TimeDuration(0) ? runtime.ToMilliseconds() : 0;
  return ms > double(UINT32_MAX) ? UINT32_MAX : uint32_t(ms);
}

bool
WorkerPrivate::SuspendInternal(JSContext* aCx)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(!mSuspended, "Already suspended!");

  mSuspended = true;
  return true;
}

bool
WorkerPrivate::ResumeInternal(JSContext* aCx)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(mSuspended, "Not yet suspended!");

  mSuspended = false;
  return true;
}

void
WorkerPrivate::TraceInternal(JSTracer* aTrc)
{
  AssertIsOnWorkerThread();

  for (uint32_t index = 0; index < mTimeouts.Length(); index++) {
    TimeoutInfo* info = mTimeouts[index];
    JS_CallHeapValueTracer(aTrc, &info->mTimeoutVal,
                           "WorkerPrivate timeout value");
    for (uint32_t index2 = 0; index2 < info->mExtraArgVals.Length(); index2++) {
      JS_CallHeapValueTracer(aTrc, &info->mExtraArgVals[index2],
                             "WorkerPrivate timeout extra argument value");
    }
  }
}

bool
WorkerPrivate::ModifyBusyCountFromWorker(JSContext* aCx, bool aIncrease)
{
  AssertIsOnWorkerThread();

  {
    MutexAutoLock lock(mMutex);

    
    
    if (mStatus >= Killing) {
      return true;
    }
  }

  nsRefPtr<ModifyBusyCountRunnable> runnable =
    new ModifyBusyCountRunnable(this, aIncrease);
  return runnable->Dispatch(aCx);
}

bool
WorkerPrivate::AddChildWorker(JSContext* aCx, ParentType* aChildWorker)
{
  AssertIsOnWorkerThread();

#ifdef DEBUG
  {
    Status currentStatus;
    {
    MutexAutoLock lock(mMutex);
    currentStatus = mStatus;
    }

    MOZ_ASSERT(currentStatus == Running);
  }
#endif

  NS_ASSERTION(!mChildWorkers.Contains(aChildWorker),
               "Already know about this one!");
  mChildWorkers.AppendElement(aChildWorker);

  return mChildWorkers.Length() == 1 ?
         ModifyBusyCountFromWorker(aCx, true) :
         true;
}

void
WorkerPrivate::RemoveChildWorker(JSContext* aCx, ParentType* aChildWorker)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(mChildWorkers.Contains(aChildWorker),
               "Didn't know about this one!");
  mChildWorkers.RemoveElement(aChildWorker);

  if (mChildWorkers.IsEmpty() && !ModifyBusyCountFromWorker(aCx, false)) {
    NS_WARNING("Failed to modify busy count!");
  }
}

bool
WorkerPrivate::AddFeature(JSContext* aCx, WorkerFeature* aFeature)
{
  AssertIsOnWorkerThread();

  {
    MutexAutoLock lock(mMutex);

    if (mStatus >= Canceling) {
      return false;
    }
  }

  NS_ASSERTION(!mFeatures.Contains(aFeature), "Already know about this one!");
  mFeatures.AppendElement(aFeature);

  return mFeatures.Length() == 1 ?
         ModifyBusyCountFromWorker(aCx, true) :
         true;
}

void
WorkerPrivate::RemoveFeature(JSContext* aCx, WorkerFeature* aFeature)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(mFeatures.Contains(aFeature), "Didn't know about this one!");
  mFeatures.RemoveElement(aFeature);

  if (mFeatures.IsEmpty() && !ModifyBusyCountFromWorker(aCx, false)) {
    NS_WARNING("Failed to modify busy count!");
  }
}

void
WorkerPrivate::NotifyFeatures(JSContext* aCx, Status aStatus)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(aStatus > Running, "Bad status!");

  if (aStatus >= Closing) {
    CancelAllTimeouts(aCx);
  }

  nsAutoTArray<WorkerFeature*, 30> features;
  features.AppendElements(mFeatures);

  for (uint32_t index = 0; index < features.Length(); index++) {
    if (!features[index]->Notify(aCx, aStatus)) {
      NS_WARNING("Failed to notify feature!");
    }
  }

  nsAutoTArray<ParentType*, 10> children;
  children.AppendElements(mChildWorkers);

  for (uint32_t index = 0; index < children.Length(); index++) {
    if (!children[index]->Notify(aCx, aStatus)) {
      NS_WARNING("Failed to notify child worker!");
    }
  }
}

void
WorkerPrivate::CancelAllTimeouts(JSContext* aCx)
{
  AssertIsOnWorkerThread();

  if (mTimerRunning) {
    NS_ASSERTION(mTimer, "Huh?!");
    NS_ASSERTION(!mTimeouts.IsEmpty(), "Huh?!");

    if (NS_FAILED(mTimer->Cancel())) {
      NS_WARNING("Failed to cancel timer!");
    }

    for (uint32_t index = 0; index < mTimeouts.Length(); index++) {
      mTimeouts[index]->mCanceled = true;
    }

    if (!RunExpiredTimeouts(aCx)) {
      JS_ReportPendingException(aCx);
    }

    mTimerRunning = false;
  }
#ifdef DEBUG
  else if (!mRunningExpiredTimeouts) {
    NS_ASSERTION(mTimeouts.IsEmpty(), "Huh?!");
  }
#endif

  mTimer = nullptr;
}

uint32_t
WorkerPrivate::CreateNewSyncLoop()
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(mSyncQueues.Length() < UINT32_MAX,
               "Should have bailed by now!");

  mSyncQueues.AppendElement(new SyncQueue());
  return mSyncQueues.Length() - 1;
}

bool
WorkerPrivate::RunSyncLoop(JSContext* aCx, uint32_t aSyncLoopKey)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(!mSyncQueues.IsEmpty() ||
               (aSyncLoopKey != mSyncQueues.Length() - 1),
               "Forgot to call CreateNewSyncLoop!");
  if (aSyncLoopKey != mSyncQueues.Length() - 1) {
    return false;
  }

  SyncQueue* syncQueue = mSyncQueues[aSyncLoopKey].get();

  for (;;) {
    WorkerRunnable* event;
    {
      MutexAutoLock lock(mMutex);

      while (!mControlQueue.Pop(event) && !syncQueue->mQueue.Pop(event)) {
        WaitForWorkerEvents();
      }
    }

    static_cast<nsIRunnable*>(event)->Run();
    NS_RELEASE(event);

    if (syncQueue->mComplete) {
      NS_ASSERTION(mSyncQueues.Length() - 1 == aSyncLoopKey,
                   "Mismatched calls!");
      NS_ASSERTION(syncQueue->mQueue.IsEmpty(), "Unprocessed sync events!");

      bool result = syncQueue->mResult;
      DestroySyncLoop(aSyncLoopKey);

#ifdef DEBUG
      syncQueue = nullptr;
#endif

      return result;
    }
  }

  NS_NOTREACHED("Shouldn't get here!");
  return false;
}

void
WorkerPrivate::StopSyncLoop(uint32_t aSyncLoopKey, bool aSyncResult)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(mSyncQueues.IsEmpty() ||
               (aSyncLoopKey == mSyncQueues.Length() - 1),
               "Forgot to call CreateNewSyncLoop!");
  if (aSyncLoopKey != mSyncQueues.Length() - 1) {
    return;
  }

  SyncQueue* syncQueue = mSyncQueues[aSyncLoopKey].get();

  NS_ASSERTION(!syncQueue->mComplete, "Already called StopSyncLoop?!");

  syncQueue->mResult = aSyncResult;
  syncQueue->mComplete = true;
}

void
WorkerPrivate::DestroySyncLoop(uint32_t aSyncLoopKey)
{
  AssertIsOnWorkerThread();

  mSyncQueues.RemoveElementAt(aSyncLoopKey);
}

bool
WorkerPrivate::PostMessageToParent(JSContext* aCx,
                                   JS::Handle<JS::Value> aMessage,
                                   JS::Handle<JS::Value> aTransferable)
{
  AssertIsOnWorkerThread();

  JSStructuredCloneCallbacks* callbacks =
    IsChromeWorker() ?
    &gChromeWorkerStructuredCloneCallbacks :
    &gWorkerStructuredCloneCallbacks;

  nsTArray<nsCOMPtr<nsISupports> > clonedObjects;

  JSAutoStructuredCloneBuffer buffer;
  if (!buffer.write(aCx, aMessage, aTransferable, callbacks, &clonedObjects)) {
    return false;
  }

  nsRefPtr<MessageEventRunnable> runnable =
    new MessageEventRunnable(this, WorkerRunnable::ParentThread, buffer,
                             clonedObjects);
  return runnable->Dispatch(aCx);
}

bool
WorkerPrivate::NotifyInternal(JSContext* aCx, Status aStatus)
{
  AssertIsOnWorkerThread();

  NS_ASSERTION(aStatus > Running && aStatus < Dead, "Bad status!");

  
  Status previousStatus;
  {
    MutexAutoLock lock(mMutex);

    if (mStatus >= aStatus) {
      return true;
    }

    previousStatus = mStatus;
    mStatus = aStatus;
  }

  
  
  if (mCrossThreadDispatcher) {
    
    
    
    
    mCrossThreadDispatcher->Forget();
  }

  NS_ASSERTION(previousStatus != Pending, "How is this possible?!");

  NS_ASSERTION(previousStatus >= Canceling || mKillTime.IsNull(),
               "Bad kill time set!");

  
  NotifyFeatures(aCx, aStatus);

  
  
  if (previousStatus == Running) {
    MutexAutoLock lock(mMutex);
    ClearQueue(&mQueue);
  }

  
  if (mCloseHandlerFinished) {
    return true;
  }

  
  
  if (!JS::CurrentGlobalOrNull(aCx)) {
    mCloseHandlerStarted = true;
    mCloseHandlerFinished = true;
    return true;
  }

  
  
  if (previousStatus == Running && aStatus != Killing) {
    NS_ASSERTION(!mCloseHandlerStarted && !mCloseHandlerFinished,
                 "This is impossible!");

    nsRefPtr<CloseEventRunnable> closeRunnable = new CloseEventRunnable(this);

    MutexAutoLock lock(mMutex);

    if (!mQueue.Push(closeRunnable)) {
      NS_WARNING("Failed to push closeRunnable!");
      return false;
    }

    closeRunnable.forget();
  }

  if (aStatus == Closing) {
    
    nsRefPtr<CloseRunnable> runnable = new CloseRunnable(this);
    if (!runnable->Dispatch(aCx)) {
      return false;
    }

    
    return true;
  }

  if (aStatus == Terminating) {
    
    return mCloseHandlerStarted;
  }

  if (aStatus == Canceling) {
    
    NS_ASSERTION(previousStatus == Running || previousStatus == Closing ||
                 previousStatus == Terminating,
                 "Bad previous status!");

    uint32_t killSeconds = IsChromeWorker() ?
      RuntimeService::GetChromeCloseHandlerTimeoutSeconds() :
      RuntimeService::GetContentCloseHandlerTimeoutSeconds();

    if (killSeconds) {
      mKillTime = TimeStamp::Now() + TimeDuration::FromSeconds(killSeconds);

      if (!mCloseHandlerFinished && !ScheduleKillCloseEventRunnable(aCx)) {
        return false;
      }
    }

    
    return mCloseHandlerStarted;
  }

  if (aStatus == Killing) {
    mKillTime = TimeStamp::Now();

    if (!mCloseHandlerFinished && !ScheduleKillCloseEventRunnable(aCx)) {
      return false;
    }

    
    return false;
  }

  NS_NOTREACHED("Should never get here!");
  return false;
}

bool
WorkerPrivate::ScheduleKillCloseEventRunnable(JSContext* aCx)
{
  AssertIsOnWorkerThread();
  NS_ASSERTION(!mKillTime.IsNull(), "Must have a kill time!");

  nsRefPtr<KillCloseEventRunnable> killCloseEventRunnable =
    new KillCloseEventRunnable(this);
  if (!killCloseEventRunnable->SetTimeout(aCx, RemainingRunTimeMS())) {
    return false;
  }

  MutexAutoLock lock(mMutex);

  if (!mQueue.Push(killCloseEventRunnable)) {
    NS_WARNING("Failed to push killCloseEventRunnable!");
    return false;
  }

  killCloseEventRunnable.forget();
  return true;
}

void
WorkerPrivate::ReportError(JSContext* aCx, const char* aMessage,
                           JSErrorReport* aReport)
{
  AssertIsOnWorkerThread();

  if (!MayContinueRunning() || mErrorHandlerRecursionCount == 2) {
    return;
  }

  NS_ASSERTION(mErrorHandlerRecursionCount == 0 ||
               mErrorHandlerRecursionCount == 1,
               "Bad recursion logic!");

  JS_ClearPendingException(aCx);

  nsString message, filename, line;
  uint32_t lineNumber, columnNumber, flags, errorNumber;

  if (aReport) {
    if (aReport->ucmessage) {
      message = aReport->ucmessage;
    }
    filename = NS_ConvertUTF8toUTF16(aReport->filename);
    line = aReport->uclinebuf;
    lineNumber = aReport->lineno;
    columnNumber = aReport->uctokenptr - aReport->uclinebuf;
    flags = aReport->flags;
    errorNumber = aReport->errorNumber;
  }
  else {
    lineNumber = columnNumber = errorNumber = 0;
    flags = nsIScriptError::errorFlag | nsIScriptError::exceptionFlag;
  }

  if (message.IsEmpty()) {
    message = NS_ConvertUTF8toUTF16(aMessage);
  }

  mErrorHandlerRecursionCount++;

  
  
  bool fireAtScope = mErrorHandlerRecursionCount == 1 &&
                     !mCloseHandlerStarted &&
                     errorNumber != JSMSG_OUT_OF_MEMORY;

  if (!ReportErrorRunnable::ReportError(aCx, this, fireAtScope, nullptr, message,
                                        filename, line, lineNumber,
                                        columnNumber, flags, errorNumber, 0)) {
    JS_ReportPendingException(aCx);
  }

  mErrorHandlerRecursionCount--;
}

bool
WorkerPrivate::SetTimeout(JSContext* aCx, unsigned aArgc, jsval* aVp,
                          bool aIsInterval)
{
  AssertIsOnWorkerThread();
  NS_ASSERTION(aArgc, "Huh?!");

  const uint32_t timerId = mNextTimeoutId++;

  Status currentStatus;
  {
    MutexAutoLock lock(mMutex);
    currentStatus = mStatus;
  }

  
  
  if (currentStatus == Closing) {
    JS_ReportError(aCx, "Cannot schedule timeouts from the close handler!");
  }

  
  
  if (currentStatus >= Closing) {
    return false;
  }

  nsAutoPtr<TimeoutInfo> newInfo(new TimeoutInfo());
  newInfo->mIsInterval = aIsInterval;
  newInfo->mId = timerId;

  if (MOZ_UNLIKELY(timerId == UINT32_MAX)) {
    NS_WARNING("Timeout ids overflowed!");
    mNextTimeoutId = 1;
  }

  JS::Value* argv = JS_ARGV(aCx, aVp);

  
  if (argv[0].isObject()) {
    if (JS_ObjectIsCallable(aCx, &argv[0].toObject())) {
      newInfo->mTimeoutVal = argv[0];
    }
    else {
      JSString* timeoutStr = JS_ValueToString(aCx, argv[0]);
      if (!timeoutStr) {
        return false;
      }
      newInfo->mTimeoutVal.setString(timeoutStr);
    }
  }
  else if (argv[0].isString()) {
    newInfo->mTimeoutVal = argv[0];
  }
  else {
    JS_ReportError(aCx, "Useless %s call (missing quotes around argument?)",
                   aIsInterval ? "setInterval" : "setTimeout");
    return false;
  }

  
  if (aArgc > 1) {
    double intervalMS = 0;
    if (!JS_ValueToNumber(aCx, argv[1], &intervalMS)) {
      return false;
    }
    newInfo->mInterval = TimeDuration::FromMilliseconds(intervalMS);

    if (aArgc > 2 && newInfo->mTimeoutVal.isObject()) {
      nsTArray<JS::Heap<JS::Value> > extraArgVals(aArgc - 2);
      for (unsigned index = 2; index < aArgc; index++) {
        extraArgVals.AppendElement(argv[index]);
      }
      newInfo->mExtraArgVals.SwapElements(extraArgVals);
    }
  }

  newInfo->mTargetTime = TimeStamp::Now() + newInfo->mInterval;

  if (newInfo->mTimeoutVal.isString()) {
    const char* filenameChars;
    uint32_t lineNumber;
    if (nsJSUtils::GetCallingLocation(aCx, &filenameChars, &lineNumber)) {
      newInfo->mFilename = filenameChars;
      newInfo->mLineNumber = lineNumber;
    }
    else {
      NS_WARNING("Failed to get calling location!");
    }
  }

  mTimeouts.InsertElementSorted(newInfo.get(), GetAutoPtrComparator(mTimeouts));

  
  
  if (mTimeouts[0] == newInfo) {
    nsresult rv;

    if (!mTimer) {
      mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      if (NS_FAILED(rv)) {
        JS_ReportError(aCx, "Failed to create timer!");
        return false;
      }

      nsRefPtr<TimerRunnable> timerRunnable = new TimerRunnable(this);

      nsCOMPtr<nsIEventTarget> target =
        new WorkerRunnableEventTarget(timerRunnable);
      rv = mTimer->SetTarget(target);
      if (NS_FAILED(rv)) {
        JS_ReportError(aCx, "Failed to set timer's target!");
        return false;
      }
    }

    if (!mTimerRunning) {
      if (!ModifyBusyCountFromWorker(aCx, true)) {
        return false;
      }
      mTimerRunning = true;
    }

    if (!RescheduleTimeoutTimer(aCx)) {
      return false;
    }
  }

  JS_SET_RVAL(aCx, aVp, INT_TO_JSVAL(timerId));

  newInfo.forget();
  return true;
}

bool
WorkerPrivate::ClearTimeout(JSContext* aCx, uint32_t aId)
{
  AssertIsOnWorkerThread();

  if (!mTimeouts.IsEmpty()) {
    NS_ASSERTION(mTimerRunning, "Huh?!");

    for (uint32_t index = 0; index < mTimeouts.Length(); index++) {
      nsAutoPtr<TimeoutInfo>& info = mTimeouts[index];
      if (info->mId == aId) {
        info->mCanceled = true;
        break;
      }
    }
  }

  return true;
}

bool
WorkerPrivate::RunExpiredTimeouts(JSContext* aCx)
{
  AssertIsOnWorkerThread();

  
  
  
  if (mRunningExpiredTimeouts || !mTimerRunning) {
    return true;
  }

  NS_ASSERTION(mTimer, "Must have a timer!");
  NS_ASSERTION(!mTimeouts.IsEmpty(), "Should have some work to do!");

  bool retval = true;

  AutoPtrComparator<TimeoutInfo> comparator = GetAutoPtrComparator(mTimeouts);
  JS::RootedObject global(aCx, JS::CurrentGlobalOrNull(aCx));
  JSPrincipals* principal = GetWorkerPrincipal();

  
  
  const TimeStamp now = std::max(TimeStamp::Now(), mTimeouts[0]->mTargetTime);

  nsAutoTArray<TimeoutInfo*, 10> expiredTimeouts;
  for (uint32_t index = 0; index < mTimeouts.Length(); index++) {
    nsAutoPtr<TimeoutInfo>& info = mTimeouts[index];
    if (info->mTargetTime > now) {
      break;
    }
    expiredTimeouts.AppendElement(info);
  }

  
  mRunningExpiredTimeouts = true;

  
  for (uint32_t index = 0; index < expiredTimeouts.Length(); index++) {
    TimeoutInfo*& info = expiredTimeouts[index];

    if (info->mCanceled) {
      continue;
    }

    
    
    

    if (info->mTimeoutVal.isString()) {
      JSString* expression = info->mTimeoutVal.toString();

      JS::CompileOptions options(aCx);
      options.setPrincipals(principal)
        .setFileAndLine(info->mFilename.get(), info->mLineNumber);

      size_t stringLength;
      const jschar* string = JS_GetStringCharsAndLength(aCx, expression,
                                                        &stringLength);
      if ((!string || !JS::Evaluate(aCx, global, options, string, stringLength, nullptr)) &&
          !JS_ReportPendingException(aCx)) {
        retval = false;
        break;
      }
    }
    else {
      JS::Rooted<JS::Value> rval(aCx);
      



      if (!JS_CallFunctionValue(aCx, global, info->mTimeoutVal,
                                info->mExtraArgVals.Length(),
                                info->mExtraArgVals.Elements()->unsafeGet(),
                                rval.address()) &&
          !JS_ReportPendingException(aCx)) {
        retval = false;
        break;
      }
    }

    NS_ASSERTION(mRunningExpiredTimeouts, "Someone changed this!");
  }

  
  mRunningExpiredTimeouts = false;

  
  
  
  
  
  
  for (uint32_t index = 0, expiredTimeoutIndex = 0,
       expiredTimeoutLength = expiredTimeouts.Length();
       index < mTimeouts.Length(); ) {
    nsAutoPtr<TimeoutInfo>& info = mTimeouts[index];
    if ((expiredTimeoutIndex < expiredTimeoutLength &&
         info == expiredTimeouts[expiredTimeoutIndex] &&
         ++expiredTimeoutIndex) ||
        info->mCanceled) {
      if (info->mIsInterval && !info->mCanceled) {
        
        info->mTargetTime = info->mTargetTime + info->mInterval;
        
        ++index;
      }
      else {
        mTimeouts.RemoveElement(info);
      }
    }
    else {
      
      
      NS_ASSERTION(!expiredTimeouts.Contains(info),
                   "Our timeouts are out of order!");
      ++index;
    }
  }

  mTimeouts.Sort(comparator);

  
  
  if (mTimeouts.IsEmpty()) {
    if (!ModifyBusyCountFromWorker(aCx, false)) {
      retval = false;
    }
    mTimerRunning = false;
  }
  else if (retval && !RescheduleTimeoutTimer(aCx)) {
    retval = false;
  }

  return retval;
}

bool
WorkerPrivate::RescheduleTimeoutTimer(JSContext* aCx)
{
  AssertIsOnWorkerThread();
  NS_ASSERTION(!mTimeouts.IsEmpty(), "Should have some timeouts!");
  NS_ASSERTION(mTimer, "Should have a timer!");

  double delta =
    (mTimeouts[0]->mTargetTime - TimeStamp::Now()).ToMilliseconds();
  uint32_t delay = delta > 0 ? std::min(delta, double(UINT32_MAX)) : 0;

  nsresult rv = mTimer->InitWithFuncCallback(DummyCallback, nullptr, delay,
                                             nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv)) {
    JS_ReportError(aCx, "Failed to start timer!");
    return false;
  }

  return true;
}

void
WorkerPrivate::UpdateJSContextOptionsInternal(JSContext* aCx,
                                              uint32_t aContentOptions,
                                              uint32_t aChromeOptions)
{
  AssertIsOnWorkerThread();

  JS_SetOptions(aCx, IsChromeWorker() ? aChromeOptions : aContentOptions);

  for (uint32_t index = 0; index < mChildWorkers.Length(); index++) {
    mChildWorkers[index]->UpdateJSContextOptions(aCx, aContentOptions,
                                                 aChromeOptions);
  }
}

void
WorkerPrivate::UpdateJSWorkerMemoryParameterInternal(JSContext* aCx,
                                                     JSGCParamKey aKey,
                                                     uint32_t aValue)
{
  AssertIsOnWorkerThread();

  
  
  
  
  if (aValue) {
    JS_SetGCParameter(JS_GetRuntime(aCx), aKey, aValue);
  }

  for (uint32_t index = 0; index < mChildWorkers.Length(); index++) {
    mChildWorkers[index]->UpdateJSWorkerMemoryParameter(aCx, aKey, aValue);
  }
}

#ifdef JS_GC_ZEAL
void
WorkerPrivate::UpdateGCZealInternal(JSContext* aCx, uint8_t aGCZeal,
                                    uint32_t aFrequency)
{
  AssertIsOnWorkerThread();

  JS_SetGCZeal(aCx, aGCZeal, aFrequency);

  for (uint32_t index = 0; index < mChildWorkers.Length(); index++) {
    mChildWorkers[index]->UpdateGCZeal(aCx, aGCZeal, aFrequency);
  }
}
#endif

void
WorkerPrivate::UpdateJITHardeningInternal(JSContext* aCx, bool aJITHardening)
{
  AssertIsOnWorkerThread();

  JS_SetJitHardening(JS_GetRuntime(aCx), aJITHardening);

  for (uint32_t index = 0; index < mChildWorkers.Length(); index++) {
    mChildWorkers[index]->UpdateJITHardening(aCx, aJITHardening);
  }
}

void
WorkerPrivate::GarbageCollectInternal(JSContext* aCx, bool aShrinking,
                                      bool aCollectChildren)
{
  AssertIsOnWorkerThread();

  if (aShrinking || aCollectChildren) {
    JSRuntime* rt = JS_GetRuntime(aCx);
    JS::PrepareForFullGC(rt);

    if (aShrinking) {
      JS::ShrinkingGC(rt, JS::gcreason::DOM_WORKER);
    }
    else {
      JS::GCForReason(rt, JS::gcreason::DOM_WORKER);
    }
  }
  else {
    JS_MaybeGC(aCx);
  }

  if (aCollectChildren) {
    for (uint32_t index = 0; index < mChildWorkers.Length(); index++) {
      mChildWorkers[index]->GarbageCollect(aCx, aShrinking);
    }
  }
}

#ifdef DEBUG
template <class Derived>
void
WorkerPrivateParent<Derived>::AssertIsOnParentThread() const
{
  if (GetParent()) {
    GetParent()->AssertIsOnWorkerThread();
  }
  else {
    AssertIsOnMainThread();
  }
}

template <class Derived>
void
WorkerPrivateParent<Derived>::AssertInnerWindowIsCorrect() const
{
  AssertIsOnParentThread();

  
  if (mParent || !mWindow) {
    return;
  }

  AssertIsOnMainThread();

  nsPIDOMWindow* outer = mWindow->GetOuterWindow();
  NS_ASSERTION(outer && outer->GetCurrentInnerWindow() == mWindow,
               "Inner window no longer correct!");
}

void
WorkerPrivate::AssertIsOnWorkerThread() const
{
  if (mThread) {
    bool current;
    if (NS_FAILED(mThread->IsOnCurrentThread(&current)) || !current) {
      NS_ERROR("Wrong thread!");
    }
  }
  else {
    NS_ERROR("Trying to assert thread identity after thread has been "
             "shutdown!");
  }
}
#endif

template <class Derived>
void
WorkerPrivateParent<Derived>::RegisterHostObjectURI(const nsACString& aURI)
{
  AssertIsOnMainThread();
  mHostObjectURIs.AppendElement(aURI);
}

template <class Derived>
void
WorkerPrivateParent<Derived>::UnregisterHostObjectURI(const nsACString& aURI)
{
  AssertIsOnMainThread();
  mHostObjectURIs.RemoveElement(aURI);
}

template <class Derived>
void
WorkerPrivateParent<Derived>::StealHostObjectURIs(nsTArray<nsCString>& aArray)
{
  aArray.SwapElements(mHostObjectURIs);
}

WorkerCrossThreadDispatcher*
WorkerPrivate::GetCrossThreadDispatcher()
{
  mozilla::MutexAutoLock lock(mMutex);
  if (!mCrossThreadDispatcher && mStatus <= Running) {
    mCrossThreadDispatcher = new WorkerCrossThreadDispatcher(this);
  }
  return mCrossThreadDispatcher;
}

void
WorkerPrivate::BeginCTypesCall()
{
  AssertIsOnWorkerThread();

  MutexAutoLock lock(mMutex);

  NS_ASSERTION(!mBlockedForMemoryReporter,
               "Can't be blocked in more than one place at the same time!");

  
  
  
  
  
  mBlockedForMemoryReporter = true;

  
  mMemoryReportCondVar.Notify();
}

void
WorkerPrivate::EndCTypesCall()
{
  AssertIsOnWorkerThread();

  MutexAutoLock lock(mMutex);

  NS_ASSERTION(mBlockedForMemoryReporter, "Somehow we got unblocked!");

  
  while (mMemoryReporterRunning) {
    mMemoryReportCondVar.Wait();
  }

  
  
  mBlockedForMemoryReporter = false;
}

BEGIN_WORKERS_NAMESPACE


template class WorkerPrivateParent<WorkerPrivate>;

JSStructuredCloneCallbacks*
WorkerStructuredCloneCallbacks(bool aMainRuntime)
{
  return aMainRuntime ?
         &gMainThreadWorkerStructuredCloneCallbacks :
         &gWorkerStructuredCloneCallbacks;
}

JSStructuredCloneCallbacks*
ChromeWorkerStructuredCloneCallbacks(bool aMainRuntime)
{
  return aMainRuntime ?
         &gMainThreadChromeWorkerStructuredCloneCallbacks :
         &gChromeWorkerStructuredCloneCallbacks;
}

END_WORKERS_NAMESPACE
