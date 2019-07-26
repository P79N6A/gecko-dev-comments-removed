




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
    , mDisplayPort(0, 0, 0, 0)
    , mCriticalDisplayPort(0, 0, 0, 0)
    , mViewport(0, 0, 0, 0)
    , mScrollOffset(0, 0)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollableRect(0, 0, 0, 0)
    , mResolution(1)
    , mZoom(1)
    , mDevPixelsPerCSSPixel(1)
    , mMayHaveTouchListeners(false)
    , mPresShellId(-1)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    return mCompositionBounds.IsEqualEdges(aOther.mCompositionBounds) &&
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

  CSSToLayerScale LayersPixelsPerCSSPixel() const
  {
    return mResolution * mDevPixelsPerCSSPixel;
  }

  LayerPoint GetScrollOffsetInLayerPixels() const
  {
    return mScrollOffset * LayersPixelsPerCSSPixel();
  }

  



  CSSToScreenScale CalculateIntrinsicScale() const
  {
    return CSSToScreenScale(float(mCompositionBounds.width) / float(mViewport.width));
  }

  





  CSSToScreenScale CalculateResolution() const
  {
    return CalculateIntrinsicScale() * mZoom;
  }

  CSSRect CalculateCompositedRectInCssPixels() const
  {
    return CSSRect(gfx::RoundedIn(mCompositionBounds / CalculateResolution()));
  }

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ScreenIntRect mCompositionBounds;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSRect mDisplayPort;

  
  
  
  
  
  
  CSSRect mCriticalDisplayPort;

  
  
  
  
  
  
  
  
  
  CSSRect mViewport;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSPoint mScrollOffset;

  
  
  ViewID mScrollId;

  
  
  
  
  
  
  
  
  
  
  
  CSSRect mScrollableRect;

  
  
  

  
  
  
  
  
  
  
  
  LayoutDeviceToLayerScale mResolution;

  
  
  
  
  
  
  
  
  
  
  
  
  ScreenToScreenScale mZoom;

  
  
  
  
  CSSToLayoutDeviceScale mDevPixelsPerCSSPixel;

  
  bool mMayHaveTouchListeners;

  uint32_t mPresShellId;
};

}
}

#endif 
