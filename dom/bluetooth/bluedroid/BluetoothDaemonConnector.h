






#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothdaemonconnector_h
#define mozilla_dom_bluetooth_bluedroid_bluetoothdaemonconnector_h

#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/ipc/UnixSocketConnector.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDaemonConnector final
  : public mozilla::ipc::UnixSocketConnector
{
public:
  BluetoothDaemonConnector(const nsACString& aSocketName);
  ~BluetoothDaemonConnector();

  
  

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
             mozilla::ipc::sockaddr_any& aAddr,
             const char* aAddress) override;
  bool SetUp(int aFd) override;
  bool SetUpListenSocket(int aFd) override;
  void GetSocketAddr(const mozilla::ipc::sockaddr_any& aAddr,
                     nsAString& aAddrStr) override;

private:
  nsresult CreateSocket(int& aFd) const;
  nsresult SetSocketFlags(int aFd) const;
  nsresult CreateAddress(struct sockaddr& aAddress,
                         socklen_t& aAddressLength) const;

  nsCString mSocketName;
};

END_BLUETOOTH_NAMESPACE

#endif
