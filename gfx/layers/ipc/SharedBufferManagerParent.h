





#ifndef SharedBufferManagerPARENT_H_
#define SharedBufferManagerPARENT_H_

#include "mozilla/Atomics.h"          
#include "mozilla/layers/PSharedBufferManagerParent.h"
#include "mozilla/StaticPtr.h"

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/Mutex.h"            

namespace android {
class GraphicBuffer;
}
#endif

namespace base {
class Thread;
}

namespace mozilla {
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
class Mutex;
#endif

namespace layers {

class SharedBufferManagerParent : public PSharedBufferManagerParent
{
friend class GrallocReporter;
public:
  


  static PSharedBufferManagerParent* Create(Transport* aTransport, ProcessId aOtherProcess);

  



  static SharedBufferManagerParent* GetInstance(ProcessId id);
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  android::sp<android::GraphicBuffer> GetGraphicBuffer(int key);
  static android::sp<android::GraphicBuffer> GetGraphicBuffer(GrallocBufferRef aRef);
#endif
  


  SharedBufferManagerParent(Transport* aTransport, ProcessId aOwner, base::Thread* aThread);
  virtual ~SharedBufferManagerParent();

  


  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool RecvAllocateGrallocBuffer(const IntSize&, const uint32_t&, const uint32_t&, mozilla::layers::MaybeMagicGrallocBufferHandle*);
  virtual bool RecvDropGrallocBuffer(const mozilla::layers::MaybeMagicGrallocBufferHandle& handle);

  


  void DropGrallocBuffer(mozilla::layers::SurfaceDescriptor aDesc);

  
  IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;
  MessageLoop* GetMessageLoop();

protected:
  


  static std::map<base::ProcessId, SharedBufferManagerParent*> sManagers;

  




  void DropGrallocBufferImpl(mozilla::layers::SurfaceDescriptor aDesc);

  
  static void DropGrallocBufferSync(SharedBufferManagerParent* mgr, mozilla::layers::SurfaceDescriptor aDesc);

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  


  std::map<int, android::sp<android::GraphicBuffer> > mBuffers;
  Mutex mBuffersMutex;
#endif
  
  Transport* mTransport;
  base::ProcessId mOwner;
  base::Thread* mThread;
  static mozilla::Atomic<uint32_t> sBufferKey;

  static StaticAutoPtr<Monitor> sManagerMonitor;
};

} 
} 
#endif 
