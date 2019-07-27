



#ifndef mozilla_system_volumemanager_h__
#define mozilla_system_volumemanager_h__

#include <vector>
#include <queue>

#include "base/message_loop.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Observer.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsTArray.h"

#include "Volume.h"
#include "VolumeCommand.h"

namespace mozilla {
namespace system {





















































class VolumeManager final : public MessageLoopForIO::LineWatcher
{
  virtual ~VolumeManager();

public:
  NS_INLINE_DECL_REFCOUNTING(VolumeManager)

  typedef nsTArray<RefPtr<Volume>> VolumeArray;

  VolumeManager();

  
  
  
  
  
  
  
  
  
  
  

  enum STATE
  {
    UNINITIALIZED,
    STARTING,
    VOLUMES_READY
  };

  static STATE State();
  static const char* StateStr(STATE aState);
  static const char* StateStr() { return StateStr(State()); }

  class StateChangedEvent
  {
  public:
    StateChangedEvent() {}
  };

  typedef mozilla::Observer<StateChangedEvent>      StateObserver;
  typedef mozilla::ObserverList<StateChangedEvent>  StateObserverList;

  static void RegisterStateObserver(StateObserver* aObserver);
  static void UnregisterStateObserver(StateObserver* aObserver);

  

  static void Start();
  static void Dump(const char* aLabel);

  static VolumeArray::size_type NumVolumes();
  static already_AddRefed<Volume> GetVolume(VolumeArray::index_type aIndex);
  static already_AddRefed<Volume> FindVolumeByName(const nsCSubstring& aName);
  static already_AddRefed<Volume> FindAddVolumeByName(const nsCSubstring& aName);
  static void InitConfig();

  static void       PostCommand(VolumeCommand* aCommand);

protected:

  virtual void OnLineRead(int aFd, nsDependentCSubstring& aMessage);
  virtual void OnFileCanWriteWithoutBlocking(int aFd);
  virtual void OnError();

  static void DefaultConfig();

private:
  bool OpenSocket();

  friend class VolumeListCallback; 

  static void SetState(STATE aNewState);

  void Restart();
  void WriteCommandData();
  void HandleBroadcast(int aResponseCode, nsCString& aResponseLine);

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
