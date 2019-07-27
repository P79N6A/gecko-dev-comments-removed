





#ifndef mozilla_ipc_listensocketconsumer_h
#define mozilla_ipc_listensocketconsumer_h

namespace mozilla {
namespace ipc {




class ListenSocketConsumer
{
public:
  virtual ~ListenSocketConsumer();

  




  virtual void OnConnectSuccess(int aIndex) = 0;

  




  virtual void OnConnectError(int aIndex) = 0;

  




  virtual void OnDisconnect(int aIndex) = 0;
};

}
}

#endif
