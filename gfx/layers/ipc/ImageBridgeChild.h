




#ifndef MOZILLA_GFX_IMAGEBRIDGECHILD_H
#define MOZILLA_GFX_IMAGEBRIDGECHILD_H

#include "mozilla/layers/PImageBridgeChild.h"
#include "nsAutoPtr.h"

class gfxSharedImageSurface;

namespace base {
class Thread;
}

namespace mozilla {
namespace layers {

class ImageContainerChild;
class ImageBridgeParent;
class SharedImage;
class Image;






bool InImageBridgeChildThread();










































class ImageBridgeChild : public PImageBridgeChild
{
  friend class ImageContainer;
public:

  





  static void StartUp();

  static PImageBridgeChild*
  StartUpInChildProcess(Transport* aTransport, ProcessId aOtherProcess);

  






  static void ShutDown();

  


  static bool StartUpOnThread(base::Thread* aThread);

  






  static void DestroyBridge();
  
  




  static bool IsCreated();

  




  static ImageBridgeChild* GetSingleton();


  


  void ConnectAsync(ImageBridgeParent* aParent);
    
  




  base::Thread * GetThread() const;
  
  




  MessageLoop * GetMessageLoop() const;

  
  PImageContainerChild* AllocPImageContainer(uint64_t*);
  
  bool DeallocPImageContainer(PImageContainerChild* aImgContainerChild);

  



  ~ImageBridgeChild() {};

  





  already_AddRefed<ImageContainerChild> CreateImageContainerChildNow();

  virtual PGrallocBufferChild*
  AllocPGrallocBuffer(const gfxIntSize&, const uint32_t&, const uint32_t&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;

  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferChild* actor) MOZ_OVERRIDE;

  


  bool
  AllocSurfaceDescriptorGralloc(const gfxIntSize& aSize,
                                const uint32_t& aFormat,
                                const uint32_t& aUsage,
                                SurfaceDescriptor* aBuffer);

  






  bool
  AllocSurfaceDescriptorGrallocNow(const gfxIntSize& aSize,
                                   const uint32_t& aContent,
                                   const uint32_t& aUsage,
                                   SurfaceDescriptor* aBuffer);

  


  bool
  DeallocSurfaceDescriptorGralloc(const SurfaceDescriptor& aBuffer);

  






  bool
  DeallocSurfaceDescriptorGrallocNow(const SurfaceDescriptor& aBuffer);

protected:
  
  ImageBridgeChild() {};

  

















  already_AddRefed<ImageContainerChild> CreateImageContainerChild();

  
};

} 
} 


#endif

