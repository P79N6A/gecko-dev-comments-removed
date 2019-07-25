




#ifndef MOZILLA_GFX_IMAGECONTAINERPARENT_H
#define MOZILLA_GFX_IMAGECONTAINERPARENT_H

#include "mozilla/layers/PImageContainerParent.h"

namespace mozilla {
namespace layers {

class ImageBridgeParent;













class ImageContainerParent : public PImageContainerParent
{
public:

  ImageContainerParent(PRUint32 aContainerID);
  ~ImageContainerParent();

  
  bool RecvPublishImage(const SharedImage& aImage) MOZ_OVERRIDE;
  
  bool RecvStop() MOZ_OVERRIDE;
  
  bool Recv__delete__() MOZ_OVERRIDE;
  
  bool RecvFlush() MOZ_OVERRIDE;

  






  void DoStop();

  static SharedImage* GetSharedImage(PRUint64 aID);
  






  static PRUint32 GetSharedImageVersion(PRUint64 aID);

  




  static bool IsExistingID(PRUint64 aID);

  









  static bool SetCompositorIDForImage(PRUint64 aImageID, PRUint64 aCompositorID);

  


  static PRUint64 GetCompositorIDForImage(PRUint64 aImageID);

  


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

  







  static SharedImage* SwapSharedImage(PRUint64 aID, SharedImage* aImage);

  







  static SharedImage* RemoveSharedImage(PRUint64 aID);

private:
  PRUint64 mID;
  bool  mStop;
};


} 
} 

#endif

