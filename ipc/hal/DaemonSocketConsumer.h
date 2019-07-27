





#ifndef mozilla_ipc_DaemonSocketConsumer_h
#define mozilla_ipc_DaemonSocketConsumer_h

namespace mozilla {
namespace ipc {

class DaemonSocketPDU;






class DaemonSocketIOConsumer
{
public:
  virtual ~DaemonSocketIOConsumer();

  virtual void Handle(DaemonSocketPDU& aPDU) = 0;
  virtual void StoreUserData(const DaemonSocketPDU& aPDU) = 0;

protected:
  DaemonSocketIOConsumer();
};




class DaemonSocketConsumer
{
public:
  




  virtual void OnConnectSuccess(int aIndex) = 0;

  




  virtual void OnConnectError(int aIndex) = 0;

  




  virtual void OnDisconnect(int aIndex) = 0;

protected:
  DaemonSocketConsumer();
  virtual ~DaemonSocketConsumer();
};

}
}

#endif

