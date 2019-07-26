




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include "gfxPoint.h"
#include "gfxTypes.h"
#include "nsRect.h"
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace layers {







struct THEBES_API FrameMetrics {
public:
  
  typedef uint64_t ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID ROOT_SCROLL_ID;   
  static const ViewID START_SCROLL_ID;  
                                        

  FrameMetrics()
    : mCompositionBounds(0, 0, 0, 0)
    , mContentRect(0, 0, 0, 0)
    , mDisplayPort(0, 0, 0, 0)
    , mViewport(0, 0, 0, 0)
    , mScrollOffset(0, 0)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollableRect(0, 0, 0, 0)
    , mResolution(1, 1)
    , mDevPixelsPerCSSPixel(1)
    , mMayHaveTouchListeners(false)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    return (mViewport.IsEqualEdges(aOther.mViewport) &&
            mScrollOffset == aOther.mScrollOffset &&
            mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
            mScrollId == aOther.mScrollId);
  }
  bool operator!=(const FrameMetrics& aOther) const
  {
    return !operator==(aOther);
  }

  bool IsDefault() const
  {
    return (FrameMetrics() == *this);
  }

  bool IsRootScrollable() const
  {
    return mScrollId == ROOT_SCROLL_ID;
  }

  bool IsScrollable() const
  {
    return mScrollId != NULL_SCROLL_ID;
  }

  gfxSize LayersPixelsPerCSSPixel() const
  {
    return mResolution * mDevPixelsPerCSSPixel;
  }

  gfx::Point GetScrollOffsetInLayerPixels() const
  {
    return gfx::Point(mScrollOffset.x * LayersPixelsPerCSSPixel().width,
                      mScrollOffset.y * LayersPixelsPerCSSPixel().height);
  }

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIntRect mCompositionBounds;

  
  
  
  
  
  
  
  
  
  nsIntRect mContentRect;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  gfx::Rect mDisplayPort;

  
  
  
  
  
  
  
  
  
  gfx::Rect mViewport;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  gfx::Point mScrollOffset;

  
  
  ViewID mScrollId;

  
  
  
  
  
  
  
  
  
  
  
  gfx::Rect mScrollableRect;

  
  
  gfxSize mResolution;

  
  
  
  
  float mDevPixelsPerCSSPixel;

  
  bool mMayHaveTouchListeners;
};

}
}

#endif 
