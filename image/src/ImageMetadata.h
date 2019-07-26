





#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"
#include "nsSize.h"

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

  void SetSize(int32_t width, int32_t height)
  {
    mSize.construct(nsIntSize(width, height));
  }

  bool HasSize() const { return !mSize.empty(); }

  int32_t GetWidth() const { return mSize.ref().width; }
  int32_t GetHeight() const { return mSize.ref().height; }

private:
  
  int32_t mHotspotX;
  int32_t mHotspotY;

  
  int32_t mLoopCount;

  Maybe<nsIntSize> mSize;

  bool mIsNonPremultiplied;
};

} 
} 
