






#ifndef mozilla_ipc_KeyStoreConnector_h
#define mozilla_ipc_KeyStoreConnector_h

#include "mozilla/ipc/UnixSocketConnector.h"

namespace mozilla {
namespace ipc {

class KeyStoreConnector final : public UnixSocketConnector
{
public:
  KeyStoreConnector(const char** const aAllowedUsers);
  ~KeyStoreConnector();

  
  

  nsresult ConvertAddressToString(const struct sockaddr& aAddress,
                                  socklen_t aAddressLength,
                                  nsACString& aAddressString) override;

  nsresult CreateListenSocket(struct sockaddr* aAddress,
                              socklen_t* aAddressLength,
                              int& aListenFd) override;

  nsresult AcceptStreamSocket(int aListenFd,
                              struct sockaddr* aAddress,
                              socklen_t* aAddressLength,
                              int& aStreamFd) override;

  nsresult CreateStreamSocket(struct sockaddr* aAddress,
                              socklen_t* aAddressLength,
                              int& aStreamFd) override;

  nsresult Duplicate(UnixSocketConnector*& aConnector) override;

private:
  nsresult CreateSocket(int& aFd) const;
  nsresult SetSocketFlags(int aFd) const;
  nsresult CheckPermission(int aFd) const;
  nsresult CreateAddress(struct sockaddr& aAddress,
                         socklen_t& aAddressLength) const;

  const char** const mAllowedUsers;
};

}
}

#endif
