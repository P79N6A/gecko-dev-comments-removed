





#ifndef mozilla_dom_bluetooth_BluetoothUnixSocketConnector_h
#define mozilla_dom_bluetooth_BluetoothUnixSocketConnector_h

#include <bluetooth/bluetooth.h>
#include "BluetoothCommon.h"
#include "mozilla/ipc/UnixSocketConnector.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothUnixSocketConnector final
  : public mozilla::ipc::UnixSocketConnector
{
public:
  BluetoothUnixSocketConnector(const nsACString& aAddressString,
                               BluetoothSocketType aType,
                               int aChannel, bool aAuth, bool aEncrypt);
  ~BluetoothUnixSocketConnector();

  
  

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
  static nsresult ConvertAddressString(const char* aAddressString,
                                       bdaddr_t& aAddress);

  nsCString mAddressString;
  BluetoothSocketType mType;
  int mChannel;
  bool mAuth;
  bool mEncrypt;
};

END_BLUETOOTH_NAMESPACE

#endif
