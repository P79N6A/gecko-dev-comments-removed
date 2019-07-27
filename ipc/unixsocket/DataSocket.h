







#ifndef mozilla_ipc_datasocket_h
#define mozilla_ipc_datasocket_h

#include "mozilla/ipc/SocketBase.h"

namespace mozilla {
namespace ipc {










class DataSocket : public SocketBase
{
public:
  virtual ~DataSocket();

  





  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer) = 0;

  





  virtual void SendSocketData(UnixSocketIOBuffer* aBuffer) = 0;
};









template <typename T>
class SocketIOReceiveRunnable final : public SocketIORunnable<T>
{
public:
  SocketIOReceiveRunnable(T* aIO, UnixSocketBuffer* aBuffer)
    : SocketIORunnable<T>(aIO)
    , mBuffer(aBuffer)
  { }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    T* io = SocketIORunnable<T>::GetIO();

    if (io->IsShutdownOnMainThread()) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }

    DataSocket* dataSocket = io->GetDataSocket();
    MOZ_ASSERT(dataSocket);

    dataSocket->ReceiveSocketData(mBuffer);

    return NS_OK;
  }

private:
  nsAutoPtr<UnixSocketBuffer> mBuffer;
};










class DataSocketIO : public SocketIOBase
{
public:
  virtual ~DataSocketIO();

  void EnqueueData(UnixSocketIOBuffer* aBuffer);
  bool HasPendingData() const;

  template <typename T>
  ssize_t ReceiveData(int aFd, T* aIO)
  {
    MOZ_ASSERT(aFd >= 0);
    MOZ_ASSERT(aIO);

    nsAutoPtr<UnixSocketRawData> incoming(
      new UnixSocketRawData(mMaxReadSize));

    ssize_t res = incoming->Receive(aFd);
    if (res < 0) {
      
      nsRefPtr<nsRunnable> r = new SocketIORequestClosingRunnable<T>(aIO);
      NS_DispatchToMainThread(r);
      return -1;
    } else if (!res) {
      
      nsRefPtr<nsRunnable> r = new SocketIORequestClosingRunnable<T>(aIO);
      NS_DispatchToMainThread(r);
      return 0;
    }

#ifdef MOZ_TASK_TRACER
    
    
    AutoSourceEvent taskTracerEvent(SourceEventType::Unixsocket);
#endif

    nsRefPtr<nsRunnable> r =
      new SocketIOReceiveRunnable<T>(aIO, incoming.forget());
    NS_DispatchToMainThread(r);

    return res;
  }

  template <typename T>
  nsresult SendPendingData(int aFd, T* aIO)
  {
    MOZ_ASSERT(aFd >= 0);
    MOZ_ASSERT(aIO);

    while (HasPendingData()) {
      UnixSocketIOBuffer* outgoing = mOutgoingQ.ElementAt(0);

      ssize_t res = outgoing->Send(aFd);
      if (res < 0) {
        
        nsRefPtr<nsRunnable> r = new SocketIORequestClosingRunnable<T>(aIO);
        NS_DispatchToMainThread(r);
        return NS_ERROR_FAILURE;
      } else if (!res && outgoing->GetSize()) {
        
        return NS_OK;
      }
      if (!outgoing->GetSize()) {
        mOutgoingQ.RemoveElementAt(0);
        delete outgoing;
      }
    }

    return NS_OK;
  }

protected:
  DataSocketIO(size_t aMaxReadSize);

private:
  const size_t mMaxReadSize;

  


  nsTArray<UnixSocketIOBuffer*> mOutgoingQ;
};









template<typename Tio, typename Tdata>
class SocketIOSendTask final : public SocketIOTask<Tio>
{
public:
  SocketIOSendTask(Tio* aIO, Tdata* aData)
  : SocketIOTask<Tio>(aIO)
  , mData(aData)
  {
    MOZ_ASSERT(aData);
  }

  void Run() override
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!SocketIOTask<Tio>::IsCanceled());

    Tio* io = SocketIOTask<Tio>::GetIO();
    MOZ_ASSERT(!io->IsShutdownOnIOThread());

    io->Send(mData);
  }

private:
  Tdata* mData;
};

}
}

#endif
