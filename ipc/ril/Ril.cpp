





#include "mozilla/ipc/Ril.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h> 
#include "jsfriendapi.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/dom/workers/Workers.h"
#include "mozilla/ipc/StreamSocket.h"
#include "mozilla/ipc/StreamSocketConsumer.h"
#include "nsThreadUtils.h" 
#include "RilConnector.h"

#ifdef CHROMIUM_LOG
#undef CHROMIUM_LOG
#endif

#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define CHROMIUM_LOG(args...)  printf(args);
#endif

USING_WORKERS_NAMESPACE

namespace mozilla {
namespace ipc {

class RilConsumer;

static const char RIL_SOCKET_NAME[] = "/dev/socket/rilproxy";

static nsTArray<nsAutoPtr<RilConsumer>> sRilConsumers;





class RilConsumer final : public StreamSocketConsumer
{
public:
  friend class RilWorker;

  RilConsumer(unsigned long aClientId,
              WorkerCrossThreadDispatcher* aDispatcher);

  void Send(UnixSocketRawData* aRawData);
  void Close();

  
  

  void ReceiveSocketData(int aIndex,
                         nsAutoPtr<UnixSocketBuffer>& aBuffer) override;
  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

private:
  nsRefPtr<StreamSocket> mSocket;
  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  nsCString mAddress;
  bool mShutdown;
};

class ConnectWorkerToRIL final : public WorkerTask
{
public:
  bool RunTask(JSContext* aCx) override;
};

class SendRilSocketDataTask final : public nsRunnable
{
public:
  SendRilSocketDataTask(unsigned long aClientId,
                        UnixSocketRawData* aRawData)
    : mRawData(aRawData)
    , mClientId(aClientId)
  { }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (sRilConsumers.Length() <= mClientId || !sRilConsumers[mClientId]) {
      
      delete mRawData;
      return NS_OK;
    }

    sRilConsumers[mClientId]->Send(mRawData);
    return NS_OK;
  }

private:
  UnixSocketRawData* mRawData;
  unsigned long mClientId;
};

static bool
PostToRIL(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
{
  JS::CallArgs args = JS::CallArgsFromVp(aArgc, aVp);
  NS_ASSERTION(!NS_IsMainThread(), "Expecting to be on the worker thread");

  if (args.length() != 2) {
    JS_ReportError(aCx, "Expecting two arguments with the RIL message");
    return false;
  }

  int clientId = args[0].toInt32();
  JS::Value v = args[1];

  UnixSocketRawData* raw = nullptr;

  if (v.isString()) {
    JSAutoByteString abs;
    JS::Rooted<JSString*> str(aCx, v.toString());
    if (!abs.encodeUtf8(aCx, str)) {
      return false;
    }

    raw = new UnixSocketRawData(abs.ptr(), abs.length());
  } else if (!v.isPrimitive()) {
    JSObject* obj = v.toObjectOrNull();
    if (!JS_IsTypedArrayObject(obj)) {
      JS_ReportError(aCx, "Object passed in wasn't a typed array");
      return false;
    }

    uint32_t type = JS_GetArrayBufferViewType(obj);
    if (type != js::Scalar::Int8 &&
        type != js::Scalar::Uint8 &&
        type != js::Scalar::Uint8Clamped) {
      JS_ReportError(aCx, "Typed array data is not octets");
      return false;
    }

    JS::AutoCheckCannotGC nogc;
    size_t size = JS_GetTypedArrayByteLength(obj);
    void* data = JS_GetArrayBufferViewData(obj, nogc);
    raw = new UnixSocketRawData(data, size);
  } else {
    JS_ReportError(
      aCx, "Incorrect argument. Expecting a string or a typed array");
    return false;
  }

  if (!raw) {
    JS_ReportError(aCx, "Unable to post to RIL");
    return false;
  }

  nsRefPtr<SendRilSocketDataTask> task = new SendRilSocketDataTask(clientId,
                                                                   raw);
  NS_DispatchToMainThread(task);
  return true;
}

bool
ConnectWorkerToRIL::RunTask(JSContext* aCx)
{
  
  
  NS_ASSERTION(!NS_IsMainThread(), "Expecting to be on the worker thread");
  NS_ASSERTION(!JS_IsRunning(aCx), "Are we being called somehow?");
  JS::Rooted<JSObject*> workerGlobal(aCx, JS::CurrentGlobalOrNull(aCx));

  
  
  JS::Rooted<JS::Value> val(aCx);
  if (!JS_GetProperty(aCx, workerGlobal, "postRILMessage", &val)) {
    JS_ReportPendingException(aCx);
    return false;
  }

  
  if (JSTYPE_FUNCTION == JS_TypeOfValue(aCx, val)) {
    return true;
  }

  return !!JS_DefineFunction(aCx, workerGlobal, "postRILMessage",
                             PostToRIL, 2, 0);
}

class DispatchRILEvent final : public WorkerTask
{
public:
  DispatchRILEvent(unsigned long aClient, UnixSocketBuffer* aBuffer)
    : mClientId(aClient)
    , mBuffer(aBuffer)
  { }

  bool RunTask(JSContext* aCx) override;

private:
  unsigned long mClientId;
  nsAutoPtr<UnixSocketBuffer> mBuffer;
};

bool
DispatchRILEvent::RunTask(JSContext* aCx)
{
  JS::Rooted<JSObject*> obj(aCx, JS::CurrentGlobalOrNull(aCx));

  JS::Rooted<JSObject*> array(aCx,
                              JS_NewUint8Array(aCx, mBuffer->GetSize()));
  if (!array) {
    return false;
  }
  {
    JS::AutoCheckCannotGC nogc;
    memcpy(JS_GetArrayBufferViewData(array, nogc),
           mBuffer->GetData(), mBuffer->GetSize());
  }

  JS::AutoValueArray<2> args(aCx);
  args[0].setNumber((uint32_t)mClientId);
  args[1].setObject(*array);

  JS::Rooted<JS::Value> rval(aCx);
  return JS_CallFunctionName(aCx, obj, "onRILMessage", args, &rval);
}

RilConsumer::RilConsumer(unsigned long aClientId,
                         WorkerCrossThreadDispatcher* aDispatcher)
  : mDispatcher(aDispatcher)
  , mShutdown(false)
{
  
  
  if (!aClientId) {
    mAddress = RIL_SOCKET_NAME;
  } else {
    struct sockaddr_un addr_un;
    snprintf(addr_un.sun_path, sizeof addr_un.sun_path, "%s%lu",
             RIL_SOCKET_NAME, aClientId);
    mAddress = addr_un.sun_path;
  }

  mSocket = new StreamSocket(this, aClientId);
  mSocket->Connect(new RilConnector(mAddress, aClientId));
}

void
RilConsumer::Send(UnixSocketRawData* aRawData)
{
  if (!mSocket || mSocket->GetConnectionStatus() != SOCKET_CONNECTED) {
    
    delete aRawData;
    return;
  }
  mSocket->SendSocketData(aRawData);
}

void
RilConsumer::Close()
{
  if (mSocket) {
    mSocket->Close();
    mSocket = nullptr;
  }
}



void
RilConsumer::ReceiveSocketData(int aIndex,
                               nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<DispatchRILEvent> dre(new DispatchRILEvent(aIndex, aBuffer.forget()));
  mDispatcher->PostTask(dre);
}

void
RilConsumer::OnConnectSuccess(int aIndex)
{
  
  CHROMIUM_LOG("RIL[%d]: %s\n", aIndex, __FUNCTION__);
}

void
RilConsumer::OnConnectError(int aIndex)
{
  CHROMIUM_LOG("RIL[%d]: %s\n", aIndex, __FUNCTION__);
  Close();
}

void
RilConsumer::OnDisconnect(int aIndex)
{
  CHROMIUM_LOG("RIL[%d]: %s\n", aIndex, __FUNCTION__);
  if (mShutdown) {
    return;
  }
  mSocket->Connect(new RilConnector(mAddress, aIndex),
                   mSocket->GetSuggestedConnectDelayMs());
}





nsTArray<nsAutoPtr<RilWorker>> RilWorker::sRilWorkers;

nsresult
RilWorker::Register(unsigned int aClientId,
                    WorkerCrossThreadDispatcher* aDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());

  sRilWorkers.EnsureLengthAtLeast(aClientId + 1);

  if (sRilWorkers[aClientId]) {
    NS_WARNING("RilWorkers already registered");
    return NS_ERROR_FAILURE;
  }

  
  sRilWorkers[aClientId] = new RilWorker(aDispatcher);

  nsresult rv = sRilWorkers[aClientId]->RegisterConsumer(aClientId);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

void
RilWorker::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  for (size_t i = 0; i < sRilWorkers.Length(); ++i) {
    if (!sRilWorkers[i]) {
      continue;
    }
    sRilWorkers[i]->UnregisterConsumer(i);
    sRilWorkers[i] = nullptr;
  }
}

RilWorker::RilWorker(WorkerCrossThreadDispatcher* aDispatcher)
  : mDispatcher(aDispatcher)
{
  MOZ_ASSERT(mDispatcher);
}

nsresult
RilWorker::RegisterConsumer(unsigned int aClientId)
{
  nsRefPtr<ConnectWorkerToRIL> connection = new ConnectWorkerToRIL();
  if (!mDispatcher->PostTask(connection)) {
    NS_WARNING("Failed to connect worker to ril");
    return NS_ERROR_UNEXPECTED;
  }

  sRilConsumers.EnsureLengthAtLeast(aClientId + 1);

  MOZ_ASSERT(!sRilConsumers[aClientId]);

  sRilConsumers[aClientId] = new RilConsumer(aClientId, mDispatcher);

  return NS_OK;
}

void
RilWorker::UnregisterConsumer(unsigned int aClientId)
{
  MOZ_ASSERT(aClientId < sRilConsumers.Length());
  MOZ_ASSERT(sRilConsumers[aClientId]);

  sRilConsumers[aClientId]->mShutdown = true;
  sRilConsumers[aClientId]->Close();
  sRilConsumers[aClientId] = nullptr;
}

} 
} 
