




#include "base/basictypes.h"

#include "nsFrameMessageManager.h"

#include "AppProcessChecker.h"
#include "ContentChild.h"
#include "ContentParent.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsError.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "nsJSUtils.h"
#include "nsJSPrincipals.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsFrameLoader.h"
#include "nsIXULRuntime.h"
#include "nsIScriptError.h"
#include "nsIConsoleService.h"
#include "nsIMemoryReporter.h"
#include "nsIProtocolHandler.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSRuntimeService.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMFile.h"
#include "xpcpublic.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/StructuredCloneUtils.h"
#include "mozilla/dom/PBlobChild.h"
#include "mozilla/dom/PBlobParent.h"
#include "JavaScriptChild.h"
#include "JavaScriptParent.h"
#include "mozilla/dom/DOMStringList.h"
#include "nsPrintfCString.h"
#include <algorithm>

#ifdef ANDROID
#include <android/log.h>
#endif
#ifdef XP_WIN
#include <windows.h>
# if defined(SendMessage)
#  undef SendMessage
# endif
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::ipc;

static PLDHashOperator
CycleCollectorTraverseListeners(const nsAString& aKey,
                                nsAutoTObserverArray<nsMessageListenerInfo, 1>* aListeners,
                                void* aCb)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*> (aCb);
  uint32_t count = aListeners->Length();
  for (uint32_t i = 0; i < count; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "listeners[i] mStrongListener");
    cb->NoteXPCOMChild(aListeners->ElementAt(i).mStrongListener.get());
  }
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsFrameMessageManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFrameMessageManager)
  tmp->mListeners.EnumerateRead(CycleCollectorTraverseListeners,
                                static_cast<void*>(&cb));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChildManagers)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFrameMessageManager)
  tmp->mListeners.Clear();
  for (int32_t i = tmp->mChildManagers.Count(); i > 0; --i) {
    static_cast<nsFrameMessageManager*>(tmp->mChildManagers[i - 1])->
      Disconnect(false);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChildManagers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentFrameMessageManager)

  



  NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIMessageListenerManager,
                                    (mIsBroadcaster ?
                                       static_cast<nsIMessageListenerManager*>(
                                         static_cast<nsIMessageBroadcaster*>(this)) :
                                       static_cast<nsIMessageListenerManager*>(
                                         static_cast<nsIMessageSender*>(this))))

  



  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsISyncMessageSender, !mChrome)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIMessageSender, !mChrome || !mIsBroadcaster)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIMessageBroadcaster, mChrome && mIsBroadcaster)

  
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIContentFrameMessageManager,
                                     !mChrome && !mIsProcessManager)

  
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIFrameScriptLoader,
                                     mChrome && !mIsProcessManager)

  
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIProcessChecker,
                                     mChrome && !mIsBroadcaster)

  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO_CONDITIONAL(ChromeMessageBroadcaster,
                                                   mChrome && mIsBroadcaster)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO_CONDITIONAL(ChromeMessageSender,
                                                   mChrome && !mIsBroadcaster)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFrameMessageManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFrameMessageManager)

enum ActorFlavorEnum {
  Parent = 0,
  Child
};

template <ActorFlavorEnum>
struct BlobTraits
{ };

template <>
struct BlobTraits<Parent>
{
  typedef mozilla::dom::BlobParent BlobType;
  typedef mozilla::dom::PBlobParent ProtocolType;
  typedef mozilla::dom::ContentParent ConcreteContentManagerType;
};

template <>
struct BlobTraits<Child>
{
  typedef mozilla::dom::BlobChild BlobType;
  typedef mozilla::dom::PBlobChild ProtocolType;
  typedef mozilla::dom::ContentChild ConcreteContentManagerType;
};

template<ActorFlavorEnum>
struct DataBlobs
{ };

template<>
struct DataBlobs<Parent>
{
  typedef BlobTraits<Parent>::ProtocolType ProtocolType;

  static InfallibleTArray<ProtocolType*>& Blobs(ClonedMessageData& aData)
  {
    return aData.blobsParent();
  }

  static const InfallibleTArray<ProtocolType*>& Blobs(const ClonedMessageData& aData)
  {
    return aData.blobsParent();
  }
};

template<>
struct DataBlobs<Child>
{
  typedef BlobTraits<Child>::ProtocolType ProtocolType;

  static InfallibleTArray<ProtocolType*>& Blobs(ClonedMessageData& aData)
  {
    return aData.blobsChild();
  }

  static const InfallibleTArray<ProtocolType*>& Blobs(const ClonedMessageData& aData)
  {
    return aData.blobsChild();
  }
};

template<ActorFlavorEnum Flavor>
static bool
BuildClonedMessageData(typename BlobTraits<Flavor>::ConcreteContentManagerType* aManager,
                       const StructuredCloneData& aData,
                       ClonedMessageData& aClonedData)
{
  SerializedStructuredCloneBuffer& buffer = aClonedData.data();
  buffer.data = aData.mData;
  buffer.dataLength = aData.mDataLength;
  const nsTArray<nsCOMPtr<nsIDOMBlob> >& blobs = aData.mClosure.mBlobs;
  if (!blobs.IsEmpty()) {
    typedef typename BlobTraits<Flavor>::ProtocolType ProtocolType;
    InfallibleTArray<ProtocolType*>& blobList = DataBlobs<Flavor>::Blobs(aClonedData);
    uint32_t length = blobs.Length();
    blobList.SetCapacity(length);
    for (uint32_t i = 0; i < length; ++i) {
      typename BlobTraits<Flavor>::BlobType* protocolActor =
        aManager->GetOrCreateActorForBlob(blobs[i]);
      if (!protocolActor) {
        return false;
      }
      blobList.AppendElement(protocolActor);
    }
  }
  return true;
}

bool
MessageManagerCallback::BuildClonedMessageDataForParent(ContentParent* aParent,
                                                        const StructuredCloneData& aData,
                                                        ClonedMessageData& aClonedData)
{
  return BuildClonedMessageData<Parent>(aParent, aData, aClonedData);
}

bool
MessageManagerCallback::BuildClonedMessageDataForChild(ContentChild* aChild,
                                                       const StructuredCloneData& aData,
                                                       ClonedMessageData& aClonedData)
{
  return BuildClonedMessageData<Child>(aChild, aData, aClonedData);
}

template<ActorFlavorEnum Flavor>
static StructuredCloneData
UnpackClonedMessageData(const ClonedMessageData& aData)
{
  const SerializedStructuredCloneBuffer& buffer = aData.data();
  typedef typename BlobTraits<Flavor>::ProtocolType ProtocolType;
  const InfallibleTArray<ProtocolType*>& blobs = DataBlobs<Flavor>::Blobs(aData);
  StructuredCloneData cloneData;
  cloneData.mData = buffer.data;
  cloneData.mDataLength = buffer.dataLength;
  if (!blobs.IsEmpty()) {
    uint32_t length = blobs.Length();
    cloneData.mClosure.mBlobs.SetCapacity(length);
    for (uint32_t i = 0; i < length; ++i) {
      auto* blob =
        static_cast<typename BlobTraits<Flavor>::BlobType*>(blobs[i]);
      MOZ_ASSERT(blob);
      nsCOMPtr<nsIDOMBlob> domBlob = blob->GetBlob();
      MOZ_ASSERT(domBlob);
      cloneData.mClosure.mBlobs.AppendElement(domBlob);
    }
  }
  return cloneData;
}

StructuredCloneData
mozilla::dom::ipc::UnpackClonedMessageDataForParent(const ClonedMessageData& aData)
{
  return UnpackClonedMessageData<Parent>(aData);
}

StructuredCloneData
mozilla::dom::ipc::UnpackClonedMessageDataForChild(const ClonedMessageData& aData)
{
  return UnpackClonedMessageData<Child>(aData);
}

bool
SameProcessCpowHolder::ToObject(JSContext* aCx,
                                JS::MutableHandle<JSObject*> aObjp)
{
  if (!mObj) {
    return true;
  }

  aObjp.set(mObj);
  return JS_WrapObject(aCx, aObjp);
}



NS_IMETHODIMP
nsFrameMessageManager::AddMessageListener(const nsAString& aMessage,
                                          nsIMessageListener* aListener)
{
  nsAutoTObserverArray<nsMessageListenerInfo, 1>* listeners =
    mListeners.Get(aMessage);
  if (!listeners) {
    listeners = new nsAutoTObserverArray<nsMessageListenerInfo, 1>();
    mListeners.Put(aMessage, listeners);
  } else {
    uint32_t len = listeners->Length();
    for (uint32_t i = 0; i < len; ++i) {
      if (listeners->ElementAt(i).mStrongListener == aListener) {
        return NS_OK;
      }
    }
  }

  nsMessageListenerInfo* entry = listeners->AppendElement();
  NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);
  entry->mStrongListener = aListener;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::RemoveMessageListener(const nsAString& aMessage,
                                             nsIMessageListener* aListener)
{
  nsAutoTObserverArray<nsMessageListenerInfo, 1>* listeners =
    mListeners.Get(aMessage);
  if (!listeners) {
    return NS_OK;
  }

  uint32_t len = listeners->Length();
  for (uint32_t i = 0; i < len; ++i) {
    if (listeners->ElementAt(i).mStrongListener == aListener) {
      listeners->RemoveElementAt(i);
      return NS_OK;
    }
  }
  return NS_OK;
}

#ifdef DEBUG
typedef struct
{
  nsCOMPtr<nsISupports> mCanonical;
  nsWeakPtr mWeak;
} CanonicalCheckerParams;

static PLDHashOperator
CanonicalChecker(const nsAString& aKey,
                 nsAutoTObserverArray<nsMessageListenerInfo, 1>* aListeners,
                 void* aParams)
{
  CanonicalCheckerParams* params =
    static_cast<CanonicalCheckerParams*> (aParams);

  uint32_t count = aListeners->Length();
  for (uint32_t i = 0; i < count; i++) {
    if (!aListeners->ElementAt(i).mWeakListener) {
      continue;
    }
    nsCOMPtr<nsISupports> otherCanonical =
      do_QueryReferent(aListeners->ElementAt(i).mWeakListener);
    MOZ_ASSERT((params->mCanonical == otherCanonical) ==
               (params->mWeak == aListeners->ElementAt(i).mWeakListener));
  }
  return PL_DHASH_NEXT;
}
#endif

NS_IMETHODIMP
nsFrameMessageManager::AddWeakMessageListener(const nsAString& aMessage,
                                              nsIMessageListener* aListener)
{
  nsWeakPtr weak = do_GetWeakReference(aListener);
  NS_ENSURE_TRUE(weak, NS_ERROR_NO_INTERFACE);

#ifdef DEBUG
  
  
  
  
  nsCOMPtr<nsISupports> canonical = do_QueryInterface(aListener);
  CanonicalCheckerParams params;
  params.mCanonical = canonical;
  params.mWeak = weak;
  mListeners.EnumerateRead(CanonicalChecker, (void*)&params);
#endif

  nsAutoTObserverArray<nsMessageListenerInfo, 1>* listeners =
    mListeners.Get(aMessage);
  if (!listeners) {
    listeners = new nsAutoTObserverArray<nsMessageListenerInfo, 1>();
    mListeners.Put(aMessage, listeners);
  } else {
    uint32_t len = listeners->Length();
    for (uint32_t i = 0; i < len; ++i) {
      if (listeners->ElementAt(i).mWeakListener == weak) {
        return NS_OK;
      }
    }
  }

  nsMessageListenerInfo* entry = listeners->AppendElement();
  entry->mWeakListener = weak;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::RemoveWeakMessageListener(const nsAString& aMessage,
                                                 nsIMessageListener* aListener)
{
  nsWeakPtr weak = do_GetWeakReference(aListener);
  NS_ENSURE_TRUE(weak, NS_OK);

  nsAutoTObserverArray<nsMessageListenerInfo, 1>* listeners =
    mListeners.Get(aMessage);
  if (!listeners) {
    return NS_OK;
  }

  uint32_t len = listeners->Length();
  for (uint32_t i = 0; i < len; ++i) {
    if (listeners->ElementAt(i).mWeakListener == weak) {
      listeners->RemoveElementAt(i);
      return NS_OK;
    }
  }

  return NS_OK;
}



NS_IMETHODIMP
nsFrameMessageManager::LoadFrameScript(const nsAString& aURL,
                                       bool aAllowDelayedLoad,
                                       bool aRunInGlobalScope)
{
  
  aRunInGlobalScope = true;

  if (aAllowDelayedLoad) {
    if (IsGlobal() || IsWindowLevel()) {
      
      mPendingScripts.AppendElement(aURL);
      mPendingScriptsGlobalStates.AppendElement(aRunInGlobalScope);
    } else if (!mCallback) {
      
      mPendingScripts.AppendElement(aURL);
      mPendingScriptsGlobalStates.AppendElement(aRunInGlobalScope);
      return NS_OK;
    }
  }

  if (mCallback) {
#ifdef DEBUG_smaug
    printf("Will load %s \n", NS_ConvertUTF16toUTF8(aURL).get());
#endif
    NS_ENSURE_TRUE(mCallback->DoLoadFrameScript(aURL, aRunInGlobalScope),
                   NS_ERROR_FAILURE);
  }

  for (int32_t i = 0; i < mChildManagers.Count(); ++i) {
    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(mChildManagers[i]);
    if (mm) {
      
      
      mm->LoadFrameScript(aURL, false, aRunInGlobalScope);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::RemoveDelayedFrameScript(const nsAString& aURL)
{
  for (uint32_t i = 0; i < mPendingScripts.Length(); ++i) {
    if (mPendingScripts[i] == aURL) {
      mPendingScripts.RemoveElementAt(i);
      mPendingScriptsGlobalStates.RemoveElementAt(i);
      break;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::GetDelayedFrameScripts(JSContext* aCx, JS::MutableHandle<JS::Value> aList)
{
  
  
  if (!IsGlobal() && !IsWindowLevel()) {
    NS_WARNING("Cannot retrieve list of pending frame scripts for frame"
               "message managers as it may be incomplete");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, mPendingScripts.Length()));
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  JS::Rooted<JSString*> url(aCx);
  JS::Rooted<JSObject*> pair(aCx);
  for (uint32_t i = 0; i < mPendingScripts.Length(); ++i) {
    url = JS_NewUCStringCopyN(aCx, mPendingScripts[i].get(), mPendingScripts[i].Length());
    NS_ENSURE_TRUE(url, NS_ERROR_OUT_OF_MEMORY);

    JS::AutoValueArray<2> pairElts(aCx);
    pairElts[0].setString(url);
    pairElts[1].setBoolean(mPendingScriptsGlobalStates[i]);

    pair = JS_NewArrayObject(aCx, pairElts);
    NS_ENSURE_TRUE(pair, NS_ERROR_OUT_OF_MEMORY);

    NS_ENSURE_TRUE(JS_SetElement(aCx, array, i, pair),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  aList.setObject(*array);
  return NS_OK;
}

static bool
JSONCreator(const jschar* aBuf, uint32_t aLen, void* aData)
{
  nsAString* result = static_cast<nsAString*>(aData);
  result->Append(static_cast<const char16_t*>(aBuf),
                 static_cast<uint32_t>(aLen));
  return true;
}

static bool
GetParamsForMessage(JSContext* aCx,
                    const JS::Value& aJSON,
                    JSAutoStructuredCloneBuffer& aBuffer,
                    StructuredCloneClosure& aClosure)
{
  JS::Rooted<JS::Value> v(aCx, aJSON);
  if (WriteStructuredClone(aCx, v, aBuffer, aClosure)) {
    return true;
  }
  JS_ClearPendingException(aCx);

  
  
  
  
  nsAutoString json;
  NS_ENSURE_TRUE(JS_Stringify(aCx, &v, JS::NullPtr(), JS::NullHandleValue,
                              JSONCreator, &json), false);
  NS_ENSURE_TRUE(!json.IsEmpty(), false);

  JS::Rooted<JS::Value> val(aCx, JS::NullValue());
  NS_ENSURE_TRUE(JS_ParseJSON(aCx, static_cast<const jschar*>(json.get()),
                              json.Length(), &val), false);

  return WriteStructuredClone(aCx, val, aBuffer, aClosure);
}




static bool sSendingSyncMessage = false;

NS_IMETHODIMP
nsFrameMessageManager::SendSyncMessage(const nsAString& aMessageName,
                                       JS::Handle<JS::Value> aJSON,
                                       JS::Handle<JS::Value> aObjects,
                                       nsIPrincipal* aPrincipal,
                                       JSContext* aCx,
                                       uint8_t aArgc,
                                       JS::MutableHandle<JS::Value> aRetval)
{
  return SendMessage(aMessageName, aJSON, aObjects, aPrincipal, aCx, aArgc,
                     aRetval, true);
}

NS_IMETHODIMP
nsFrameMessageManager::SendRpcMessage(const nsAString& aMessageName,
                                      JS::Handle<JS::Value> aJSON,
                                      JS::Handle<JS::Value> aObjects,
                                      nsIPrincipal* aPrincipal,
                                      JSContext* aCx,
                                      uint8_t aArgc,
                                      JS::MutableHandle<JS::Value> aRetval)
{
  return SendMessage(aMessageName, aJSON, aObjects, aPrincipal, aCx, aArgc,
                     aRetval, false);
}

nsresult
nsFrameMessageManager::SendMessage(const nsAString& aMessageName,
                                   JS::Handle<JS::Value> aJSON,
                                   JS::Handle<JS::Value> aObjects,
                                   nsIPrincipal* aPrincipal,
                                   JSContext* aCx,
                                   uint8_t aArgc,
                                   JS::MutableHandle<JS::Value> aRetval,
                                   bool aIsSync)
{
  NS_ASSERTION(!IsGlobal(), "Should not call SendSyncMessage in chrome");
  NS_ASSERTION(!IsWindowLevel(), "Should not call SendSyncMessage in chrome");
  NS_ASSERTION(!mParentManager, "Should not have parent manager in content!");

  aRetval.setUndefined();
  NS_ENSURE_TRUE(mCallback, NS_ERROR_NOT_INITIALIZED);

  if (sSendingSyncMessage && aIsSync) {
    
    return NS_ERROR_UNEXPECTED;
  }

  StructuredCloneData data;
  JSAutoStructuredCloneBuffer buffer;
  if (aArgc >= 2 &&
      !GetParamsForMessage(aCx, aJSON, buffer, data.mClosure)) {
    return NS_ERROR_DOM_DATA_CLONE_ERR;
  }
  data.mData = buffer.data();
  data.mDataLength = buffer.nbytes();

  JS::Rooted<JSObject*> objects(aCx);
  if (aArgc >= 3 && aObjects.isObject()) {
    objects = &aObjects.toObject();
  }

  InfallibleTArray<nsString> retval;

  sSendingSyncMessage |= aIsSync;
  bool rv = mCallback->DoSendBlockingMessage(aCx, aMessageName, data, objects,
                                             aPrincipal, &retval, aIsSync);
  if (aIsSync) {
    sSendingSyncMessage = false;
  }

  if (!rv) {
    return NS_OK;
  }

  uint32_t len = retval.Length();
  JS::Rooted<JSObject*> dataArray(aCx, JS_NewArrayObject(aCx, len));
  NS_ENSURE_TRUE(dataArray, NS_ERROR_OUT_OF_MEMORY);

  for (uint32_t i = 0; i < len; ++i) {
    if (retval[i].IsEmpty()) {
      continue;
    }

    JS::Rooted<JS::Value> ret(aCx);
    if (!JS_ParseJSON(aCx, static_cast<const jschar*>(retval[i].get()),
                      retval[i].Length(), &ret)) {
      return NS_ERROR_UNEXPECTED;
    }
    NS_ENSURE_TRUE(JS_SetElement(aCx, dataArray, i, ret),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  aRetval.setObject(*dataArray);
  return NS_OK;
}

nsresult
nsFrameMessageManager::DispatchAsyncMessageInternal(JSContext* aCx,
                                                    const nsAString& aMessage,
                                                    const StructuredCloneData& aData,
                                                    JS::Handle<JSObject *> aCpows,
                                                    nsIPrincipal* aPrincipal)
{
  if (mIsBroadcaster) {
    int32_t len = mChildManagers.Count();
    for (int32_t i = 0; i < len; ++i) {
      static_cast<nsFrameMessageManager*>(mChildManagers[i])->
         DispatchAsyncMessageInternal(aCx, aMessage, aData, aCpows, aPrincipal);
    }
    return NS_OK;
  }

  NS_ENSURE_TRUE(mCallback, NS_ERROR_NOT_INITIALIZED);
  if (!mCallback->DoSendAsyncMessage(aCx, aMessage, aData, aCpows, aPrincipal)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsFrameMessageManager::DispatchAsyncMessage(const nsAString& aMessageName,
                                            const JS::Value& aJSON,
                                            const JS::Value& aObjects,
                                            nsIPrincipal* aPrincipal,
                                            JSContext* aCx,
                                            uint8_t aArgc)
{
  StructuredCloneData data;
  JSAutoStructuredCloneBuffer buffer;

  if (aArgc >= 2 &&
      !GetParamsForMessage(aCx, aJSON, buffer, data.mClosure)) {
    return NS_ERROR_DOM_DATA_CLONE_ERR;
  }

  JS::Rooted<JSObject*> objects(aCx);
  if (aArgc >= 3 && aObjects.isObject()) {
    objects = &aObjects.toObject();
  }

  data.mData = buffer.data();
  data.mDataLength = buffer.nbytes();

  return DispatchAsyncMessageInternal(aCx, aMessageName, data, objects,
                                      aPrincipal);
}




NS_IMETHODIMP
nsFrameMessageManager::SendAsyncMessage(const nsAString& aMessageName,
                                        JS::Handle<JS::Value> aJSON,
                                        JS::Handle<JS::Value> aObjects,
                                        nsIPrincipal* aPrincipal,
                                        JSContext* aCx,
                                        uint8_t aArgc)
{
  return DispatchAsyncMessage(aMessageName, aJSON, aObjects, aPrincipal, aCx,
                              aArgc);
}




NS_IMETHODIMP
nsFrameMessageManager::BroadcastAsyncMessage(const nsAString& aMessageName,
                                             JS::Handle<JS::Value> aJSON,
                                             JS::Handle<JS::Value> aObjects,
                                             JSContext* aCx,
                                             uint8_t aArgc)
{
  return DispatchAsyncMessage(aMessageName, aJSON, aObjects, nullptr, aCx,
                              aArgc);
}

NS_IMETHODIMP
nsFrameMessageManager::GetChildCount(uint32_t* aChildCount)
{
  *aChildCount = static_cast<uint32_t>(mChildManagers.Count());
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::GetChildAt(uint32_t aIndex,
                                  nsIMessageListenerManager** aMM)
{
  *aMM = nullptr;
  nsCOMPtr<nsIMessageListenerManager> mm =
    do_QueryInterface(mChildManagers.SafeObjectAt(static_cast<uint32_t>(aIndex)));
  mm.swap(*aMM);
  return NS_OK;
}




NS_IMETHODIMP
nsFrameMessageManager::Dump(const nsAString& aStr)
{
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "Gecko", "%s", NS_ConvertUTF16toUTF8(aStr).get());
#endif
#ifdef XP_WIN
  if (IsDebuggerPresent()) {
    OutputDebugStringW(PromiseFlatString(aStr).get());
  }
#endif
  fputs(NS_ConvertUTF16toUTF8(aStr).get(), stdout);
  fflush(stdout);
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::PrivateNoteIntentionalCrash()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFrameMessageManager::GetContent(nsIDOMWindow** aContent)
{
  *aContent = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::GetDocShell(nsIDocShell** aDocShell)
{
  *aDocShell = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::Btoa(const nsAString& aBinaryData,
                            nsAString& aAsciiBase64String)
{
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::Atob(const nsAString& aAsciiString,
                            nsAString& aBinaryData)
{
  return NS_OK;
}



nsresult
nsFrameMessageManager::AssertProcessInternal(ProcessCheckerType aType,
                                             const nsAString& aCapability,
                                             bool* aValid)
{
  *aValid = false;

  
  if (!mChrome || mIsBroadcaster) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  if (!mCallback) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  switch (aType) {
    case PROCESS_CHECKER_PERMISSION:
      *aValid = mCallback->CheckPermission(aCapability);
      break;
    case PROCESS_CHECKER_MANIFEST_URL:
      *aValid = mCallback->CheckManifestURL(aCapability);
      break;
    case ASSERT_APP_HAS_PERMISSION:
      *aValid = mCallback->CheckAppHasPermission(aCapability);
      break;
    default:
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameMessageManager::AssertPermission(const nsAString& aPermission,
                                        bool* aHasPermission)
{
  return AssertProcessInternal(PROCESS_CHECKER_PERMISSION,
                               aPermission,
                               aHasPermission);
}

NS_IMETHODIMP
nsFrameMessageManager::AssertContainApp(const nsAString& aManifestURL,
                                        bool* aHasManifestURL)
{
  return AssertProcessInternal(PROCESS_CHECKER_MANIFEST_URL,
                               aManifestURL,
                               aHasManifestURL);
}

NS_IMETHODIMP
nsFrameMessageManager::AssertAppHasPermission(const nsAString& aPermission,
                                              bool* aHasPermission)
{
  return AssertProcessInternal(ASSERT_APP_HAS_PERMISSION,
                               aPermission,
                               aHasPermission);
}

NS_IMETHODIMP
nsFrameMessageManager::AssertAppHasStatus(unsigned short aStatus,
                                          bool* aHasStatus)
{
  *aHasStatus = false;

  
  if (!mChrome || mIsBroadcaster) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  if (!mCallback) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  *aHasStatus = mCallback->CheckAppHasStatus(aStatus);

  return NS_OK;
}

class MMListenerRemover
{
public:
  MMListenerRemover(nsFrameMessageManager* aMM)
    : mWasHandlingMessage(aMM->mHandlingMessage)
    , mMM(aMM)
  {
    mMM->mHandlingMessage = true;
  }
  ~MMListenerRemover()
  {
    if (!mWasHandlingMessage) {
      mMM->mHandlingMessage = false;
      if (mMM->mDisconnected) {
        mMM->mListeners.Clear();
      }
    }
  }

  bool mWasHandlingMessage;
  nsRefPtr<nsFrameMessageManager> mMM;
};




nsresult
nsFrameMessageManager::ReceiveMessage(nsISupports* aTarget,
                                      const nsAString& aMessage,
                                      bool aIsSync,
                                      const StructuredCloneData* aCloneData,
                                      CpowHolder* aCpows,
                                      nsIPrincipal* aPrincipal,
                                      InfallibleTArray<nsString>* aJSONRetVal)
{
  AutoSafeJSContext cx;
  nsAutoTObserverArray<nsMessageListenerInfo, 1>* listeners =
    mListeners.Get(aMessage);
  if (listeners) {

    MMListenerRemover lr(this);

    nsAutoTObserverArray<nsMessageListenerInfo, 1>::EndLimitedIterator
      iter(*listeners);
    while(iter.HasMore()) {
      nsMessageListenerInfo& listener = iter.GetNext();
      
      nsCOMPtr<nsISupports> weakListener;
      if (listener.mWeakListener) {
        weakListener = do_QueryReferent(listener.mWeakListener);
        if (!weakListener) {
          listeners->RemoveElement(listener);
          continue;
        }
      }

      nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS;
      if (weakListener) {
        wrappedJS = do_QueryInterface(weakListener);
      } else {
        wrappedJS = do_QueryInterface(listener.mStrongListener);
      }

      if (!wrappedJS) {
        continue;
      }
      JS::Rooted<JSObject*> object(cx, wrappedJS->GetJSObject());
      if (!object) {
        continue;
      }
      JSAutoCompartment ac(cx, object);

      
      JS::Rooted<JSObject*> param(cx,
        JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
      NS_ENSURE_TRUE(param, NS_ERROR_OUT_OF_MEMORY);

      JS::Rooted<JS::Value> targetv(cx);
      js::AssertSameCompartment(cx, object);
      nsresult rv = nsContentUtils::WrapNative(cx, aTarget, &targetv);
      NS_ENSURE_SUCCESS(rv, rv);

      JS::Rooted<JSObject*> cpows(cx);
      if (aCpows) {
        if (!aCpows->ToObject(cx, &cpows)) {
          return NS_ERROR_UNEXPECTED;
        }
      }

      if (!cpows) {
        cpows = JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr());
        if (!cpows) {
          return NS_ERROR_UNEXPECTED;
        }
      }

      JS::Rooted<JS::Value> cpowsv(cx, JS::ObjectValue(*cpows));

      JS::Rooted<JS::Value> json(cx, JS::NullValue());
      if (aCloneData && aCloneData->mDataLength &&
          !ReadStructuredClone(cx, *aCloneData, &json)) {
        JS_ClearPendingException(cx);
        return NS_OK;
      }
      JS::Rooted<JSString*> jsMessage(cx,
        JS_NewUCStringCopyN(cx,
                            static_cast<const jschar*>(aMessage.BeginReading()),
                            aMessage.Length()));
      NS_ENSURE_TRUE(jsMessage, NS_ERROR_OUT_OF_MEMORY);
      JS::Rooted<JS::Value> syncv(cx, JS::BooleanValue(aIsSync));
      JS_DefineProperty(cx, param, "target", targetv, JSPROP_ENUMERATE);
      JS_DefineProperty(cx, param, "name", jsMessage, JSPROP_ENUMERATE);
      JS_DefineProperty(cx, param, "sync", syncv, JSPROP_ENUMERATE);
      JS_DefineProperty(cx, param, "json", json, JSPROP_ENUMERATE); 
      JS_DefineProperty(cx, param, "data", json, JSPROP_ENUMERATE);
      JS_DefineProperty(cx, param, "objects", cpowsv, JSPROP_ENUMERATE);

      
      if (!aPrincipal) {
        JS_DefineProperty(cx, param, "principal", JS::UndefinedHandleValue, JSPROP_ENUMERATE);
      }

      
      else {
        JS::Rooted<JSObject*> principalObj(cx,
          JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

        uint32_t appId;
        nsresult rv = aPrincipal->GetAppId(&appId);
        NS_ENSURE_SUCCESS(rv, rv);

        JS_DefineProperty(cx, principalObj, "appId", appId, JSPROP_ENUMERATE);

        nsCString origin;
        rv = aPrincipal->GetOrigin(getter_Copies(origin));
        NS_ENSURE_SUCCESS(rv, rv);

        JS::Rooted<JSString*> originStr(cx, JS_NewStringCopyN(cx, origin.get(), origin.Length()));
        NS_ENSURE_TRUE(originStr, NS_ERROR_OUT_OF_MEMORY);
        JS_DefineProperty(cx, principalObj, "origin", originStr, JSPROP_ENUMERATE);

        bool browser;
        rv = aPrincipal->GetIsInBrowserElement(&browser);
        NS_ENSURE_SUCCESS(rv, rv);

        JS::Rooted<JS::Value> browserValue(cx, JS::BooleanValue(browser));
        JS_DefineProperty(cx, principalObj, "isInBrowserElement", browserValue, JSPROP_ENUMERATE);

        JS_DefineProperty(cx, param, "principal", principalObj, JSPROP_ENUMERATE);
      }

      JS::Rooted<JS::Value> thisValue(cx, JS::UndefinedValue());

      JS::Rooted<JS::Value> funval(cx);
      if (JS_ObjectIsCallable(cx, object)) {
        
        funval.setObject(*object);

        
        
        nsCOMPtr<nsISupports> defaultThisValue;
        if (mChrome) {
          defaultThisValue = do_QueryObject(this);
        } else {
          defaultThisValue = aTarget;
        }
        js::AssertSameCompartment(cx, object);
        nsresult rv = nsContentUtils::WrapNative(cx, defaultThisValue, &thisValue);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        if (!JS_GetProperty(cx, object, "receiveMessage", &funval) ||
            !funval.isObject())
          return NS_ERROR_UNEXPECTED;

        
        NS_ENSURE_STATE(JS_ObjectIsCallable(cx, &funval.toObject()));
        thisValue.setObject(*object);
      }

      JS::Rooted<JS::Value> rval(cx, JSVAL_VOID);
      JS::Rooted<JS::Value> argv(cx, JS::ObjectValue(*param));

      {
        JS::Rooted<JSObject*> thisObject(cx, thisValue.toObjectOrNull());

        JSAutoCompartment tac(cx, thisObject);
        if (!JS_WrapValue(cx, &argv)) {
          return NS_ERROR_UNEXPECTED;
        }

        if (!JS_CallFunctionValue(cx, thisObject, funval, JS::HandleValueArray(argv), &rval)) {
          nsJSUtils::ReportPendingException(cx);
          continue;
        }
        if (aJSONRetVal) {
          nsString json;
          if (!JS_Stringify(cx, &rval, JS::NullPtr(), JS::NullHandleValue,
                           JSONCreator, &json)) {
            nsJSUtils::ReportPendingException(cx);
            continue;
          }
          aJSONRetVal->AppendElement(json);
        }
      }
    }
  }
  nsRefPtr<nsFrameMessageManager> kungfuDeathGrip = mParentManager;
  return mParentManager ? mParentManager->ReceiveMessage(aTarget, aMessage,
                                                         aIsSync, aCloneData,
                                                         aCpows, aPrincipal,
                                                         aJSONRetVal) : NS_OK;
}

void
nsFrameMessageManager::AddChildManager(nsFrameMessageManager* aManager)
{
  mChildManagers.AppendObject(aManager);

  nsRefPtr<nsFrameMessageManager> kungfuDeathGrip = this;
  nsRefPtr<nsFrameMessageManager> kungfuDeathGrip2 = aManager;
  
  
  
  if (mParentManager) {
    nsRefPtr<nsFrameMessageManager> globalMM = mParentManager;
    for (uint32_t i = 0; i < globalMM->mPendingScripts.Length(); ++i) {
      aManager->LoadFrameScript(globalMM->mPendingScripts[i], false,
                                globalMM->mPendingScriptsGlobalStates[i]);
    }
  }
  for (uint32_t i = 0; i < mPendingScripts.Length(); ++i) {
    aManager->LoadFrameScript(mPendingScripts[i], false,
                              mPendingScriptsGlobalStates[i]);
  }
}

void
nsFrameMessageManager::SetCallback(MessageManagerCallback* aCallback)
{
  MOZ_ASSERT(!mIsBroadcaster || !mCallback,
             "Broadcasters cannot have callbacks!");
  if (aCallback && mCallback != aCallback) {
    mCallback = aCallback;
    if (mOwnsCallback) {
      mOwnedCallback = aCallback;
    }
  }
}

void
nsFrameMessageManager::InitWithCallback(MessageManagerCallback* aCallback)
{
  if (mCallback) {
    
    return;
  }

  SetCallback(aCallback);

  
  if (mParentManager) {
    mParentManager->AddChildManager(this);
  }

  for (uint32_t i = 0; i < mPendingScripts.Length(); ++i) {
    LoadFrameScript(mPendingScripts[i], false, mPendingScriptsGlobalStates[i]);
  }
}

void
nsFrameMessageManager::RemoveFromParent()
{
  if (mParentManager) {
    mParentManager->RemoveChildManager(this);
  }
  mParentManager = nullptr;
  mCallback = nullptr;
  mOwnedCallback = nullptr;
}

void
nsFrameMessageManager::Disconnect(bool aRemoveFromParent)
{
  if (!mDisconnected) {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
       obs->NotifyObservers(NS_ISUPPORTS_CAST(nsIContentFrameMessageManager*, this),
                            "message-manager-disconnect", nullptr);
    }
  }
  if (mParentManager && aRemoveFromParent) {
    mParentManager->RemoveChildManager(this);
  }
  mDisconnected = true;
  mParentManager = nullptr;
  mCallback = nullptr;
  mOwnedCallback = nullptr;
  if (!mHandlingMessage) {
    mListeners.Clear();
  }
}

namespace {

struct MessageManagerReferentCount
{
  MessageManagerReferentCount() : mStrong(0), mWeakAlive(0), mWeakDead(0) {}
  size_t mStrong;
  size_t mWeakAlive;
  size_t mWeakDead;
  nsTArray<nsString> mSuspectMessages;
  nsDataHashtable<nsStringHashKey, uint32_t> mMessageCounter;
};

} 

namespace mozilla {
namespace dom {

class MessageManagerReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  static const size_t kSuspectReferentCount = 300;
protected:
  void CountReferents(nsFrameMessageManager* aMessageManager,
                      MessageManagerReferentCount* aReferentCount);
};

NS_IMPL_ISUPPORTS(MessageManagerReporter, nsIMemoryReporter)

static PLDHashOperator
CollectMessageListenerData(const nsAString& aKey,
                           nsAutoTObserverArray<nsMessageListenerInfo, 1>* aListeners,
                           void* aData)
{
  MessageManagerReferentCount* referentCount =
    static_cast<MessageManagerReferentCount*>(aData);

  uint32_t listenerCount = aListeners->Length();
  if (!listenerCount) {
    return PL_DHASH_NEXT;
  }

  nsString key(aKey);
  uint32_t oldCount = 0;
  referentCount->mMessageCounter.Get(key, &oldCount);
  uint32_t currentCount = oldCount + listenerCount;
  referentCount->mMessageCounter.Put(key, currentCount);

  
  
  if (currentCount == MessageManagerReporter::kSuspectReferentCount) {
    referentCount->mSuspectMessages.AppendElement(key);
  }

  for (uint32_t i = 0; i < listenerCount; ++i) {
    const nsMessageListenerInfo& listenerInfo =
      aListeners->ElementAt(i);
    if (listenerInfo.mWeakListener) {
      nsCOMPtr<nsISupports> referent =
        do_QueryReferent(listenerInfo.mWeakListener);
      if (referent) {
        referentCount->mWeakAlive++;
      } else {
        referentCount->mWeakDead++;
      }
    } else {
      referentCount->mStrong++;
    }
  }
  return PL_DHASH_NEXT;
}

void
MessageManagerReporter::CountReferents(nsFrameMessageManager* aMessageManager,
                                       MessageManagerReferentCount* aReferentCount)
{
  aMessageManager->mListeners.EnumerateRead(CollectMessageListenerData,
                                            aReferentCount);

  
  
  for (uint32_t i = 0; i < aMessageManager->mChildManagers.Length(); ++i) {
    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(aMessageManager->mChildManagers[i]);
    CountReferents(mm, aReferentCount);
  }
}

static nsresult
ReportReferentCount(const char* aManagerType,
                    const MessageManagerReferentCount& aReferentCount,
                    nsIMemoryReporterCallback* aCb,
                    nsISupports* aClosure)
{
#define REPORT(_path, _amount, _desc)                                         \
    do {                                                                      \
      nsresult rv;                                                            \
      rv = aCb->Callback(EmptyCString(), _path,                               \
                         nsIMemoryReporter::KIND_OTHER,                       \
                         nsIMemoryReporter::UNITS_COUNT, _amount,             \
                         _desc, aClosure);                                    \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    } while (0)

  REPORT(nsPrintfCString("message-manager/referent/%s/strong", aManagerType),
         aReferentCount.mStrong,
         nsPrintfCString("The number of strong referents held by the message "
                         "manager in the %s manager.", aManagerType));
  REPORT(nsPrintfCString("message-manager/referent/%s/weak/alive", aManagerType),
         aReferentCount.mWeakAlive,
         nsPrintfCString("The number of weak referents that are still alive "
                         "held by the message manager in the %s manager.",
                         aManagerType));
  REPORT(nsPrintfCString("message-manager/referent/%s/weak/dead", aManagerType),
         aReferentCount.mWeakDead,
         nsPrintfCString("The number of weak referents that are dead "
                         "held by the message manager in the %s manager.",
                         aManagerType));

  for (uint32_t i = 0; i < aReferentCount.mSuspectMessages.Length(); i++) {
    uint32_t totalReferentCount = 0;
    aReferentCount.mMessageCounter.Get(aReferentCount.mSuspectMessages[i],
                                       &totalReferentCount);
    NS_ConvertUTF16toUTF8 suspect(aReferentCount.mSuspectMessages[i]);
    REPORT(nsPrintfCString("message-manager-suspect/%s/referent(message=%s)",
                           aManagerType, suspect.get()), totalReferentCount,
           nsPrintfCString("A message in the %s message manager with a "
                           "suspiciously large number of referents (symptom "
                           "of a leak).", aManagerType));
  }

#undef REPORT

  return NS_OK;
}

NS_IMETHODIMP
MessageManagerReporter::CollectReports(nsIMemoryReporterCallback* aCb,
                                       nsISupports* aClosure)
{
  nsresult rv;

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsCOMPtr<nsIMessageBroadcaster> globalmm =
      do_GetService("@mozilla.org/globalmessagemanager;1");
    if (globalmm) {
      nsRefPtr<nsFrameMessageManager> mm =
        static_cast<nsFrameMessageManager*>(globalmm.get());
      MessageManagerReferentCount count;
      CountReferents(mm, &count);
      rv = ReportReferentCount("global-manager", count, aCb, aClosure);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (nsFrameMessageManager::sParentProcessManager) {
    MessageManagerReferentCount count;
    CountReferents(nsFrameMessageManager::sParentProcessManager, &count);
    rv = ReportReferentCount("parent-process-manager", count, aCb, aClosure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (nsFrameMessageManager::sChildProcessManager) {
    MessageManagerReferentCount count;
    CountReferents(nsFrameMessageManager::sChildProcessManager, &count);
    rv = ReportReferentCount("child-process-manager", count, aCb, aClosure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

} 
} 

nsresult
NS_NewGlobalMessageManager(nsIMessageBroadcaster** aResult)
{
  NS_ENSURE_TRUE(XRE_GetProcessType() == GeckoProcessType_Default,
                 NS_ERROR_NOT_AVAILABLE);
  nsFrameMessageManager* mm = new nsFrameMessageManager(nullptr,
                                                        nullptr,
                                                        MM_CHROME | MM_GLOBAL | MM_BROADCASTER);
  RegisterStrongMemoryReporter(new MessageManagerReporter());
  return CallQueryInterface(mm, aResult);
}

nsDataHashtable<nsStringHashKey, nsFrameScriptObjectExecutorHolder*>*
  nsFrameScriptExecutor::sCachedScripts = nullptr;
nsScriptCacheCleaner* nsFrameScriptExecutor::sScriptCacheCleaner = nullptr;

void
nsFrameScriptExecutor::DidCreateGlobal()
{
  NS_ASSERTION(mGlobal, "Should have mGlobal!");
  if (!sCachedScripts) {
    sCachedScripts =
      new nsDataHashtable<nsStringHashKey, nsFrameScriptObjectExecutorHolder*>;

    nsRefPtr<nsScriptCacheCleaner> scriptCacheCleaner =
      new nsScriptCacheCleaner();
    scriptCacheCleaner.forget(&sScriptCacheCleaner);
  }
}

static PLDHashOperator
RemoveCachedScriptEntry(const nsAString& aKey,
                        nsFrameScriptObjectExecutorHolder*& aData,
                        void* aUserArg)
{
  delete aData;
  return PL_DHASH_REMOVE;
}


void
nsFrameScriptExecutor::Shutdown()
{
  if (sCachedScripts) {
    AutoSafeJSContext cx;
    NS_ASSERTION(sCachedScripts != nullptr, "Need cached scripts");
    sCachedScripts->Enumerate(RemoveCachedScriptEntry, nullptr);

    delete sCachedScripts;
    sCachedScripts = nullptr;

    nsRefPtr<nsScriptCacheCleaner> scriptCacheCleaner;
    scriptCacheCleaner.swap(sScriptCacheCleaner);
  }
}

void
nsFrameScriptExecutor::LoadFrameScriptInternal(const nsAString& aURL,
                                               bool aRunInGlobalScope)
{
  if (!mGlobal || !sCachedScripts) {
    return;
  }

  AutoSafeJSContext cx;
  JS::Rooted<JSScript*> script(cx);
  JS::Rooted<JSObject*> funobj(cx);

  nsFrameScriptObjectExecutorHolder* holder = sCachedScripts->Get(aURL);
  if (holder && holder->WillRunInGlobalScope() == aRunInGlobalScope) {
    script = holder->mScript;
    funobj = holder->mFunction;
  } else {
    
    
    bool shouldCache = !holder;
    TryCacheLoadAndCompileScript(aURL, aRunInGlobalScope,
                                 shouldCache, &script, &funobj);
  }

  JS::Rooted<JSObject*> global(cx, mGlobal->GetJSObject());
  if (global) {
    JSAutoCompartment ac(cx, global);
    bool ok = true;
    if (funobj) {
      JS::Rooted<JSObject*> method(cx, JS_CloneFunctionObject(cx, funobj, global));
      if (!method) {
        return;
      }
      JS::Rooted<JS::Value> rval(cx);
      JS::Rooted<JS::Value> methodVal(cx, JS::ObjectValue(*method));
      ok = JS_CallFunctionValue(cx, global, methodVal,
                                JS::HandleValueArray::empty(), &rval);
    } else if (script) {
      ok = JS::CloneAndExecuteScript(cx, global, script);
    }

    if (!ok) {
      nsJSUtils::ReportPendingException(cx);
    }
  }
}

void
nsFrameScriptExecutor::TryCacheLoadAndCompileScript(const nsAString& aURL,
                                                    bool aRunInGlobalScope,
                                                    bool aShouldCache,
                                                    JS::MutableHandle<JSScript*> aScriptp,
                                                    JS::MutableHandle<JSObject*> aFunp)
{
  nsCString url = NS_ConvertUTF16toUTF8(aURL);
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), url);
  if (NS_FAILED(rv)) {
    return;
  }

  bool hasFlags;
  rv = NS_URIChainHasFlags(uri,
                           nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                           &hasFlags);
  if (NS_FAILED(rv) || !hasFlags) {
    NS_WARNING("Will not load a frame script!");
    return;
  }

  nsCOMPtr<nsIChannel> channel;
  NS_NewChannel(getter_AddRefs(channel), uri);
  if (!channel) {
    return;
  }

  nsCOMPtr<nsIInputStream> input;
  channel->Open(getter_AddRefs(input));
  nsString dataString;
  jschar* dataStringBuf = nullptr;
  size_t dataStringLength = 0;
  uint64_t avail64 = 0;
  if (input && NS_SUCCEEDED(input->Available(&avail64)) && avail64) {
    if (avail64 > UINT32_MAX) {
      return;
    }
    nsCString buffer;
    uint32_t avail = (uint32_t)std::min(avail64, (uint64_t)UINT32_MAX);
    if (NS_FAILED(NS_ReadInputStreamToString(input, buffer, avail))) {
      return;
    }
    nsScriptLoader::ConvertToUTF16(channel, (uint8_t*)buffer.get(), avail,
                                   EmptyString(), nullptr,
                                   dataStringBuf, dataStringLength);
  }

  JS::SourceBufferHolder srcBuf(dataStringBuf, dataStringLength,
                                JS::SourceBufferHolder::GiveOwnership);

  if (dataStringBuf && dataStringLength > 0) {
    AutoSafeJSContext cx;
    JS::Rooted<JSObject*> global(cx, mGlobal->GetJSObject());
    if (global) {
      JSAutoCompartment ac(cx, global);
      JS::CompileOptions options(cx);
      options.setFileAndLine(url.get(), 1);
      JS::Rooted<JSScript*> script(cx);
      JS::Rooted<JSObject*> funobj(cx);
      if (aRunInGlobalScope) {
        options.setNoScriptRval(true);
        script = JS::Compile(cx, JS::NullPtr(), options, srcBuf);
      } else {
        JS::Rooted<JSFunction *> fun(cx);
        fun = JS::CompileFunction(cx, JS::NullPtr(), options,
                                  nullptr, 0, nullptr, 
                                  srcBuf);
        if (!fun) {
          return;
        }
        funobj = JS_GetFunctionObject(fun);
      }

      if (!script && !funobj) {
        return;
      }

      aScriptp.set(script);
      aFunp.set(funobj);

      nsAutoCString scheme;
      uri->GetScheme(scheme);
      
      if (aShouldCache && !scheme.EqualsLiteral("data")) {
        nsFrameScriptObjectExecutorHolder* holder;

        
        if (script) {
          holder = new nsFrameScriptObjectExecutorHolder(cx, script);
        } else {
          holder = new nsFrameScriptObjectExecutorHolder(cx, funobj);
        }
        sCachedScripts->Put(aURL, holder);
      }
    }
  }
}

void
nsFrameScriptExecutor::TryCacheLoadAndCompileScript(const nsAString& aURL,
                                                    bool aRunInGlobalScope)
{
  AutoSafeJSContext cx;
  JS::Rooted<JSScript*> script(cx);
  JS::Rooted<JSObject*> funobj(cx);
  TryCacheLoadAndCompileScript(aURL, aRunInGlobalScope, true, &script, &funobj);
}

bool
nsFrameScriptExecutor::InitTabChildGlobalInternal(nsISupports* aScope,
                                                  const nsACString& aID)
{

  nsCOMPtr<nsIJSRuntimeService> runtimeSvc =
    do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
  NS_ENSURE_TRUE(runtimeSvc, false);

  JSRuntime* rt = nullptr;
  runtimeSvc->GetRuntime(&rt);
  NS_ENSURE_TRUE(rt, false);

  AutoSafeJSContext cx;
  nsContentUtils::GetSecurityManager()->GetSystemPrincipal(getter_AddRefs(mPrincipal));

  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  const uint32_t flags = nsIXPConnect::INIT_JS_STANDARD_CLASSES;

  JS::CompartmentOptions options;
  options.setZone(JS::SystemZone)
         .setVersion(JSVERSION_LATEST);

  nsresult rv =
    xpc->InitClassesWithNewWrappedGlobal(cx, aScope, mPrincipal,
                                         flags, options, getter_AddRefs(mGlobal));
  NS_ENSURE_SUCCESS(rv, false);


  JS::Rooted<JSObject*> global(cx, mGlobal->GetJSObject());
  NS_ENSURE_TRUE(global, false);

  
  
  xpc::SetLocationForGlobal(global, aID);

  DidCreateGlobal();
  return true;
}

NS_IMPL_ISUPPORTS(nsScriptCacheCleaner, nsIObserver)

nsFrameMessageManager* nsFrameMessageManager::sChildProcessManager = nullptr;
nsFrameMessageManager* nsFrameMessageManager::sParentProcessManager = nullptr;
nsFrameMessageManager* nsFrameMessageManager::sSameProcessParentManager = nullptr;
nsTArray<nsCOMPtr<nsIRunnable> >* nsFrameMessageManager::sPendingSameProcessAsyncMessages = nullptr;

class nsAsyncMessageToSameProcessChild : public nsSameProcessAsyncMessageBase,
                                         public nsRunnable
{
public:
  nsAsyncMessageToSameProcessChild(JSContext* aCx,
                                   const nsAString& aMessage,
                                   const StructuredCloneData& aData,
                                   JS::Handle<JSObject *> aCpows,
                                   nsIPrincipal* aPrincipal)
    : nsSameProcessAsyncMessageBase(aCx, aMessage, aData, aCpows, aPrincipal)
  {
  }

  NS_IMETHOD Run()
  {
    nsFrameMessageManager* ppm = nsFrameMessageManager::sChildProcessManager;
    ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm), ppm);
    return NS_OK;
  }
};





class SameParentProcessMessageManagerCallback : public MessageManagerCallback
{
public:
  SameParentProcessMessageManagerCallback()
  {
    MOZ_COUNT_CTOR(SameParentProcessMessageManagerCallback);
  }
  virtual ~SameParentProcessMessageManagerCallback()
  {
    MOZ_COUNT_DTOR(SameParentProcessMessageManagerCallback);
  }

  virtual bool DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows,
                                  nsIPrincipal* aPrincipal)
  {
    nsRefPtr<nsIRunnable> ev =
      new nsAsyncMessageToSameProcessChild(aCx, aMessage, aData, aCpows,
                                           aPrincipal);
    NS_DispatchToCurrentThread(ev);
    return true;
  }

  bool CheckPermission(const nsAString& aPermission)
  {
    
    return true;
  }

  bool CheckManifestURL(const nsAString& aManifestURL)
  {
    
    return true;
  }

  bool CheckAppHasPermission(const nsAString& aPermission)
  {
    
    return true;
  }

  virtual bool CheckAppHasStatus(unsigned short aStatus)
  {
    
    return true;
  }
};





class ChildProcessMessageManagerCallback : public MessageManagerCallback
{
public:
  ChildProcessMessageManagerCallback()
  {
    MOZ_COUNT_CTOR(ChildProcessMessageManagerCallback);
  }
  virtual ~ChildProcessMessageManagerCallback()
  {
    MOZ_COUNT_DTOR(ChildProcessMessageManagerCallback);
  }

  virtual bool DoSendBlockingMessage(JSContext* aCx,
                                     const nsAString& aMessage,
                                     const mozilla::dom::StructuredCloneData& aData,
                                     JS::Handle<JSObject *> aCpows,
                                     nsIPrincipal* aPrincipal,
                                     InfallibleTArray<nsString>* aJSONRetVal,
                                     bool aIsSync) MOZ_OVERRIDE
  {
    mozilla::dom::ContentChild* cc =
      mozilla::dom::ContentChild::GetSingleton();
    if (!cc) {
      return true;
    }
    ClonedMessageData data;
    if (!BuildClonedMessageDataForChild(cc, aData, data)) {
      return false;
    }
    InfallibleTArray<mozilla::jsipc::CpowEntry> cpows;
    if (!cc->GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
      return false;
    }
    if (aIsSync) {
      return cc->SendSyncMessage(PromiseFlatString(aMessage), data, cpows,
                                 aPrincipal, aJSONRetVal);
    }
    return cc->CallRpcMessage(PromiseFlatString(aMessage), data, cpows,
                              aPrincipal, aJSONRetVal);
  }

  virtual bool DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows,
                                  nsIPrincipal* aPrincipal) MOZ_OVERRIDE
  {
    mozilla::dom::ContentChild* cc =
      mozilla::dom::ContentChild::GetSingleton();
    if (!cc) {
      return true;
    }
    ClonedMessageData data;
    if (!BuildClonedMessageDataForChild(cc, aData, data)) {
      return false;
    }
    InfallibleTArray<mozilla::jsipc::CpowEntry> cpows;
    if (!cc->GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
      return false;
    }
    return cc->SendAsyncMessage(PromiseFlatString(aMessage), data, cpows,
                                aPrincipal);
  }

};


class nsAsyncMessageToSameProcessParent : public nsSameProcessAsyncMessageBase,
                                          public nsRunnable
{
public:
  nsAsyncMessageToSameProcessParent(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows,
                                    nsIPrincipal* aPrincipal)
    : nsSameProcessAsyncMessageBase(aCx, aMessage, aData, aCpows, aPrincipal)
    , mDelivered(false)
  {
  }

  NS_IMETHOD Run()
  {
    if (nsFrameMessageManager::sPendingSameProcessAsyncMessages) {
      nsFrameMessageManager::sPendingSameProcessAsyncMessages->RemoveElement(this);
    }
    if (!mDelivered) {
      mDelivered = true;
      nsFrameMessageManager* ppm = nsFrameMessageManager::sSameProcessParentManager;
      ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm), ppm);
    }
    return NS_OK;
  }

private:
  bool mDelivered;
};




class SameChildProcessMessageManagerCallback : public MessageManagerCallback
{
public:
  SameChildProcessMessageManagerCallback()
  {
    MOZ_COUNT_CTOR(SameChildProcessMessageManagerCallback);
  }
  virtual ~SameChildProcessMessageManagerCallback()
  {
    MOZ_COUNT_DTOR(SameChildProcessMessageManagerCallback);
  }

  virtual bool DoSendBlockingMessage(JSContext* aCx,
                                     const nsAString& aMessage,
                                     const mozilla::dom::StructuredCloneData& aData,
                                     JS::Handle<JSObject *> aCpows,
                                     nsIPrincipal* aPrincipal,
                                     InfallibleTArray<nsString>* aJSONRetVal,
                                     bool aIsSync) MOZ_OVERRIDE
  {
    nsTArray<nsCOMPtr<nsIRunnable> > asyncMessages;
    if (nsFrameMessageManager::sPendingSameProcessAsyncMessages) {
      asyncMessages.SwapElements(*nsFrameMessageManager::sPendingSameProcessAsyncMessages);
      uint32_t len = asyncMessages.Length();
      for (uint32_t i = 0; i < len; ++i) {
        nsCOMPtr<nsIRunnable> async = asyncMessages[i];
        async->Run();
      }
    }
    if (nsFrameMessageManager::sSameProcessParentManager) {
      SameProcessCpowHolder cpows(js::GetRuntime(aCx), aCpows);
      nsRefPtr<nsFrameMessageManager> ppm = nsFrameMessageManager::sSameProcessParentManager;
      ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()), aMessage,
                          true, &aData, &cpows, aPrincipal, aJSONRetVal);
    }
    return true;
  }

  virtual bool DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows,
                                  nsIPrincipal* aPrincipal)
  {
    if (!nsFrameMessageManager::sPendingSameProcessAsyncMessages) {
      nsFrameMessageManager::sPendingSameProcessAsyncMessages = new nsTArray<nsCOMPtr<nsIRunnable> >;
    }
    nsCOMPtr<nsIRunnable> ev =
      new nsAsyncMessageToSameProcessParent(aCx, aMessage, aData, aCpows, aPrincipal);
    nsFrameMessageManager::sPendingSameProcessAsyncMessages->AppendElement(ev);
    NS_DispatchToCurrentThread(ev);
    return true;
  }

};



nsresult
NS_NewParentProcessMessageManager(nsIMessageBroadcaster** aResult)
{
  NS_ASSERTION(!nsFrameMessageManager::sParentProcessManager,
               "Re-creating sParentProcessManager");
  NS_ENSURE_TRUE(XRE_GetProcessType() == GeckoProcessType_Default,
                 NS_ERROR_NOT_AVAILABLE);
  nsRefPtr<nsFrameMessageManager> mm = new nsFrameMessageManager(nullptr,
                                                                 nullptr,
                                                                 MM_CHROME | MM_PROCESSMANAGER | MM_BROADCASTER);
  nsFrameMessageManager::sParentProcessManager = mm;
  nsFrameMessageManager::NewProcessMessageManager(nullptr); 
  return CallQueryInterface(mm, aResult);
}


nsFrameMessageManager*
nsFrameMessageManager::NewProcessMessageManager(mozilla::dom::ContentParent* aProcess)
{
  if (!nsFrameMessageManager::sParentProcessManager) {
     nsCOMPtr<nsIMessageBroadcaster> dummy =
       do_GetService("@mozilla.org/parentprocessmessagemanager;1");
  }

  MOZ_ASSERT(nsFrameMessageManager::sParentProcessManager,
             "parent process manager not created");
  nsFrameMessageManager* mm;
  if (aProcess) {
    mm = new nsFrameMessageManager(aProcess,
                                   nsFrameMessageManager::sParentProcessManager,
                                   MM_CHROME | MM_PROCESSMANAGER);
  } else {
    mm = new nsFrameMessageManager(new SameParentProcessMessageManagerCallback(),
                                   nsFrameMessageManager::sParentProcessManager,
                                   MM_CHROME | MM_PROCESSMANAGER | MM_OWNSCALLBACK);
    sSameProcessParentManager = mm;
  }
  return mm;
}

nsresult
NS_NewChildProcessMessageManager(nsISyncMessageSender** aResult)
{
  NS_ASSERTION(!nsFrameMessageManager::sChildProcessManager,
               "Re-creating sChildProcessManager");

  MessageManagerCallback* cb;
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    cb = new SameChildProcessMessageManagerCallback();
  } else {
    cb = new ChildProcessMessageManagerCallback();
    RegisterStrongMemoryReporter(new MessageManagerReporter());
  }
  nsFrameMessageManager* mm = new nsFrameMessageManager(cb,
                                                        nullptr,
                                                        MM_PROCESSMANAGER | MM_OWNSCALLBACK);
  nsFrameMessageManager::sChildProcessManager = mm;
  return CallQueryInterface(mm, aResult);
}

static PLDHashOperator
CycleCollectorMarkListeners(const nsAString& aKey,
                            nsAutoTObserverArray<nsMessageListenerInfo, 1>* aListeners,
                            void* aData)
{
  uint32_t count = aListeners->Length();
  for (uint32_t i = 0; i < count; i++) {
    if (aListeners->ElementAt(i).mStrongListener) {
      xpc_TryUnmarkWrappedGrayObject(aListeners->ElementAt(i).mStrongListener);
    }
  }
  return PL_DHASH_NEXT;
}

bool
nsFrameMessageManager::MarkForCC()
{
  mListeners.EnumerateRead(CycleCollectorMarkListeners, nullptr);

  if (mRefCnt.IsPurple()) {
    mRefCnt.RemovePurple();
  }
  return true;
}

nsSameProcessAsyncMessageBase::nsSameProcessAsyncMessageBase(JSContext* aCx,
                                                             const nsAString& aMessage,
                                                             const StructuredCloneData& aData,
                                                             JS::Handle<JSObject*> aCpows,
                                                             nsIPrincipal* aPrincipal)
  : mRuntime(js::GetRuntime(aCx)),
    mMessage(aMessage),
    mCpows(aCx, aCpows),
    mPrincipal(aPrincipal)
{
  if (aData.mDataLength && !mData.copy(aData.mData, aData.mDataLength)) {
    NS_RUNTIMEABORT("OOM");
  }
  mClosure = aData.mClosure;
}

void
nsSameProcessAsyncMessageBase::ReceiveMessage(nsISupports* aTarget,
                                              nsFrameMessageManager* aManager)
{
  if (aManager) {
    StructuredCloneData data;
    data.mData = mData.data();
    data.mDataLength = mData.nbytes();
    data.mClosure = mClosure;

    SameProcessCpowHolder cpows(mRuntime, mCpows);

    nsRefPtr<nsFrameMessageManager> mm = aManager;
    mm->ReceiveMessage(aTarget, mMessage, false, &data, &cpows,
                       mPrincipal, nullptr);
  }
}
