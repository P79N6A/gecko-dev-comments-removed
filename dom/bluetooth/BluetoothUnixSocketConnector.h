





#ifndef mozilla_dom_bluetooth_BluetoothUnixSocketConnector_h
#define mozilla_dom_bluetooth_BluetoothUnixSocketConnector_h

#include "BluetoothCommon.h"
#include <mozilla/ipc/UnixSocket.h>

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothUnixSocketConnector : public mozilla::ipc::UnixSocketConnector
{
public:
  BluetoothUnixSocketConnector(BluetoothSocketType aType, int aChannel,
                               bool aAuth, bool aEncrypt);
  virtual ~BluetoothUnixSocketConnector()
  {}
  virtual int Create() MOZ_OVERRIDE;
  virtual void CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          struct sockaddr* aAddr,
                          const char* aAddress) MOZ_OVERRIDE;
  virtual bool Setup(int aFd) MOZ_OVERRIDE;
private:
  BluetoothSocketType mType;
  int mChannel;
  bool mAuth;
  bool mEncrypt;
};

END_BLUETOOTH_NAMESPACE

#endif
