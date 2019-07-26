





#ifndef MOZILLA_IMAGELIB_IMAGEOPS_H_
#define MOZILLA_IMAGELIB_IMAGEOPS_H_

#include "nsCOMPtr.h"
#include "nsRect.h"
#include "Orientation.h"

class imgIContainer;

namespace mozilla {
namespace image {

class Image;

class ImageOps
{
public:
  





  static already_AddRefed<Image> Freeze(Image* aImage);
  static already_AddRefed<imgIContainer> Freeze(imgIContainer* aImage);

  





  static already_AddRefed<Image> Clip(Image* aImage, nsIntRect aClip);
  static already_AddRefed<imgIContainer> Clip(imgIContainer* aImage, nsIntRect aClip);

  






  static already_AddRefed<Image> Orient(Image* aImage, Orientation aOrientation);
  static already_AddRefed<imgIContainer> Orient(imgIContainer* aImage, Orientation aOrientation);

private:
  
  virtual ~ImageOps() = 0;
};

} 
} 

#endif 
