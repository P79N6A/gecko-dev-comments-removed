




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include "gfxPoint.h"
#include "gfxTypes.h"
#include "nsRect.h"
#include "mozilla/gfx/Rect.h"
#include "Units.h"

namespace mozilla {
namespace layers {







struct FrameMetrics {
public:
  
  typedef uint64_t ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID ROOT_SCROLL_ID;   
  static const ViewID START_SCROLL_ID;  
                                        

  FrameMetrics()
    : mCompositionBounds(0, 0, 0, 0)
    , mContentRect(0, 0, 0, 0)
    , mDisplayPort(0, 0, 0, 0)
    , mCriticalDisplayPort(0, 0, 0, 0)
    , mViewport(0, 0, 0, 0)
    , mScrollOffset(0, 0)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollableRect(0, 0, 0, 0)
    , mResolution(1, 1)
    , mZoom(1, 1)
    , mDevPixelsPerCSSPixel(1)
    , mMayHaveTouchListeners(false)
    , mPresShellId(-1)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    return mCompositionBounds.IsEqualEdges(aOther.mCompositionBounds) &&
           mContentRect.IsEqualEdges(aOther.mContentRect) &&
           mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
           mCriticalDisplayPort.IsEqualEdges(aOther.mCriticalDisplayPort) &&
           mViewport.IsEqualEdges(aOther.mViewport) &&
           mScrollOffset == aOther.mScrollOffset &&
           mScrollId == aOther.mScrollId &&
           mScrollableRect.IsEqualEdges(aOther.mScrollableRect) &&
           mResolution == aOther.mResolution &&
           mDevPixelsPerCSSPixel == aOther.mDevPixelsPerCSSPixel &&
           mMayHaveTouchListeners == aOther.mMayHaveTouchListeners &&
           mPresShellId == aOther.mPresShellId;
  }
  bool operator!=(const FrameMetrics& aOther) const
  {
    return !operator==(aOther);
  }

  bool IsDefault() const
  {
    FrameMetrics def;

    def.mPresShellId = mPresShellId;
    return (def == *this);
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

  gfxPoint GetScrollOffsetInLayerPixels() const
  {
    return gfxPoint(
      static_cast<gfx::Float>(
        mScrollOffset.x * LayersPixelsPerCSSPixel().width),
      static_cast<gfx::Float>(
        mScrollOffset.y * LayersPixelsPerCSSPixel().height));
  }

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  LayerIntRect mCompositionBounds;

  
  
  
  
  
  
  
  
  
  nsIntRect mContentRect;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  gfx::Rect mDisplayPort;

  
  
  
  
  
  
  gfx::Rect mCriticalDisplayPort;

  
  
  
  
  
  
  
  
  
  gfx::Rect mViewport;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  mozilla::CSSPoint mScrollOffset;

  
  
  ViewID mScrollId;

  
  
  
  
  
  
  
  
  
  
  
  mozilla::CSSRect mScrollableRect;

  
  
  

  
  
  
  
  
  
  
  
  gfxSize mResolution;

  
  
  
  
  
  
  
  
  
  
  
  
  
  gfxSize mZoom;

  
  
  
  
  float mDevPixelsPerCSSPixel;

  
  bool mMayHaveTouchListeners;

  uint32_t mPresShellId;
};

}
}

#endif 
