







#ifndef mozilla_ipc_datasocket_h
#define mozilla_ipc_datasocket_h

#include "mozilla/ipc/SocketBase.h"

namespace mozilla {
namespace ipc {










class DataSocket : public SocketBase
{
public:
  virtual ~DataSocket();

  





  virtual void SendSocketData(UnixSocketIOBuffer* aBuffer) = 0;
};










class DataSocketIO : public SocketIOBase
{
public:
  virtual ~DataSocketIO();

  









  virtual nsresult QueryReceiveBuffer(UnixSocketIOBuffer** aBuffer) = 0;

  








  virtual void ConsumeBuffer() = 0;

  





  virtual void DiscardBuffer() = 0;

  void EnqueueData(UnixSocketIOBuffer* aBuffer);
  bool HasPendingData() const;

  template <typename T>
  ssize_t ReceiveData(int aFd, T* aIO)
  {
    MOZ_ASSERT(aFd >= 0);
    MOZ_ASSERT(aIO);

    UnixSocketIOBuffer* incoming;
    nsresult rv = QueryReceiveBuffer(&incoming);
    if (NS_FAILED(rv)) {
      
      NS_DispatchToMainThread(new SocketIORequestClosingRunnable(aIO));
      return -1;
    }

    ssize_t res = incoming->Receive(aFd);
    if (res < 0) {
      
      DiscardBuffer();
      NS_DispatchToMainThread(new SocketIORequestClosingRunnable(aIO));
      return -1;
    } else if (!res) {
      
      DiscardBuffer();
      NS_DispatchToMainThread(new SocketIORequestClosingRunnable(aIO));
      return 0;
    }

#ifdef MOZ_TASK_TRACER
    
    
    AutoSourceEvent taskTracerEvent(SourceEventType::Unixsocket);
#endif

    ConsumeBuffer();

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
        
        NS_DispatchToMainThread(new SocketIORequestClosingRunnable(aIO));
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
  DataSocketIO();

private:
  


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
