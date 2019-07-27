






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

  

  int Create();
  bool CreateAddr(bool aIsServer,
                  socklen_t& aAddrSize,
                  sockaddr_any& aAddr,
                  const char* aAddress);
  bool SetUp(int aFd);
  bool SetUpListenSocket(int aFd);
  void GetSocketAddr(const sockaddr_any& aAddr,
                     nsAString& aAddrStr);

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
