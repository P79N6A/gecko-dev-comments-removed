






#ifndef mozilla_ipc_RilConnector_h
#define mozilla_ipc_RilConnector_h

#include "mozilla/ipc/UnixSocketConnector.h"

namespace mozilla {
namespace ipc {




class RilConnector final : public UnixSocketConnector
{
public:
  RilConnector(const nsACString& aAddressString,
               unsigned long aClientId);
  ~RilConnector();

  
  

  nsresult ConvertAddressToString(const struct sockaddr& aAddress,
                                  socklen_t aAddressLength,
                                  nsACString& aAddressString) override;

  nsresult CreateListenSocket(struct sockaddr* aAddress,
                              socklen_t* aAddressLength,
                              int& aListenFd) override;

  nsresult AcceptStreamSocket(int aListenFd,
                              struct sockaddr* aAddress,
                              socklen_t* aAddressLen,
                              int& aStreamFd) override;

  nsresult CreateStreamSocket(struct sockaddr* aAddress,
                              socklen_t* aAddressLength,
                              int& aStreamFd) override;

  

  int Create() override;
  bool CreateAddr(bool aIsServer,
                  socklen_t& aAddrSize,
                  sockaddr_any& aAddr,
                  const char* aAddress) override;
  bool SetUp(int aFd) override;
  bool SetUpListenSocket(int aFd) override;
  void GetSocketAddr(const sockaddr_any& aAddr,
                     nsAString& aAddrStr) override;

private:
  nsresult CreateSocket(int aDomain, int& aFd) const;
  nsresult SetSocketFlags(int aFd) const;
  nsresult CreateAddress(int aDomain,
                         struct sockaddr& aAddress,
                         socklen_t& aAddressLength) const;

  nsCString mAddressString;
  unsigned long mClientId;
};

}
}

#endif
