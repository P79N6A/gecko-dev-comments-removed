





#ifndef mozilla_ipc_UnixSocketWatcher_h
#define mozilla_ipc_UnixSocketWatcher_h

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#ifdef MOZ_B2G_BT_BLUEZ
#include <bluetooth/bluetooth.h>
#include <bluetooth/sco.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#endif
#include "UnixFdWatcher.h"

namespace mozilla {
namespace ipc {

union sockaddr_any {
  sockaddr_storage storage; 
  sockaddr_un un;
  sockaddr_in in;
  sockaddr_in6 in6;
#ifdef MOZ_B2G_BT_BLUEZ
  sockaddr_sco sco;
  sockaddr_rc rc;
  sockaddr_l2 l2;
#endif
  
};

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

  
  virtual void OnAccepted(int aFd, const sockaddr_any* aAddr,
                          socklen_t aAddrLen) {};

  
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
