





#ifndef mozilla_SandboxChroot_h
#define mozilla_SandboxChroot_h

#include <pthread.h>

#include "mozilla/Attributes.h"
























namespace mozilla {

class SandboxChroot final {
public:
  SandboxChroot();
  ~SandboxChroot();
  bool Prepare();
  void Invoke();
private:
  enum Command {
    NO_THREAD,
    NO_COMMAND,
    DO_CHROOT,
    JUST_EXIT,
  };

  pthread_t mThread;
  pthread_mutex_t mMutex;
  pthread_cond_t mWakeup;
  Command mCommand;
  int mFd;

  void ThreadMain();
  static void* StaticThreadMain(void* aVoidPtr);
  bool SendCommand(Command aComm);
};

} 

#endif
