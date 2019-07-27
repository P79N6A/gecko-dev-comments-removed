





#include "mozilla/ipc/Ril.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "jsfriendapi.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/workers/Workers.h"
#include "mozilla/ipc/RilSocket.h"
#include "mozilla/ipc/RilSocketConsumer.h"
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

namespace mozilla {
namespace ipc {

USING_WORKERS_NAMESPACE

class RilConsumer;

static const char RIL_SOCKET_NAME[] = "/dev/socket/rilproxy";

static nsTArray<nsAutoPtr<RilConsumer>> sRilConsumers;





class RilConsumer final : public RilSocketConsumer
{
public:
  RilConsumer();

  void Send(UnixSocketRawData* aRawData);
  void Close();

  nsresult Register(unsigned long aClientId,
                    WorkerCrossThreadDispatcher* aDispatcher);
  void Unregister();

  
  

  void ReceiveSocketData(JSContext* aCx,
                         int aIndex,
                         nsAutoPtr<UnixSocketBuffer>& aBuffer) override;
  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

private:
  nsRefPtr<RilSocket> mSocket;
  nsCString mAddress;
  bool mShutdown;
};

class ConnectWorkerToRIL final : public WorkerTask
{
public:
  bool RunTask(JSContext* aCx) override;
};

static bool
PostToRIL(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
{
  JS::CallArgs args = JS::CallArgsFromVp(aArgc, aVp);

  if (args.length() != 2) {
    JS_ReportError(aCx, "Expecting two arguments with the RIL message");
    return false;
  }

  int clientId = args[0].toInt32();
  JS::Value v = args[1];

  nsAutoPtr<UnixSocketRawData> raw;

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

  if ((ssize_t)sRilConsumers.Length() <= clientId || !sRilConsumers[clientId]) {
    
    return true;
  }

  sRilConsumers[clientId]->Send(raw.forget());

  return true;
}

bool
ConnectWorkerToRIL::RunTask(JSContext* aCx)
{
  
  
  
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

RilConsumer::RilConsumer()
  : mShutdown(false)
{ }

nsresult
RilConsumer::Register(unsigned long aClientId,
                      WorkerCrossThreadDispatcher* aDispatcher)
{
  
  
  if (!aClientId) {
    mAddress = RIL_SOCKET_NAME;
  } else {
    struct sockaddr_un addr_un;
    snprintf(addr_un.sun_path, sizeof addr_un.sun_path, "%s%lu",
             RIL_SOCKET_NAME, aClientId);
    mAddress = addr_un.sun_path;
  }

  mSocket = new RilSocket(aDispatcher, this, aClientId);

  nsresult rv = mSocket->Connect(new RilConnector(mAddress, aClientId));
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

void
RilConsumer::Unregister()
{
  mShutdown = true;
  Close();
}

void
RilConsumer::Send(UnixSocketRawData* aRawData)
{
  if (!mSocket || mSocket->GetConnectionStatus() == SOCKET_DISCONNECTED) {
    
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
RilConsumer::ReceiveSocketData(JSContext* aCx,
                               int aIndex,
                               nsAutoPtr<UnixSocketBuffer>& aBuffer)
{
  JS::Rooted<JSObject*> obj(aCx, JS::CurrentGlobalOrNull(aCx));

  JS::Rooted<JSObject*> array(aCx, JS_NewUint8Array(aCx, aBuffer->GetSize()));
  if (!array) {
    return;
  }
  {
    JS::AutoCheckCannotGC nogc;
    memcpy(JS_GetArrayBufferViewData(array, nogc),
           aBuffer->GetData(), aBuffer->GetSize());
  }

  JS::AutoValueArray<2> args(aCx);
  args[0].setNumber((uint32_t)aIndex);
  args[1].setObject(*array);

  JS::Rooted<JS::Value> rval(aCx);
  JS_CallFunctionName(aCx, obj, "onRILMessage", args, &rval);
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

class RilWorker::RegisterConsumerTask : public WorkerTask
{
public:
  RegisterConsumerTask(unsigned int aClientId,
                       WorkerCrossThreadDispatcher* aDispatcher)
    : mClientId(aClientId)
    , mDispatcher(aDispatcher)
  {
    MOZ_ASSERT(mDispatcher);
  }

  bool RunTask(JSContext* aCx) override
  {
    sRilConsumers.EnsureLengthAtLeast(mClientId + 1);

    MOZ_ASSERT(!sRilConsumers[mClientId]);

    nsAutoPtr<RilConsumer> rilConsumer(new RilConsumer());

    nsresult rv = rilConsumer->Register(mClientId, mDispatcher);
    if (NS_FAILED(rv)) {
      return false;
    }
    sRilConsumers[mClientId] = rilConsumer;

    return true;
  }

private:
  unsigned int mClientId;
  nsRefPtr<WorkerCrossThreadDispatcher> mDispatcher;
};

nsresult
RilWorker::RegisterConsumer(unsigned int aClientId)
{
  nsRefPtr<ConnectWorkerToRIL> connection = new ConnectWorkerToRIL();
  if (!mDispatcher->PostTask(connection)) {
    NS_WARNING("Failed to connect worker to ril");
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<RegisterConsumerTask> task = new RegisterConsumerTask(aClientId,
                                                                 mDispatcher);
  if (!mDispatcher->PostTask(task)) {
    NS_WARNING("Failed to post register-consumer task.");
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

class RilWorker::UnregisterConsumerTask : public WorkerTask
{
public:
  UnregisterConsumerTask(unsigned int aClientId)
    : mClientId(aClientId)
  { }

  bool RunTask(JSContext* aCx) override
  {
    MOZ_ASSERT(mClientId < sRilConsumers.Length());
    MOZ_ASSERT(sRilConsumers[mClientId]);

    sRilConsumers[mClientId]->Unregister();
    sRilConsumers[mClientId] = nullptr;

    return true;
  }

private:
  unsigned int mClientId;
};

void
RilWorker::UnregisterConsumer(unsigned int aClientId)
{
  nsRefPtr<UnregisterConsumerTask> task =
    new UnregisterConsumerTask(aClientId);

  if (!mDispatcher->PostTask(task)) {
    NS_WARNING("Failed to post unregister-consumer task.");
    return;
  }
}

} 
} 
