





#ifndef mozilla_ipc_bluetooth_DaemonSocket_h
#define mozilla_ipc_bluetooth_DaemonSocket_h

#include "mozilla/Attributes.h"
#include "mozilla/FileUtils.h"
#include "mozilla/ipc/ConnectionOrientedSocket.h"
#include "nsAutoPtr.h"

class MessageLoop;

namespace mozilla {
namespace ipc {

class DaemonSocketConsumer;
class DaemonSocketIO;
class DaemonSocketIOConsumer;





















class DaemonSocketPDU final : public UnixSocketIOBuffer
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

  DaemonSocketPDU(uint8_t aService, uint8_t aOpcode,
                     uint16_t aPayloadSize);
  DaemonSocketPDU(size_t aPayloadSize);
  ~DaemonSocketPDU();

  void SetConsumer(DaemonSocketIOConsumer* aConsumer)
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

  DaemonSocketIOConsumer* mConsumer;
  void* mUserData;
  ScopedClose mReceivedFd;
};






class DaemonSocketIOConsumer
{
public:
  virtual ~DaemonSocketIOConsumer();

  virtual void Handle(DaemonSocketPDU& aPDU) = 0;
  virtual void StoreUserData(const DaemonSocketPDU& aPDU) = 0;

protected:
  DaemonSocketIOConsumer();
};






class DaemonSocket : public ConnectionOrientedSocket
{
public:
  DaemonSocket(DaemonSocketIOConsumer* aIOConsumer,
               DaemonSocketConsumer* aConsumer,
               int aIndex);
  virtual ~DaemonSocket();

  
  

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
  DaemonSocketIO* mIO;
  DaemonSocketIOConsumer* mIOConsumer;
  DaemonSocketConsumer* mConsumer;
  int mIndex;
};

}
}

#endif
