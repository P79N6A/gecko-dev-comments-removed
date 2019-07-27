





#ifndef mozilla_ipc_DaemonSocketPDU_h
#define mozilla_ipc_DaemonSocketPDU_h

#include "mozilla/FileUtils.h"
#include "mozilla/ipc/SocketBase.h"

namespace mozilla {
namespace ipc {

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

  DaemonSocketPDU(uint8_t aService, uint8_t aOpcode, uint16_t aPayloadSize);
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

}
}

#endif

