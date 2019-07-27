






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

  nsresult Duplicate(UnixSocketConnector*& aConnector) override;

private:
  nsresult CreateSocket(int& aFd) const;
  nsresult SetSocketFlags(int aFd) const;
  nsresult CreateAddress(struct sockaddr& aAddress,
                         socklen_t& aAddressLength) const;

  nsCString mSocketName;
};

END_BLUETOOTH_NAMESPACE

#endif
