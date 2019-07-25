




#ifndef MOZILLA_GFX_IMAGECONTAINERCHILD_H
#define MOZILLA_GFX_IMAGECONTAINERCHILD_H

#include "mozilla/layers/PImageContainerChild.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "ImageTypes.h"

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

  




  void DispatchSetIdle();

  


  void SetIdleNow();

  



  already_AddRefed<Image> CreateImage(const ImageFormat *aFormats,
                                      uint32_t aNumFormats);

  






  void AllocateSharedBufferForImage(Image* aImage);

  






  void RecycleSharedImage(SharedImage* aImage);

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

  inline void SetID(uint64_t id)
  {
    mImageContainerID = id;
  }

  



  void DestroySharedImage(const SharedImage& aSurface);

  






  bool AddSharedImageToPool(SharedImage* img);

  



  SharedImage* GetSharedImageFor(Image* aImage);
  



  void ClearSharedImagePool();
  








  SharedImage* ImageToSharedImage(Image* toCopy);

  


  bool CopyDataIntoSharedImage(Image* src, SharedImage* dest);

  



  SharedImage * CreateSharedImageFromData(Image* aImage);
  



  void AllocateSharedBufferForImageNow(Image* aImage);

  




  void AllocateSharedBufferForImageSync(ReentrantMonitor* aBarrier,
                                        bool* aDone,
                                        Image* aImage);

  




  void RecycleSharedImageNow(SharedImage* aImage);


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

