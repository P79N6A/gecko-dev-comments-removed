



#ifndef mozilla_system_volumeserviceiothread_h__
#define mozilla_system_volumeserviceiothread_h__

#include "Volume.h"
#include "VolumeManager.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace system {





class VolumeServiceIOThread : public VolumeManager::StateObserver,
                              public Volume::EventObserver,
                              public RefCounted<VolumeServiceIOThread>
{
public:
  VolumeServiceIOThread();
  ~VolumeServiceIOThread();

private:
  void  UpdateAllVolumes();

  virtual void Notify(const VolumeManager::StateChangedEvent &aEvent);
  virtual void Notify(Volume * const &aVolume);

};

void InitVolumeServiceIOThread();
void ShutdownVolumeServiceIOThread();

} 
} 

#endif  
