





#ifndef mozilla_image_src_ImageMetadata_h
#define mozilla_image_src_ImageMetadata_h

#include <stdint.h>
#include "mozilla/Maybe.h"
#include "nsSize.h"
#include "Orientation.h"

namespace mozilla {
namespace image {

class RasterImage;


class ImageMetadata
{
public:
  ImageMetadata()
    : mHotspotX(-1)
    , mHotspotY(-1)
    , mLoopCount(-1)
  { }

  
  void SetOnImage(RasterImage* image);

  void SetHotspot(uint16_t hotspotx, uint16_t hotspoty)
  {
    mHotspotX = hotspotx;
    mHotspotY = hotspoty;
  }
  void SetLoopCount(int32_t loopcount)
  {
    mLoopCount = loopcount;
  }

  void SetSize(int32_t width, int32_t height, Orientation orientation)
  {
    if (!HasSize()) {
      mSize.emplace(nsIntSize(width, height));
      mOrientation.emplace(orientation);
    }
  }

  bool HasSize() const { return mSize.isSome(); }
  bool HasOrientation() const { return mOrientation.isSome(); }

  int32_t GetWidth() const { return mSize->width; }
  int32_t GetHeight() const { return mSize->height; }
  nsIntSize GetSize() const { return *mSize; }
  Orientation GetOrientation() const { return *mOrientation; }

private:
  
  int32_t mHotspotX;
  int32_t mHotspotY;

  
  int32_t mLoopCount;

  Maybe<nsIntSize> mSize;
  Maybe<Orientation> mOrientation;
};

} 
} 

#endif 
