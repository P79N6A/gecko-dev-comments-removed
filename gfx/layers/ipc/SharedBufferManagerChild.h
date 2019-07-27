





#ifndef SharedBufferManagerCHILD_H_
#define SharedBufferManagerCHILD_H_

#include "mozilla/layers/PSharedBufferManagerChild.h"
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#include "mozilla/Mutex.h"
#endif

namespace base {
class Thread;
}

namespace mozilla {
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
class Mutex;
#endif

namespace layers {
class SharedBufferManagerParent;

struct GrallocParam {
  gfx::IntSize size;
  uint32_t format;
  uint32_t usage;
  mozilla::layers::MaybeMagicGrallocBufferHandle* buffer;

  GrallocParam(const gfx::IntSize& aSize,
               const uint32_t& aFormat,
               const uint32_t& aUsage,
               mozilla::layers::MaybeMagicGrallocBufferHandle* aBuffer)
    : size(aSize)
    , format(aFormat)
    , usage(aUsage)
    , buffer(aBuffer)
  {}
};

class SharedBufferManagerChild : public PSharedBufferManagerChild {
public:
  SharedBufferManagerChild();
  





  static void StartUp();

  static PSharedBufferManagerChild*
  StartUpInChildProcess(Transport* aTransport, ProcessId aOtherProcess);

  


  static bool StartUpOnThread(base::Thread* aThread);

  






  static void DestroyManager();

  






  static void ShutDown();

  




  static SharedBufferManagerChild* GetSingleton();

  


  void ConnectAsync(SharedBufferManagerParent* aParent);

  


  bool
  AllocGrallocBuffer(const gfx::IntSize&, const uint32_t&, const uint32_t&, mozilla::layers::MaybeMagicGrallocBufferHandle*);

  




  void
  DeallocGrallocBuffer(const mozilla::layers::MaybeMagicGrallocBufferHandle& aBuffer);

  void
  DropGrallocBuffer(const mozilla::layers::MaybeMagicGrallocBufferHandle& aHandle);

  virtual bool RecvDropGrallocBuffer(const mozilla::layers::MaybeMagicGrallocBufferHandle& aHandle);

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  android::sp<android::GraphicBuffer> GetGraphicBuffer(int64_t key);
#endif

  bool IsValidKey(int64_t key);

  base::Thread* GetThread() const;

  MessageLoop* GetMessageLoop() const;

  static bool IsCreated();

  static base::Thread* sSharedBufferManagerChildThread;
  static SharedBufferManagerChild* sSharedBufferManagerChildSingleton;
  static SharedBufferManagerParent* sSharedBufferManagerParentSingleton; 

protected:
  






  bool
  AllocGrallocBufferNow(const gfx::IntSize& aSize,
                        const uint32_t& aFormat,
                        const uint32_t& aUsage,
                        mozilla::layers::MaybeMagicGrallocBufferHandle* aBuffer);

  
  static void
  AllocGrallocBufferSync(const GrallocParam& aParam,
                         Monitor* aBarrier,
                         bool* aDone);

  






  void
  DeallocGrallocBufferNow(const mozilla::layers::MaybeMagicGrallocBufferHandle& handle);

  
  static void
  DeallocGrallocBufferSync(const mozilla::layers::MaybeMagicGrallocBufferHandle& handle);

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  std::map<int64_t, android::sp<android::GraphicBuffer> > mBuffers;
  Mutex mBufferMutex;
#endif
};

} 
} 
#endif 
