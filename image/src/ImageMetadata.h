





#include "mozilla/StandardInteger.h"

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
    , mIsNonPremultiplied(false)
  {}

  
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

  void SetIsNonPremultiplied(bool nonPremult)
  {
    mIsNonPremultiplied = nonPremult;
  }

private:
  
  int32_t mHotspotX;
  int32_t mHotspotY;

  
  int32_t mLoopCount;

  bool mIsNonPremultiplied;
};

} 
} 
