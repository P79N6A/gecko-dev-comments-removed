





#include "imgIContainer.h"
#include "ClippedImage.h"
#include "FrozenImage.h"
#include "Image.h"

#include "ImageOps.h"

namespace mozilla {
namespace image {

 already_AddRefed<Image>
ImageOps::Freeze(Image* aImage)
{
  nsRefPtr<Image> frozenImage = new FrozenImage(aImage);
  return frozenImage.forget();
}

 already_AddRefed<imgIContainer>
ImageOps::Freeze(imgIContainer* aImage)
{
  nsCOMPtr<imgIContainer> frozenImage =
    new FrozenImage(static_cast<Image*>(aImage));
  return frozenImage.forget();
}

 already_AddRefed<Image>
ImageOps::Clip(Image* aImage, nsIntRect aClip)
{
  nsRefPtr<Image> clippedImage = new ClippedImage(aImage, aClip);
  return clippedImage.forget();
}

 already_AddRefed<imgIContainer>
ImageOps::Clip(imgIContainer* aImage, nsIntRect aClip)
{
  nsCOMPtr<imgIContainer> clippedImage =
    new ClippedImage(static_cast<Image*>(aImage), aClip);
  return clippedImage.forget();
}

} 
} 
