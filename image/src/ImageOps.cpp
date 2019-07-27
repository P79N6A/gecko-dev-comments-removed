





#include "imgIContainer.h"
#include "ClippedImage.h"
#include "DynamicImage.h"
#include "FrozenImage.h"
#include "OrientedImage.h"
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

 already_AddRefed<Image>
ImageOps::Orient(Image* aImage, Orientation aOrientation)
{
  nsRefPtr<Image> orientedImage = new OrientedImage(aImage, aOrientation);
  return orientedImage.forget();
}

 already_AddRefed<imgIContainer>
ImageOps::Orient(imgIContainer* aImage, Orientation aOrientation)
{
  nsCOMPtr<imgIContainer> orientedImage =
    new OrientedImage(static_cast<Image*>(aImage), aOrientation);
  return orientedImage.forget();
}

 already_AddRefed<imgIContainer>
ImageOps::CreateFromDrawable(gfxDrawable* aDrawable)
{
  nsCOMPtr<imgIContainer> drawableImage = new DynamicImage(aDrawable);
  return drawableImage.forget();
}

} 
} 
