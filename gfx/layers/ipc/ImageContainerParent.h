




#ifndef MOZILLA_GFX_IMAGECONTAINERPARENT_H
#define MOZILLA_GFX_IMAGECONTAINERPARENT_H

#include "mozilla/layers/PImageContainerParent.h"

namespace mozilla {
namespace layers {

class ImageBridgeParent;













class ImageContainerParent : public PImageContainerParent
{
public:

  ImageContainerParent(uint32_t aContainerID);
  ~ImageContainerParent();

  
  bool RecvPublishImage(const SharedImage& aImage) MOZ_OVERRIDE;
  
  bool RecvStop() MOZ_OVERRIDE;
  
  bool Recv__delete__() MOZ_OVERRIDE;
  
  bool RecvFlush() MOZ_OVERRIDE;

  






  void DoStop();

  static SharedImage* GetSharedImage(uint64_t aID);
  






  static uint32_t GetSharedImageVersion(uint64_t aID);

  




  static bool IsExistingID(uint64_t aID);

  









  static bool SetCompositorIDForImage(uint64_t aImageID, uint64_t aCompositorID);

  


  static uint64_t GetCompositorIDForImage(uint64_t aImageID);

  


  static void CreateSharedImageMap();

  


  static void DestroySharedImageMap();

protected:
  virtual PGrallocBufferParent*
  AllocPGrallocBuffer(const gfxIntSize&, const gfxContentType&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE
  { return nullptr; }

  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferParent* actor) MOZ_OVERRIDE
  { return false; }

  







  static SharedImage* SwapSharedImage(uint64_t aID, SharedImage* aImage);

  







  static SharedImage* RemoveSharedImage(uint64_t aID);

private:
  uint64_t mID;
  bool  mStop;
};


} 
} 

#endif

