




#ifndef MOZILLA_GFX_IMAGECONTAINERCHILD_H
#define MOZILLA_GFX_IMAGECONTAINERCHILD_H

#include "mozilla/layers/PImageContainerChild.h"
#include "mozilla/layers/ImageBridgeChild.h"

namespace mozilla {
namespace layers {

class ImageBridgeCopyAndSendTask;














class ImageContainerChild : public PImageContainerChild
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainerChild)

  friend class ImageBridgeChild;
  friend class ImageBridgeCopyAndSendTask;

public:
  ImageContainerChild();
  virtual ~ImageContainerChild();

  





  void SendImageAsync(ImageContainer* aContainer, Image* aImage);

  




  const PRUint64& GetID() const
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
  
protected:
  virtual PGrallocBufferChild*
  AllocPGrallocBuffer(const gfxIntSize&, const gfxContentType&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE

  { return nsnull; }

  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferChild* actor) MOZ_OVERRIDE
  { return false; }

  inline MessageLoop* GetMessageLoop() const 
  {
    return ImageBridgeChild::GetSingleton()->GetMessageLoop();
  }

  


  void DestroyNow();

  inline void SetID(PRUint64 id)
  {
    mImageContainerID = id;
  }

  



  void DestroySharedImage(const SharedImage& aSurface);

  






  bool AddSharedImageToPool(SharedImage* img);

  



  SharedImage* PopSharedImageFromPool();
  



  void ClearSharedImagePool();
  








  SharedImage* ImageToSharedImage(Image* toCopy);

  


  bool CopyDataIntoSharedImage(Image* src, SharedImage* dest);

  



  SharedImage * CreateSharedImageFromData(Image* aImage);

private:

  PRUint64 mImageContainerID;
  nsIntSize mSize;
  nsTArray<SharedImage*> mSharedImagePool;
  int mActiveImageCount;
  bool mStop;
  bool mDispatchedDestroy;
};


} 
} 

#endif

