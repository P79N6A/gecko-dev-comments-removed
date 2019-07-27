





#ifndef mozilla_image_ImageOps_h
#define mozilla_image_ImageOps_h

#include "nsCOMPtr.h"
#include "nsRect.h"

class gfxDrawable;
class imgIContainer;

namespace mozilla {
namespace image {

class Image;
struct Orientation;

class ImageOps
{
public:
  





  static already_AddRefed<Image> Freeze(Image* aImage);
  static already_AddRefed<imgIContainer> Freeze(imgIContainer* aImage);

  





  static already_AddRefed<Image> Clip(Image* aImage, nsIntRect aClip);
  static already_AddRefed<imgIContainer> Clip(imgIContainer* aImage,
                                              nsIntRect aClip);

  






  static already_AddRefed<Image> Orient(Image* aImage,
                                        Orientation aOrientation);
  static already_AddRefed<imgIContainer> Orient(imgIContainer* aImage,
                                                Orientation aOrientation);

  




  static already_AddRefed<imgIContainer>
  CreateFromDrawable(gfxDrawable* aDrawable);

private:
  
  virtual ~ImageOps() = 0;
};

} 
} 

#endif 
