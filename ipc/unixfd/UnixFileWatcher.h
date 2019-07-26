





#ifndef mozilla_ipc_UnixFileWatcher_h
#define mozilla_ipc_UnixFileWatcher_h

#include "UnixFdWatcher.h"

namespace mozilla {
namespace ipc {

class UnixFileWatcher : public UnixFdWatcher
{
public:
  virtual ~UnixFileWatcher();

  nsresult Open(const char* aFilename, int aFlags, mode_t aMode = 0);

  
  virtual void OnOpened() {};

protected:
  UnixFileWatcher(MessageLoop* aIOLoop);
  UnixFileWatcher(MessageLoop* aIOLoop, int aFd);
};

}
}

#endif
