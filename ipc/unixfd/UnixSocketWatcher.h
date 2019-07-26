





#ifndef mozilla_ipc_UnixSocketWatcher_h
#define mozilla_ipc_UnixSocketWatcher_h

#include "UnixFdWatcher.h"

namespace mozilla {
namespace ipc {

class UnixSocketWatcher : public UnixFdWatcher
{
public:
  enum ConnectionStatus {
    SOCKET_IS_DISCONNECTED = 0,
    SOCKET_IS_LISTENING,
    SOCKET_IS_CONNECTING,
    SOCKET_IS_CONNECTED
  };

  virtual ~UnixSocketWatcher();

  virtual void Close() MOZ_OVERRIDE;

  ConnectionStatus GetConnectionStatus() const
  {
    return mConnectionStatus;
  }

  
  nsresult Connect(const struct sockaddr* aAddr, socklen_t aAddrLen);

  
  nsresult Listen(const struct sockaddr* aAddr, socklen_t aAddrLen);

  
  virtual void OnAccepted(int aFd) {};

  
  virtual void OnConnected() {};

  
  virtual void OnListening() {};

  
  virtual void OnSocketCanReceiveWithoutBlocking() {};

  
  virtual void OnSocketCanSendWithoutBlocking() {};

protected:
  UnixSocketWatcher(MessageLoop* aIOLoop);
  UnixSocketWatcher(MessageLoop* aIOLoop, int aFd,
                    ConnectionStatus aConnectionStatus);
  void SetSocket(int aFd, ConnectionStatus aConnectionStatus);

private:
  void OnFileCanReadWithoutBlocking(int aFd) MOZ_OVERRIDE;
  void OnFileCanWriteWithoutBlocking(int aFd) MOZ_OVERRIDE;

  ConnectionStatus mConnectionStatus;
};

}
}

#endif
