






































#include "Radio.h"
#include "nsITelephonyWorker.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIObserverService.h"
#include "mozilla/dom/workers/Workers.h"
#include "jstypedarray.h"

#include "nsThreadUtils.h"

#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define LOG(args...)  printf(args);
#endif

USING_WORKERS_NAMESPACE
using namespace mozilla::ipc;

static NS_DEFINE_CID(kTelephonyWorkerCID, NS_TELEPHONYWORKER_CID);


#define PROFILE_BEFORE_CHANGE_TOPIC "profile-before-change"

USING_TELEPHONY_NAMESPACE

namespace {


Radio* gInstance = nsnull;

class ConnectWorkerToRIL : public WorkerTask {
public:
  virtual bool RunTask(JSContext *aCx);
};

JSBool
PostToRIL(JSContext *cx, uintN argc, jsval *vp)
{
  NS_ASSERTION(!NS_IsMainThread(), "Expecting to be on the worker thread");

  if (argc != 1) {
    JS_ReportError(cx, "Expecting a single argument with the RIL message");
    return false;
  }

  jsval v = JS_ARGV(cx, vp)[0];

  nsAutoPtr<RilMessage> rm(new RilMessage());
  JSAutoByteString abs;
  void *data;
  size_t size;
  if (JSVAL_IS_STRING(v)) {
    JSString *str = JSVAL_TO_STRING(v);
    if (!abs.encode(cx, str)) {
      return false;
    }

    size = JS_GetStringLength(str);
    data = abs.ptr();
  } else if (!JSVAL_IS_PRIMITIVE(v)) {
    JSObject *obj = JSVAL_TO_OBJECT(v);
    if (!js_IsTypedArray(obj)) {
      JS_ReportError(cx, "Object passed in wasn't a typed array");
      return false;
    }

    JSUint32 type = JS_GetTypedArrayType(obj);
    if (type != js::TypedArray::TYPE_INT8 &&
        type != js::TypedArray::TYPE_UINT8 &&
        type != js::TypedArray::TYPE_UINT8_CLAMPED) {
      JS_ReportError(cx, "Typed array data is not octets");
      return false;
    }

    size = JS_GetTypedArrayByteLength(obj);
    data = JS_GetTypedArrayData(obj);
  } else {
    JS_ReportError(cx,
                   "Incorrect argument. Expecting a string or a typed array");
    return false;
  }

  if (size > RilMessage::DATA_SIZE) {
    JS_ReportError(cx, "Passed-in data is too large");
    return false;
  }

  rm->mSize = size;
  memcpy(rm->mData, data, size);

  RilMessage *tosend = rm.forget();
  JS_ALWAYS_TRUE(SendRilMessage(&tosend));
  return true;
}

bool
ConnectWorkerToRIL::RunTask(JSContext *aCx)
{
  
  
  NS_ASSERTION(!NS_IsMainThread(), "Expecting to be on the worker thread");
  NS_ASSERTION(!JS_IsRunning(aCx), "Are we being called somehow?");
  JSObject *workerGlobal = JS_GetGlobalObject(aCx);

  return JS_DefineFunction(aCx, workerGlobal, "postRILMessage", PostToRIL, 1, 0);
}

class RILReceiver : public RilConsumer
{
  class DispatchRILEvent : public WorkerTask {
  public:
    DispatchRILEvent(RilMessage *aMessage)
      : mMessage(aMessage)
    { }

    virtual bool RunTask(JSContext *aCx);

  private:
    nsAutoPtr<RilMessage> mMessage;
  };

public:
  RILReceiver(WorkerCrossThreadDispatcher *aDispatcher)
    : mDispatcher(aDispatcher)
  { }

  virtual void MessageReceived(RilMessage *aMessage) {
    nsRefPtr<DispatchRILEvent> dre(new DispatchRILEvent(aMessage));
    mDispatcher->PostTask(dre);
  }

private:
  nsRefPtr<WorkerCrossThreadDispatcher> mDispatcher;
};

bool
RILReceiver::DispatchRILEvent::RunTask(JSContext *aCx)
{
  JSObject *obj = JS_GetGlobalObject(aCx);

  JSObject *array =
    js_CreateTypedArray(aCx, js::TypedArray::TYPE_UINT8, mMessage->mSize);
  if (!array) {
    return false;
  }

  memcpy(JS_GetTypedArrayData(array), mMessage->mData, mMessage->mSize);
  jsval argv[] = { OBJECT_TO_JSVAL(array) };
  return JS_CallFunctionName(aCx, obj, "onRILMessage", NS_ARRAY_LENGTH(argv),
                             argv, argv);
}

} 

Radio::Radio()
  : mShutdown(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance, "There should only be one instance!");
}

Radio::~Radio()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance || gInstance == this,
               "There should only be one instance!");
  gInstance = nsnull;
}

nsresult
Radio::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "We can only initialize on the main thread");

  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (!obs) {
    NS_WARNING("Failed to get observer service!");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = obs->AddObserver(this, PROFILE_BEFORE_CHANGE_TOPIC, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsITelephonyWorker> worker(do_CreateInstance(kTelephonyWorkerCID));
  if (!worker) {
    return NS_ERROR_FAILURE;
  }

  jsval workerval;
  rv = worker->GetWorker(&workerval);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(!JSVAL_IS_PRIMITIVE(workerval), "bad worker value");

  JSContext *cx;
  rv = nsContentUtils::ThreadJSContextStack()->GetSafeJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCxPusher pusher;
  if (!cx || !pusher.Push(cx, false)) {
    return NS_ERROR_FAILURE;
  }

  JSObject *workerobj = JSVAL_TO_OBJECT(workerval);

  JSAutoRequest ar(cx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, workerobj)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  WorkerCrossThreadDispatcher *wctd = GetWorkerCrossThreadDispatcher(cx, workerval);
  if (!wctd) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ConnectWorkerToRIL> connection = new ConnectWorkerToRIL();
  if (!wctd->PostTask(connection)) {
    return NS_ERROR_UNEXPECTED;
  }

  
  mozilla::RefPtr<RILReceiver> receiver = new RILReceiver(wctd);
  StartRil(receiver);

  mRadioInterface = do_QueryInterface(worker);
  NS_ENSURE_TRUE(mRadioInterface, NS_ERROR_FAILURE);

  return NS_OK;
}

void
Radio::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  StopRil();
  mRadioInterface = nsnull;

  mShutdown = true;
}


already_AddRefed<Radio>
Radio::FactoryCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<Radio> instance(gInstance);

  if (!instance) {
    instance = new Radio();
    if (NS_FAILED(instance->Init())) {
      return nsnull;
    }

    gInstance = instance;
  }

  return instance.forget();
}


already_AddRefed<nsIRadioInterface>
Radio::GetRadioInterface()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (gInstance) {
    nsCOMPtr<nsIRadioInterface> retval = gInstance->mRadioInterface;
    return retval.forget();
  }

  return nsnull;
}


NS_IMPL_ISUPPORTS1(Radio, nsIObserver)

NS_IMETHODIMP
Radio::Observe(nsISupports* aSubject, const char* aTopic,
               const PRUnichar* aData)
{
  if (!strcmp(aTopic, PROFILE_BEFORE_CHANGE_TOPIC)) {
    Shutdown();

    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    if (obs) {
      if (NS_FAILED(obs->RemoveObserver(this, aTopic))) {
        NS_WARNING("Failed to remove observer!");
      }
    }
    else {
      NS_WARNING("Failed to get observer service!");
    }
  }

  return NS_OK;
}
