






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

  nsresult Duplicate(UnixSocketConnector*& aConnector) override;


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
