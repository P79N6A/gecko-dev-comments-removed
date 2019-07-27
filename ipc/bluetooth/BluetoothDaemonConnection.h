





#ifndef mozilla_ipc_bluetooth_BluetoothDaemonConnection_h
#define mozilla_ipc_bluetooth_BluetoothDaemonConnection_h

#include "mozilla/Attributes.h"
#include "mozilla/FileUtils.h"
#include "mozilla/ipc/ConnectionOrientedSocket.h"
#include "mozilla/ipc/DataSocket.h"
#include "nsError.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace ipc {

class BluetoothDaemonConnectionIO;
class BluetoothDaemonPDUConsumer;





















class BluetoothDaemonPDU final : public UnixSocketIOBuffer
{
public:
  enum {
    OFF_SERVICE = 0,
    OFF_OPCODE = 1,
    OFF_LENGTH = 2,
    OFF_PAYLOAD = 4,
    HEADER_SIZE = OFF_PAYLOAD,
    MAX_PAYLOAD_LENGTH = 1 << 16
  };

  BluetoothDaemonPDU(uint8_t aService, uint8_t aOpcode,
                     uint16_t aPayloadSize);
  BluetoothDaemonPDU(size_t aPayloadSize);

  void SetConsumer(BluetoothDaemonPDUConsumer* aConsumer)
  {
    mConsumer = aConsumer;
  }

  void SetUserData(void* aUserData)
  {
    mUserData = aUserData;
  }

  void* GetUserData() const
  {
    return mUserData;
  }

  void GetHeader(uint8_t& aService, uint8_t& aOpcode,
                 uint16_t& aPayloadSize);

  ssize_t Send(int aFd) override;
  ssize_t Receive(int aFd) override;

  int AcquireFd();

  nsresult UpdateHeader();

private:
  size_t GetPayloadSize() const;
  void OnError(const char* aFunction, int aErrno);

  BluetoothDaemonPDUConsumer* mConsumer;
  void* mUserData;
  ScopedClose mReceivedFd;
};






class BluetoothDaemonPDUConsumer
{
public:
  virtual ~BluetoothDaemonPDUConsumer();

  virtual void Handle(BluetoothDaemonPDU& aPDU) = 0;
  virtual void StoreUserData(const BluetoothDaemonPDU& aPDU) = 0;

protected:
  BluetoothDaemonPDUConsumer();
};






class BluetoothDaemonConnection : public DataSocket
                                , public ConnectionOrientedSocket
{
public:
  BluetoothDaemonConnection();
  virtual ~BluetoothDaemonConnection();

  nsresult ConnectSocket(BluetoothDaemonPDUConsumer* aConsumer);

  
  

  virtual ConnectionOrientedSocketIO* GetIO() override;

  
  

  void SendSocketData(UnixSocketIOBuffer* aBuffer) override;

  
  

  void CloseSocket() override;

protected:

  
  
  
  ConnectionOrientedSocketIO*
    PrepareAccept(BluetoothDaemonPDUConsumer* aConsumer);

private:
  BluetoothDaemonConnectionIO* mIO;
};

}
}

#endif
