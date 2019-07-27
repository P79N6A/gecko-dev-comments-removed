





#include "base/message_loop.h"
#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketResultHandler;















class SocketMessageWatcher : public MessageLoopForIO::Watcher
{
public:
  static const unsigned char MSG1_SIZE = 4;
  static const unsigned char MSG2_SIZE = 16;

  static const unsigned char OFF_CHANNEL1 = 0;
  static const unsigned char OFF_SIZE = 4;
  static const unsigned char OFF_BDADDRESS = 6;
  static const unsigned char OFF_CHANNEL2 = 12;
  static const unsigned char OFF_STATUS = 16;

  virtual ~SocketMessageWatcher();

  virtual void Proceed(BluetoothStatus aStatus) = 0;

  void OnFileCanReadWithoutBlocking(int aFd) MOZ_OVERRIDE;
  void OnFileCanWriteWithoutBlocking(int aFd) MOZ_OVERRIDE;

  void Watch();
  void StopWatching();

  bool IsComplete() const;

  int      GetFd() const;
  int32_t  GetChannel1() const;
  int32_t  GetSize() const;
  nsString GetBdAddress() const;
  int32_t  GetChannel2() const;
  int32_t  GetConnectionStatus() const;
  int      GetClientFd() const;

  BluetoothSocketResultHandler* GetResultHandler() const;

protected:
  SocketMessageWatcher(int aFd, BluetoothSocketResultHandler* aRes);

private:
  BluetoothStatus RecvMsg1();
  BluetoothStatus RecvMsg2();

  int16_t ReadInt16(unsigned long aOffset) const;
  int32_t ReadInt32(unsigned long aOffset) const;
  void    ReadBdAddress(unsigned long aOffset, nsAString& aBdAddress) const;

  MessageLoopForIO::FileDescriptorWatcher mWatcher;
  int mFd;
  int mClientFd;
  unsigned char mLen;
  uint8_t mBuf[MSG1_SIZE + MSG2_SIZE];
  nsRefPtr<BluetoothSocketResultHandler> mRes;
};




class SocketMessageWatcherTask MOZ_FINAL : public Task
{
public:
  SocketMessageWatcherTask(SocketMessageWatcher* aWatcher);

  void Run() MOZ_OVERRIDE;

private:
  SocketMessageWatcher* mWatcher;
};




class DeleteSocketMessageWatcherTask MOZ_FINAL : public Task
{
public:
  DeleteSocketMessageWatcherTask(BluetoothSocketResultHandler* aRes);

  void Run() MOZ_OVERRIDE;

private:
  BluetoothSocketResultHandler* mRes;
};

END_BLUETOOTH_NAMESPACE
