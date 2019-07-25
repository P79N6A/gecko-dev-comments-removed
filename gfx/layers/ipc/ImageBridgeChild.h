




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

  






  static void ShutDown();

  


  static bool StartUpOnThread(base::Thread* aThread);

  






  static void DestroyBridge();
  
  




  static bool IsCreated();

  




  static ImageBridgeChild* GetSingleton();


  


  void ConnectAsync(ImageBridgeParent* aParent);
    
  




  base::Thread * GetThread() const;
  
  




  MessageLoop * GetMessageLoop() const;

  
  PImageContainerChild* AllocPImageContainer(PRUint64*);
  
  bool DeallocPImageContainer(PImageContainerChild* aImgContainerChild);

  



  ~ImageBridgeChild() {};

  





  already_AddRefed<ImageContainerChild> CreateImageContainerChildNow();
protected:
  
  ImageBridgeChild() {};

  

















  already_AddRefed<ImageContainerChild> CreateImageContainerChild();

  
};

} 
} 


#endif

