



#ifndef mozilla_system_volumemanager_h__
#define mozilla_system_volumemanager_h__

#include <vector>
#include <queue>

#include "base/message_loop.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Observer.h"
#include "mozilla/RefPtr.h"
#include "nsString.h"
#include "nsTArray.h"

#include "Volume.h"
#include "VolumeCommand.h"

namespace mozilla {
namespace system {





















































class VolumeManager : public MessageLoopForIO::LineWatcher,
                      public RefCounted<VolumeManager>
{
public:

  typedef nsTArray<RefPtr<Volume> > VolumeArray;

  VolumeManager();
  virtual ~VolumeManager();

  
  
  
  
  
  
  
  
  
  
  

  enum STATE
  {
    UNINITIALIZED,
    STARTING,
    VOLUMES_READY
  };

  static STATE State();
  static const char *StateStr(STATE aState);
  static const char *StateStr() { return StateStr(State()); }

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

  static VolumeArray::size_type NumVolumes();
  static TemporaryRef<Volume> GetVolume(VolumeArray::index_type aIndex);
  static TemporaryRef<Volume> FindVolumeByName(const nsCSubstring &aName);
  static TemporaryRef<Volume> FindAddVolumeByName(const nsCSubstring &aName);

  static void       PostCommand(VolumeCommand *aCommand);

protected:

  virtual void OnLineRead(int aFd, nsDependentCSubstring& aMessage);
  virtual void OnFileCanWriteWithoutBlocking(int aFd);
  virtual void OnError();

private:
  bool OpenSocket();

  friend class VolumeListCallback; 

  static void SetState(STATE aNewState);

  void Restart();
  void WriteCommandData();
  void HandleBroadcast(int aResponseCode, nsCString &aResponseLine);

  typedef std::queue<RefPtr<VolumeCommand> > CommandQueue;

  static STATE              mState;
  static StateObserverList  mStateObserverList;

  static const int    kRcvBufSize = 1024;
  ScopedClose         mSocket;
  VolumeArray         mVolumeArray;
  CommandQueue        mCommands;
  bool                mCommandPending;
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;
  RefPtr<VolumeResponseCallback>          mBroadcastCallback;
};













void InitVolumeManager();




void ShutdownVolumeManager();

} 
} 

#endif  
