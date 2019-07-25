



#ifndef mozilla_system_volumemanager_h__
#define mozilla_system_volumemanager_h__

#include <vector>
#include <queue>

#include "base/message_loop.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Observer.h"
#include "mozilla/RefPtr.h"
#include "nsString.h"

#include "Volume.h"
#include "VolumeCommand.h"

namespace mozilla {
namespace system {





















































class VolumeManager : public MessageLoopForIO::Watcher,
                      public RefCounted<VolumeManager>
{
public:

  typedef std::vector<RefPtr<Volume> > VolumeArray;

  VolumeManager();
  virtual ~VolumeManager();

  
  
  
  
  
  
  
  
  
  
  

  enum STATE
  {
    UNINITIALIZED,
    STARTING,
    VOLUMES_READY
  };

  static STATE State();
  static const char *StateStr();

  class StateChangedEvent
  {
  public:
    StateChangedEvent() {}
  };

  typedef mozilla::Observer<StateChangedEvent>      StateObserver;
  typedef mozilla::ObserverList<StateChangedEvent>  StateObserverList;

  static void RegisterStateObserver(StateObserver *aObserver);
  static void UnregisterStateObserver(StateObserver *aObserver);

  

  static void Start();

  static TemporaryRef<Volume> FindVolumeByName(const nsCSubstring &aName);
  static TemporaryRef<Volume> FindAddVolumeByName(const nsCSubstring &aName);

  static void       PostCommand(VolumeCommand *aCommand);

protected:

  virtual void OnFileCanReadWithoutBlocking(int aFd);
  virtual void OnFileCanWriteWithoutBlocking(int aFd);

private:
  bool OpenSocket();

  friend class VolumeListCallback; 

  static void SetState(STATE aNewState);

  void Restart();
  void WriteCommandData();
  void HandleBroadcast(int aResponseCode, nsCString &aResponseLine);

  typedef std::queue<RefPtr<VolumeCommand> > CommandQueue;

  static const int    kRcvBufSize = 1024;
  STATE               mState;
  ScopedClose         mSocket;
  VolumeArray         mVolumeArray;
  CommandQueue        mCommands;
  bool                mCommandPending;
  char                mRcvBuf[kRcvBufSize];
  size_t              mRcvIdx;
  StateObserverList   mStateObserverList;
  MessageLoopForIO                       *mIOLoop;
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;
  RefPtr<VolumeResponseCallback>          mBroadcastCallback;
};













void InitVolumeManager();




void ShutdownVolumeManager();

} 
} 

#endif  
