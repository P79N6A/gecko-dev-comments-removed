




#ifndef MOZILLA_GFX_IMAGECONTAINERCHILD_H
#define MOZILLA_GFX_IMAGECONTAINERCHILD_H

#include "mozilla/layers/PImageContainerChild.h"
#include "mozilla/layers/ImageBridgeChild.h"

namespace mozilla {
class ReentrantMonitor;
namespace layers {

class ImageBridgeCopyAndSendTask;
class ImageContainer;














class ImageContainerChild : public PImageContainerChild
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainerChild)

  friend class ImageBridgeChild;
  friend class ImageBridgeCopyAndSendTask;

public:
  ImageContainerChild();
  virtual ~ImageContainerChild();

  





  void SendImageAsync(ImageContainer* aContainer, Image* aImage);

  




  const uint64_t& GetID() const
  {
    return mImageContainerID;
  }

  







  bool RecvReturnImage(const SharedImage& aImage) MOZ_OVERRIDE;

  






  void StopChildAndParent();
  
  










  void StopChild();
  
  






  void DispatchStop();

  



  void DispatchDestroy();

  


  void SetIdle();

  






  void RecycleSharedImage(SharedImage* aImage);

  



  already_AddRefed<Image> CreateImage(const uint32_t *aFormats,
                                      uint32_t aNumFormats);

  







  bool AllocUnsafeShmemSync(size_t aBufSize,
                            SharedMemory::SharedMemoryType aType,
                            ipc::Shmem* aShmem);

  


  void DeallocShmemAsync(ipc::Shmem& aShmem);

protected:

  virtual PGrallocBufferChild*
  AllocPGrallocBuffer(const gfxIntSize&, const gfxContentType&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE

  { return nullptr; }

  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferChild* actor) MOZ_OVERRIDE
  { return false; }

  inline MessageLoop* GetMessageLoop() const 
  {
    return ImageBridgeChild::GetSingleton()->GetMessageLoop();
  }

  


  void DestroyNow();
  
  




  void SetIdleSync(Monitor* aBarrier, bool* aDone);

  


  void SetIdleNow();

  inline void SetID(uint64_t id)
  {
    mImageContainerID = id;
  }

  


  void RecycleSharedImageNow(SharedImage* aImage);

  



  void DestroySharedImage(const SharedImage& aSurface);

  






  bool AddSharedImageToPool(SharedImage* img);

  










  void SendImageNow(Image* aImage);


  



  SharedImage* AsSharedImage(Image* aImage);

  





  SharedImage* GetSharedImageFor(Image* aImage);

  






  SharedImage* AllocateSharedImageFor(Image* aImage);

  


  bool CopyDataIntoSharedImage(Image* src, SharedImage* dest);


  


  void ClearSharedImagePool();

private:
  uint64_t mImageContainerID;
  nsTArray<SharedImage*> mSharedImagePool;

  






  nsTArray<nsRefPtr<Image> > mImageQueue;

  int mActiveImageCount;
  bool mStop;
  bool mDispatchedDestroy;
};


} 
} 

#endif

