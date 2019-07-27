







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

  ssize_t ReceiveData(int aFd);

  nsresult SendPendingData(int aFd);

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
