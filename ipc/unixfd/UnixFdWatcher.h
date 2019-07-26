





#ifndef mozilla_ipc_UnixFdWatcher_h
#define mozilla_ipc_UnixFdWatcher_h

#include "base/message_loop.h"
#include "mozilla/FileUtils.h"

namespace mozilla {
namespace ipc {

class UnixFdWatcher : public MessageLoopForIO::Watcher
{
public:
  enum {
    READ_WATCHER = 1<<0,
    WRITE_WATCHER = 1<<1
  };

  virtual ~UnixFdWatcher();

  MessageLoop* GetIOLoop() const
  {
    return mIOLoop;
  }

  int GetFd() const
  {
    return mFd;
  }

  bool IsOpen() const
  {
    return GetFd() >= 0;
  }

  virtual void Close();

  void AddWatchers(unsigned long aWatchers, bool aPersistent);
  void RemoveWatchers(unsigned long aWatchers);

  
  virtual void OnClose() {};

  
  virtual void OnError(const char* aFunction, int aErrno);

protected:
  UnixFdWatcher(MessageLoop* aIOLoop);
  UnixFdWatcher(MessageLoop* aIOLoop, int aFd);
  void SetFd(int aFd);

private:
  static bool FdIsNonBlocking(int aFd);

  MessageLoop* mIOLoop;
  ScopedClose mFd;
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;
};

}
}

#endif
