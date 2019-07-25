



#ifndef mozilla_system_netd_h__
#define mozilla_system_netd_h__

#include "mozilla/RefPtr.h"
#include "nsAutoPtr.h"
#include "base/message_loop.h"
#include "mozilla/FileUtils.h"

#define MAX_COMMAND_SIZE  4096

namespace mozilla {
namespace ipc {




struct NetdCommand
{
  uint8_t mData[MAX_COMMAND_SIZE];

  
  size_t mSize;
};

class NetdConsumer : public mozilla::RefCounted<NetdConsumer>
{
public:
  virtual ~NetdConsumer() { }
  virtual void MessageReceived(NetdCommand* aMessage) = 0;
};

class NetdWriteTask : public Task
{
  virtual void Run();
};

class NetdClient : public MessageLoopForIO::Watcher,
                   public RefCounted<NetdClient>
{
public:
  typedef std::queue<NetdCommand*> NetdCommandQueue;

  NetdClient();
  virtual ~NetdClient();
  static void Start();
  static void SendNetdCommandIOThread(NetdCommand* aMessage);

private:
  void WriteNetdCommand();
  void Restart();
  virtual void OnFileCanReadWithoutBlocking(int aFd);
  virtual void OnFileCanWriteWithoutBlocking(int aFd);
  bool OpenSocket();

  MessageLoopForIO *mIOLoop;
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;
  ScopedClose mSocket;
  NetdCommandQueue mOutgoingQ;
  char mReceiveBuffer[MAX_COMMAND_SIZE];
  nsAutoPtr<NetdCommand> mCurrentNetdCommand;
  size_t mCurrentWriteOffset;
  size_t mReceivedIndex;
  size_t mReConnectTimes;
};

void StartNetd(NetdConsumer *);
void StopNetd();
void SendNetdCommand(NetdCommand *);

} 
} 

#endif  
