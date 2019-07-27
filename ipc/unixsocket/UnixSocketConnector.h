





#ifndef mozilla_ipc_unixsocketconnector_h
#define mozilla_ipc_unixsocketconnector_h

#include <sys/socket.h>
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "nsString.h"

namespace mozilla {
namespace ipc {








class UnixSocketConnector
{
public:
  virtual ~UnixSocketConnector();

  







  virtual nsresult ConvertAddressToString(const struct sockaddr& aAddress,
                                          socklen_t aAddressLength,
                                          nsACString& aAddressString) = 0;

  







  virtual nsresult CreateListenSocket(struct sockaddr* aAddress,
                                      socklen_t* aAddressLength,
                                      int& aListenFd) = 0;

  








  virtual nsresult AcceptStreamSocket(int aListenFd,
                                      struct sockaddr* aAddress,
                                      socklen_t* aAddressLen,
                                      int& aStreamFd) = 0;

  







  virtual nsresult CreateStreamSocket(struct sockaddr* aAddress,
                                      socklen_t* aAddressLength,
                                      int& aStreamFd) = 0;

protected:
  UnixSocketConnector();
};

}
}

#endif
