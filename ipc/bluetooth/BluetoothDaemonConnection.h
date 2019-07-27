





#ifndef mozilla_ipc_bluetooth_BluetoothDaemonConnection_h
#define mozilla_ipc_bluetooth_BluetoothDaemonConnection_h

#include "mozilla/Attributes.h"
#include "mozilla/FileUtils.h"
#include "mozilla/ipc/ConnectionOrientedSocket.h"
#include "nsAutoPtr.h"

class MessageLoop;

namespace mozilla {
namespace ipc {

class DaemonSocketConsumer;
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
  ~BluetoothDaemonPDU();

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






class BluetoothDaemonConnection : public ConnectionOrientedSocket
{
public:
  BluetoothDaemonConnection(BluetoothDaemonPDUConsumer* aPDUConsumer,
                            DaemonSocketConsumer* aConsumer,
                            int aIndex);
  virtual ~BluetoothDaemonConnection();

  
  

  nsresult PrepareAccept(UnixSocketConnector* aConnector,
                         MessageLoop* aConsumerLoop,
                         MessageLoop* aIOLoop,
                         ConnectionOrientedSocketIO*& aIO) override;

  
  

  void SendSocketData(UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;
  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

private:
  BluetoothDaemonConnectionIO* mIO;
  BluetoothDaemonPDUConsumer* mPDUConsumer;
  DaemonSocketConsumer* mConsumer;
  int mIndex;
};

}
}

#endif
